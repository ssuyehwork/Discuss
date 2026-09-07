#include "ThumbnailPipelineService.h"
#include "ColorPaletteEngine.h"
#include <QImageReader>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QtConcurrent>
#include <QMutexLocker>

namespace QuarkMeta {

ThumbnailPipelineService& ThumbnailPipelineService::instance() {
    static ThumbnailPipelineService s_instance;
    return s_instance;
}

ThumbnailPipelineService::ThumbnailPipelineService(QObject* parent)
    : QObject(parent) {
    m_memoryCache.setMaxCost(kMaxMemoryCacheCount);
}

QString ThumbnailPipelineService::getDiskCachePath(const QString& filePath, int targetSize) {
    QByteArray normalized = QDir::toNativeSeparators(filePath).toLower().toUtf8();
    QByteArray hash = QCryptographicHash::hash(normalized, QCryptographicHash::Sha256).toHex();
    
    QString cacheDir = QDir::temp().filePath("QuarkMeta_Thumbnails");
    QDir().mkpath(cacheDir);

    return QDir(cacheDir).filePath(QString("%1_%2.png").arg(QString(hash.left(32))).arg(targetSize));
}

QPixmap ThumbnailPipelineService::getFromMemoryCache(const QString& filePath, int targetSize) const {
    QString key = QString("%1@%2").arg(QDir::toNativeSeparators(filePath).toLower()).arg(targetSize);
    QMutexLocker locker(&m_cacheMutex);
    QPixmap* cached = m_memoryCache.object(key);
    if (cached && !cached->isNull()) {
        return *cached;
    }
    return QPixmap();
}

void ThumbnailPipelineService::clearMemoryCache() {
    QMutexLocker locker(&m_cacheMutex);
    m_memoryCache.clear();
}

void ThumbnailPipelineService::incrementGeneration() {
    m_currentGeneration.fetch_add(1, std::memory_order_relaxed);
}

void ThumbnailPipelineService::cancelAll() {
    incrementGeneration();
}

QImage ThumbnailPipelineService::decodeImageToThumbnail(const QString& filePath, int targetSize) const {
    if (!ColorPaletteEngine::isGraphicsFile(QFileInfo(filePath).suffix())) {
        return QImage();
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    reader.setDecideFormatFromContent(true);

    QSize origSize = reader.size();
    if (origSize.isValid() && origSize.width() > 0 && origSize.height() > 0) {
        QSize scaled = origSize.scaled(QSize(targetSize * 2, targetSize * 2), Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }

    QImage img = reader.read();
    if (img.isNull()) return QImage();

    return img.scaled(QSize(targetSize, targetSize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void ThumbnailPipelineService::loadBatchAsync(const QStringList& filePaths, 
                                              int targetSize, 
                                              std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded) {
    if (filePaths.isEmpty()) return;

    uint64_t taskGen = m_currentGeneration.load(std::memory_order_relaxed);

    QStringList pathsToFetch;
    for (const QString& path : filePaths) {
        QPixmap memPix = getFromMemoryCache(path, targetSize);
        if (!memPix.isNull()) {
            if (onSingleLoaded) onSingleLoaded(path, memPix);
        } else {
            pathsToFetch << path;
        }
    }

    if (pathsToFetch.isEmpty()) return;

    (void)QtConcurrent::run([this, pathsToFetch, targetSize, taskGen, onSingleLoaded]() {
        for (const QString& path : pathsToFetch) {
            if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                return;
            }

            QString diskPath = getDiskCachePath(path, targetSize);
            QImage finalImg;

            if (QFile::exists(diskPath)) {
                finalImg.load(diskPath);
            }

            if (finalImg.isNull()) {
                finalImg = decodeImageToThumbnail(path, targetSize);
                if (!finalImg.isNull()) {
                    finalImg.save(diskPath, "PNG");
                }
            }

            if (!finalImg.isNull()) {
                if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                    return;
                }

                QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                    if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                        return;
                    }

                    QPixmap pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }

                        if (onSingleLoaded) {
                            onSingleLoaded(path, pix);
                        }
                    }
                }, Qt::QueuedConnection);
            }
        }
    });
}

} // namespace QuarkMeta
