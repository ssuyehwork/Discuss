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
 
void MediaExtractorPipeline::cancelAll() { 
    { 
        std::lock_guard<std::mutex> queueLock(m_queueMutex); 
        m_queue.clear(); 
        // 取消当前代令牌（正在解码的任务会被中断），并换上新一代令牌供后续任务使用 
        m_token->cancel(); 
        m_token = std::make_shared<CancellationToken>(); 
    } 
    m_activeCount.store(0); 
} 
 
void MediaExtractorPipeline::cancelBatch(const std::vector<std::wstring>& paths) { 
    if (paths.empty()) return; 
    std::lock_guard<std::mutex> queueLock(m_queueMutex); 
     
    // 收集标准化的前缀用于批量匹配过滤 
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
    Q_UNUSED(removedFromQueue); 
} 
 
void MediaExtractorPipeline::enqueue(const std::wstring& path) { 
    enqueueBatch({path}); 
} 
 
void MediaExtractorPipeline::enqueueBatch(const std::vector<std::wstring>& paths) { 
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
    // 1500ms 定时器作为心跳兜底调度，防止在边缘并发场景下工作线程挂起导致队列未消费完 
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
            token = m_token; // 本批任务绑定取到任务时的当代令牌 
 
            m_activeCount.fetch_add(static_cast<int>(batch.size())); 
        } 
 
        auto startTime = std::chrono::steady_clock::now();
        int batchSuccessCount = 0;
        int batchFailureCount = 0;

        std::vector<MetadataManager::ExtractedFeatureItem> results; 
        results.reserve(batch.size()); 

        for (const auto& path : batch) { 
            if (token->isCanceled() || CoreController::isShuttingDown()) {
                qDebug() << "[AutoColor] [MediaExtractorPipeline] Item skipped - Cause: Canceled for:" << QString::fromStdWString(path);
                batchFailureCount++;
                break;
            }

            QString qPath = QString::fromStdWString(path); 
            QFileInfo info(qPath); 
            if (!info.exists()) {
                qDebug() << "[AutoColor] [MediaExtractorPipeline] Item skipped - Cause: File not exists:" << qPath;
                batchFailureCount++;
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
                    qDebug() << "[AutoColor] [MediaExtractorPipeline] Item skipped - Cause: Thumbnail extraction failed for:" << qPath;
                    batchFailureCount++;
                    continue;
                }

                QSize sz = res.originalSize.isValid() ? res.originalSize : DiskMediaExtractor::fastExtractImageSize(qPath);
                if (sz.isValid()) {
                    item.width = sz.width();
                    item.height = sz.height();
                } 

                // 使用 Section I 的权威算法 extractWeightedPalette 提取调色板（最多 10 项）
                auto pal = ColorPaletteEngine::extractWeightedPalette(res.thumbnail512);
                if (pal.isEmpty()) {
                    QImage fullImg(qPath);
                    if (!fullImg.isNull()) {
                        pal = ColorPaletteEngine::extractWeightedPalette(fullImg);
                    }
                }

                if (pal.isEmpty()) {
                    qDebug() << "[AutoColor] [MediaExtractorPipeline] Item skipped - Cause: Palette extraction empty for:" << qPath;
                    batchFailureCount++;
                    continue;
                }

                // autoColor = 调色板第一项颜色，直接转大写 #RRGGBB，不做任何标准色量化
                QColor domColor = pal.first().first;
                item.autoColor = domColor.name().toUpper().toStdWString();
                item.palettes.assign(pal.begin(), pal.end());

                results.push_back(item);
                batchSuccessCount++;
            } else {
                qDebug() << "[AutoColor] [MediaExtractorPipeline] Item skipped - Cause: Not a graphics file:" << qPath;
                batchFailureCount++;
            }
        } 

        // 即使 token 被取消，本批已经完成的单个文件结果仍要写入！
        if (!results.empty()) {
            MetadataManager::instance().updateExtractedMediaFeaturesBatch(results); 
        } 

        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
        qDebug() << "[AutoColor] [MediaExtractorPipeline] Batch process finished - Success:" << batchSuccessCount
                 << "Failure:" << batchFailureCount << "ElapsedMs:" << elapsedMs;
 
        { 
            std::lock_guard<std::mutex> lock(m_queueMutex); 
            m_activeCount.fetch_sub(static_cast<int>(batch.size())); 
        } 
    } 
 
    m_activeWorkers.fetch_sub(1); 
 
#ifdef Q_OS_WIN 
    CoUninitialize(); 
#endif 
} 
 
} // namespace QuarkMeta 
