# Fast Image Size Extraction Refinement Implementation Plan (fast_size_extraction_refinement.md)

## Overview
This implementation plan addresses 3 production-critical defects in the fast size extraction pipeline:
1. **Full Format Dimension Extraction**: Replaces plain `QImageReader::size()` in `DiskMediaExtractor::fastExtractImageSize` with `ImageDecoderFacade::readImageDimensions(filePath)` so professional design formats (PSD, AI, EPS, SVG) can extract header dimensions in under 1ms.
2. **Directory-Level Aggregated Persistence**: Refactors `DiskItemModel::preloadDimensionsAsync()` to collect resolved image sizes in memory and perform a single batch write (`jsonCache.save()`) per directory, preventing Windows I/O disk thrashing (`ERROR_SHARING_VIOLATION`).
3. **Full-Row UI Refresh Signal**: Updates `DiskItemModel::reloadThumbnailForPath()` to emit `dataChanged` across the full row (`columnCount() - 1`) including `Qt::DisplayRole`, so Column 3 ("尺寸") updates live when a thumbnail is manually re-extracted.

## Modified Files List
1. `src/util/DiskMediaExtractor.cpp`
2. `src/ui/models/DiskItemModel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/util/DiskMediaExtractor.cpp`

<<<<<<< SEARCH
QSize DiskMediaExtractor::fastExtractImageSize(const QString& filePath) {
    QImageReader reader(filePath);
    QSize sz = reader.size();
    if (sz.isValid() && sz.width() > 0 && sz.height() > 0) {
        return sz;
    }
    return QSize();
}
=======
QSize DiskMediaExtractor::fastExtractImageSize(const QString& filePath) {
    return ImageDecoderFacade::readImageDimensions(filePath);
}
>>>>>>> REPLACE

---

### 2. `src/ui/models/DiskItemModel.cpp`

