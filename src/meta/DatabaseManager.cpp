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
        -- 索引：覆盖按热门使用量排序、按最后使用时间排序
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
        -- 3. 【核心补全】物理文件索引表（消灭全盘递归扫描的根基！）
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS library_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL UNIQUE,         -- 绝对物理路径（主键级别排重）
            file_name TEXT NOT NULL,                -- 文件名（含后缀）
            base_name TEXT NOT NULL,                -- 纯文件名（不含后缀，搜索用）
            extension TEXT NOT NULL COLLATE NOCASE, -- 后缀（如 png、md）
            file_size INTEGER DEFAULT 0,
            rating INTEGER DEFAULT 0,               -- 评级 0~5
            color_hex TEXT DEFAULT '',              -- 标记颜色
            note TEXT DEFAULT '',                   -- 备注
            url TEXT DEFAULT '',                    -- 关联网址
            created_at INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT 0
        );
        -- 为智能文件夹的高频过滤字段建立 B-Tree 索引
        CREATE INDEX IF NOT EXISTS idx_items_rating ON library_items(rating);
        CREATE INDEX IF NOT EXISTS idx_items_ext ON library_items(extension);
        CREATE INDEX IF NOT EXISTS idx_items_color ON library_items(color_hex);
        CREATE INDEX IF NOT EXISTS idx_items_updated ON library_items(updated_at DESC);

        -- ===================================================================
        -- 4. 【核心补全】文件与标签的「多对多关联表」
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS file_tags (
            item_id INTEGER NOT NULL,
            tag_name TEXT NOT NULL COLLATE NOCASE,
            PRIMARY KEY (item_id, tag_name),
            FOREIGN KEY (item_id) REFERENCES library_items(id) ON DELETE CASCADE,
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

bool DatabaseManager::init() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (m_isInitialized && m_globalDb.diskDb) {
        return true;
    }

    AppDirectoryInitializer::initializeStoragePath(getAppDir());

    std::wstring globalPath = getGlobalDbPath().toStdWString();
    if (!loadDb(globalPath, m_globalDb)) {
        return false;
    }

    DriveMetaDao::initTable();
    m_isInitialized = true;
    return true;
}

void DatabaseManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    std::lock_guard<std::mutex> dbLock(m_globalDbMutex);
    closeDb(m_globalDb);
    m_isInitialized = false;
}

sqlite3* DatabaseManager::getGlobalDb() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (!m_isInitialized || !m_globalDb.diskDb) {
        init();
    }
    return m_globalDb.diskDb;
}

} // namespace QuarkMeta