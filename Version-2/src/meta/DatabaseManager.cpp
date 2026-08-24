#include "DatabaseManager.h"
#include "DatabaseMigrator.h"
#include "DriveMetaDao.h"
#include <chrono>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
#include <windows.h>
#include "../util/AppDirectoryInitializer.h"

namespace {
#ifdef Q_OS_WIN
    inline void ensureHidden(const std::wstring& path) {
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
    }
#else
    inline void ensureHidden(const std::wstring&) {}
#endif
} // anonymous namespace

namespace QuarkMeta {

SqlTransaction::SqlTransaction(struct sqlite3* db) : m_db(db) {
    DatabaseManager::instance().incrementWriteSources();
    if (m_db) {
        // 2026-07-xx 物理修复 (1.22)：通过检测 autocommit 状态支持伪嵌套事务。
        // 如果 autocommit 为 0，说明已经处于外部事务中。
        m_isNested = (sqlite3_get_autocommit(m_db) == 0);
        
        if (!m_isNested) {
            // 彻底剥离 L30 忙等 Sleep(50) 补丁，完全基于连接建立时内置的 sqlite3_busy_timeout(25000) 机制进行优雅挂起
            sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        }
    }
}

SqlTransaction::~SqlTransaction() {
    if (m_db && !m_committed && !m_isNested) {
        sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
    }
    DatabaseManager::instance().decrementWriteSources();
}

bool SqlTransaction::commit() {
    if (m_db && !m_committed) {
        if (m_isNested) {
            m_committed = true;
            return true;
        }
        if (sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr) == SQLITE_OK) {
            m_committed = true;
            return true;
        }
    }
    return false;
}

void SqlTransaction::rollback() {
    if (m_db && !m_committed) {
        if (!m_isNested) {
            sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
        }
        m_committed = true; // Mark as "processed" to prevent dtor rollback
    }
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::SyncTaskToken::SyncTaskToken() {
    DatabaseManager::instance().incrementPendingTasks();
}

DatabaseManager::SyncTaskToken::SyncTaskToken(SyncTaskToken&& other) noexcept {
    other.m_moved = true;
}

DatabaseManager::SyncTaskToken::~SyncTaskToken() {
    if (!m_moved) {
        DatabaseManager::instance().decrementPendingTasks();
    }
}

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
    startWorkerThread();

    m_syncTimer = new QTimer(this);
    m_syncTimer->setInterval(15000);
    connect(m_syncTimer, &QTimer::timeout, this, [this]() {
        enqueueSyncTask([this]() {
            flushAll();
        });
    });
    m_syncTimer->start();

    // 【修复】必须放在定时器创建完成之后：moveToThread 只会带走
    // 调用时已存在的子对象，先创建子对象、最后再整体迁移线程，
    // 才能保证 m_syncTimer 真正跟随主线程事件循环运行，保障后台定期兜底存盘保险正常运行。
    if (QCoreApplication::instance()) {
        this->moveToThread(QCoreApplication::instance()->thread());
    }
}

DatabaseManager::~DatabaseManager() {
    if (m_syncTimer) {
        m_syncTimer->stop();
    }
    stopWorkerThread();
    flushAll(true);
    closeDb(m_globalDb);
}

QString DatabaseManager::getAppDir() {
    return QCoreApplication::applicationDirPath();
}

