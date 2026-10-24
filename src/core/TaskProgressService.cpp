#include "TaskProgressService.h"
#include <QMutexLocker>
#include <QCoreApplication>
#include <algorithm>

namespace QuarkMeta {

// --- TaskProgressJob ---

TaskProgressJob::TaskProgressJob(int id, std::shared_ptr<CancellationToken> token)
    : m_id(id), m_token(std::move(token)) {}

TaskProgressJob::~TaskProgressJob() {
    finish();
}

TaskProgressJob::TaskProgressJob(TaskProgressJob&& other) noexcept
    : m_id(other.m_id), m_token(std::move(other.m_token)) {
    other.m_id = -1;
}

TaskProgressJob& TaskProgressJob::operator=(TaskProgressJob&& other) noexcept {
    if (this != &other) {
        finish();
        m_id = other.m_id;
        m_token = std::move(other.m_token);
        other.m_id = -1;
    }
    return *this;
}

void TaskProgressJob::addTotal(int delta) {
    if (m_id != -1) {
        TaskProgressService::instance().jobAddTotal(m_id, delta);
    }
}

void TaskProgressJob::advance(int delta) {
    if (m_id != -1) {
        TaskProgressService::instance().jobAdvance(m_id, delta);
    }
}

void TaskProgressJob::setDetail(const QString& detail) {
    if (m_id != -1) {
        TaskProgressService::instance().jobSetDetail(m_id, detail);
    }
}

bool TaskProgressJob::isComplete() const {
    if (m_id != -1) {
        return TaskProgressService::instance().jobIsComplete(m_id);
    }
    return true;
}

bool TaskProgressJob::isValid() const {
    if (m_id != -1) {
        return TaskProgressService::instance().jobIsValid(m_id);
    }
    return false;
}

void TaskProgressJob::finish() {
    if (m_id != -1) {
        TaskProgressService::instance().jobFinish(m_id);
        m_id = -1;
    }
}

// --- TaskProgressService ---

TaskProgressService& TaskProgressService::instance() {
    static TaskProgressService s_instance;
    return s_instance;
}

TaskProgressService::TaskProgressService(QObject* parent) : QObject(parent) {
    moveToThread(QCoreApplication::instance()->thread());

    m_throttleTimer = new QTimer(this);
    m_throttleTimer->setSingleShot(true);
    m_throttleTimer->setInterval(100);
    connect(m_throttleTimer, &QTimer::timeout, this, [this]() {
        emitSnapshotNow();
    });

    m_secondTimer = new QTimer(this);
    m_secondTimer->setInterval(1000);
    connect(m_secondTimer, &QTimer::timeout, this, [this]() {
        if (hasActiveJobs()) {
            scheduleSnapshotEmit();
        }
    });
    m_secondTimer->start();
}

TaskProgressJob TaskProgressService::createJobHandle(const TaskJobOptions& options) {
    int id = 0;
    std::shared_ptr<CancellationToken> token = std::make_shared<CancellationToken>();

    {
        QMutexLocker locker(&m_mutex);
        id = m_nextJobId++;
        InternalJobInfo info;
        info.id = id;
        info.options = options;
        info.current = 0;
        info.total = options.total;
        info.startTimeMs = QDateTime::currentMSecsSinceEpoch();
        info.token = token;
        m_jobs.insert(id, info);
    }

    scheduleSnapshotEmit();

    return TaskProgressJob(id, token);
}

void TaskProgressService::jobAddTotal(int id, int delta) {
    if (delta <= 0) return;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_jobs.contains(id)) return;
        m_jobs[id].total += delta;
    }
    scheduleSnapshotEmit();
}

void TaskProgressService::jobAdvance(int id, int delta) {
    if (delta <= 0) return;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_jobs.contains(id)) return;
        m_jobs[id].current += delta;
    }
    scheduleSnapshotEmit();
}

void TaskProgressService::jobSetDetail(int id, const QString& detail) {
    {
        QMutexLocker locker(&m_mutex);
        if (!m_jobs.contains(id)) return;
        m_jobs[id].detail = detail;
    }
    scheduleSnapshotEmit();
}

