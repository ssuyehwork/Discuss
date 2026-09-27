#pragma once

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
     
    // 通用兼容接口（默认路由至缩略图策略）
    static QImage extractEpsPreview(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // 策略 1：日常缩略图（版本 30 策略：内嵌优先，-r72 GS 兜底，极限速度）
    static QImage extractEpsThumbnail(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // 策略 2：QuickLook 快速大图（版本 31 策略：-r144 GS 矢量优先，内嵌降级兜底，极致画质）
    static QImage extractEpsQuickLook(const QString& filePath, int targetSize = 2048, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // External Process: Ghostscript 降采样渲染 (customTimeoutMs > 0 时使用自定义长效超时，默认 72 DPI)
    static QImage renderGhostscriptSafely(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr, int dpi = 72);

private:
    static QString findGhostscriptExecutable();
    static QImage renderPdfAiFirstPage(const QString& filePath, int targetSize = 512);
};

} // namespace QuarkMeta
