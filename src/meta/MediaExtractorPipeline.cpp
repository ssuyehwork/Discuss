#ifndef NOMINMAX 
#define NOMINMAX 
#endif 
 
#include "MediaExtractorPipeline.h" 
#include "MetadataManager.h" 
#include "../core/CoreController.h" 
#include "../util/DiskMediaExtractor.h" 
#include "../util/ColorPaletteEngine.h" 
#include <QFileInfo> 
#include <QDir> 
#include <QThread> 
#include <QtConcurrent/QtConcurrent> 
#include <QDebug> 
#include <QCoreApplication> 
#include <algorithm> 
 
#ifdef Q_OS_WIN 
#include <windows.h> 
#include <objbase.h> 
#endif 
 
namespace QuarkMeta { 
 
MediaExtractorPipeline& MediaExtractorPipeline::instance() { 
    static MediaExtractorPipeline inst; 
    return inst; 
} 
 
MediaExtractorPipeline::MediaExtractorPipeline(QObject* parent) : QObject(parent) { 
    m_timer = new QTimer(this); 
    m_timer->setInterval(1500); 
    connect(m_timer, &QTimer::timeout, this, &MediaExtractorPipeline::processNextBatch); 
 
    m_workerPool.setMaxThreadCount(qMax(2, QThread::idealThreadCount() / 2)); 
 
    if (QCoreApplication::instance()) { 
        this->moveToThread(QCoreApplication::instance()->thread()); 
    } 
} 
 
MediaExtractorPipeline::~MediaExtractorPipeline() { 
    m_timer->stop(); 
} 

void MediaExtractorPipeline::addWork(int n) {
    if (n <= 0) return;
    std::lock_guard<std::mutex> lock(m_jobMutex);
    if (!m_job.isValid()) {
        TaskJobOptions opts;
        opts.title = "正在提取缩略图与色值";
        opts.total = 0;
        opts.showCount = true;
        opts.showEta = true;
        opts.finishTextTemplate = "提取完成  数量：{total}  |  实际耗时：{elapsed}";
        opts.onTrip = [this]() {
            cancelAll();
        };
        m_job = TaskProgressService::instance().createJobHandle(opts);
    }
    m_job.addTotal(n);
}

void MediaExtractorPipeline::reportDone(int n) {
    if (n <= 0) return;
    std::lock_guard<std::mutex> lock(m_jobMutex);
    if (m_job.isValid()) {
        m_job.advance(n);
        if (m_job.isComplete()) {
            m_job.finish();
        }
    }
}
 
void MediaExtractorPipeline::cancelAll() { 
    std::shared_ptr<CancellationToken> oldToken;
    { 
        std::lock_guard<std::mutex> queueLock(m_queueMutex); 
        m_queue.clear(); 
        oldToken = m_token;
        m_token = std::make_shared<CancellationToken>(); 
    } 
    if (oldToken) {
        oldToken->cancel();
    }
    std::lock_guard<std::mutex> jobLock(m_jobMutex);
    if (m_job.isValid()) {
        m_job.finish();
    }
} 
 
void MediaExtractorPipeline::cancelBatch(const std::vector<std::wstring>& paths) { 
    if (paths.empty()) return; 
    std::lock_guard<std::mutex> queueLock(m_queueMutex); 
     
    std::vector<std::wstring> normPrefixes; 
    normPrefixes.reserve(paths.size()); 
    for (const auto& p : paths) { 
        normPrefixes.push_back(MetadataManager::normalizePath(p)); 
    } 
 
    auto isPrefixMatched = [&](const std::wstring& targetPath) { 
        std::wstring normTarget = MetadataManager::normalizePath(targetPath); 
        for (const auto& prefix : normPrefixes) { 
            if (normTarget == prefix) return true; 
            if (normTarget.find(prefix + L"\\") == 0 || normTarget.find(prefix + L"/") == 0) return true; 
        } 
        return false; 
    }; 
 
    int originalQueueSize = static_cast<int>(m_queue.size()); 
    m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(), isPrefixMatched), m_queue.end()); 
    int removedFromQueue = originalQueueSize - static_cast<int>(m_queue.size()); 
    if (removedFromQueue > 0) {
        reportDone(removedFromQueue);
    }
} 
 
void MediaExtractorPipeline::enqueue(const std::wstring& path, bool countedByCaller) { 
    enqueueBatch({path}, countedByCaller); 
} 
 
