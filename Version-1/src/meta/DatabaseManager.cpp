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
#include "MetadataManager.h"
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
        CREATE TABLE IF NOT EXISTS metadata (
            folder_id TEXT PRIMARY KEY,
            path TEXT NOT NULL,
            is_folder INTEGER DEFAULT 0,
            rating INTEGER DEFAULT 0,
            color TEXT,
            tags TEXT,
            note TEXT,
            url TEXT,
            ctime INTEGER,
            mtime INTEGER,
            atime INTEGER,
            file_size INTEGER,
            palettes BLOB,
            is_trash INTEGER DEFAULT 0,
            original_path TEXT,
            width INTEGER DEFAULT 0,
            height INTEGER DEFAULT 0,
            ingestion_status INTEGER DEFAULT -1,
            auto_color TEXT DEFAULT '',
            base_name TEXT DEFAULT '',
            ext TEXT DEFAULT '',
            added_at INTEGER DEFAULT 0,
            sha256 TEXT DEFAULT ''
        );
        CREATE INDEX IF NOT EXISTS idx_path ON metadata(path);
        CREATE INDEX IF NOT EXISTS idx_metadata_added_at ON metadata(added_at);
        CREATE INDEX IF NOT EXISTS idx_metadata_hash ON metadata(file_size, sha256);

        -- 系统统计表
        CREATE TABLE IF NOT EXISTS system_stats (
            key TEXT PRIMARY KEY,
            value INTEGER DEFAULT 0
        );

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

        -- 物理磁盘回收站独立表 (双轨隔离)
        CREATE TABLE IF NOT EXISTS disk_trash (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id TEXT NOT NULL,           -- 项目自身 File_ID 隔离盒标识
            trash_path TEXT NOT NULL,        -- 暂存区物理路径
            original_path TEXT NOT NULL,     -- 原始物理绝对路径
            drive_letter TEXT NOT NULL,      -- 所属盘符
            file_name TEXT NOT NULL,         -- 原始文件名
            is_folder INTEGER DEFAULT 0,     -- 是否为文件夹 (1: 是, 0: 否)
            file_size INTEGER DEFAULT 0,     -- 文件大小
            created_at INTEGER DEFAULT 0,    -- 原始创建时间戳 (毫秒)
            deleted_at INTEGER DEFAULT 0     -- 删除时间戳 (毫秒)
        );
        CREATE INDEX IF NOT EXISTS idx_disk_trash_drive_letter ON disk_trash(drive_letter);
    )";
    char* errMsg = nullptr;
    sqlite3_exec(conn.memDb, schema, nullptr, nullptr, &errMsg);
    if (errMsg) {
        sqlite3_free(errMsg);
    } else {
        // FTS5 trigram 模糊匹配与自动触发器同步
        const char* ftsSchema = R"(
            CREATE VIRTUAL TABLE IF NOT EXISTS metadata_fts USING fts5(
                folder_id UNINDEXED,  
                path,  
                tags,  
                note,  
                content='metadata', 
                content_rowid='rowid', 
                tokenize="trigram"
            );
            CREATE TRIGGER IF NOT EXISTS tb_metadata_insert AFTER INSERT ON metadata BEGIN
                INSERT INTO metadata_fts(rowid, folder_id, path, tags, note)
                VALUES (new.rowid, new.folder_id, new.path, new.tags, new.note);
            END;
            CREATE TRIGGER IF NOT EXISTS tb_metadata_update AFTER UPDATE ON metadata BEGIN
                INSERT INTO metadata_fts(metadata_fts, rowid, folder_id, path, tags, note)
                VALUES('delete', old.rowid, old.folder_id, old.path, old.tags, old.note);
                INSERT INTO metadata_fts(rowid, folder_id, path, tags, note)
                VALUES(new.rowid, new.folder_id, new.path, new.tags, new.note);
            END;
            CREATE TRIGGER IF NOT EXISTS tb_metadata_delete AFTER DELETE ON metadata BEGIN
                INSERT INTO metadata_fts(metadata_fts, rowid, folder_id, path, tags, note)
                VALUES('delete', old.rowid, old.folder_id, old.path, old.tags, old.note);
            END;
        )";
        char* ftsErrMsg = nullptr;
        sqlite3_exec(conn.memDb, ftsSchema, nullptr, nullptr, &ftsErrMsg);
        if (ftsErrMsg) {
                sqlite3_free(ftsErrMsg);
        } else {
            // Rebuild FTS index to populate any data loaded from disk
            sqlite3_exec(conn.memDb, "INSERT INTO metadata_fts(metadata_fts) VALUES('rebuild');", nullptr, nullptr, nullptr);
        }
    }

    // 2026-07-xx 物理加固：自动迁移旧版本数据库字段 (Plan-29)
    sqlite3_stmt* checkStmt;
    
    // 🚨 2026-08-xx 物理对齐：自动检测并合并迁移历史遗留的 file_id 字段为标准 folder_id 字段
    bool hasFileIdInMeta = false;
    bool hasFolderIdInMeta = false;
    bool hasWidthColumn = false;
    bool hasHeightColumn = false;
    bool hasIngestionStatusColumn = false;
    bool hasAutoColorColumn = false;
    bool hasAddedAtColumn = false;

    if (sqlite3_prepare_v2(conn.memDb, "PRAGMA table_info(metadata)", -1, &checkStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(checkStmt) == SQLITE_ROW) {
            const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(checkStmt, 1));
            if (colName) {
                std::string name(colName);
                if (name == "file_id") hasFileIdInMeta = true;
                if (name == "folder_id") hasFolderIdInMeta = true;
                if (name == "width") hasWidthColumn = true;
                if (name == "height") hasHeightColumn = true;
                if (name == "ingestion_status") hasIngestionStatusColumn = true;
                if (name == "auto_color") hasAutoColorColumn = true;
                if (name == "added_at") hasAddedAtColumn = true;
            }
        }
        sqlite3_finalize(checkStmt);
    }

    if (hasFileIdInMeta && !hasFolderIdInMeta) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata RENAME COLUMN file_id TO folder_id;", nullptr, nullptr, nullptr);
    }

    if (!hasWidthColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata ADD COLUMN width INTEGER DEFAULT 0", nullptr, nullptr, nullptr);
    }
    if (!hasHeightColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata ADD COLUMN height INTEGER DEFAULT 0", nullptr, nullptr, nullptr);
    }
    if (!hasIngestionStatusColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata ADD COLUMN ingestion_status INTEGER DEFAULT -1", nullptr, nullptr, nullptr);
    }
    if (!hasAutoColorColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata ADD COLUMN auto_color TEXT DEFAULT ''", nullptr, nullptr, nullptr);
    }
    if (!hasAddedAtColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata ADD COLUMN added_at INTEGER DEFAULT 0", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.memDb, "CREATE INDEX IF NOT EXISTS idx_metadata_added_at ON metadata(added_at);", nullptr, nullptr, nullptr);
    }

    // 自动检测并补全 sha256 字段
    bool hasSha256Column = false;
    sqlite3_stmt* shaCheckStmt = nullptr;
    if (sqlite3_prepare_v2(conn.memDb, "PRAGMA table_info(metadata)", -1, &shaCheckStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(shaCheckStmt) == SQLITE_ROW) {
            const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(shaCheckStmt, 1));
            if (colName && std::string(colName) == "sha256") {
                hasSha256Column = true;
            }
        }
        sqlite3_finalize(shaCheckStmt);
    }
    if (!hasSha256Column) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata ADD COLUMN sha256 TEXT DEFAULT ''", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.memDb, "CREATE INDEX IF NOT EXISTS idx_metadata_hash ON metadata(file_size, sha256);", nullptr, nullptr, nullptr);
    }

    // 迁移 disk_trash 补充 file_id 和 created_at 字段
    bool hasFileIdInTrash = false;
    bool hasCreatedAtInTrash = false;
    sqlite3_stmt* trashCheckStmt = nullptr;
    if (sqlite3_prepare_v2(conn.memDb, "PRAGMA table_info(disk_trash)", -1, &trashCheckStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(trashCheckStmt) == SQLITE_ROW) {
            const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(trashCheckStmt, 1));
            if (colName) {
                std::string name(colName);
                if (name == "file_id") hasFileIdInTrash = true;
                if (name == "created_at") hasCreatedAtInTrash = true;
            }
        }
        sqlite3_finalize(trashCheckStmt);
    }
    if (!hasFileIdInTrash) {
        sqlite3_exec(conn.memDb, "ALTER TABLE disk_trash ADD COLUMN file_id TEXT DEFAULT ''", nullptr, nullptr, nullptr);
    }
    if (!hasCreatedAtInTrash) {
        sqlite3_exec(conn.memDb, "ALTER TABLE disk_trash ADD COLUMN created_at INTEGER DEFAULT 0", nullptr, nullptr, nullptr);
    }



    // 2026-08-xx 新增字段：持久化基名与后缀名，避免每次启动现算并优化回填
    bool hasBaseNameColumn = false;
    bool hasExtColumn = false;
    sqlite3_stmt* checkStmt2 = nullptr;
    if (sqlite3_prepare_v2(conn.memDb, "PRAGMA table_info(metadata)", -1, &checkStmt2, nullptr) == SQLITE_OK) {
        while (sqlite3_step(checkStmt2) == SQLITE_ROW) {
            const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(checkStmt2, 1));
            if (colName) {
                std::string name(colName);
                if (name == "base_name") hasBaseNameColumn = true;
                if (name == "ext") hasExtColumn = true;
            }
        }
        sqlite3_finalize(checkStmt2);
    }

    if (!hasBaseNameColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata ADD COLUMN base_name TEXT DEFAULT ''", nullptr, nullptr, nullptr);
    }
    if (!hasExtColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE metadata ADD COLUMN ext TEXT DEFAULT ''", nullptr, nullptr, nullptr);

        // 回填存量数据
        sqlite3_stmt* selStmt = nullptr;
        if (sqlite3_prepare_v2(conn.memDb, "SELECT folder_id, path, is_folder FROM metadata", -1, &selStmt, nullptr) == SQLITE_OK) {
            sqlite3_stmt* updStmt = nullptr;
            if (sqlite3_prepare_v2(conn.memDb, "UPDATE metadata SET base_name = ?, ext = ? WHERE folder_id = ?", -1, &updStmt, nullptr) == SQLITE_OK) {
                // 暂时用局部逻辑来实现旧数据的解析和回填（不调用未初始化完全的 MetadataManager 的实例）
                while (sqlite3_step(selStmt) == SQLITE_ROW) {
                    const char* fid = reinterpret_cast<const char*>(sqlite3_column_text(selStmt, 0));
                    const wchar_t* wpath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(selStmt, 1));
                    bool isFolder = sqlite3_column_int(selStmt, 2) != 0;
                    if (fid && wpath) {
                        std::wstring normPath(wpath);
                        size_t lastSlash = normPath.find_last_of(L"\\/");
                        std::wstring fullName = (lastSlash == std::wstring::npos) ? normPath : normPath.substr(lastSlash + 1);

                        std::wstring name, ext;
                        if (isFolder) {
                            name = fullName;
                            ext = L"";
                        } else {
                            name = fullName;
                            size_t lastDot = fullName.find_last_of(L'.');
                            if (lastDot != std::wstring::npos && lastDot > 0) {
                                ext = fullName.substr(lastDot + 1);
                                std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
                                name = fullName.substr(0, lastDot);
                            }
                        }

                        sqlite3_bind_text16(updStmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text16(updStmt, 2, ext.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(updStmt, 3, fid, -1, SQLITE_TRANSIENT);
                        sqlite3_step(updStmt);
                        sqlite3_reset(updStmt);
                    }
                }
                sqlite3_finalize(updStmt);
            }
            sqlite3_finalize(selStmt);
        }
    }

    sqlite3_exec(conn.memDb, "CREATE INDEX IF NOT EXISTS idx_metadata_ext ON metadata(ext);", nullptr, nullptr, nullptr);

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
    // 24h 滑动窗口 15s 剪枝
    MetadataManager::instance().slideRecentWindow();

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
