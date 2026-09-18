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
     
    // EPS 预览图与 Ghostscript 矢量渲染
    static QImage extractEpsPreview(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);
     
    // External Process: Ghostscript 降采样渲染 (customTimeoutMs > 0 时使用自定义长效超时)
    static QImage renderGhostscriptSafely(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

private:
    static QString findGhostscriptExecutable();
    static QImage renderPdfAiFirstPage(const QString& filePath, int targetSize = 512);
};

} // namespace QuarkMeta
