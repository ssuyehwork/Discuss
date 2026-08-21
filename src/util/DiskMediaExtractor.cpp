#include "DiskMediaExtractor.h"
#include "../ui/ImageDecoderFacade.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

std::mutex DiskMediaExtractor::s_qtGuiMutex;

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

QString DiskMediaExtractor::getDiskThumbCachePathByFileId(uint32_t volSerial, uint64_t fileId) {
    QString volStr = QString("%1").arg(volSerial, 8, 16, QChar('0')).toUpper();
    QString bucket = QString("%1").arg((fileId >> 8) & 0xFF, 2, 16, QChar('0')).toUpper();
    QString fileKey = QString("%1.png").arg(fileId, 16, 16, QChar('0')).toUpper();

    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/" + volStr + "/" + bucket;
    QDir().mkpath(cacheDir);

    return cacheDir + "/" + fileKey;
}

QString DiskMediaExtractor::getDiskThumbCachePath(const QString& filePath) {
    uint32_t vol = 0;
    uint64_t frn = 0;
    if (fetchPhysicalFileId(filePath, vol, frn)) {
        return getDiskThumbCachePathByFileId(vol, frn);
    }
    quint64 h = qHash(QDir::toNativeSeparators(filePath).toLower(), 0);
    QString bucket = QString("%1").arg((h >> 32) & 0xFF, 2, 16, QChar('0'));
    QString fileKey = QString("%1.png").arg(h, 16, 16, QChar('0'));
    QString cacheDir = QCoreApplication::applicationDirPath() + "/.QuarkMeta/disk_thumbs/fallback/" + bucket;
    QDir().mkpath(cacheDir);
    return cacheDir + "/" + fileKey;
}

bool DiskMediaExtractor::saveDiskThumbnail(const QString& filePath, const QImage& img512) {
    if (img512.isNull()) return false;
    QString diskCachePath = getDiskThumbCachePath(filePath);
    return img512.save(diskCachePath, "PNG");
}

QImage DiskMediaExtractor::getCapsuleThumbnailReadOnly(const QString& filePath) {
    QString diskCachePath = getDiskThumbCachePath(filePath);
    if (QFile::exists(diskCachePath)) {
        QImage img;
        if (img.load(diskCachePath)) return img;
    }
    return QImage();
}

QImage DiskMediaExtractor::getCapsuleThumbnail(const QString& filePath, int size) {
    QImage cached = getCapsuleThumbnailReadOnly(filePath);
    if (!cached.isNull()) return cached;

    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size);
    if (dec.isValid && !dec.thumbnail512.isNull()) {
        saveDiskThumbnail(filePath, dec.thumbnail512);
        return dec.thumbnail512;
    }
    return QImage();
}

QImage DiskMediaExtractor::getDiskThumbnail(const QString& path, int size) {
    return getCapsuleThumbnail(path, size);
}

} // namespace QuarkMeta
