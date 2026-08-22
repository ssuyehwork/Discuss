#include "DiskMediaExtractor.h"
#include "../ui/ImageDecoderFacade.h"
#include "../meta/QuarkMetaJson.h"
#include "../core/CoreController.h"
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

DiskMediaExtractor::ExtractResult DiskMediaExtractor::getCapsuleExtractResult(const QString& filePath, int size, std::shared_ptr<CancellationToken> token) {
    ExtractResult res;
    if ((token && token->isCanceled()) || CoreController::isShuttingDown()) return res;

    res.thumbnail512 = getCapsuleThumbnailReadOnly(filePath);

    QFileInfo fi(filePath);
    QString parentDir = QDir::toNativeSeparators(fi.absolutePath());
    QString fileName = fi.fileName();

    // 1. 极速缓存命中路径：若磁盘已存在缩略图缓存且 .QuarkMeta.json 中已记录尺寸，免解码瞬间返回
    if (!res.thumbnail512.isNull()) {
        static std::mutex s_jsonSaveMutex;
        std::lock_guard<std::mutex> lock(s_jsonSaveMutex);
        QuarkMetaJson jsonCache(parentDir.toStdWString());
        jsonCache.load();
        const auto& cachedItems = jsonCache.items();
        std::wstring wFileName = fileName.toStdWString();
        auto it = cachedItems.find(wFileName);
        if (it != cachedItems.end() && it->second.width > 0 && it->second.height > 0) {
            res.originalSize = QSize(it->second.width, it->second.height);
            res.isValid = true;
            return res;
        }
    }

    if ((token && token->isCanceled()) || CoreController::isShuttingDown()) return res;

    // 2. 解码路径：单次解码同时获取原始分辨率与 512px 缩略图
    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size, 0, token);
    if (dec.isValid) {
        res.originalSize = dec.originalSize;
        if (res.thumbnail512.isNull() && !dec.thumbnail512.isNull()) {
            saveDiskThumbnail(filePath, dec.thumbnail512);
            res.thumbnail512 = dec.thumbnail512;
        }
        res.isValid = true;

        // 3. 线程安全原子落盘尺寸数据至 .QuarkMeta.json
        if (res.originalSize.isValid() && res.originalSize.width() > 0) {
            static std::mutex s_jsonSaveMutex;
            std::lock_guard<std::mutex> lock(s_jsonSaveMutex);

            QuarkMetaJson jsonCache(parentDir.toStdWString());
            jsonCache.load();
            auto& cachedItems = jsonCache.items();
            std::wstring wFileName = fileName.toStdWString();
            if (cachedItems.find(wFileName) == cachedItems.end()) {
                ItemMeta emptyMeta;
                emptyMeta.type = L"file";
                cachedItems[wFileName] = emptyMeta;
            }
            auto& fileMeta = cachedItems[wFileName];
            if (fileMeta.width != res.originalSize.width() || fileMeta.height != res.originalSize.height()) {
                fileMeta.width = res.originalSize.width();
                fileMeta.height = res.originalSize.height();
                jsonCache.save();
            }
        }
    } else if (!res.thumbnail512.isNull()) {
        res.isValid = true;
    }
    return res;
}

QImage DiskMediaExtractor::getCapsuleThumbnail(const QString& filePath, int size, std::shared_ptr<CancellationToken> token) {
    ExtractResult res = getCapsuleExtractResult(filePath, size, token);
    return res.thumbnail512;
}

QImage DiskMediaExtractor::getDiskThumbnail(const QString& path, int size, std::shared_ptr<CancellationToken> token) {
    return getCapsuleThumbnail(path, size, token);
}

QImage DiskMediaExtractor::forceExtractDeepThumbnail(const QString& filePath, int size, std::shared_ptr<CancellationToken> token) {
    // 强制调用单遍解码，且对耗时格式赋予 45 秒超时
    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size, 45000, token);
    if (dec.isValid && !dec.thumbnail512.isNull()) {
        saveDiskThumbnail(filePath, dec.thumbnail512);
        return dec.thumbnail512;
    }
    return QImage();
}

} // namespace QuarkMeta
