# QuarkMeta 纯磁盘模式缩略图提取优化与图像尺寸持久化终极实施方案 (Thumbnail Optimization & Image Dimensions Persistence Plan)

## Overview
针对缩略图提取卡顿、Ghostscript 进程抢占 CPU、切目录时旧任务无法打断以及图片尺寸未持久化等病灶，本方案建立纯磁盘模式下的终极重构：
1. **Ghostscript 进程级 50ms 轮询可中断取消与降频**：将 Ghostscript 并发限流信号量设为 1，生成 UUID 独占临时文件名，在等待进程完成时采用 50ms 轮询检测 `CancellationToken` 与代际信号，一旦切走目录立即 `process.kill()` 自杀并清理临时文件。
2. **提图线程池物理隔离与最低优先级**：在 `DiskItemModel` 中引入独立专属线程池 `thumbnailPool()`（最大并发锁定为 2，线程优先级设为 `QThread::LowestPriority`），完全剥离全局主线程池。
3. **切目录秒级代际熔断**：在 `DiskItemModel::incrementGeneration()` 递增代际时，触发旧代际 `CancellationToken` 取消信号，10ms 内释放上一个文件夹所有挂起/正在解码的耗时解图任务。
4. **真实尺寸原子落盘与 UI 实时刷新**：解图时同时获取真实尺寸 `originalSize`，线程安全原子写入 `.QuarkMeta.json`；主线程回调更新 `rec.width/height` 并发射全列 `dataChanged`，第 3 列尺寸实时渲染且下次 0ms 秒显。
5. **废除内存态 `s_failedPaths` 补丁**：彻底删除易失的 `s_failedPaths` 补丁，改由纯物理缩略图文件是否存在直接判断。

---

