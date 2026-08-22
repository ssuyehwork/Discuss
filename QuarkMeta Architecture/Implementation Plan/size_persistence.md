# 图像尺寸（宽高）全链路持久化实施方案 (Image Dimensions Persistence Implementation Plan)

## Overview
目前 QuarkMeta 在解图（提取 512px 缩略图）时，丢弃了解码器解析出的图片真实物理尺寸（`originalSize`），导致 `DiskItemModel` 内存记录中 `rec.width` 与 `rec.height` 始终为 0，第 3 列（尺寸列）无法实时渲染展示且无法持久化落盘至 `.QuarkMeta.json`。

本方案旨在：
1. **改造 `DiskMediaExtractor`**：新增 `ExtractResult` 结构体，在解图时同时获取真实尺寸 `originalSize`，并在后台线程自动将尺寸数据离散落盘至当前目录下的 `.QuarkMeta.json`。
2. **改造 `DiskItemModel`**：在解图回调中回填 `rec.width` 与 `rec.height`，计算准确的宽高比，并触发 `dataChanged` 全列刷新，使第 3 列实时显示如 `3840 x 2160`。
3. **支持秒开装载**：利用已有的 `MetaCacheDecorator` 机制，后续打开浏览过的文件夹时，尺寸数据直接由 `.QuarkMeta.json` 0 毫秒装载呈现。

---

## Modified Files List
- `src/util/DiskMediaExtractor.h`
- `src/util/DiskMediaExtractor.cpp`
- `src/ui/models/DiskItemModel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/util/DiskMediaExtractor.h`
<<<<<<< SEARCH
#include <QImage>
#include <QString>
=======
#include <QImage>
#include <QSize>
#include <QString>
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 强制执行深度长效提取（不走只读缓存，超时放宽至 45 秒）
    static QImage forceExtractDeepThumbnail(const QString& filePath, int size = 512);
};
=======
    struct ExtractResult {
        QImage thumbnail512;
        QSize  originalSize; // 真实物理分辨率 (如 3840x2160)
        bool   isValid = false;
    };

    static ExtractResult getCapsuleExtractResult(const QString& filePath, int size = 512);

    // 强制执行深度长效提取（不走只读缓存，超时放宽至 45 秒）
    static QImage forceExtractDeepThumbnail(const QString& filePath, int size = 512);
};
>>>>>>> REPLACE

### 2. `src/util/DiskMediaExtractor.cpp`
<<<<<<< SEARCH
#include "../ui/ImageDecoderFacade.h"
=======
#include "../ui/ImageDecoderFacade.h"
#include "../meta/QuarkMetaJson.h"
#include <QFileInfo>
#include <QDir>
>>>>>>> REPLACE

<<<<<<< SEARCH
QImage DiskMediaExtractor::getCapsuleThumbnail(const QString& filePath, int size) {
    QImage cached = getCapsuleThumbnailReadOnly(filePath);
    if (!cached.isNull()) return cached;

    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size);
    if (dec.isValid && !dec.thumbnail512.isNull()) {
        saveDiskThumbnail(filePath, dec.thumbnail512);
        return dec.thumbnail512;
    }
    return QImage();
}
=======
DiskMediaExtractor::ExtractResult DiskMediaExtractor::getCapsuleExtractResult(const QString& filePath, int size) {
    ExtractResult res;
    res.thumbnail512 = getCapsuleThumbnailReadOnly(filePath);

    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size);
    if (dec.isValid) {
        res.originalSize = dec.originalSize;
        if (res.thumbnail512.isNull() && !dec.thumbnail512.isNull()) {
            saveDiskThumbnail(filePath, dec.thumbnail512);
            res.thumbnail512 = dec.thumbnail512;
        }
        res.isValid = true;

        if (res.originalSize.isValid() && res.originalSize.width() > 0) {
            QFileInfo fi(filePath);
            QString parentDir = QDir::toNativeSeparators(fi.absolutePath());
            QString fileName = fi.fileName();
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
            if (fileMeta.width != res.originalSize.width() || fileMeta.height != res.originalSize.height()) {
                fileMeta.width = res.originalSize.width();
                fileMeta.height = res.originalSize.height();
                jsonCache.save();
            }
        }
    } else if (!res.thumbnail512.isNull()) {
        res.isValid = true;
    }
    return res;
}

QImage DiskMediaExtractor::getCapsuleThumbnail(const QString& filePath, int size) {
    ExtractResult res = getCapsuleExtractResult(filePath, size);
    return res.thumbnail512;
}
>>>>>>> REPLACE

### 3. `src/ui/models/DiskItemModel.cpp`
<<<<<<< SEARCH
        QThreadPool::globalInstance()->start([weakThis, path, thisGen]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

            QImage img = DiskMediaExtractor::getCapsuleThumbnail(path, 512);

            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

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
                        if (weakThis->isSuspended()) {
                            weakThis->m_pendingUpdateRows.insert(rIdx);
                        } else {
                            emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, 0),
                                                      {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
                        }
                    }
=======
        QThreadPool::globalInstance()->start([weakThis, path, thisGen]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

            DiskMediaExtractor::ExtractResult res = DiskMediaExtractor::getCapsuleExtractResult(path, 512);

            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

            QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, res, thisGen]() {
                if (weakThis && weakThis->currentGeneration() == thisGen) {
                    QIcon icon = res.thumbnail512.isNull() ? ShellIconManager::getFileIcon(path, 128) : QIcon(QPixmap::fromImage(res.thumbnail512));
                    weakThis->m_iconCache.insert(path, new QIcon(icon));

                    auto it = weakThis->m_pathToIndex.find(path);
                    if (it != weakThis->m_pathToIndex.end()) {
                        int rIdx = it->second;
                        if (rIdx >= 0 && rIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                            auto& rec = weakThis->m_allRecords[rIdx];
                            if (res.originalSize.isValid() && res.originalSize.width() > 0) {
                                rec.width = res.originalSize.width();
                                rec.height = res.originalSize.height();
                                weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = (double)rec.width / rec.height;
                            } else if (!res.thumbnail512.isNull()) {
                                weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = (double)res.thumbnail512.width() / res.thumbnail512.height();
                            } else {
                                weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = -1.0;
                            }
                        }

                        weakThis->m_requestedPaths.remove(path);

                        if (weakThis->isSuspended()) {
                            weakThis->m_pendingUpdateRows.insert(rIdx);
                        } else {
                            emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, weakThis->columnCount() - 1),
                                                      {Qt::DecorationRole, Qt::DisplayRole, AspectRatioRole, HasThumbnailRole});
                        }
                    }
>>>>>>> REPLACE

---

## Build & Verification Steps
1. **Compilation Check**:
   ```bash
   cmake --build build --config Release
   ```
2. **Verification**:
   - Open a folder with images.
   - Verify Column 3 (尺寸) updates from `-` to real dimensions (e.g. `3840 x 2160`) as thumbnails finish extracting.
   - Reopen the same folder; verify dimensions display 0 ms instantly from `.QuarkMeta.json`.