bool DatabaseManager::loadDb(const std::wstring& diskPath, DbConnection& conn) {
    std::string utf8Path = QString::fromStdWString(diskPath).toUtf8().toStdString();
    
    // 打开独立的磁盘数据库连接
    if (sqlite3_open_v2(utf8Path.c_str(), &conn.diskDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_busy_timeout(conn.diskDb, 25000);

    // 打开独立的内存数据库连接
    if (sqlite3_open_v2(":memory:", &conn.memDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        sqlite3_close_v2(conn.diskDb);
        conn.diskDb = nullptr;
        return false;
    }
    sqlite3_busy_timeout(conn.memDb, 25000);
    // 🚀【修改方案一】：彻底删去对 ShellHelper::ensureHidden 的直接耦合，保持 DAL 纯粹性

    // 使用 SQLite Backup API 将 conn.diskDb 的数据一次性导入内存 conn.memDb
    sqlite3_backup* backup = sqlite3_backup_init(conn.memDb, "main", conn.diskDb, "main");
    if (backup) {
        sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
    } else {
    }

    // 配置高性能 WAL 模式
    sqlite3_exec(conn.diskDb, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.memDb, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.memDb, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);

    // 初始化表结构 (Schema)
    const char* schema = R"(
        -- 标签组表
        CREATE TABLE IF NOT EXISTS tag_groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            color TEXT,
            sort_order INTEGER DEFAULT 0
        );

        -- 标签与标签组关联表
        CREATE TABLE IF NOT EXISTS tag_group_items (
            group_id INTEGER,
            tag_name TEXT,
            PRIMARY KEY (group_id, tag_name)
        );

        -- 物理磁盘回收站独立表
        CREATE TABLE IF NOT EXISTS disk_trash (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id TEXT NOT NULL,
            trash_path TEXT NOT NULL,
            original_path TEXT NOT NULL,
            drive_letter TEXT NOT NULL,
            file_name TEXT NOT NULL,
            is_folder INTEGER DEFAULT 0,
            file_size INTEGER DEFAULT 0,
            created_at INTEGER DEFAULT 0,
            deleted_at INTEGER DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_disk_trash_drive_letter ON disk_trash(drive_letter);
    )";
    char* errMsg = nullptr;
    sqlite3_exec(conn.memDb, schema, nullptr, nullptr, &errMsg);
    if (errMsg) {
        sqlite3_free(errMsg);
    }

    conn.diskPath = diskPath;
    return true;
}

bool DatabaseManager::saveDb(DbConnection& conn, bool forceFull) {
    if (!conn.diskDb || !conn.memDb) {
        return false;
    }

    (void)forceFull;
    sqlite3_backup* backup = sqlite3_backup_init(conn.diskDb, "main", conn.memDb, "main");
    if (backup) {
        int rc = SQLITE_OK;
        // 每次只备份 64 个 Pager 页，分片让路，避免长时间锁死 SQLite 数据库
        do {
            rc = sqlite3_backup_step(backup, 64);
            if (rc == SQLITE_OK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2)); // 主动让出数据库锁
            }
        } while (rc == SQLITE_OK);

        sqlite3_backup_finish(backup);
        if (rc == SQLITE_DONE) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void DatabaseManager::closeDb(DbConnection& conn) {
    if (conn.memDb) {
        sqlite3_close_v2(conn.memDb);
    }
    if (conn.diskDb) {
        sqlite3_close_v2(conn.diskDb);
    }
    conn.memDb = nullptr;
    conn.diskDb = nullptr;
}

bool DatabaseManager::init() {
    std::lock_guard<std::mutex> lock(m_mutex);
    AppDirectoryInitializer::initializeStoragePath(getAppDir());

    QString metaDir = getAppDir() + "/.QuarkMeta";

    // 加载全局库
    std::wstring globalPath = (metaDir + "/global.db").toStdWString();
    loadDb(globalPath, m_globalDb);

    // 初始化盘符元数据表
    DriveMetaDao::initTable();

    return true;
}

void DatabaseManager::flushAll(bool forceFull) {
    if (!m_isDirty.load()) {
        return;
    }

    DbConnection globalConn;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        globalConn = m_globalDb;
    }

    bool allSucceeded = true;
    
    // 全局库独立加锁落盘
    {
        std::lock_guard<std::mutex> lockGlobal(m_globalDbMutex);
        if (!saveDb(globalConn, forceFull)) {
            allSucceeded = false;
        }
    }
    
    if (allSucceeded) {
        m_isDirty.store(false);
    }
}

bool DatabaseManager::flushStep() {
    // [Plan-130] 秒退架构：彻底废除 flushStep
    return true;
}

void DatabaseManager::shutdown() {
    if (m_syncTimer) {
        m_syncTimer->stop();
    }
    stopWorkerThread();

    flushAll(true);

    std::lock_guard<std::mutex> lock(m_mutex);
    closeDb(m_globalDb);
}

sqlite3* DatabaseManager::getGlobalDb() {
    return m_globalDb.memDb;
}

void DatabaseManager::incrementWriteSources() {
    m_activeWriteSources.fetch_add(1);
    m_isDirty.store(true);
}

void DatabaseManager::decrementWriteSources() {
    m_activeWriteSources.fetch_sub(1);
}

void DatabaseManager::incrementPendingTasks() {
    int count = ++m_pendingTasksCount;
    emit pendingTasksCountChanged(count);
}

void DatabaseManager::decrementPendingTasks() {
    int count = --m_pendingTasksCount;
    emit pendingTasksCountChanged(count);
}

void DatabaseManager::enqueueSyncTask(std::function<void()> task) {
    auto token = std::make_shared<SyncTaskToken>(); 
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_syncQueue.push_back([task, token]() {
            task();
        });
    }
    m_queueCv.notify_one();
}

void DatabaseManager::startWorkerThread() {
    m_stopWorker = false;
    m_workerThread = std::thread(&DatabaseManager::workerLoop, this);
}

void DatabaseManager::stopWorkerThread() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stopWorker = true;
    }
    m_queueCv.notify_all();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void DatabaseManager::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait(lock, [this] { return m_stopWorker || !m_syncQueue.empty(); });
            if (m_stopWorker && m_syncQueue.empty()) break;
            task = std::move(m_syncQueue.front());
            m_syncQueue.pop_front();
        }
        if (task) {
            task();
        }
    }
}


} // namespace QuarkMeta
