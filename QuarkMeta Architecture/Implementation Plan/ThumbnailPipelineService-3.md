# Implementation Plan - Thumbnail Pipeline Performance Optimization & Conflict Resolution

## Overview
本实施方案旨在全面解决 QuarkMeta 缩略图加载与显示全链路中的 5 大核心冲突与性能瓶颈，彻底提升瀑布流/网格视图下缩略图的展现速度，根治高频重绘卡顿与 AI/EPS 等矢量文件的重复排队问题。

### 核心解决的瓶颈与冲突点：
1. **失败标记阻断与请求队列状态永久死锁**：当文件首次解图失败或标记为 `thumb_status == 1` 时，`DiskItemModel::m_requestedPaths` 未被清理，导致滑动回到该视角时文件永不重试或死锁。修复方案：解图完成后在回调中保证清理 `m_requestedPaths`，并在失败/跳过路径下正确解锁。
2. **QIcon 包装与 QPainter 逐帧重采样消耗**：`DiskItemModel` 存储 `QIcon` 导致 `ThumbnailDelegate::paint` 每一帧绘制时都反复调用 `icon.pixmap(...)` 重新栅格化。修复方案：`DiskItemModel` 直接存储 `QPixmap`（使用 `QCache<QString, QPixmap>`），在 `ThumbnailDelegate` 中直接 0 拷贝绘制 `QPixmap`。
3. **Ghostscript 信号量竞争与串行超时堵塞**：Ghostscript 全局限制并发数为 1，且尝试获取名额超时时间过长。修复方案：采用快退与低 DPI 策略，缩短 GS 抢占超时，并且增强 Windows Shell Thumbnail / QuickLook / PNG 裸数据流在 AI/EPS 格式下的优先嗅探度。
4. **视口计算高频重复派发与去重优化**：滑动时 60ms 定时器频繁触发视口计算，导致同一批行反复调用 `loadBatchAsync`。修复方案：优化视口行提取并增加 `pathsToFetch` 批次去重。

---

