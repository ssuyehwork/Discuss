# QuarkMeta 缩略图流水线与三级缓存实施方案 (ThumbnailPipelineService)

## 1. 目标与范围
- 新建 `ThumbnailPipelineService`（位于 `src/util/`）：建立标准化的“一级内存 LRU + 二级磁盘持久化 Hash + 三级后台无锁解码”三级缓存流水线。
- 确立原子代际熔断机制：在目录切换或高速滚动时，通过原子代际号（`generationId`）毫秒级熔断并丢弃旧队列任务，CPU/GPU 算力 100% 聚焦当前视口可见区域。
- 彻底释放 GUI 互斥锁瓶颈：后台子线程纯粹基于线程安全的 `QImageReader` / `QImage` 并行解码，切断对主线程 GUI 锁的依赖，确保 60FPS 丝滑滚动。

---

## 2. 核心模块独立实现

### 2.1 `src/util/ThumbnailPipelineService.h`
```cpp
#pragma once

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QCache>
#include <QMutex>
#include <QSet>
#include <atomic>
#include <functional>

namespace QuarkMeta {

class ThumbnailPipelineService : public QObject {
    Q_OBJECT

public:
    static ThumbnailPipelineService& instance();

    /**
     * @brief 一级内存 LRU 缓存直取 (0ms 耗时，UI 主线程安全)
     */
    QPixmap getFromMemoryCache(const QString& filePath, int targetSize) const;

    /**
     * @brief 异步按批次并发提取缩略图 (自动走三级缓存降级流水线)
     * @param filePaths 待提图的物理路径列表
     * @param targetSize 目标正方形边长像素 (如 96, 128, 230)
     * @param onSingleLoaded 单个缩略图就绪回调 (在 UI 主线程安全触发)
     */
    void loadBatchAsync(const QStringList& filePaths, 
                        int targetSize, 
                        std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded);

    /**
     * @brief 递增代际号并瞬间熔断所有正在排队的旧任务
     */
    void incrementGeneration();
    void cancelAll();

    /**
     * @brief 计算二级磁盘 Hash 缓存路径
     */
    static QString getDiskCachePath(const QString& filePath, int targetSize);

    /**
     * @brief 内存缓存清理
     */
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
    static constexpr int kMaxMemoryCacheCount = 800; // 内存最多缓存 800 张缩略图 (约 50~80MB)
};

} // namespace QuarkMeta
```

### 2.2 `src/util/ThumbnailPipelineService.cpp`
```cpp
#include "ThumbnailPipelineService.h"
#include "ColorPaletteEngine.h"
#include <QImageReader>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
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
    // 基于规范化路径与目标尺寸生成唯一 SHA-256 Hash 键名
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
        // 快速低消耗下采样解码
        QSize scaled = origSize.scaled(QSize(targetSize * 2, targetSize * 2), Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }

    QImage img = reader.read();
    if (img.isNull()) return QImage();

    // 平滑高质量缩放至目标尺寸
    return img.scaled(QSize(targetSize, targetSize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void ThumbnailPipelineService::loadBatchAsync(const QStringList& filePaths, 
                                              int targetSize, 
                                              std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded) {
    if (filePaths.isEmpty()) return;

    uint64_t taskGen = m_currentGeneration.load(std::memory_order_relaxed);

    // 过滤出未在一级内存命中的项目，已命中的直接在主线程 0ms 激发回调
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

    // 抛入后台无锁线程池执行 二级磁盘检查 与 三级多线程解码
    (void)QtConcurrent::run([this, pathsToFetch, targetSize, taskGen, onSingleLoaded]() {
        for (const QString& path : pathsToFetch) {
            // 🚀【代际即时熔断】：只要代际号改变（用户切了目录或高速滑动），秒级退出当前循环！
            if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                return;
            }

            QString diskPath = getDiskCachePath(path, targetSize);
            QImage finalImg;

            // 二级缓存：检查磁盘 Hash 缓存文件是否存在
            if (QFile::exists(diskPath)) {
                finalImg.load(diskPath);
            }

            // 三级提取：磁盘未命中，执行后台无锁解码
            if (finalImg.isNull()) {
                finalImg = decodeImageToThumbnail(path, targetSize);
                if (!finalImg.isNull()) {
                    // 异步写盘固化为二级缓存
                    finalImg.save(diskPath, "PNG");
                }
            }

            if (!finalImg.isNull()) {
                if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                    return;
                }

                // 安全切回主 UI 线程将 QImage 转换为 QPixmap 并填入一级内存
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

---

## 3. `ContentPanel.cpp` 接入与流水线调用

在 `ContentPanel.cpp` 中全面接入三级缓存流水线，彻底消除白块与卡顿：

```cpp
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

    // 🚀【三级缓存流水线接入】：仅对当前视口可见的行发起异步提图
    QPointer<ContentPanel> weakThis(this);
    ThumbnailPipelineService::instance().loadBatchAsync(
        visiblePaths, 
        m_zoomLevel, 
        [weakThis](const QString& path, const QPixmap&) {
            if (weakThis) {
                weakThis->updateItemMetadata(path); // 局部触发单个 Item 刷新绘制
            }
        }
    );
}

void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    // 🚀【目录切换代际熔断】：瞬间中断前一个目录的所有在途排队提图任务！
    ThumbnailPipelineService::instance().cancelAll();

    // ... [其余加载逻辑保持不变] ...
}
```

---

## 4. `CMakeLists.txt` 构建配置注册
```cmake
set(UTIL_SOURCES
    # ... 现有 util 源文件 ...
    src/util/ThumbnailPipelineService.h
    src/util/ThumbnailPipelineService.cpp
)
```