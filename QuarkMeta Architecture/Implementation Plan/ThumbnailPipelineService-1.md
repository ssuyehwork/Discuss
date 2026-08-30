# 缩略图管线与视图驱动正本清源重构实施方案

## 1. Overview（概述与解决的问题）

本实施方案旨在彻底消除 QuarkMeta 缩略图异步提取与渲染管线中的**双轨空转、格式判断碎片化、硬编码熔断与 UI 边框残留**问题。
通过确立以 `UiHelper::isGraphicsFile` 为媒体格式判断单一真理源 (SSOT)，将 `ThumbnailPipelineService` 重构为全局唯一的缩略图二级缓存与异步解码调度中枢，剥离 `DiskItemModel` 内部私有的任务分派逻辑，并由 `DiskItemModel` 统一向上承接视口请求，下接 `ThumbnailPipelineService`，构建无缝的单向数据流架构。

---

## 2. Modified Files List（影响文件清单）

1. `src/util/ThumbnailPipelineService.h`
2. `src/util/ThumbnailPipelineService.cpp`
3. `src/ui/models/DiskItemModel.h`
4. `src/ui/models/DiskItemModel.cpp`
5. `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/util/ThumbnailPipelineService.cpp`

在 `ThumbnailPipelineService::decodeImageToThumbnail` 中，统一接入底层 `DiskMediaExtractor` 成果：

```
<<<<<<< SEARCH
QImage ThumbnailPipelineService::decodeImageToThumbnail(const QString& filePath, int targetSize) const {
    if (!ColorPaletteEngine::isGraphicsFile(QFileInfo(filePath).suffix())) {
        return QImage();
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    reader.setDecideFormatFromContent(true);

    QSize origSize = reader.size();
    if (origSize.isValid() && origSize.width() > 0 && origSize.height() > 0) {
        QSize scaled = origSize.scaled(QSize(targetSize * 2, targetSize * 2), Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }

    QImage img = reader.read();
    if (img.isNull()) return QImage();

    return img.scaled(QSize(targetSize, targetSize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
=======
QImage ThumbnailPipelineService::decodeImageToThumbnail(const QString& filePath, int targetSize) const {
    if (!ColorPaletteEngine::isGraphicsFile(QFileInfo(filePath).suffix())) {
        return QImage();
    }

    return DiskMediaExtractor::getCapsuleThumbnail(filePath, targetSize);
}
>>>>>>> REPLACE
```

### 3.2 `src/ui/models/DiskItemModel.cpp`

#### (1) 在 `DiskItemModel::loadThumbnailsForRows` 中剥离重复派发逻辑，委托给 `ThumbnailPipelineService`

```
<<<<<<< SEARCH
void DiskItemModel::loadThumbnailsForRows(const QList<int>& rows) {
    if (rows.isEmpty() || CoreController::isShuttingDown()) return;

    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);

    QStringList pathsToLoad;
    for (int r : rows) {
        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        const auto& rec = m_allRecords[r];

        QString ext = rec.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext) || ext == "svg" || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf";
        if (rec.isDir || !isGraphic) continue;

        QString path = rec.path;
        if (m_iconCache.contains(path) || m_requestedPaths.contains(path)) continue;

        m_requestedPaths.insert(path);
        pathsToLoad << path;
    }

    if (pathsToLoad.isEmpty()) return;

    QPointer<DiskItemModel> weakThis(this);

    std::shared_ptr<CancellationToken> token;
    {
        QMutexLocker locker(&m_genTokenMutex);
        auto it = m_genTokens.find(thisGen);
        if (it != m_genTokens.end()) {
            token = it.value();
        } else {
            token = std::make_shared<CancellationToken>();
            m_genTokens[thisGen] = token;
        }
    }

    for (const QString& path : pathsToLoad) {
        QFileInfo fi(path);
        QString ext = fi.suffix().toLower();
        int priority = (ext == "ai" || ext == "eps" || ext == "pdf") ? -10 : 0;

        thumbnailPool()->start([weakThis, path, thisGen, token]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown() || (token && token->isCanceled())) return;

            QImage img = DiskMediaExtractor::getCapsuleThumbnail(path, 512, token);

            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown() || (token && token->isCanceled())) return;

            double ar = 1.0;
            bool hasThumb = false;
            if (!img.isNull()) {
                ar = (double)img.width() / img.height();
                hasThumb = true;
            }

            QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, img, ar, hasThumb, thisGen]() {
                if (weakThis && weakThis->currentGeneration() == thisGen) {
                    QIcon icon = img.isNull() ? ShellIconManager::getFileIcon(path, 128) : QIcon(QPixmap::fromImage(img));
                    weakThis->m_iconCache.insert(path, new QIcon(icon));
                    weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = hasThumb ? ar : -1.0;
                    weakThis->m_requestedPaths.remove(path);

                    auto it = weakThis->m_pathToIndex.find(path);
                    if (it != weakThis->m_pathToIndex.end()) {
                        int rIdx = it->second;
                        emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, 0), 
                                                  {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
                        emit weakThis->thumbnailLoaded(rIdx);
                    }
                }
            }, Qt::QueuedConnection);
        }, priority);
    }
}
=======
void DiskItemModel::loadThumbnailsForRows(const QList<int>& rows) {
    if (rows.isEmpty() || CoreController::isShuttingDown()) return;

    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);

    QStringList pathsToLoad;
    for (int r : rows) {
        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        const auto& rec = m_allRecords[r];

        QString ext = rec.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        if (rec.isDir || !isGraphic) continue;

        QString path = rec.path;
        if (m_iconCache.contains(path) || m_requestedPaths.contains(path)) continue;

        m_requestedPaths.insert(path);
        pathsToLoad << path;
    }

    if (pathsToLoad.isEmpty()) return;

    QPointer<DiskItemModel> weakThis(this);
    ThumbnailPipelineService::instance().loadBatchAsync(pathsToLoad, 230, [weakThis, thisGen](const QString& path, const QPixmap& pixmap) {
        if (!weakThis || weakThis->currentGeneration() != thisGen) return;

        weakThis->m_requestedPaths.remove(path);
        if (!pixmap.isNull()) {
            QIcon icon(pixmap);
            weakThis->m_iconCache.insert(path, new QIcon(icon));
            double ar = (double)pixmap.width() / pixmap.height();
            weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;

            auto it = weakThis->m_pathToIndex.find(path);
            if (it != weakThis->m_pathToIndex.end()) {
                int rIdx = it->second;
                emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, 0), 
                                          {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
                emit weakThis->thumbnailLoaded(rIdx);
            }
        }
    });
}
>>>>>>> REPLACE
```

