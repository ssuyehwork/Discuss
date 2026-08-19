#include "CapsuleMediaExtractor.h"
#include "../ui/ImageDecoderFacade.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

std::mutex CapsuleMediaExtractor::s_qtGuiMutex;

// 免管理员权限、标准 Win32 API 顺手提取物理卷ID与 64 位 File ID (FRN)
static bool fetchPhysicalFileId(const QString& filePath, uint32_t& outVol, uint64_t& outFrn) {
#ifdef Q_OS_WIN
    std::wstring wPath = QDir::toNativeSeparators(filePath).toStdWString();
    HANDLE hFile = CreateFileW(wPath.c_str(), FILE_READ_ATTRIBUTES, 
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
                               NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileInformationByHandle(hFile, &info)) {
        outVol = info.dwVolumeSerialNumber;
        outFrn = (static_cast<uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
        CloseHandle(hFile);
        return true;
    }
    CloseHandle(hFile);
    return false;
#else
    Q_UNUSED(filePath);
    Q_UNUSED(outVol);
    Q_UNUSED(outFrn);
    return false;
#endif
}

QString CapsuleMediaExtractor::getDiskThumbCachePathByFileId(uint32_t volSerial, uint64_t fileId) {
    // 结构：.QuarkMeta/disk_thumbs/<卷ID_Hex>/<前2位子分桶>/<FileId_Hex>.png
    QString volStr = QString("%1").arg(volSerial, 8, 16, QChar('0')).toUpper();
    QString bucket = QString("%1").arg((fileId >> 8) & 0xFF, 2, 16, QChar('0')).toUpper();
    QString fileKey = QString("%1.png").arg(fileId, 16, 16, QChar('0')).toUpper();

    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/" + volStr + "/" + bucket;
    QDir().mkpath(cacheDir);

    return cacheDir + "/" + fileKey;
}

QString CapsuleMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    uint32_t vol = 0;
    uint64_t frn = 0;
    if (fetchPhysicalFileId(filePath, vol, frn)) {
        return getDiskThumbCachePathByFileId(vol, frn);
    }
    // 极端退化兜底：无法获取 FileID 时使用轻量哈希
    quint64 h = qHash(QDir::toNativeSeparators(filePath).toLower(), 0);
    QString bucket = QString("%1").arg((h >> 32) & 0xFF, 2, 16, QChar('0'));
    QString fileKey = QString("%1.png").arg(h, 16, 16, QChar('0'));
    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/fallback/" + bucket;
    QDir().mkpath(cacheDir);
    return cacheDir + "/" + fileKey;
}

bool CapsuleMediaExtractor::saveDiskThumbnail(const QString& filePath, const QImage& img512) {
    if (img512.isNull()) return false;
    QString diskCachePath = getDiskThumbCachePath(filePath);
    // 严格锁定 PNG 格式落盘，完美保留 alpha 透明通道
    return img512.save(diskCachePath, "PNG");
}

QImage CapsuleMediaExtractor::getCapsuleThumbnailReadOnly(const QString& filePath) {
    QString diskCachePath = getDiskThumbCachePath(filePath);
    if (QFile::exists(diskCachePath)) {
        QImage img;
        if (img.load(diskCachePath)) return img;
    }
    return QImage();
}

QImage CapsuleMediaExtractor::getCapsuleThumbnail(const QString& filePath, int size) {
    // 1. 优先查缓存 (0ms)
    QImage cached = getCapsuleThumbnailReadOnly(filePath);
    if (!cached.isNull()) return cached;

    // 2. 单次解码提取
    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size);
    if (dec.isValid && !dec.thumbnail512.isNull()) {
        saveDiskThumbnail(filePath, dec.thumbnail512);
        return dec.thumbnail512;
    }
    return QImage();
}

} // namespace QuarkMeta
