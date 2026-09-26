#include "ThumbnailPipelineService.h"
#include "ColorPaletteEngine.h"
#include "DiskMediaExtractor.h"
#include <QImageReader>
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

    return DiskMediaExtractor::getCapsuleThumbnail(filePath, targetSize);
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

            QImage finalImg = DiskMediaExtractor::getCapsuleThumbnailReadOnly(path);
            if (finalImg.isNull()) {
                finalImg = decodeImageToThumbnail(path, targetSize);
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
