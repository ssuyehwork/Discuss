#ifndef ARCMETA_ASYNC_JOB_SCHEDULER_H
#define ARCMETA_ASYNC_JOB_SCHEDULER_H

#include <QFuture>
#include <QThreadPool>
#include <functional>

namespace ArcMeta {

class AsyncJobScheduler {
public:
    template<typename T>
    static QFuture<T> run(std::function<T()> job) {
        return QtConcurrent::run(QThreadPool::globalInstance(), job);
    }

    static QFuture<void> run(std::function<void()> job) {
        return QtConcurrent::run(QThreadPool::globalInstance(), job);
    }
};

} // namespace ArcMeta

#endif // ARCMETA_ASYNC_JOB_SCHEDULER_H