## Modified Files List
- `src/ui/FormatDecoders.h`
- `src/ui/FormatDecoders.cpp`
- `src/ui/ImageDecoderFacade.h`
- `src/ui/ImageDecoderFacade.cpp`
- `src/util/DiskMediaExtractor.h`
- `src/util/DiskMediaExtractor.cpp`
- `src/ui/models/DiskItemModel.h`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/FormatDecoders.h`
<<<<<<< SEARCH
#include <QImage>
#include <QString>
#include <QByteArray>

namespace QuarkMeta {

class FormatDecoders {
public:
    // TIFF 物理内存解码（含安全防御）
    static QImage decodeTiffMemorySafely(const QByteArray& tiffData, int maxMemoryMB = 64);

    // PSD 嵌套缩略图提取
    static QImage extractPsdHeaderThumbnail(const QString& filePath);

    // AI 嵌套预览图与 XMP 提取
    static QImage extractAiPreview(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0);

    // EPS 预览图与 Ghostscript 矢量渲染
    static QImage extractEpsPreview(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0);

    // External Process: Ghostscript 降采样渲染 (customTimeoutMs > 0 时使用自定义长效超时)
    static QImage renderGhostscriptSafely(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0);
=======
#include <QImage>
#include <QString>
#include <QByteArray>
#include <memory>
#include "../core/CoreEngine.h"

namespace QuarkMeta {

class FormatDecoders {
public:
    // TIFF 物理内存解码（含安全防御）
    static QImage decodeTiffMemorySafely(const QByteArray& tiffData, int maxMemoryMB = 64);

    // PSD 嵌套缩略图提取
    static QImage extractPsdHeaderThumbnail(const QString& filePath);

    // AI 嵌套预览图与 XMP 提取
    static QImage extractAiPreview(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // EPS 预览图与 Ghostscript 矢量渲染
    static QImage extractEpsPreview(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // External Process: Ghostscript 降采样渲染 (customTimeoutMs > 0 时使用自定义长效超时)
    static QImage renderGhostscriptSafely(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);
>>>>>>> REPLACE

### 2. `src/ui/FormatDecoders.cpp`
<<<<<<< SEARCH
#include "FormatDecoders.h"
#include "Logger.h"
#include "../core/CoreController.h"
=======
#include "FormatDecoders.h"
#include "Logger.h"
#include "../core/CoreController.h"
#include <QUuid>
>>>>>>> REPLACE

<<<<<<< SEARCH
static QSemaphore g_gsConcurrencyLimit(2); // 最多2个Ghostscript进程并发跑

QImage FormatDecoders::renderGhostscriptSafely(const QString& filePath, int targetSize, int customTimeoutMs) {
=======
static QSemaphore g_gsConcurrencyLimit(1); // 最多1个Ghostscript进程并发跑，避免多进程抢占CPU导致切换文件夹卡顿

QImage FormatDecoders::renderGhostscriptSafely(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
>>>>>>> REPLACE

<<<<<<< SEARCH
    QString tempPng = QDir::tempPath() + QString("/gs_thumb_%1.png").arg(QString::number(qHash(filePath), 16));
=======
    QString tempPng = QDir::tempPath() + QString("/gs_thumb_%1_%2.png").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)).arg(QString::number(qHash(filePath), 16));
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (process.waitForFinished(timeoutMs)) {
        if (QFile::exists(tempPng)) {
            QImageReader reader(tempPng);
            reader.setAllocationLimit(512); // 放宽Qt默认256MB安全上限，避免大幅面AI文件渲染出的高分辨率PNG被直接拒收
            QSize origSize = reader.size();
            if (origSize.isValid() && (origSize.width() > targetSize || origSize.height() > targetSize)) {
                reader.setScaledSize(origSize.scaled(targetSize, targetSize, Qt::KeepAspectRatio));
            }
            QImage img = reader.read();
            QFile::remove(tempPng);

            if (!img.isNull()) {
                return img;
            }
            qWarning() << "[GS诊断] 进程正常结束但输出图片解码为空，文件:" << filePath;
        } else {
            qWarning() << "[GS诊断] 进程正常结束但输出文件不存在，文件:" << filePath;
        }
    } else {
        qWarning() << "[GS诊断] 等待" << timeoutMs << "ms后仍未结束，判定超时，文件:" << filePath;
    }
=======
    qint64 elapsed = 0;
    bool finished = false;
    while (elapsed < timeoutMs) {
        if ((token && token->isCanceled()) || CoreController::isShuttingDown()) {
            qWarning() << "[GS诊断] 收到取消信号，主动杀死 Ghostscript 进程，文件:" << filePath;
            process.kill();
            process.waitForFinished(200);
            if (QFile::exists(tempPng)) QFile::remove(tempPng);
            return QImage();
        }
        if (process.waitForFinished(50)) {
            finished = true;
            break;
        }
        elapsed += 50;
    }

    if (finished && QFile::exists(tempPng)) {
        QImageReader reader(tempPng);
        reader.setAllocationLimit(512);
        QSize origSize = reader.size();
        if (origSize.isValid() && (origSize.width() > targetSize || origSize.height() > targetSize)) {
            reader.setScaledSize(origSize.scaled(targetSize, targetSize, Qt::KeepAspectRatio));
        }
        QImage img = reader.read();
        QFile::remove(tempPng);

        if (!img.isNull()) {
            return img;
        }
        qWarning() << "[GS诊断] 进程正常结束但输出图片解码为空，文件:" << filePath;
    } else if (!finished) {
        qWarning() << "[GS诊断] 等待" << timeoutMs << "ms后仍未结束，判定超时，文件:" << filePath;
        process.kill();
        process.waitForFinished(200);
        if (QFile::exists(tempPng)) QFile::remove(tempPng);
    }
>>>>>>> REPLACE

### 3. `src/ui/ImageDecoderFacade.h` & `src/ui/ImageDecoderFacade.cpp`
传递 `std::shared_ptr<CancellationToken> token` 至 `decodeSinglePass`，并转交矢量解码逻辑。

### 4. `src/util/DiskMediaExtractor.h` & `src/util/DiskMediaExtractor.cpp`
透传 `token` 并注销 `s_failedPaths` 内存态补丁。

### 5. `src/ui/models/DiskItemModel.h` & `src/ui/models/DiskItemModel.cpp`
在 `DiskItemModel` 中引入专属缩略图线程池 `thumbnailPool()`（并发数 2，`QThread::LowestPriority`）与 `CancellationToken` 代际映射表；`incrementGeneration()` 时自动取消旧代际 `token`。

---

## Build & Verification Steps
1. **Compilation Check**:
   ```bash
   cmake --build build --config Release
   ```
2. **Verification**:
   - Open a folder with large EPS/AI files, immediately switch to another folder; verify Ghostscript process kills within 50ms and UI remains responsive.
   - Verify Column 3 dimensions display and persist to `.QuarkMeta.json`.