#### (2) 统一收敛 `DiskItemModel::data` 中格式硬编码

```
<<<<<<< SEARCH
    } else if (role == HasThumbnailRole) {
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        QString ext = record.suffix.toLower();
        if (iconOnlyExts.contains(ext)) return false;
        if (UiHelper::isGraphicsFile(ext) || ext == "svg" || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf") return true;
        if (record.width > 0 && record.height > 0) return true;
        return m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0;
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        QString cacheKey = path;
        QIcon* cached = m_iconCache.object(cacheKey);
        if (cached) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext) || ext == "svg" || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf";
        
        if (isGraphic) return QIcon();
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        if (ShellIconManager::isIconCached(path, record.isDir, ext)) {
            m_iconCache.insert(cacheKey, new QIcon(icon));
        }
        return icon;
    }
=======
    } else if (role == HasThumbnailRole) {
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        QString ext = record.suffix.toLower();
        if (iconOnlyExts.contains(ext)) return false;
        if (UiHelper::isGraphicsFile(ext)) return true;
        if (record.width > 0 && record.height > 0) return true;
        return m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0;
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        QString cacheKey = path;
        QIcon* cached = m_iconCache.object(cacheKey);
        if (cached) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        
        if (isGraphic) return QIcon();
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        if (ShellIconManager::isIconCached(path, record.isDir, ext)) {
            m_iconCache.insert(cacheKey, new QIcon(icon));
        }
        return icon;
    }
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.cpp`

移除 `refreshVisibleThumbnails()` 中对 `ThumbnailPipelineService` 的冗余双轨调用：

```
<<<<<<< SEARCH
void ContentPanel::refreshVisibleThumbnails() {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    if (!view || !m_model || CoreController::isShuttingDown()) return;

    int top = 0, bottom = m_proxyModel->rowCount() - 1;
    QModelIndex topIdx = view->indexAt(QPoint(10, 10));
    QModelIndex btmIdx = view->indexAt(QPoint(view->viewport()->width() - 10, view->viewport()->height() - 10));
    if (topIdx.isValid()) top = qMax(0, topIdx.row() - 4);
    if (btmIdx.isValid()) bottom = qMin(m_proxyModel->rowCount() - 1, btmIdx.row() + 4);

    QList<int> visibleRows;
    QStringList visiblePaths;
    for (int r = top; r <= bottom; ++r) {
        QModelIndex proxyIdx = m_proxyModel->index(r, 0);
        QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        if (srcIdx.isValid()) visibleRows.append(srcIdx.row());
        QString p = proxyIdx.data(PathRole).toString();
        if (!p.isEmpty()) visiblePaths << p;
    }

    QPointer<ContentPanel> weakThis(this);
    ThumbnailPipelineService::instance().loadBatchAsync(visiblePaths, m_zoomLevel, [weakThis](const QString& path, const QPixmap&) {
        if (weakThis) weakThis->updateItemMetadata(path);
    });
    m_model->loadThumbnailsForRows(visibleRows);
}
=======
void ContentPanel::refreshVisibleThumbnails() {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    if (!view || !m_model || CoreController::isShuttingDown()) return;

    int top = 0, bottom = m_proxyModel->rowCount() - 1;
    QModelIndex topIdx = view->indexAt(QPoint(10, 10));
    QModelIndex btmIdx = view->indexAt(QPoint(view->viewport()->width() - 10, view->viewport()->height() - 10));
    if (topIdx.isValid()) top = qMax(0, topIdx.row() - 4);
    if (btmIdx.isValid()) bottom = qMin(m_proxyModel->rowCount() - 1, btmIdx.row() + 4);

    QList<int> visibleRows;
    for (int r = top; r <= bottom; ++r) {
        QModelIndex proxyIdx = m_proxyModel->index(r, 0);
        QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        if (srcIdx.isValid()) visibleRows.append(srcIdx.row());
    }

    m_model->loadThumbnailsForRows(visibleRows);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   在 Linux / Windows MSVC 环境中，运行以下构建脚本，确保无任何 MOC、C++ 语法与 unresolved symbol 错误：
   ```bash
   cmake --build build --config Release
   ```
2. **缩略图加载验证**：
   打开包含 SVG, PSD, AI, EPS, JPG, PNG 的图片文件夹，验证：
   - 视图中所有格式项目均能流畅异步加载出缩略图；
   - 快速上下滚动时不出现空转卡顿与内存爆满现象；
   - 切换 GridView / ListView 模式缩略图保持稳定显示，界面边框无 1px 亮线。
