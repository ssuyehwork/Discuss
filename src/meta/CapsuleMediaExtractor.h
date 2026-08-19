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
