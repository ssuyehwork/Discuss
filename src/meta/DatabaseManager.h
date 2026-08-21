#ifndef QuarkMeta_DATABASE_MANAGER_H
#define QuarkMeta_DATABASE_MANAGER_H

#include <QString>
#include <QObject>
#include <QTimer>
#include <QRecursiveMutex>
#include "sqlite3.h"
#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <memory>
#include <deque>
#include <thread>
#include <condition_variable>
#include <atomic>

struct sqlite3;

namespace QuarkMeta {

/**
 * @brief 数据库事务 RAII 守卫
 * 确保即使在逻辑分支提前返回时事务也能安全关闭。
 */
class SqlTransaction {
public:
    explicit SqlTransaction(struct sqlite3* db);
    ~SqlTransaction();

    bool commit();
    void rollback();

private:
    struct sqlite3* m_db;
    bool m_committed = false;
    bool m_isNested = false;
};

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager& instance();

    QRecursiveMutex* dbMutex() { return &m_dbMutex; }

    /**
     * @brief 初始化数据库（加载所有挂载驱动器的数据库到内存）
     */
    bool init();

    /**
     * @brief 持久化所有内存库到磁盘
     */
    void flushAll(bool forceFull = false);

    /**
     * @brief 2026-07-xx 按照用户要求 (1.21)：步进式持久化接口
     * @return 如果所有备份已完成，返回 true；否则返回 false。
     */
    bool flushStep();

    /**
     * @brief 显式关闭并释放所有数据库资源 (1.21)
     */
    void shutdown();

    /**
     * @brief 获取全局数据库内存连接
     */
    sqlite3* getGlobalDb();

    /**
     * @brief 支持高并发 WAL 模式
     */
    std::mutex& getGlobalMutex() { return m_globalDbMutex; }

    /**
     * @brief 增减并发写入源计数以及控制脏标记
     */
    void incrementWriteSources();
    void decrementWriteSources();
    int getActiveWriteSources() const { return m_activeWriteSources.load(); }
    bool isDirty() const { return m_isDirty.load(); }
    void setDirty(bool dirty) { m_isDirty.store(dirty); }

    /**
     * @brief 将任务投递到异步 I/O 队列
     */
    void enqueueSyncTask(std::function<void()> task);

    /**
     * @brief 获取当前挂起的同步任务总数 (Atomic)
     */
    int getPendingTasksCount() const { return m_pendingTasksCount.load(); }

    /**
     * @brief 内部接口：增减任务计数 (Plan-131 方案 D)
     */
    void incrementPendingTasks();
    void decrementPendingTasks();

signals:
    /**
     * @brief 异步任务计数变更信号
     */
    void pendingTasksCountChanged(int count);

private:
    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    struct DbConnection {
        sqlite3* diskDb = nullptr;
        sqlite3* memDb = nullptr;
        sqlite3_backup* activeBackup = nullptr;
        std::wstring diskPath;
    };

    void startWorkerThread();
    void stopWorkerThread();
    void workerLoop();

    std::deque<std::function<void()>> m_syncQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::thread m_workerThread;
    std::atomic<bool> m_stopWorker{false};
    std::atomic<int> m_pendingTasksCount{0};
    QTimer* m_syncTimer = nullptr;

    /**
     * @brief 异步任务 RAII 令牌 (Plan-131 方案 D)
     */
    struct SyncTaskToken {
        SyncTaskToken();
        SyncTaskToken(const SyncTaskToken&) = delete;
        SyncTaskToken& operator=(const SyncTaskToken&) = delete;
        SyncTaskToken(SyncTaskToken&& other) noexcept;
        SyncTaskToken& operator=(SyncTaskToken&&) = delete;
        ~SyncTaskToken();
    private:
        bool m_moved = false;
    };

    DbConnection m_globalDb;
    std::mutex m_mutex;
    QRecursiveMutex m_dbMutex;

    std::atomic<int> m_activeWriteSources{0};
    std::atomic<bool> m_isBackupRunning{false};
    std::atomic<bool> m_isDirty{false};

    std::mutex m_globalDbMutex;

    bool loadDb(const std::wstring& diskPath, DbConnection& conn);
    bool saveDb(DbConnection& conn, bool forceFull = false);
    void closeDb(DbConnection& conn);

    QString getAppDir();
};

class WriteGuard {
public:
    WriteGuard() {
        DatabaseManager::instance().incrementWriteSources();
    }
    ~WriteGuard() {
        DatabaseManager::instance().decrementWriteSources();
    }
};

} // namespace QuarkMeta

#endif // QuarkMeta_DATABASE_MANAGER_H