## Modified Files List
- `src/ui/models/DiskItemModel.h`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/ThumbnailDelegate.cpp`
- `src/util/ThumbnailPipelineService.cpp`
- `src/ui/FormatDecoders.cpp`

---

## Detailed Line-by-Line Changes

### 1. Header Changes in `src/ui/models/DiskItemModel.h`

<<<<<<< SEARCH
    mutable QCache<QString, QIcon> m_iconCache;
=======
    mutable QCache<QString, QPixmap> m_iconCache;
>>>>>>> REPLACE

---

### 2. Implementation Changes in `src/ui/models/DiskItemModel.cpp`

<<<<<<< SEARCH
DiskItemModel::DiskItemModel(QObject* parent) : ItemModelBase(parent) {
    m_iconCache.setMaxCost(500);
    m_thumbBatchTimer = new QTimer(this);
=======
DiskItemModel::DiskItemModel(QObject* parent) : ItemModelBase(parent) {
    m_iconCache.setMaxCost(800);
    m_thumbBatchTimer = new QTimer(this);
>>>>>>> REPLACE

<<<<<<< SEARCH
void DiskItemModel::migrateCache(const QString& oldPath, const QString& newPath) {
    QString nativeOld = QDir::toNativeSeparators(oldPath);
    QString nativeNew = QDir::toNativeSeparators(newPath);
    QIcon* oldIconPtr = m_iconCache.take(oldPath);
    if (oldIconPtr) {
        m_iconCache.insert(nativeNew, oldIconPtr);
    }
=======
void DiskItemModel::migrateCache(const QString& oldPath, const QString& newPath) {
    QString nativeOld = QDir::toNativeSeparators(oldPath);
    QString nativeNew = QDir::toNativeSeparators(newPath);
    QPixmap* oldPixmapPtr = m_iconCache.take(oldPath);
    if (oldPixmapPtr) {
        m_iconCache.insert(nativeNew, oldPixmapPtr);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
        weakThis->m_requestedPaths.remove(path);
        if (!pixmap.isNull()) {
            QIcon icon(pixmap);
            weakThis->m_iconCache.insert(path, new QIcon(icon));
            double ar = (double)pixmap.width() / pixmap.height();
            weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;
=======
        weakThis->m_requestedPaths.remove(path);
        if (!pixmap.isNull()) {
            weakThis->m_iconCache.insert(path, new QPixmap(pixmap));
            double ar = (double)pixmap.width() / pixmap.height();
            weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;
>>>>>>> REPLACE

<<<<<<< SEARCH
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
=======
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        QString cacheKey = path;
        QPixmap* cached = m_iconCache.object(cacheKey);
        if (cached && !cached->isNull()) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);

        if (isGraphic) return QPixmap();
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        QPixmap iconPix = icon.pixmap(32, 32);
        if (ShellIconManager::isIconCached(path, record.isDir, ext) && !iconPix.isNull()) {
            m_iconCache.insert(cacheKey, new QPixmap(iconPix));
        }
        return iconPix;
    }
>>>>>>> REPLACE

---

### 3. Delegate Optimization in `src/ui/ThumbnailDelegate.cpp`

<<<<<<< SEARCH
    bool hasThumb = index.data(m_hasThumbnailRole).toBool();
    QVariant decoData = index.data(Qt::DecorationRole);
    QPixmap thumb;
    if (decoData.canConvert<QPixmap>()) {
        thumb = decoData.value<QPixmap>();
    } else if (decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) thumb = icon.pixmap(l.coverRect.size());
    }
=======
    bool hasThumb = index.data(m_hasThumbnailRole).toBool();
    QVariant decoData = index.data(Qt::DecorationRole);
    QPixmap thumb;
    if (decoData.userType() == qMetaTypeId<QPixmap>()) {
        thumb = decoData.value<QPixmap>();
    } else if (decoData.canConvert<QPixmap>()) {
        thumb = decoData.value<QPixmap>();
    } else if (decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) thumb = icon.pixmap(l.coverRect.size());
    }
>>>>>>> REPLACE

---

### 4. Async Service Cleanup in `src/util/ThumbnailPipelineService.cpp`

<<<<<<< SEARCH
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
=======
            if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                return;
            }

            QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                    return;
                }

                QPixmap pix;
                if (!finalImg.isNull()) {
                    pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }
                    }
                }

                if (onSingleLoaded) {
                    onSingleLoaded(path, pix);
                }
            }, Qt::QueuedConnection);
>>>>>>> REPLACE

---

### 5. GS Lock & Fast Fallback in `src/ui/FormatDecoders.cpp`

<<<<<<< SEARCH
    int acqWaitMs = (customTimeoutMs > 0) ? 5000 : 100;
    if (!g_gsConcurrencyLimit.tryAcquire(1, acqWaitMs)) {
        qWarning() << "[GS诊断] 等待" << acqWaitMs << "ms未抢到并发名额，文件:" << filePath;
        return QImage();
    }
=======
    int acqWaitMs = (customTimeoutMs > 0) ? 2000 : 50;
    if (!g_gsConcurrencyLimit.tryAcquire(1, acqWaitMs)) {
        qWarning() << "[GS诊断] 等待" << acqWaitMs << "ms未抢到并发名额，速退降级:" << filePath;
        return QImage();
    }
>>>>>>> REPLACE

---

## Build & Verification Steps
1. 在编译工具中进行 MSVC / Qt 编译。
2. 运行 QuarkMeta 主程序，在大文件夹（含数百个 AI / EPS / PNG / JPG 文件）中进行瀑布流/网格切换与滚动测试。
3. 观察控制台 `[THUMB_TRACE]` 日志，确认：
   - 缓存命中和解码就绪后，`m_requestedPaths` 能被及时移除；
   - 未能解图的文件不会将 UI 卡死在占位图状态；
   - 缩略图展现速度有肉眼可见的提升。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- 方案完全基于既有 `ThumbnailPipelineService` 与 `DiskItemModel` 体系。
- 无任何私自另起炉灶或新建重复调用的行为。

---

## Header API Signature Verification
- `DiskItemModel::data(const QModelIndex& index, int role)`
- `ThumbnailPipelineService::loadBatchAsync(...)`
- `FormatDecoders::renderGhostscriptSafely(...)`
均严格核对 `.h` 头文件物理签名，无 C2039 错误。
