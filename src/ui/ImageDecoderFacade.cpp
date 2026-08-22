#include "ImageDecoderFacade.h"
#include "FormatDecoders.h"
#include <QFileInfo>
#include <QFile>
#include <QImageReader>
#include <QSvgRenderer>
#include <QPainter>

namespace QuarkMeta {

DecodedMediaResult ImageDecoderFacade::decodeSinglePass(const QString& filePath, int targetSize, int customTimeoutMs, std::shared_ptr<CancellationToken> token) {
    DecodedMediaResult result;
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();

    // 1. 特殊矢量与设计格式分流
    if (ext == "svg") {
        QSvgRenderer renderer(filePath);
        if (renderer.isValid()) {
            QRectF vb = renderer.viewBoxF();
            result.originalSize = vb.isValid() ? vb.size().toSize() : renderer.defaultSize();
            if (result.originalSize.isEmpty()) result.originalSize = QSize(512, 512);

            QImage img(targetSize, targetSize, QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::transparent);
            QPainter p(&img);
            renderer.render(&p);
            result.thumbnail512 = img;
            result.isValid = true;
        }
        return result;
    }
    if (ext == "psd" || ext == "psb") {
        result.thumbnail512 = FormatDecoders::extractPsdHeaderThumbnail(filePath);
        result.originalSize = readImageDimensions(filePath);
        result.isValid = !result.thumbnail512.isNull();
        return result;
    }
    if (ext == "ai" || ext == "pdf") {
        result.thumbnail512 = FormatDecoders::extractAiPreview(filePath, targetSize, customTimeoutMs, token);
        result.originalSize = result.thumbnail512.size();
        result.isValid = !result.thumbnail512.isNull();
        return result;
    }
    if (ext == "eps") {
        result.thumbnail512 = FormatDecoders::extractEpsPreview(filePath, targetSize, customTimeoutMs, token);
        result.originalSize = result.thumbnail512.size();
        result.isValid = !result.thumbnail512.isNull();
        return result;
    }

    // 2. 常规栅格图像（JPG / PNG / WEBP / BMP / GIF）硬件分块降采样
    QImageReader reader(filePath);
    reader.setAutoTransform(true); // 自动处理 EXIF 旋转
    if (!reader.canRead()) return result;

    result.originalSize = reader.size(); // 获取真实尺寸

    // 设置底层 DCT 硬件级缩放
    if (result.originalSize.isValid() && (result.originalSize.width() > targetSize || result.originalSize.height() > targetSize)) {
        QSize scaled = result.originalSize.scaled(targetSize, targetSize, Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }

    result.thumbnail512 = reader.read();
    result.isValid = !result.thumbnail512.isNull();
    return result;
}

QImage ImageDecoderFacade::loadScaledImage(const QString& filePath, int targetSize, int maxAllocationMB) {
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();

    // 1. 路由至专用格式解码器 (PSD / AI / EPS)
    if (ext == "psd" || ext == "psb") {
        return FormatDecoders::extractPsdHeaderThumbnail(filePath);
    }
    if (ext == "ai" || ext == "pdf") {
        return FormatDecoders::extractAiPreview(filePath, targetSize);
    }
    if (ext == "eps") {
        return FormatDecoders::extractEpsPreview(filePath, targetSize);
    }
    if (ext == "tif" || ext == "tiff") {
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            return FormatDecoders::decodeTiffMemorySafely(f.readAll(), 64);
        }
    }

    // 2. 普通图像 (PNG / JPG / WEBP / BMP 等) 走 QImageReader 降采样加载
    QImageReader reader(filePath);
    reader.setAllocationLimit(maxAllocationMB);
     
    if (!reader.canRead()) return QImage();
     
    QSize origSize = reader.size();
    if (origSize.isValid() && (origSize.width() > targetSize || origSize.height() > targetSize)) {
        QSize scaledSize = origSize.scaled(targetSize, targetSize, Qt::KeepAspectRatio);
        reader.setScaledSize(scaledSize);
    }
     
    return reader.read();
}

QSize ImageDecoderFacade::readImageDimensions(const QString& filePath) {
    QImageReader reader(filePath);
    if (!reader.canRead()) return QSize();
    return reader.size();
}

} // namespace QuarkMeta