bool TaskProgressService::jobIsComplete(int id) const {
    QMutexLocker locker(&m_mutex);
    if (!m_jobs.contains(id)) return true;
    const auto& info = m_jobs[id];
    return info.current >= info.total;
}

bool TaskProgressService::jobIsValid(int id) const {
    QMutexLocker locker(&m_mutex);
    return m_jobs.contains(id);
}

static QList<TextSegment> parseTemplateToSegments(const QString& tmpl, int current, int total, qint64 elapsedMs) {
    QList<TextSegment> segments;
    qint64 elapsedSec = elapsedMs / 1000;
    qint64 mm = elapsedSec / 60;
    qint64 ss = elapsedSec % 60;
    QString elapsedStr = QString("%1:%2")
        .arg(mm, 2, 10, QChar('0'))
        .arg(ss, 2, 10, QChar('0'));

    int pos = 0;
    while (pos < tmpl.length()) {
        int idxCurrent = tmpl.indexOf("{current}", pos);
        int idxTotal = tmpl.indexOf("{total}", pos);
        int idxElapsed = tmpl.indexOf("{elapsed}", pos);

        int nextIdx = -1;
        QString token;
        if (idxCurrent != -1 && (nextIdx == -1 || idxCurrent < nextIdx)) { nextIdx = idxCurrent; token = "{current}"; }
        if (idxTotal != -1 && (nextIdx == -1 || idxTotal < nextIdx)) { nextIdx = idxTotal; token = "{total}"; }
        if (idxElapsed != -1 && (nextIdx == -1 || idxElapsed < nextIdx)) { nextIdx = idxElapsed; token = "{elapsed}"; }

        if (nextIdx == -1) {
            QString plain = tmpl.mid(pos);
            if (!plain.isEmpty()) segments.append({plain, false});
            break;
        }

        if (nextIdx > pos) {
            segments.append({tmpl.mid(pos, nextIdx - pos), false});
        }

        if (token == "{current}") {
            segments.append({QString::number(current), true});
        } else if (token == "{total}") {
            segments.append({QString::number(total), true});
        } else if (token == "{elapsed}") {
            segments.append({elapsedStr, true});
        }

        pos = nextIdx + token.length();
    }
    return segments;
}

void TaskProgressService::jobFinish(int id) {
    TaskProgressSnapshot finishSnapshot;
    bool shouldEmitFinish = false;

    {
        QMutexLocker locker(&m_mutex);
        if (!m_jobs.contains(id)) return;
        InternalJobInfo info = m_jobs.take(id);

        if (!info.options.finishTextTemplate.isEmpty()) {
            qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - info.startTimeMs;
            finishSnapshot.active = false;
            finishSnapshot.finished = true;
            finishSnapshot.finishSegments = parseTemplateToSegments(
                info.options.finishTextTemplate,
                info.current,
                info.total,
                elapsedMs
            );
            shouldEmitFinish = true;
        }
    }

    if (shouldEmitFinish) {
        QMetaObject::invokeMethod(this, [this, finishSnapshot]() {
            emit snapshotChanged(finishSnapshot);
        }, Qt::QueuedConnection);
    }

    scheduleSnapshotEmit();
}

void TaskProgressService::tripAll() {
    QList<std::shared_ptr<CancellationToken>> tokensToCancel;
    QList<std::function<void()>> callbacksToCall;
    TaskProgressSnapshot tripSnapshot;

    {
        QMutexLocker locker(&m_mutex);
        if (m_jobs.isEmpty()) return;

        int totalCurrent = 0;
        int totalTotal = 0;

        for (const auto& job : m_jobs) {
            if (job.token) tokensToCancel.append(job.token);
            if (job.options.onTrip) callbacksToCall.append(job.options.onTrip);
            totalCurrent += job.current;
            totalTotal += job.total;
        }

        m_jobs.clear();

        tripSnapshot.active = false;
        tripSnapshot.finished = true;
        tripSnapshot.finishSegments.append({"已中止  已处理 ", false});
        tripSnapshot.finishSegments.append({QString("%1/%2").arg(totalCurrent).arg(totalTotal), true});
    }

    // 锁外触发 cancel 和 onTrip 回调
    for (auto& token : tokensToCancel) {
        if (token) token->cancel();
    }
    for (auto& cb : callbacksToCall) {
        if (cb) cb();
    }

    QMetaObject::invokeMethod(this, [this, tripSnapshot]() {
        emit snapshotChanged(tripSnapshot);
    }, Qt::QueuedConnection);

    scheduleSnapshotEmit();
}

