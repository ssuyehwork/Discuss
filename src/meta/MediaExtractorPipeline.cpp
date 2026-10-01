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

        std::vector<MetadataManager::ExtractedFeatureItem> results;
        results.reserve(batch.size());

        for (const auto& path : batch) {
            if (token->isCanceled() || CoreController::isShuttingDown()) break;

            QString qPath = QString::fromStdWString(path);
            QFileInfo info(qPath);

            MetadataManager::ExtractedFeatureItem item;
            item.path = path;
            item.mtime = info.lastModified().toMSecsSinceEpoch();
            item.fileSize = info.size();

            if (info.isFile() && ColorPaletteEngine::isGraphicsFile(info.suffix().toLower())) {
                // 唯一入口：读缓存 / 失败拦截 / 解码 / 写缓存 全部在 DiskMediaExtractor 内完成
                DiskMediaExtractor::ExtractResult res = DiskMediaExtractor::getCapsuleExtractResult(qPath, DiskMediaExtractor::kThumbSize, token);
                if (res.isValid) {
                    // 缓存命中时不带原始尺寸，补读文件头尺寸
                    QSize sz = res.originalSize.isValid() ? res.originalSize : DiskMediaExtractor::fastExtractImageSize(qPath);
                    if (sz.isValid()) {
                        item.width = sz.width();
                        item.height = sz.height();
                    }

                    // 内存 128 像素内快速测色
                    auto pal = ColorPaletteEngine::extractPaletteFromImage(res.thumbnail512);
                    if (!pal.isEmpty()) {
                        QColor dominant = ColorPaletteEngine::quantizeToStandardColor(pal.first().first);
                        item.autoColor = dominant.name().toUpper().toStdWString();
                        item.palettes.assign(pal.begin(), pal.end());
                    }
                }
            }

            results.push_back(item);
        }

        if (!results.empty() && !token->isCanceled() && !CoreController::isShuttingDown()) {
            MetadataManager::instance().updateExtractedMediaFeaturesBatch(results);
        }

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
