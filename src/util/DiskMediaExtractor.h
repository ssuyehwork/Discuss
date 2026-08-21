#ifndef DISKMEDIAEXTRACTOR_H
#define DISKMEDIAEXTRACTOR_H

#include <QImage>
#include <QString>
#include <mutex>
#include <cstdint>

namespace QuarkMeta {

class DiskMediaExtractor {
public:
    static std::mutex s_qtGuiMutex;

    static QString getDiskThumbCachePathByFileId(uint32_t volSerial, uint64_t fileId);
    static QString getDiskThumbCachePath(const QString& filePath);
    static QImage getCapsuleThumbnailReadOnly(const QString& filePath);
    static QImage getCapsuleThumbnail(const QString& filePath, int size = 512);
    static QImage getDiskThumbnail(const QString& path, int size = 512);
    static bool saveDiskThumbnail(const QString& filePath, const QImage& img512);
};

} // namespace QuarkMeta

#endif // DISKMEDIAEXTRACTOR_H
