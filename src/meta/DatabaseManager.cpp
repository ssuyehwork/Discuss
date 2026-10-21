#include "DatabaseManager.h"
#include "DriveMetaDao.h"
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
#include <windows.h>
#include "../util/AppDirectoryInitializer.h"

namespace QuarkMeta {

SqlTransaction::SqlTransaction(sqlite3* db) : m_db(db) {
    if (m_db) {
        m_isNested = (sqlite3_get_autocommit(m_db) == 0);
        if (!m_isNested) {
            sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr, nullptr);
        }
    }
}

SqlTransaction::~SqlTransaction() {
    if (m_db && !m_committed && !m_isNested) {
        sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
    }
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
        m_committed = true;
    }
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
    if (QCoreApplication::instance()) {
        this->moveToThread(QCoreApplication::instance()->thread());
    }
}

DatabaseManager::~DatabaseManager() {
    shutdown();
}

QString DatabaseManager::getAppDir() {
    return QCoreApplication::applicationDirPath();
}

QString DatabaseManager::getGlobalDbPath() {
    QString dir = QDir::toNativeSeparators(getAppDir() + "/.QuarkMeta");
    return QDir::toNativeSeparators(dir + "/QuarkMeta.store");
}

bool DatabaseManager::loadDb(const std::wstring& diskPath, DbConnection& conn) {
    QString qDiskPath = QDir::toNativeSeparators(QString::fromStdWString(diskPath));
    QFileInfo fileInfo(qDiskPath);
    
    QDir().mkpath(fileInfo.absolutePath());
    SetFileAttributesW(fileInfo.absolutePath().toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);

    std::wstring nativeWPath = qDiskPath.toStdWString();

    int rc = sqlite3_open16(nativeWPath.c_str(), &conn.diskDb);
    if (rc != SQLITE_OK || !conn.diskDb) {
        qCritical() << "[DatabaseManager] 无法打开/创建全局数据库:" << qDiskPath;
        if (conn.diskDb) {
            sqlite3_close_v2(conn.diskDb);
            conn.diskDb = nullptr;
        }
        return false;
    }

    sqlite3_busy_timeout(conn.diskDb, 25000);
    sqlite3_exec(conn.diskDb, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA temp_store = MEMORY;", nullptr, nullptr, nullptr);

    // 自动自愈：探测预存库中 library_items 是否具备新字段（sync_state, auto_color），若为旧表则擦除以触发全新建表
    sqlite3_stmt* checkStmt = nullptr;
    if (sqlite3_prepare_v2(conn.diskDb, "SELECT sync_state, auto_color FROM library_items LIMIT 1;", -1, &checkStmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS item_palettes;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS file_tags;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS library_items;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS library_item_index;", nullptr, nullptr, nullptr);
    } else {
        sqlite3_finalize(checkStmt);
    }

    const char* schema = R"(
        -- ===================================================================
        -- 1. 基础字典表：标签管理
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS tags (
            name TEXT PRIMARY KEY COLLATE NOCASE,   -- 忽略大小写
            color TEXT DEFAULT '',
            pinned INTEGER DEFAULT 0,
            use_count INTEGER DEFAULT 0,            -- 使用频次
            last_used INTEGER DEFAULT 0             -- 最后使用时间
        );
        CREATE INDEX IF NOT EXISTS idx_tags_use_count ON tags(use_count DESC);
        CREATE INDEX IF NOT EXISTS idx_tags_last_used ON tags(last_used DESC);

        -- ===================================================================
        -- 2. 标签组及多对多关联
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS tag_groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,              -- 组名禁止重复
            color TEXT DEFAULT '',
            sort_order INTEGER DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_tag_groups_sort ON tag_groups(sort_order ASC);

        CREATE TABLE IF NOT EXISTS tag_group_items (
            group_id INTEGER,
            tag_name TEXT COLLATE NOCASE,
            PRIMARY KEY (group_id, tag_name),
            FOREIGN KEY (group_id) REFERENCES tag_groups(id) ON DELETE CASCADE,
            FOREIGN KEY (tag_name) REFERENCES tags(name) ON DELETE CASCADE ON UPDATE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_tag_group_items_tag ON tag_group_items(tag_name);

        -- ===================================================================
        -- 3. 全局文件索引表 (Sidecar File_ID 主键契约，补全 25 列)
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS library_items (
            file_id TEXT PRIMARY KEY,                       -- 1. 持久化唯一标识 (UUID)
            file_path TEXT NOT NULL UNIQUE COLLATE NOCASE,  -- 2. 绝对物理路径
            file_name TEXT NOT NULL,                        -- 3. 文件名（含后缀）
            base_name TEXT NOT NULL,                        -- 4. 纯文件名
            extension TEXT NOT NULL COLLATE NOCASE,         -- 5. 后缀
            file_size INTEGER DEFAULT 0,                    -- 6. 字节大小
            is_dir INTEGER DEFAULT 0,                       -- 7. 是否文件夹
            is_empty INTEGER DEFAULT 0,                     -- 8. 是否空文件夹
            is_hidden INTEGER DEFAULT 0,                    -- 9. 是否隐藏文件
            is_managed INTEGER DEFAULT 0,                   -- 10.受控状态
            encrypted INTEGER DEFAULT 0,                    -- 11.是否加密
            pinned INTEGER DEFAULT 0,                       -- 12.置顶
            rating INTEGER DEFAULT 0,                       -- 13.评级 0~5
            color_hex TEXT DEFAULT '',                      -- 14.手动色标
            auto_color TEXT DEFAULT '',                     -- 15.自动主色
            width INTEGER DEFAULT 0,                        -- 16.图像宽
            height INTEGER DEFAULT 0,                       -- 17.图像高
            thumb_status INTEGER DEFAULT 0,                 -- 18.缩略图状态
            note TEXT DEFAULT '',                           -- 19.备注
            url TEXT DEFAULT '',                            -- 20.关联 URL
            sha256 TEXT DEFAULT '',                         -- 21.SHA256 哈希
            created_at INTEGER DEFAULT 0,                   -- 22.创建时间 ctime
            updated_at INTEGER DEFAULT 0,                   -- 23.修改时间 mtime
            added_at INTEGER DEFAULT 0,                     -- 24.收录/添加时间
            sync_state INTEGER DEFAULT 0,                   -- 25.同步状态 (0正常/1待刷新/2缺失)
            synced_at INTEGER DEFAULT 0                     -- 26.上次同步时间戳
        );
        CREATE INDEX IF NOT EXISTS idx_items_path ON library_items(file_path);
        CREATE INDEX IF NOT EXISTS idx_items_rating ON library_items(rating);
        CREATE INDEX IF NOT EXISTS idx_items_ext ON library_items(extension);
        CREATE INDEX IF NOT EXISTS idx_items_color ON library_items(color_hex);
        CREATE INDEX IF NOT EXISTS idx_items_updated ON library_items(updated_at DESC);

        -- ===================================================================
        -- 4. 色板关联表 item_palettes
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS item_palettes (
            file_id TEXT NOT NULL,
            idx INTEGER NOT NULL,
            color_hex TEXT NOT NULL,
            ratio REAL NOT NULL DEFAULT 0.0,
            PRIMARY KEY (file_id, idx),
            FOREIGN KEY (file_id) REFERENCES library_items(file_id) ON DELETE CASCADE ON UPDATE CASCADE
        );

        -- ===================================================================
        -- 5. 文件与标签的多对多解耦关联表
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS file_tags (
            file_id TEXT NOT NULL,
            tag_name TEXT NOT NULL COLLATE NOCASE,
            PRIMARY KEY (file_id, tag_name),
            FOREIGN KEY (file_id) REFERENCES library_items(file_id) ON DELETE CASCADE ON UPDATE CASCADE,
            FOREIGN KEY (tag_name) REFERENCES tags(name) ON DELETE CASCADE ON UPDATE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_file_tags_tag ON file_tags(tag_name);

        -- ===================================================================
        -- 5. 回收站表（补全高频检索与恢复索引）
        -- ===================================================================
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
        CREATE UNIQUE INDEX IF NOT EXISTS idx_disk_trash_file_id ON disk_trash(file_id);
        CREATE INDEX IF NOT EXISTS idx_disk_trash_orig_path ON disk_trash(original_path);
        CREATE INDEX IF NOT EXISTS idx_disk_trash_deleted ON disk_trash(deleted_at DESC);

        -- ===================================================================
        -- 6. 驱动器与智能文件夹元数据
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS drive_metadata (
            drive_path TEXT PRIMARY KEY,
            rating INTEGER DEFAULT 0,
            color TEXT DEFAULT '',
            pinned INTEGER DEFAULT 0,
            note TEXT DEFAULT '',
            url TEXT DEFAULT '',
            updated_at INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS smart_folders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            match_mode TEXT DEFAULT 'ALL',
            icon_key TEXT DEFAULT 'folder_filled',
            color_hex TEXT DEFAULT '#888888',
            sort_order INTEGER DEFAULT 0,
            rules_json TEXT DEFAULT '[]',
            created_at INTEGER
        );
        CREATE INDEX IF NOT EXISTS idx_smart_folders_sort ON smart_folders(sort_order ASC);
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(conn.diskDb, schema, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        qCritical() << "[DatabaseManager] 表结构创建失败:" << errMsg;
        sqlite3_free(errMsg);
        sqlite3_close_v2(conn.diskDb);
        conn.diskDb = nullptr;
        return false;
    }

    // 🚨 核心加固：立即执行 TRUNCATE 检查点，将表结构瞬间从 -wal 固化回 global.db 主文件！
    sqlite3_wal_checkpoint_v2(conn.diskDb, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);

    conn.diskPath = nativeWPath;
    qInfo() << "[DatabaseManager] 4 张全局表已成功固化到物理数据库:" << qDiskPath;
    return true;
}

void DatabaseManager::closeDb(DbConnection& conn) {
    if (conn.diskDb) {
        sqlite3_close_v2(conn.diskDb);
        conn.diskDb = nullptr;
    }
}

bool DatabaseManager::initLocked() {
    if (m_isInitialized && m_globalDb.diskDb) {
        return true;
    }

    AppDirectoryInitializer::initializeStoragePath(getAppDir());

    std::wstring globalPath = getGlobalDbPath().toStdWString();
    if (!loadDb(globalPath, m_globalDb)) {
        return false;
    }

    m_isInitialized = true;
    return true;
}

bool DatabaseManager::init() {
    bool ok = false;
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        ok = initLocked();
    }
    if (ok) {
        DriveMetaDao::initTable();
    }
    return ok;
}

void DatabaseManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    std::lock_guard<std::mutex> dbLock(m_globalDbMutex);
    closeDb(m_globalDb);
    m_isInitialized = false;
}

sqlite3* DatabaseManager::getGlobalDb() {
    bool needTableInit = false;
    sqlite3* dbPtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        if (!m_isInitialized || !m_globalDb.diskDb) {
            if (initLocked()) {
                needTableInit = true;
            }
        }
        dbPtr = m_globalDb.diskDb;
    }
    if (needTableInit) {
        DriveMetaDao::initTable();
    }
    return dbPtr;
}

} // namespace QuarkMeta