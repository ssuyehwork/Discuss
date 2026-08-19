#include "DiskMediaExtractor.h"
#include "../meta/CapsuleMediaExtractor.h"

namespace QuarkMeta {

QString DiskMediaExtractor::diskThumbCachePath(const QString& path, int size) {
    (void)size;
    return CapsuleMediaExtractor::getDiskThumbCachePath(path);
}

// 原本这里重复了一份 SVG/PSD/AI/EPS/Shell 提取逻辑，且 SVG 渲染没有加
// CapsuleMediaExtractor::s_qtGuiMutex 锁保护 —— 在 loadThumbnailsForRows 的
// 后台线程里并发调用时存在数据竞争/崩溃风险。现根除重复实现，唯一提取逻辑
// 收口到 CapsuleMediaExtractor::getCapsuleThumbnail。
QImage DiskMediaExtractor::getDiskThumbnail(const QString& path, int size) {
    return CapsuleMediaExtractor::getCapsuleThumbnail(path, size);
}

} // namespace QuarkMeta
