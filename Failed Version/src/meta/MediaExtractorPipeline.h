#pragma once 
 
#include <QObject> 
#include <QTimer> 
#include <QColor> 
#include <QPair> 
#include <QVector> 
#include <QThreadPool> 
#include <vector> 
#include <string> 
#include <mutex> 
#include <atomic> 
#include <memory> 
#include "../util/DiskMediaExtractor.h" 
#include "../core/TaskProgressService.h"
 
namespace QuarkMeta { 
 
class MediaExtractorPipeline : public QObject { 
    Q_OBJECT 
public: 
    static MediaExtractorPipeline& instance(); 
 
    void enqueue(const std::wstring& path, bool countedByCaller = false); 
    void enqueueBatch(const std::vector<std::wstring>& paths, bool countedByCaller = false); 
 
    void addWork(int n);
    void reportDone(int n = 1);

    void cancelAll(); 
    void cancelBatch(const std::vector<std::wstring>& paths); 
 
private slots: 
    void processNextBatch(); 
 
private: 
    MediaExtractorPipeline(QObject* parent = nullptr); 
    ~MediaExtractorPipeline() override; 
 
    void dispatchWorkersIfNeeded(); 
    void dispatchWorkerLoop(); 
 
    std::vector<std::wstring> m_queue; 
    QTimer* m_timer; 
    std::mutex m_queueMutex; 
    std::mutex m_jobMutex;
    TaskProgressJob m_job;

    std::atomic<int> m_activeWorkers{0}; // 活跃工作线程数 
    std::shared_ptr<CancellationToken> m_token = std::make_shared<CancellationToken>(); // 当前代取消令牌，受 m_queueMutex 保护 
    QThreadPool m_workerPool; // 后台提取专用线程池，不占全局线程池 
}; 
 
} // namespace QuarkMeta 
