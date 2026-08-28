# ThumbnailPipelineService Implementation Plan

## 1. Overview
This implementation plan establishes `ThumbnailPipelineService` (`src/util/ThumbnailPipelineService.h/cpp`) as the single source of truth for high-concurrency image thumbnail processing.
It implements a 3-level cache architecture ("Level-1 Memory LRU -> Level-2 Disk Hash Storage -> Level-3 Parallel Lockless Async Decoder") paired with atomic generation fused-task cancellation (`std::atomic<uint64_t> generationId`) to guarantee 60 FPS smooth scrolling in `ContentPanel`.

---

## 2. Modified Files List
- `src/util/ThumbnailPipelineService.h` *(New)*
- `src/util/ThumbnailPipelineService.cpp` *(New)*
- `src/ui/ContentPanel.cpp` *(Modified)*
- `CMakeLists.txt` *(Modified)*

---

## 3. Detailed Line-by-Line Changes

### 3.1 Create `src/util/ThumbnailPipelineService.h`
```cpp
#pragma once

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QCache>
#include <QMutex>
#include <atomic>
#include <functional>

namespace QuarkMeta {

class ThumbnailPipelineService : public QObject {
    Q_OBJECT

public:
    static ThumbnailPipelineService& instance();

    QPixmap getFromMemoryCache(const QString& filePath, int targetSize) const;

    void loadBatchAsync(const QStringList& filePaths, 
                        int targetSize, 
                        std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded);

    void incrementGeneration();
    void cancelAll();

    static QString getDiskCachePath(const QString& filePath, int targetSize);
    void clearMemoryCache();

private:
    explicit ThumbnailPipelineService(QObject* parent = nullptr);
    ~ThumbnailPipelineService() override = default;
    ThumbnailPipelineService(const ThumbnailPipelineService&) = delete;
    ThumbnailPipelineService& operator=(const ThumbnailPipelineService&) = delete;

    QImage decodeImageToThumbnail(const QString& filePath, int targetSize) const;

    mutable QMutex m_cacheMutex;
    mutable QCache<QString, QPixmap> m_memoryCache;

    std::atomic<uint64_t> m_currentGeneration{1};
    static constexpr int kMaxMemoryCacheCount = 800;
};

} // namespace QuarkMeta
```

