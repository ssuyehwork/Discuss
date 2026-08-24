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
    return QDir::toNativeSeparators(dir + "/global.db");
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
    sqlite3_exec(conn.diskDb, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);

    // 4 张表 + 1 索引 DDL
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS tag_groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            color TEXT,
            sort_order INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS tag_group_items (
            group_id INTEGER,
            tag_name TEXT,
            PRIMARY KEY (group_id, tag_name)
        );

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

        CREATE TABLE IF NOT EXISTS drive_metadata (
            drive_path TEXT PRIMARY KEY,
            rating INTEGER DEFAULT 0,
            color TEXT DEFAULT '',
            pinned INTEGER DEFAULT 0,
            note TEXT DEFAULT '',
            url TEXT DEFAULT '',
            updated_at INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS tags (
            name TEXT PRIMARY KEY,
            color TEXT DEFAULT '',
            pinned INTEGER DEFAULT 0,
            use_count INTEGER DEFAULT 0,
            last_used INTEGER DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_tags_last_used ON tags(last_used DESC);
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
    if (!m_globalDb.diskDb) {
        init();
    }
    return m_globalDb.diskDb;
}

} // namespace QuarkMeta