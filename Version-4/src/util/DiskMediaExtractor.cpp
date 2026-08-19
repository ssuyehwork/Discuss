#include "DiskMediaExtractor.h"
#include "../meta/CapsuleMediaExtractor.h"

namespace QuarkMeta {

QString DiskMediaExtractor::diskThumbCachePath(const QString& path, int size) {
    (void)size;
    // 统一收口至 CapsuleMediaExtractor::getDiskThumbCachePath（具备父文件夹 SHA-256 哈希子目录隔离功能）
    return CapsuleMediaExtractor::getDiskThumbCachePath(path);
}

QImage DiskMediaExtractor::getDiskThumbnail(const QString& path, int size) {
    return CapsuleMediaExtractor::getCapsuleThumbnail(path, size);
}

} // namespace QuarkMeta
