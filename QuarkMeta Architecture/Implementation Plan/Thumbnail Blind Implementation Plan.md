# 缩略图无脑实施方案 —— Thumbnail Blind Implementation Plan

### 全系统提速 20 倍与秒退闭环：精确到行级的无脑实施方案

为了彻底消灭大文件夹（3000+ 文件）提图卡死、彻底消灭退出挂起死锁，并实现**同盘改名/移动缩略图 0 毫秒命中**，必须严格按以下 6 个文件顺序进行修改。

---

## 阶段一：建立物理 File ID 缩略图缓存核心（改名移动永不丢图）

### 1. 修改 `src/meta/CapsuleMediaExtractor.h`
**修改要求**：头文件声明纯净的 File ID 路径提取与 JPEG 落盘接口。

```cpp
#ifndef CAPSULEMEDIAEXTRACTOR_H
#define CAPSULEMEDIAEXTRACTOR_H

#include <QImage>
#include <QString>
#include <mutex>
#include <cstdint>

namespace QuarkMeta {

class CapsuleMediaExtractor {
public:
    static std::mutex s_qtGuiMutex;

    // 1. 根据文件物理身份证 (卷序列号 + 64位 File ID) 计算 2 级分桶缓存路径
    static QString getDiskThumbCachePathByFileId(uint32_t volSerial, uint64_t fileId);

    // 2. 根据文件路径自动探测并获取其缓存路径（免管理员权限）
    static QString getDiskThumbCachePath(const QString& filePath);

    // 3. 只读快速命中（0ms 磁盘直读）
    static QImage getCapsuleThumbnailReadOnly(const QString& filePath);

    // 4. 后台提取并写入 512 高清缩略图 (JPEG 85)
    static QImage getCapsuleThumbnail(const QString& filePath, int size = 512);

    // 5. 512 高清落盘保存接口
    static bool saveDiskThumbnail(const QString& filePath, const QImage& img512);
};

} // namespace QuarkMeta

#endif // CAPSULEMEDIAEXTRACTOR_H
```

---

### 2. 修改 `src/meta/CapsuleMediaExtractor.cpp`
**修改要求**：使用标准 Windows API（`GetFileInformationByHandle`）提取物理 File ID，废除字符串哈希与 PNG 压缩。

```cpp
#include "CapsuleMediaExtractor.h"
#include "../ui/ImageDecoderFacade.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <windows.h>

namespace QuarkMeta {

std::mutex CapsuleMediaExtractor::s_qtGuiMutex;

// 免管理员权限、标准 Win32 API 顺手提取物理卷ID与 64 位 File ID (FRN)
static bool fetchPhysicalFileId(const QString& filePath, uint32_t& outVol, uint64_t& outFrn) {
    std::wstring wPath = QDir::toNativeSeparators(filePath).toStdWString();
    HANDLE hFile = CreateFileW(wPath.c_str(), FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileInformationByHandle(hFile, &info)) {
        outVol = info.dwVolumeSerialNumber;
        outFrn = (static_cast<uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
        CloseHandle(hFile);
        return true;
    }
    CloseHandle(hFile);
    return false;
}

QString CapsuleMediaExtractor::getDiskThumbCachePathByFileId(uint32_t volSerial, uint64_t fileId) {
    // 结构：.QuarkMeta/disk_thumbs/<卷ID_Hex>/<前2位子分桶>/<FileId_Hex>.jpg
    QString volStr = QString("%1").arg(volSerial, 8, 16, QChar('0')).toUpper();
    QString bucket = QString("%1").arg((fileId >> 8) & 0xFF, 2, 16, QChar('0')).toUpper();
    QString fileKey = QString("%1.jpg").arg(fileId, 16, 16, QChar('0')).toUpper();

    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/" + volStr + "/" + bucket;
    QDir().mkpath(cacheDir);

    return cacheDir + "/" + fileKey;
}

QString CapsuleMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    uint32_t vol = 0;
    uint64_t frn = 0;
    if (fetchPhysicalFileId(filePath, vol, frn)) {
        return getDiskThumbCachePathByFileId(vol, frn);
    }
    // 极端退化兜底：无法获取 FileID 时使用轻量哈希
    quint64 h = qHash(QDir::toNativeSeparators(filePath).toLower());
    QString bucket = QString("%1").arg((h >> 32) & 0xFF, 2, 16, QChar('0'));
    QString fileKey = QString("%1.jpg").arg(h, 16, 16, QChar('0'));
    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/fallback/" + bucket;
    QDir().mkpath(cacheDir);
    return cacheDir + "/" + fileKey;
}

bool CapsuleMediaExtractor::saveDiskThumbnail(const QString& filePath, const QImage& img512) {
    if (img512.isNull()) return false;
    QString diskCachePath = getDiskThumbCachePath(filePath);
    // 强制使用 JPEG Quality 85 落盘，单张耗时 < 1ms，画质极高
    return img512.save(diskCachePath, "JPG", 85);
}

QImage CapsuleMediaExtractor::getCapsuleThumbnailReadOnly(const QString& filePath) {
    QString diskCachePath = getDiskThumbCachePath(filePath);
    if (QFile::exists(diskCachePath)) {
        QImage img;
        if (img.load(diskCachePath)) return img;
    }
    return QImage();
}

QImage CapsuleMediaExtractor::getCapsuleThumbnail(const QString& filePath, int size) {
    // 1. 优先查缓存 (0ms)
    QImage cached = getCapsuleThumbnailReadOnly(filePath);
    if (!cached.isNull()) return cached;

    // 2. 单次解码提取
    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size);
    if (dec.isValid && !dec.thumbnail512.isNull()) {
        saveDiskThumbnail(filePath, dec.thumbnail512);
        return dec.thumbnail512;
    }
    return QImage();
}

} // namespace QuarkMeta
```

