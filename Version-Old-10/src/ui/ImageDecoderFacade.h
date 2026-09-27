#pragma once

#include <QImage>
#include <QSize>
#include <QString>
#include <memory>
#include "../core/CoreEngine.h"

namespace QuarkMeta {

struct DecodedMediaResult {
    QSize originalSize;       // 原始图像物理尺寸 (如 6000x4000)
    QImage thumbnail512;      // 512x512 高清解码图
    bool isValid = false;     // 解码是否成功
};

class ImageDecoderFacade {
public:
    // 【唯一指定提图接口】单次读盘同时获取原始尺寸与 512 高清图 (customTimeoutMs > 0 时透传给长效模式)
    static DecodedMediaResult decodeSinglePass(const QString& filePath, int targetSize = 512, int customTimeoutMs = 0, std::shared_ptr<CancellationToken> token = nullptr);

    // 保留辅助接口
    static QImage loadScaledImage(const QString& filePath, int targetSize = 512, int maxAllocationMB = 128);
    static QSize readImageDimensions(const QString& filePath);
};

} // namespace QuarkMeta
