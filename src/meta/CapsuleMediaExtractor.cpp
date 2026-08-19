#include "CapsuleMediaExtractor.h"
#include "../ui/WindowsShellThumbnailProvider.h"
#include "../ui/MediaColorExtractor.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSvgRenderer>
#include <QPainter>
#include <QCryptographicHash>
#include <QCoreApplication>

namespace QuarkMeta {

std::mutex CapsuleMediaExtractor::s_qtGuiMutex;

QString CapsuleMediaExtractor::getDiskThumbCachePath(const QString& mainAssetPath) {
    if (mainAssetPath.isEmpty()) return "";

    QString normPath = QDir::toNativeSeparators(mainAssetPath).toLower();
    quint64 h = qHash(normPath);

    // 两级哈希存储桶，防止单一目录下文件过多引起 NTFS 检索变慢
    QString bucket = QString("%1").arg((h >> 32) & 0xFF, 2, 16, QChar('0'));
    QString fileKey = QString("%1.jpg").arg(h, 16, 16, QChar('0'));

    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/" + bucket;
    QDir().mkpath(cacheDir);

    return cacheDir + "/" + fileKey;
}

bool CapsuleMediaExtractor::saveDiskThumbnail(const QString& mainAssetPath, const QImage& img512) {
    if (img512.isNull()) return false;
    QString diskCachePath = getDiskThumbCachePath(mainAssetPath);
    // 强制使用 JPEG Quality 85 落盘，保持 512x512 高清且压缩耗时 < 1ms
    return img512.save(diskCachePath, "JPG", 85);
}

QImage CapsuleMediaExtractor::getCapsuleThumbnailReadOnly(const QString& mainAssetPath) {
    QString diskCachePath = getDiskThumbCachePath(mainAssetPath);
    if (QFile::exists(diskCachePath)) {
        QImage img;
        if (img.load(diskCachePath)) return img;
    }
    return QImage();
}

// 【唯一提取实现】原 DiskMediaExtractor::getDiskThumbnail 中的重复提取逻辑已根除，
// 统一收口到这一个函数。SVG 渲染统一加 s_qtGuiMutex 锁保护，消除并发崩溃风险。
QImage CapsuleMediaExtractor::getCapsuleThumbnail(const QString& mainAssetPath, int size) {
    // 先尝试只读快速命中
    QImage cached = getCapsuleThumbnailReadOnly(mainAssetPath);
    if (!cached.isNull()) return cached;

    // 实时提取图像
    QFileInfo fi(mainAssetPath);
    QString ext = fi.suffix().toLower();
    QImage img;

    if (ext == "svg") {
        std::lock_guard<std::mutex> guiLock(CapsuleMediaExtractor::s_qtGuiMutex);
        QSvgRenderer renderer(mainAssetPath);
        if (renderer.isValid()) {
            img = QImage(size, size, QImage::Format_ARGB32);
            img.fill(Qt::transparent);
            QPainter painter(&img);
            renderer.render(&painter);
        }
    } else if (ext == "psd" || ext == "psb") {
        img = MediaColorExtractor::extractEmbeddedPsdThumbnail(mainAssetPath);
    } else if (ext == "ai") {
        img = MediaColorExtractor::extractEmbeddedAiPreview(mainAssetPath, size);
    } else if (ext == "eps") {
        img = MediaColorExtractor::extractEmbeddedEpsPreview(mainAssetPath, size);
    }

    if (img.isNull()) {
        img = WindowsShellThumbnailProvider::getShellThumbnail(mainAssetPath, size);
        if (img.isNull()) img.load(mainAssetPath);
    }

    // 唯一落盘出口：统一走 JPEG85，不再有任何地方硬编码 "PNG"
    if (!img.isNull()) {
        saveDiskThumbnail(mainAssetPath, img);
    }
    return img;
}

} // namespace QuarkMeta