void MediaExtractorPipeline::enqueueBatch(const std::vector<std::wstring>& paths, bool countedByCaller) { 
    if (paths.empty()) return;

    if (!countedByCaller) {
        addWork(static_cast<int>(paths.size()));
    }

    { 
        std::lock_guard<std::mutex> lock(m_queueMutex); 
        m_queue.insert(m_queue.end(), paths.begin(), paths.end()); 
    } 
 
    dispatchWorkersIfNeeded(); 
    QMetaObject::invokeMethod(m_timer, "start", Qt::QueuedConnection); 
} 
 
void MediaExtractorPipeline::dispatchWorkersIfNeeded() { 
    size_t qSize = 0; 
    { 
        std::lock_guard<std::mutex> lock(m_queueMutex); 
        qSize = m_queue.size(); 
    } 
    if (qSize == 0) return; 
 
    int maxWorkers = m_workerPool.maxThreadCount(); 
    int targetWorkers = std::min(maxWorkers, static_cast<int>((qSize + 31) / 32)); 
 
    while (m_activeWorkers.load() < targetWorkers) { 
        int current = m_activeWorkers.load(); 
        if (m_activeWorkers.compare_exchange_strong(current, current + 1)) { 
            (void)QtConcurrent::run(&m_workerPool, [this]() { 
                dispatchWorkerLoop(); 
            }); 
        } 
    } 
} 
 
void MediaExtractorPipeline::processNextBatch() { 
    dispatchWorkersIfNeeded(); 
} 
 
void MediaExtractorPipeline::dispatchWorkerLoop() { 
#ifdef Q_OS_WIN 
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); 
#endif 
 
    while (!CoreController::isShuttingDown()) { 
        std::vector<std::wstring> batch; 
        std::shared_ptr<CancellationToken> token; 
        { 
            std::lock_guard<std::mutex> lock(m_queueMutex); 
            if (m_queue.empty()) break; 
            size_t batchSize = std::min(m_queue.size(), static_cast<size_t>(32)); 
            batch.assign(m_queue.begin(), m_queue.begin() + batchSize); 
            m_queue.erase(m_queue.begin(), m_queue.begin() + batchSize); 
            token = m_token; 
        } 
 
        std::vector<MetadataManager::ExtractedFeatureItem> results; 
        results.reserve(batch.size()); 

        int processedInBatch = 0;

        for (const auto& path : batch) { 
            if (token->isCanceled() || CoreController::isShuttingDown()) {
                break;
            }

            QString qPath = QString::fromStdWString(path); 
            QFileInfo info(qPath); 
            if (!info.exists()) {
                processedInBatch++;
                reportDone(1);
                continue;
            }

            long long origSize = info.size();
            long long origMtime = info.lastModified().toMSecsSinceEpoch();

            MetadataManager::ExtractedFeatureItem item; 
            item.path = path; 
            item.mtime = origMtime; 
            item.fileSize = origSize; 

            if (info.isFile() && ColorPaletteEngine::isGraphicsFile(info.suffix().toLower())) { 
                DiskMediaExtractor::ExtractResult res = DiskMediaExtractor::getCapsuleExtractResult(qPath, DiskMediaExtractor::kThumbSize, token); 
                if (!res.isValid || res.thumbnail512.isNull()) {
                    processedInBatch++;
                    reportDone(1);
                    continue;
                }

                QSize sz = res.originalSize.isValid() ? res.originalSize : DiskMediaExtractor::fastExtractImageSize(qPath); 
                if (sz.isValid()) { 
                    item.width = sz.width(); 
                    item.height = sz.height(); 
                } 

                auto pal = ColorPaletteEngine::extractWeightedPalette(res.thumbnail512); 
                if (pal.isEmpty()) {
                    QImage fullImg(qPath);
                    if (!fullImg.isNull()) {
                        pal = ColorPaletteEngine::extractWeightedPalette(fullImg);
                    }
                }

                if (pal.isEmpty()) {
                    processedInBatch++;
                    reportDone(1);
                    continue;
                }

                QColor domColor = pal.first().first;
                item.autoColor = domColor.name().toUpper().toStdWString();
                item.palettes.assign(pal.begin(), pal.end());

                results.push_back(item);
            }

            processedInBatch++;
            reportDone(1);
        } 

        int remainingInBatch = static_cast<int>(batch.size()) - processedInBatch;
        if (remainingInBatch > 0) {
            reportDone(remainingInBatch);
        }

        if (!results.empty()) { 
            MetadataManager::instance().updateExtractedMediaFeaturesBatch(results); 
        } 
    } 
 
    m_activeWorkers.fetch_sub(1); 
 
#ifdef Q_OS_WIN 
    CoUninitialize(); 
#endif 
} 
 
} // namespace QuarkMeta 