<<<<<<< SEARCH
void DiskItemModel::preloadDimensionsAsync() {
    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);

    // 主线程构建 SizeTarget 拷贝，规避多线程引用 m_allRecords 导致的竞态与崩溃
    std::vector<SizeTarget> targets;
    targets.reserve(m_allRecords.size());
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        const auto& rec = m_allRecords[i];
        if (!rec.isDir && rec.width == 0 && UiHelper::isGraphicsFile(rec.suffix)) {
            targets.push_back({i, rec.path, rec.suffix});
        }
    }

    if (targets.empty()) return;

    QPointer<DiskItemModel> weakThis(this);

    thumbnailPool()->start([weakThis, targets = std::move(targets), thisGen]() {
        if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

        for (const auto& target : targets) {
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

            QSize sz = DiskMediaExtractor::fastExtractImageSize(target.path);
            if (sz.isValid() && sz.width() > 0) {
                QFileInfo fi(target.path);
                QString parentDir = QDir::toNativeSeparators(fi.absolutePath());
                QString fileName = fi.fileName();

                static std::mutex s_jsonSaveMutex;
                std::lock_guard<std::mutex> lock(s_jsonSaveMutex);

                QuarkMetaJson jsonCache(parentDir.toStdWString());
                jsonCache.load();
                auto& cachedItems = jsonCache.items();
                std::wstring wFileName = fileName.toStdWString();
                if (cachedItems.find(wFileName) == cachedItems.end()) {
                    ItemMeta emptyMeta;
                    emptyMeta.type = L"file";
                    cachedItems[wFileName] = emptyMeta;
                }
                auto& fileMeta = cachedItems[wFileName];
                if (fileMeta.width != sz.width() || fileMeta.height != sz.height()) {
                    fileMeta.width = sz.width();
                    fileMeta.height = sz.height();
                    jsonCache.save();
                }

                int i = target.index;
                QString targetPath = target.path;
                QMetaObject::invokeMethod(weakThis.data(), [weakThis, i, targetPath, sz, thisGen]() {
                    if (weakThis && weakThis->currentGeneration() == thisGen && i < static_cast<int>(weakThis->m_allRecords.size())) {
                        if (weakThis->m_allRecords[i].path == targetPath) {
                            weakThis->m_allRecords[i].width = sz.width();
                            weakThis->m_allRecords[i].height = sz.height();
                            emit weakThis->dataChanged(weakThis->index(i, 3), weakThis->index(i, 3));
                        }
                    }
                }, Qt::QueuedConnection);
            }
        }
    });
}
=======
void DiskItemModel::preloadDimensionsAsync() {
    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);

    std::vector<SizeTarget> targets;
    targets.reserve(m_allRecords.size());
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        const auto& rec = m_allRecords[i];
        if (!rec.isDir && rec.width == 0 && UiHelper::isGraphicsFile(rec.suffix)) {
            targets.push_back({i, rec.path, rec.suffix});
        }
    }
    if (targets.empty()) return;

    QPointer<DiskItemModel> weakThis(this);

    thumbnailPool()->start([weakThis, targets = std::move(targets), thisGen]() {
        if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

        // 1. 内存批量收集尺寸（避免每张图都写盘）
        std::unordered_map<std::wstring, std::pair<int, int>> dimMap;
        std::vector<std::pair<QString, QSize>> resolvedSizes;

        for (const auto& target : targets) {
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

            QSize sz = DiskMediaExtractor::fastExtractImageSize(target.path);
            if (sz.isValid() && sz.width() > 0) {
                QFileInfo fi(target.path);
                dimMap[fi.fileName().toStdWString()] = {sz.width(), sz.height()};
                resolvedSizes.push_back({target.path, sz});
            }
        }

        if (dimMap.empty() || !weakThis || weakThis->currentGeneration() != thisGen) return;

        // 2. 一次性批量落盘（仅写 1 次 JSON！）
        QFileInfo firstFi(targets.front().path);
        QString parentDir = QDir::toNativeSeparators(firstFi.absolutePath());

        static std::mutex s_jsonSaveMutex;
        {
            std::lock_guard<std::mutex> lock(s_jsonSaveMutex);
            QuarkMetaJson jsonCache(parentDir.toStdWString());
            jsonCache.load();
            auto& cachedItems = jsonCache.items();

            for (const auto& [fileName, dims] : dimMap) {
                if (cachedItems.find(fileName) == cachedItems.end()) {
                    ItemMeta emptyMeta;
                    emptyMeta.type = L"file";
                    cachedItems[fileName] = emptyMeta;
                }
                auto& fileMeta = cachedItems[fileName];
                fileMeta.width = dims.first;
                fileMeta.height = dims.second;
            }
            jsonCache.save(); // 全批次合并为 1 次原子落盘
        }

        // 3. 回到主线程通过 m_pathToIndex 精准更新并刷新表格
        QMetaObject::invokeMethod(weakThis.data(), [weakThis, resolvedSizes = std::move(resolvedSizes), thisGen]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen) return;

            for (const auto& item : resolvedSizes) {
                const QString& path = item.first;
                const QSize& sz = item.second;

                auto it = weakThis->m_pathToIndex.find(path);
                if (it != weakThis->m_pathToIndex.end()) {
                    int rIdx = it->second;
                    if (rIdx >= 0 && rIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                        auto& rec = weakThis->m_allRecords[rIdx];
                        rec.width = sz.width();
                        rec.height = sz.height();
                        weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = (double)sz.width() / sz.height();
                        // 刷新整行 (包含第 3 列尺寸文本和自适应卡片)
                        emit weakThis->dataChanged(
                            weakThis->index(rIdx, 0),
                            weakThis->index(rIdx, weakThis->columnCount() - 1),
                            {Qt::DisplayRole, AspectRatioRole}
                        );
                    }
                }
            }
        }, Qt::QueuedConnection);
    });
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void DiskItemModel::reloadThumbnailForPath(const QString& path) {
    QString nPath = QDir::toNativeSeparators(path);
    m_iconCache.remove(nPath);
    m_iconCache.remove(path);
    m_aspectRatios.remove(nPath);
    m_requestedPaths.remove(nPath);
    m_requestedPaths.remove(path);

    auto it = m_pathToIndex.find(nPath);
    if (it != m_pathToIndex.end()) {
        int rIdx = it->second;
        // 重新异步加载该行的缩略图
        loadThumbnailsForRows({rIdx});
        emit dataChanged(index(rIdx, 0), index(rIdx, 0), {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
    }
}
=======
void DiskItemModel::reloadThumbnailForPath(const QString& path) {
    QString nPath = QDir::toNativeSeparators(path);
    m_iconCache.remove(nPath);
    m_iconCache.remove(path);
    m_aspectRatios.remove(nPath);
    m_requestedPaths.remove(nPath);
    m_requestedPaths.remove(path);

    auto it = m_pathToIndex.find(nPath);
    if (it != m_pathToIndex.end()) {
        int rIdx = it->second;
        // 重新异步加载该行的缩略图
        loadThumbnailsForRows({rIdx});
        emit dataChanged(
            index(rIdx, 0),
            index(rIdx, columnCount() - 1),
            {Qt::DecorationRole, Qt::DisplayRole, AspectRatioRole, HasThumbnailRole}
        );
    }
}
>>>>>>> REPLACE

---

## Build & Verification Steps

1. **Compilation Check**:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --config Debug
   ```

2. **Functional Verification**:
   - Navigate into a directory containing PSD, AI, EPS, and SVG files.
   - Verify Column 3 ("尺寸") instantly populates real dimensions for all professional design formats.
   - Verify that `.QuarkMeta.json` is written exactly once for the entire batch.
   - Right-click an item and select "重新提取缩略图", verifying Column 3 ("尺寸") refreshes live.
