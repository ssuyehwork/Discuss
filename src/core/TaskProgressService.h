#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include <QDateTime>
#include <memory>
#include <functional>
#include "CoreEngine.h" // For CancellationToken

namespace QuarkMeta {

struct TextSegment {
    QString text;
    bool highlight = false;
};

struct TaskProgressSnapshot {
    bool active = false;
    int percent = 0;
    QString title;
    QString detail;
    int current = 0;
    int total = 0;
    int etaSeconds = -1;
    int activeCount = 0;
    QList<TextSegment> statusSegments;
    bool finished = false;
    QList<TextSegment> finishSegments;
};

struct TaskJobOptions {
    QString title;
    int total = 0;
    bool showCount = true;
    bool showEta = true;
    QString finishTextTemplate; // e.g. "提取完成  数量：{total}  |  实际耗时：{elapsed}"
    std::function<void()> onTrip;
};

class TaskProgressJob {
public:
    TaskProgressJob() = default;
    TaskProgressJob(int id, std::shared_ptr<CancellationToken> token);
    ~TaskProgressJob();

    TaskProgressJob(const TaskProgressJob&) = delete;
    TaskProgressJob& operator=(const TaskProgressJob&) = delete;

    TaskProgressJob(TaskProgressJob&& other) noexcept;
    TaskProgressJob& operator=(TaskProgressJob&& other) noexcept;

    void addTotal(int delta);
    void advance(int delta = 1);
    void setDetail(const QString& detail);
    bool isComplete() const;
    bool isValid() const;
    void finish();

    std::shared_ptr<CancellationToken> cancellationToken() const { return m_token; }

private:
    int m_id = -1;
    std::shared_ptr<CancellationToken> m_token;
};

class TaskProgressService : public QObject {
    Q_OBJECT

public:
    static TaskProgressService& instance();

    TaskProgressJob createJobHandle(const TaskJobOptions& options);

    void tripAll();
    bool hasActiveJobs() const;

signals:
    void snapshotChanged(const TaskProgressSnapshot& snapshot);

private:
    friend class TaskProgressJob;

    explicit TaskProgressService(QObject* parent = nullptr);
    ~TaskProgressService() override = default;
    TaskProgressService(const TaskProgressService&) = delete;
    TaskProgressService& operator=(const TaskProgressService&) = delete;

    void jobAddTotal(int id, int delta);
    void jobAdvance(int id, int delta);
    void jobSetDetail(int id, const QString& detail);
    bool jobIsComplete(int id) const;
    bool jobIsValid(int id) const;
    void jobFinish(int id);

    void scheduleSnapshotEmit();
    void emitSnapshotNow();

    struct InternalJobInfo {
        int id = -1;
        TaskJobOptions options;
        int current = 0;
        int total = 0;
        QString detail;
        qint64 startTimeMs = 0;
        std::shared_ptr<CancellationToken> token;
    };

    mutable QMutex m_mutex;
    int m_nextJobId = 1;
    QMap<int, InternalJobInfo> m_jobs;

    QTimer* m_throttleTimer = nullptr;
    QTimer* m_secondTimer = nullptr;
    bool m_pendingEmit = false;
};

} // namespace QuarkMeta