bool TaskProgressService::hasActiveJobs() const {
    QMutexLocker locker(&m_mutex);
    return !m_jobs.isEmpty();
}

void TaskProgressService::scheduleSnapshotEmit() {
    QMetaObject::invokeMethod(this, [this]() {
        m_pendingEmit = true;
        if (!m_throttleTimer->isActive()) {
            m_throttleTimer->start();
        }
    }, Qt::QueuedConnection);
}

void TaskProgressService::emitSnapshotNow() {
    m_pendingEmit = false;
    TaskProgressSnapshot snapshot;

    {
        QMutexLocker locker(&m_mutex);
        if (m_jobs.isEmpty()) {
            snapshot.active = false;
            snapshot.percent = 0;
            snapshot.finished = false;
            emit snapshotChanged(snapshot);
            return;
        }

        snapshot.active = true;
        snapshot.activeCount = m_jobs.size();

        // 以最后一个创办或综合数据组装快照
        const auto& mainJob = m_jobs.last();
        snapshot.title = mainJob.options.title;
        snapshot.detail = mainJob.detail;
        snapshot.current = mainJob.current;
        snapshot.total = mainJob.total;

        double sumPercent = 0.0;
        for (const auto& job : m_jobs) {
            double p = (job.total > 0) ? (static_cast<double>(job.current) / job.total * 100.0) : 0.0;
            sumPercent += p;
        }
        snapshot.percent = std::clamp(static_cast<int>(sumPercent / m_jobs.size()), 0, 100);

        // 组装 statusSegments
        snapshot.statusSegments.append({snapshot.title + " ", false});
        snapshot.statusSegments.append({QString("%1%").arg(snapshot.percent), true});

        if (mainJob.options.showCount) {
            snapshot.statusSegments.append({"  数量：", false});
            snapshot.statusSegments.append({QString("%1/%2").arg(snapshot.current).arg(snapshot.total), true});
        }

        if (mainJob.options.showEta) {
            snapshot.statusSegments.append({"  |  剩余 ", false});
            if (snapshot.percent < 5) {
                snapshot.statusSegments.append({"计算中", true});
            } else {
                qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - mainJob.startTimeMs;
                qint64 estTotalMs = (elapsedMs * (100 - snapshot.percent)) / snapshot.percent;
                qint64 etaSec = estTotalMs / 1000;
                snapshot.etaSeconds = static_cast<int>(etaSec);

                QString etaStr;
                if (etaSec >= 3600) {
                    qint64 hh = etaSec / 3600;
                    qint64 mm = (etaSec % 3600) / 60;
                    qint64 ss = etaSec % 60;
                    etaStr = QString("%1:%2:%3")
                        .arg(hh, 2, 10, QChar('0'))
                        .arg(mm, 2, 10, QChar('0'))
                        .arg(ss, 2, 10, QChar('0'));
                } else {
                    qint64 mm = etaSec / 60;
                    qint64 ss = etaSec % 60;
                    etaStr = QString("%1:%2")
                        .arg(mm, 2, 10, QChar('0'))
                        .arg(ss, 2, 10, QChar('0'));
                }
                snapshot.statusSegments.append({etaStr, true});
            }
        }

        if (!mainJob.detail.isEmpty()) {
            snapshot.statusSegments.append({"  " + mainJob.detail, false});
        }

        if (snapshot.activeCount > 1) {
            snapshot.statusSegments.append({"（", false});
            snapshot.statusSegments.append({QString::number(snapshot.activeCount), true});
            snapshot.statusSegments.append({" 项并发任务）", false});
        }
    }

    emit snapshotChanged(snapshot);
}

} // namespace QuarkMeta