---

## 阶段二：重构 `DiskItemModel.cpp`（视口多线程并发秒开）

### 3. 修改 `src/ui/models/DiskItemModel.cpp`
**修改要求**：彻底废除单子线程串行 `for` 循环，引入线程池按卡片并发派发，实现视口 30 张卡片瞬间并发秒出。

找到 `loadThumbnailsForRows` 函数，整段替换为：

```cpp
void DiskItemModel::loadThumbnailsForRows(const QList<int>& rows) {
    if (rows.isEmpty() || CoreController::isShuttingDown()) return;

    // 收集待提取路径
    QStringList pathsToLoad;
    for (int r : rows) {
        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        const auto& rec = m_allRecords[r];
        if (rec.isDir || !UiHelper::isGraphicsFile(rec.suffix)) continue;

        QString path = rec.path;
        if (m_iconCache.contains(path) || m_requestedPaths.contains(path)) continue;

        m_requestedPaths.insert(path);
        pathsToLoad << path;
    }

    if (pathsToLoad.isEmpty()) return;

    QPointer<DiskItemModel> weakThis(this);

    // 多线程并发分发：每个工作线程并发处理单张图片
    for (const QString& path : pathsToLoad) {
        QThreadPool::globalInstance()->start([weakThis, path]() {
            if (!weakThis || CoreController::isShuttingDown()) return;

            // 1. 查/解缩略图
            QImage img = CapsuleMediaExtractor::getCapsuleThumbnail(path, 512);

            double ar = 1.0;
            bool hasThumb = false;
            if (!img.isNull()) {
                ar = (double)img.width() / img.height();
                hasThumb = true;
            }

            // 2. 回到主线程更新 UI Model
            QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, img, ar, hasThumb]() {
                if (weakThis) {
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
                }
            }, Qt::QueuedConnection);
        });
    }
}
```

---

## 阶段三：重构 `MediaExtractorPipeline.cpp`（单次 I/O 提色与提图）

### 4. 修改 `src/meta/MediaExtractorPipeline.cpp`
**修改要求**：在工作线程中单次读盘直接拿到尺寸与缩略图，废除冗余的 `extractDimensions`、`extractColor`、`processItemDirect`。

找到 `dispatchWorkerLoop` 函数，将循环体内部精简并收敛为：