### 3.2 Create `src/util/ThumbnailPipelineService.cpp`
```cpp
#include "ThumbnailPipelineService.h"
#include "ColorPaletteEngine.h"
#include <QImageReader>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QtConcurrent>
#include <QMutexLocker>

namespace QuarkMeta {

ThumbnailPipelineService& ThumbnailPipelineService::instance() {
    static ThumbnailPipelineService s_instance;
    return s_instance;
}

ThumbnailPipelineService::ThumbnailPipelineService(QObject* parent)
    : QObject(parent) {
    m_memoryCache.setMaxCost(kMaxMemoryCacheCount);
}

QString ThumbnailPipelineService::getDiskCachePath(const QString& filePath, int targetSize) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QByteArray hash = QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex();
    
    QString cacheDir = QDir::temp().filePath("QuarkMeta_Thumbnails");
    QDir().mkpath(cacheDir);

    return QDir(cacheDir).filePath(QString("%1_%2.png").arg(QString(hash.left(32))).arg(targetSize));
}

QPixmap ThumbnailPipelineService::getFromMemoryCache(const QString& filePath, int targetSize) const {
    QString key = QString("%1@%2").arg(QDir::toNativeSeparators(filePath).toLower()).arg(targetSize);
    QMutexLocker locker(&m_cacheMutex);
    QPixmap* cached = m_memoryCache.object(key);
    if (cached && !cached->isNull()) {
        return *cached;
    }
    return QPixmap();
}

void ThumbnailPipelineService::clearMemoryCache() {
    QMutexLocker locker(&m_cacheMutex);
    m_memoryCache.clear();
}

void ThumbnailPipelineService::incrementGeneration() {
    m_currentGeneration.fetch_add(1, std::memory_order_relaxed);
}

void ThumbnailPipelineService::cancelAll() {
    incrementGeneration();
}

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

void ThumbnailPipelineService::loadBatchAsync(const QStringList& filePaths, 
                                              int targetSize, 
                                              std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded) {
    if (filePaths.isEmpty()) return;

    uint64_t taskGen = m_currentGeneration.load(std::memory_order_relaxed);

    QStringList pathsToFetch;
    for (const QString& path : filePaths) {
        QPixmap memPix = getFromMemoryCache(path, targetSize);
        if (!memPix.isNull()) {
            if (onSingleLoaded) onSingleLoaded(path, memPix);
        } else {
            pathsToFetch << path;
        }
    }

    if (pathsToFetch.isEmpty()) return;

    (void)QtConcurrent::run([this, pathsToFetch, targetSize, taskGen, onSingleLoaded]() {
        for (const QString& path : pathsToFetch) {
            if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                return;
            }

            QString diskPath = getDiskCachePath(path, targetSize);
            QImage finalImg;

            if (QFile::exists(diskPath)) {
                finalImg.load(diskPath);
            }

            if (finalImg.isNull()) {
                finalImg = decodeImageToThumbnail(path, targetSize);
                if (!finalImg.isNull()) {
                    finalImg.save(diskPath, "PNG");
                }
            }

            if (!finalImg.isNull()) {
                if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                    return;
                }

                QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                    if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                        return;
                    }

                    QPixmap pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }

                        if (onSingleLoaded) {
                            onSingleLoaded(path, pix);
                        }
                    }
                }, Qt::QueuedConnection);
            }
        }
    });
}

} // namespace QuarkMeta
```

### 3.3 Update `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
void ContentPanel::refreshVisibleThumbnails() {
=======
#include "../util/ThumbnailPipelineService.h"

void ContentPanel::refreshVisibleThumbnails() {
    QWidget* current = m_viewStack->currentWidget();
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(current);
    if (!view || !m_model || CoreController::isShuttingDown()) return;

    int top = 0;
    int bottom = m_proxyModel->rowCount() - 1;

    QModelIndex topIdx = view->indexAt(QPoint(10, 10));
    QModelIndex bottomIdx = view->indexAt(QPoint(view->viewport()->width() - 10, view->viewport()->height() - 10));

    if (topIdx.isValid()) top = topIdx.row();
    if (bottomIdx.isValid()) bottom = bottomIdx.row();

    top = std::max(0, top - 2);
    bottom = std::min(m_proxyModel->rowCount() - 1, bottom + 2);

    QStringList visiblePaths;
    for (int r = top; r <= bottom; ++r) {
        QModelIndex proxyIdx = m_proxyModel->index(r, 0);
        QString path = proxyIdx.data(PathRole).toString();
        if (!path.isEmpty()) {
            visiblePaths << path;
        }
    }

    QPointer<ContentPanel> weakThis(this);
    ThumbnailPipelineService::instance().loadBatchAsync(
        visiblePaths, 
        m_zoomLevel, 
        [weakThis](const QString& path, const QPixmap&) {
            if (weakThis) {
                weakThis->updateItemMetadata(path);
            }
        }
    );
>>>>>>> REPLACE
```

### 3.4 Update `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/util/ColorPaletteEngine.h
    src/util/ColorPaletteEngine.cpp
=======
    src/util/ColorPaletteEngine.h
    src/util/ColorPaletteEngine.cpp
    src/util/ThumbnailPipelineService.h
    src/util/ThumbnailPipelineService.cpp
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `QuarkMeta-Architecture-Planning.md` contains the updated `ThumbnailPipelineService` architecture specification.
2. Verify `ThumbnailPipelineService.md` is strictly created under `QuarkMeta Architecture/Implementation Plan/` with precise 1:1 class name mapping.
3. Run pre-commit instructions checks and submit.
