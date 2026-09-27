#ifndef DISKMEDIAEXTRACTOR_H
#define DISKMEDIAEXTRACTOR_H

#include <QImage>
#include <QString>
#include <QSize>
#include <QMutex>
#include <QHash>
#include <QSet>
#include <mutex>
#include <cstdint>
#include <memory>
#include "../core/CoreController.h"
#include "../core/CoreEngine.h"

namespace QuarkMeta {

class DiskMediaExtractor {
private:
    static QMutex s_pendingFailureMutex;
    static QHash<QString, QSet<QString>> s_pendingFailures;

public:
    static std::mutex s_qtGuiMutex;
    static std::mutex s_jsonSaveMutex;
    static std::mutex s_thumbFileMutex;

    static void scheduleFailureMark(const QString& folderPath, const QString& fileName);
    static void flushPendingFailures();

    struct ExtractResult {
        QImage thumbnail512;
        QSize originalSize;
        bool isValid = false;
    };

    static QString getDiskThumbCachePathByFileId(uint32_t volSerial, uint64_t fileId);
    static QString getDiskThumbCachePath(const QString& filePath);
    static QImage getCapsuleThumbnailReadOnly(const QString& filePath);
    static ExtractResult getCapsuleExtractResult(const QString& filePath, int size = 512, std::shared_ptr<CancellationToken> token = {});
    static QSize fastExtractImageSize(const QString& filePath);
    static QImage getCapsuleThumbnail(const QString& filePath, int size = 512, std::shared_ptr<CancellationToken> token = {});
    static QImage getDiskThumbnail(const QString& path, int size = 512, std::shared_ptr<CancellationToken> token = {});
    static bool saveDiskThumbnail(const QString& filePath, const QImage& img512);
    static void roamThumbnailCache(const QString& oldFilePath, const QString& newFilePath, bool isMove);

    // 强制执行深度长效提取（不走只读缓存，超时放宽至 45 秒）
    static QImage forceExtractDeepThumbnail(const QString& filePath, int size = 512, std::shared_ptr<CancellationToken> token = {});
};

} // namespace QuarkMeta

#endif // DISKMEDIAEXTRACTOR_H