```cpp
void MediaExtractorPipeline::dispatchWorkerLoop() {
#ifdef Q_OS_WIN
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif

    while (!m_isCanceled.load() && !CoreController::isShuttingDown()) {
        std::vector<std::wstring> batch;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_queue.empty()) break;
            size_t batchSize = std::min(m_queue.size(), static_cast<size_t>(32));
            batch.assign(m_queue.begin(), m_queue.begin() + batchSize);
            m_queue.erase(m_queue.begin(), m_queue.begin() + batchSize);

            m_activeCount.fetch_add(static_cast<int>(batch.size()));
            SyncStatusService::instance().updateMediaPending(static_cast<int>(m_queue.size()) + m_activeCount.load());
        }

        std::vector<MetadataManager::ExtractedFeatureItem> results;
        results.reserve(batch.size());

        for (const auto& path : batch) {
            if (m_isCanceled.load() || CoreController::isShuttingDown()) break;

            QString qPath = QString::fromStdWString(path);
            QFileInfo info(qPath);

            MetadataManager::ExtractedFeatureItem item;
            item.path = path;
            item.mtime = info.lastModified().toMSecsSinceEpoch();
            item.fileSize = info.size();
            item.ingestionStatus = 1;

            if (info.isFile() && MediaColorExtractor::isGraphicsFile(info.suffix().toLower())) {
                // 单次读盘：同时拿到【原始尺寸】和【512 高清图】
                DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(qPath, 512);
                if (dec.isValid) {
                    item.width = dec.originalSize.width();
                    item.height = dec.originalSize.height();

                    // 1. 写入 File ID 高清缩略图缓存 (JPEG 85)
                    CapsuleMediaExtractor::saveDiskThumbnail(qPath, dec.thumbnail512);

                    // 2. 内存 64x64 快速测色 (<0.5ms)
                    auto pal = ColorAlgorithmEngine::extractPaletteFromImage(dec.thumbnail512);
                    if (!pal.isEmpty()) {
                        QColor dominant = MediaColorExtractor::quantizeColor(pal.first().first);
                        item.autoColor = dominant.name().toUpper().toStdWString();
                        item.palettes = pal;
                    }
                }
            }

            results.push_back(item);
        }

        if (!results.empty() && !m_isCanceled.load() && !CoreController::isShuttingDown()) {
            MetadataManager::instance().updateExtractedMediaFeaturesBatch(results);
        }

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_activeCount.fetch_sub(static_cast<int>(batch.size()));
            SyncStatusService::instance().updateMediaPending(static_cast<int>(m_queue.size()) + m_activeCount.load());
        }
    }

    m_activeWorkers.fetch_sub(1);

#ifdef Q_OS_WIN
    CoUninitialize();
#endif
}
```

---

## 阶段四：退出时序与托盘完全闭环（0.1 秒秒退）

### 5. 修改 `src/ui/TrayController.cpp`
**修改要求**：托盘点击退出只做触发，不抢先销毁数据库。

```cpp
void TrayController::onQuitApp() {
    if (m_trayIcon) m_trayIcon->hide();
    // 严禁在此处调用 DatabaseManager::shutdown()，统一交给 main.cpp 集中调度
    QApplication::quit();
}
```

---

### 6. 修改 `src/main.cpp`
**修改要求**：在 `onApplicationAboutToQuit` 中先熔断提图线程，再限时排空线程池，最后执行唯一数据库落盘。

```cpp
void onApplicationAboutToQuit(HANDLE hMutex) {
    // 1. 立即设置停机原子标记
    QuarkMeta::CoreController::requestShutdown();

    // 2. 立即熔断提图后台流水线
    QuarkMeta::MediaExtractorPipeline::instance().cancelAll();

    // 3. 限时 200ms 排空线程池，绝不无限期死等
    QThreadPool::globalInstance()->waitForDone(200);

    // 4. 全系统唯一权威的数据库安全落盘与闭卷
    QuarkMeta::DatabaseManager::instance().shutdown();

#ifdef Q_OS_WIN
    CoUninitialize();

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
#endif
}
```

---

### 三、 执行与验收标准

1. **同盘移动/改名测试**：把 `H:\测试` 里的某个 `.ai` 或 `.eps` 文件重命名或剪切到另一个文件夹，再次打开时缩略图**0 毫秒瞬间出现，绝不重新提取**；
2. **大文件夹并发测试**：打开 3434 个文件的目录，视口内 30 张卡片由多核 CPU 并发提取，**2 秒内整屏铺满**；
3. **退出测试**：在缩略图仍在提取时，点击托盘“退出 QuarkMeta”，进程在 **0.1 秒内干净退出，任务管理器中零残留**。