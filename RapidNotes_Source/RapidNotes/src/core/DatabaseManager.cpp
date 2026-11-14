#include "DatabaseManager.h"
#include "ItemType.h"
#include <QDebug>

SqlTransaction::SqlTransaction(sqlite3* db) : m_db(db) {
    if (m_db) {
        m_isNested = (sqlite3_get_autocommit(m_db) == 0);
        if (m_isNested) {
            qWarning() << "[SqlTransaction] ⚠️ 警告：检测到数据库连接已处于未提交事务中 (autocommit=0)，本事务判定为嵌套事务！";
        } else {
            int rc = sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr, nullptr);
            if (rc != SQLITE_OK) {
                qCritical() << "[SqlTransaction] ❌ BEGIN IMMEDIATE TRANSACTION 失败，错误码:" << rc << sqlite3_errmsg(m_db);
            }
        }
    }
}

SqlTransaction::~SqlTransaction() {
    if (m_db && !m_committed) {
        if (!m_isNested) {
            sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
        }
    }
}

bool SqlTransaction::commit() {
    if (m_db && !m_committed) {
        if (m_isNested) {
            m_committed = true;
            return true;
        }
        int rc = sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
        if (rc == SQLITE_OK) {
            m_committed = true;
            return true;
        } else {
            qCritical() << "[SqlTransaction] ❌ COMMIT 失败，错误码:" << rc << sqlite3_errmsg(m_db);
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
#include "DatabaseManagerInternal.h"

// ===================================================================
// 合并架构辅助（仅本文件可见）
// ===================================================================
namespace RapidNotes {
    // QuarkMeta 侧的目录/DAO 初始化（沿用其原有调用，类名按其所在命名空间解析）
    static void prepareStorageDirs(const QString& appDir) {
        AppDirectoryInitializer::initializeStoragePath(appDir);
    }
    static void initMetaDaos() {
        DriveMetaDao::initTable();
    }
}

namespace {
    bool isSqliteFile(const QString& path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return false;
        return f.read(16).startsWith("SQLite format 3");
    }

    // 用原生句柄只读探测笔记数量；无 notes 表或无法打开返回 -1
    int countNotesNative(const QString& path) {
        sqlite3* db = nullptr;
        std::wstring w = QDir::toNativeSeparators(path).toStdWString();
        if (sqlite3_open16(w.c_str(), &db) != SQLITE_OK || !db) {
            if (db) sqlite3_close_v2(db);
            return -1;
        }
        sqlite3_busy_timeout(db, 5000);
        int count = -1;
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM notes;", -1, &st, nullptr) == SQLITE_OK) {
            if (sqlite3_step(st) == SQLITE_ROW) count = sqlite3_column_int(st, 0);
            sqlite3_finalize(st);
        }
        sqlite3_close_v2(db);   // 关闭时 SQLite 会把 -wal 合并回主文件
        return count;
    }

    // 连同 -wal / -shm 一起搬走（先重命名，失败再复制后删除）
    bool moveDbWithSidecars(const QString& from, const QString& to) {
        const QStringList sfx = {"", "-wal", "-shm"};
        for (const QString& s : sfx) {
            if (!QFile::exists(from + s)) continue;
            QFile::remove(to + s);
            if (QFile::rename(from + s, to + s)) continue;
            if (QFile::copy(from + s, to + s)) { QFile::remove(from + s); continue; }
            if (s.isEmpty()) return false;   // 主文件搬不动才算失败
        }
        return true;
    }
}



DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

QStringList DatabaseManager::s_tagClipboard;

QMutex DatabaseManager::s_tagClipboardMutex;

void DatabaseManager::setTagClipboard(const QStringList& tags) {
    QMutexLocker locker(&s_tagClipboardMutex);
    s_tagClipboard = tags;
}

QStringList DatabaseManager::getTagClipboard() {
    QMutexLocker locker(&s_tagClipboardMutex);
    return s_tagClipboard;
}

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
    QSettings settings("RapidNotes", "QuickWindow");
    m_autoCategorizeEnabled = settings.value("autoCategorizeClipboard", false).toBool();
    m_lockedCategoriesHidden = settings.value("lockedCategoriesHidden", false).toBool();
    // 2026-05-27 按照用户要求：持久化读取“归类到此分类”的目标分类 ID，默认 -1
    m_activeCategoryId = settings.value("activeCategoryId", -1).toInt();

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(7000); // 7秒增量同步间隔
    connect(m_autoSaveTimer, &QTimer::timeout, this, &DatabaseManager::handleAutoSave);
}

void DatabaseManager::setAutoCategorizeEnabled(bool enabled) {
    if (m_autoCategorizeEnabled != enabled) {
        m_autoCategorizeEnabled = enabled;
        QSettings settings("RapidNotes", "QuickWindow");
        settings.setValue("autoCategorizeClipboard", enabled);
        emit autoCategorizeEnabledChanged(enabled);
    }
}

void DatabaseManager::setActiveCategoryId(int id) {
    if (m_activeCategoryId != id) {
        m_activeCategoryId = id;
        // 2026-05-27 按照用户要求：将“归类到此分类”的目标分类 ID 持久化写入配置
        QSettings settings("RapidNotes", "QuickWindow");
        settings.setValue("activeCategoryId", id);
        emit activeCategoryIdChanged(id);
    }
}

DatabaseManager::~DatabaseManager() {
    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
    }
    if (m_db.isOpen()) {
        m_db.close();
    }
    if (m_nativeDb) {
        sqlite3_close_v2(m_nativeDb);
        m_nativeDb = nullptr;
    }
}

void DatabaseManager::logStartup(const QString& msg) {
    // 2026-03-xx 按照用户要求：彻底移除 startup_debug.log 文件写入逻辑，仅保留控制台调试输出
    qDebug() << "[DB-STARTUP]" << msg;
}

bool DatabaseManager::init(const QString& legacyDbPath) {
    QMutexLocker locker(&m_mutex);

    // 【核心修复】：已完整初始化则严禁重复关闭连接（杜绝句柄失效引发 c0000005 闪退）
    if (m_isInitialized && m_db.isOpen() && m_nativeDb) {
        return true;
    }

    m_isInitialized = false;
    m_lastError.clear();

    logStartup("--- [MERGED-DB] 架构初始化开始 ---");

    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        m_lastError = "Qt 环境缺失 QSQLITE 驱动支持。";
        logStartup("[ERR] " + m_lastError);
        return false;
    }

    // 路径单一真实数据源：程序目录/.RapidNotes/inspiration.db（Qt 连接与原生连接共用这一个文件）
    m_dbPath = getGlobalDbPath();
    logStartup("目标主路径: " + m_dbPath);

    RapidNotes::prepareStorageDirs(getAppDir());
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());
#ifdef Q_OS_WIN
    SetFileAttributesW(QFileInfo(m_dbPath).absolutePath().toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

    // [MIGRATE] 旧位置(程序目录/inspiration.db)的真实数据迁入新位置，绝不丢笔记
    {
        QString legacy = legacyDbPath.isEmpty() ? (getAppDir() + "/inspiration.db") : legacyDbPath;
        legacy = QDir::toNativeSeparators(QDir::cleanPath(legacy));
        if (legacy.compare(QDir::toNativeSeparators(QDir::cleanPath(m_dbPath)), Qt::CaseInsensitive) != 0
            && QFile::exists(legacy) && isSqliteFile(legacy)) {
            int legacyNotes = countNotesNative(legacy);
            if (!QFile::exists(m_dbPath)) {
                logStartup("[MIGRATE] 新位置无数据库，迁入旧库 (笔记数 " + QString::number(legacyNotes) + ")");
                if (!moveDbWithSidecars(legacy, m_dbPath)) {
                    m_lastError = "旧数据库迁移失败：" + legacy;
                    logStartup("[ERR] " + m_lastError);
                    return false;
                }
            } else {
                int targetNotes = countNotesNative(m_dbPath);
                if (targetNotes <= 0 && legacyNotes > 0) {
                    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
                    logStartup("[MIGRATE] 新位置库为空而旧库有 " + QString::number(legacyNotes) + " 条笔记，旧库迁入，空库另存备查");
                    if (!moveDbWithSidecars(m_dbPath, m_dbPath + ".premerge_" + stamp) ||
                        !moveDbWithSidecars(legacy, m_dbPath)) {
                        m_lastError = "旧数据库迁移失败：" + legacy;
                        logStartup("[ERR] " + m_lastError);
                        return false;
                    }
                } else {
                    logStartup("[MIGRATE] 新旧位置均已有数据，保留新位置，旧库原样未动: " + legacy);
                }
            }
        }
    }

    auto isSqlite = [](const QString& path) -> bool { return isSqliteFile(path); };

    bool loaded = false;
    // [LEVEL 1] 优先检测主路径是否已是明文 SQLite
    if (QFile::exists(m_dbPath) && isSqlite(m_dbPath)) {
        logStartup("[L1] 命中主路径明文数据库，直接加载。");
        loaded = true;
    }

    // [LEVEL 2] 抢救 AppData 残留 ( rapidnotes_kernel.db )
    if (!loaded) {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (appDataPath.isEmpty()) appDataPath = QCoreApplication::applicationDirPath() + "/data";
        QString residuePath = appDataPath + "/rapidnotes_kernel.db";
        
        if (QFile::exists(residuePath) && isSqlite(residuePath)) {
            logStartup("[L2] 监测到上次异常退出的 AppData 内核残留，正在执行原地抢救...");
            if (QFile::exists(m_dbPath)) QFile::remove(m_dbPath);
            if (QFile::copy(residuePath, m_dbPath)) {
                loaded = true;
                logStartup("[L2] 抢救成功，残留已同步至主路径。");
            }
        }
    }

    // [LEVEL 3] 检测旧版明文 notes.db 迁移
    if (!loaded) {
        QString legacyPlain = getAppDir() + "/notes.db";
        if (QFile::exists(legacyPlain) && isSqlite(legacyPlain)) {
            logStartup("[L3] 检测到旧版 notes.db 明文，执行架构平滑迁移...");
            if (QFile::copy(legacyPlain, m_dbPath)) {
                loaded = true;
                FileCryptoHelper::secureDelete(legacyPlain);
            }
        }
    }

    // [LEVEL 4] 核心解壳：尝试对主路径进行多指纹解密 (用于从旧版加密架构初次升级)
    if (!loaded && QFile::exists(m_dbPath)) {
        logStartup("[L4] 正在尝试对加密外壳进行多密钥自适应解密迁移...");
        
        QString appDriveSN = HardwareInfoHelper::getAppDrivePhysicalSerialNumber();
        QString cDriveSN = HardwareInfoHelper::getCDiskPhysicalSerialNumber();
        QStringList candidateKeys;
        if (!appDriveSN.isEmpty()) candidateKeys << FileCryptoHelper::getCombinedKeyBySN(appDriveSN);
        if (!cDriveSN.isEmpty()) candidateKeys << FileCryptoHelper::getCombinedKeyBySN(cDriveSN);
        candidateKeys << FileCryptoHelper::getLegacyCombinedKey();

        QString tempDecPath = m_dbPath + ".dec_mig";
        for (const QString& key : candidateKeys) {
            if (FileCryptoHelper::decryptFileWithShell(m_dbPath, tempDecPath, key)) {
                if (isSqlite(tempDecPath)) {
                    logStartup("[L4] 解壳成功！数据已成功转换为明文。");
                    QFile::remove(m_dbPath);
                    QFile::rename(tempDecPath, m_dbPath);
                    loaded = true;
                    break;
                }
                QFile::remove(tempDecPath);
            }
        }
        
        if (!loaded && FileCryptoHelper::decryptFileLegacy(m_dbPath, tempDecPath, FileCryptoHelper::getCombinedKeyBySN(cDriveSN))) {
             if (isSqlite(tempDecPath)) {
                logStartup("[L4] Legacy 解壳成功。");
                QFile::remove(m_dbPath);
                QFile::rename(tempDecPath, m_dbPath);
                loaded = true;
            } else {
                QFile::remove(tempDecPath);
            }
        }
    }

    // [LEVEL 5] 备份血包恢复
    if (!loaded) {
        logStartup("[L5] 主路径失效，尝试从备份恢复...");
        if (tryRecoverFromBackup()) {
            if (isSqlite(m_dbPath)) loaded = true;
            else {
                // 如果备份也是加密的，则递归执行一次 L4 逻辑 (简化实现，这里直接报错由人工介入)
                logStartup("[ERR] 备份文件需要二次解密，请联系技术支持或手动重命名。");
            }
        }
    }

    if (!loaded) {
        logStartup("[INIT] 所有抢救链均失效，初始化全新明文数据库。");
    }

    // 打开 Qt 连接并建表（RapidNotes 笔记模块）
    if (!initQtSqlDb(m_dbPath)) {
        return false;
    }

    // 打开原生连接并建表（QuarkMeta 全套 DAO，句柄由本类持有，不再借用 Qt 驱动的句柄）
    if (!initNativeDb(m_dbPath)) {
        if (m_db.isOpen()) m_db.close();
        return false;
    }

    logStartup("执行建表及升级检查...");
    if (!createAllTables()) {
        if (m_lastError.isEmpty()) m_lastError = "建表或表升级失败: " + m_db.lastError().text();
        logStartup("[ERR] " + m_lastError);
        if (m_db.isOpen()) m_db.close();
        std::lock_guard<std::mutex> g(m_globalDbMutex);
        if (m_nativeDb) { sqlite3_close_v2(m_nativeDb); m_nativeDb = nullptr; }
        return false;
    }

    m_isInitialized = true;
    logStartup("--- 初始化全部成功 ---");
    m_autoSaveTimer->start();

    // 释放 Qt 侧锁后再调用 DAO（DAO 会取原生互斥锁，避免锁序交叉）
    locker.unlock();
    RapidNotes::initMetaDaos();
    {
        std::lock_guard<std::mutex> g(m_globalDbMutex);
        if (m_nativeDb) sqlite3_wal_checkpoint_v2(m_nativeDb, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);
    }
    performDailyBackup();
    return true;
}

bool DatabaseManager::initQtSqlDb(const QString& dbPath) {
    if (m_db.isOpen()) m_db.close();

    QString connectionName = "RapidNotes_Main_Conn";
    if (QSqlDatabase::contains(connectionName)) {
        m_db = QSqlDatabase::database(connectionName, false);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    }

    m_db.setDatabaseName(dbPath);
    // 与原生连接共用同一文件，写冲突时等待而不是立刻报 busy
    m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=25000");

    logStartup("正在建立 SQL 连接...");
    if (!m_db.open()) {
        m_lastError = QString("[%1] %2").arg(m_db.lastError().nativeErrorCode()).arg(m_db.lastError().text());
        logStartup("[ERR] SQL 打开失败: " + m_lastError);
        return false;
    }
    logStartup("SQL 连接成功。");

    QSqlQuery walQuery(m_db);
    logStartup("正在配置高可靠性写入模式 (WAL + FULL)...");
    if (!walQuery.exec("PRAGMA journal_mode = WAL;")) {
        m_lastError = "无法开启 WAL 模式: " + walQuery.lastError().text();
        logStartup("[ERR] " + m_lastError);
        return false;
    }
    walQuery.exec("PRAGMA synchronous = FULL;");

    logStartup("执行完整性预检...");
    if (!walQuery.exec("PRAGMA integrity_check;")) {
        m_lastError = "数据库文件结构损坏: " + walQuery.lastError().text();
        logStartup("[ERR] " + m_lastError);
        return false;
    }

    walQuery.exec("PRAGMA foreign_keys = ON;");   // 标签关联表依赖外键级联
    if (!renameLegacyNoteTagDict()) return false;
    return true;
}

bool DatabaseManager::initNativeDb(const QString& dbPath) {
    std::lock_guard<std::mutex> g(m_globalDbMutex);
    if (m_nativeDb) return true;

    sqlite3* db = nullptr;
    std::wstring wpath = QDir::toNativeSeparators(dbPath).toStdWString();
    int rc = sqlite3_open16(wpath.c_str(), &db);
    if (rc != SQLITE_OK || !db) {
        m_lastError = "无法打开原生数据库连接: " + dbPath;
        logStartup("[ERR] " + m_lastError);
        if (db) sqlite3_close_v2(db);
        return false;
    }

    sqlite3_busy_timeout(db, 25000);
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA temp_store = MEMORY;", nullptr, nullptr, nullptr);

    RapidNotes::SmartFolderDao::registerColorFunctions(db);

    constexpr int kSchemaVersion = 1;
    int currentVersion = 0;
    sqlite3_stmt* verStmt = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &verStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(verStmt) == SQLITE_ROW) currentVersion = sqlite3_column_int(verStmt, 0);
        sqlite3_finalize(verStmt);
    }

    if (currentVersion != kSchemaVersion) {
        char* err = nullptr;
        if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &err) == SQLITE_OK) {
            sqlite3_exec(db, "DROP TABLE IF EXISTS item_palettes;", nullptr, nullptr, nullptr);
            sqlite3_exec(db, "DROP TABLE IF EXISTS file_tags;", nullptr, nullptr, nullptr);
            sqlite3_exec(db, "DROP TABLE IF EXISTS library_items;", nullptr, nullptr, nullptr);
            if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &err) != SQLITE_OK) {
                logSqlError("Version upgrade commit", db, "COMMIT;");
                if (err) sqlite3_free(err);
                sqlite3_close_v2(db);
                m_lastError = "原生库版本升级失败";
                return false;
            }
        } else {
            logSqlError("Version upgrade begin", db, "BEGIN TRANSACTION;");
            if (err) sqlite3_free(err);
            sqlite3_close_v2(db);
            m_lastError = "原生库版本升级失败";
            return false;
        }
    }

    static const char* kNativeSchema = R"(
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
        -- 3. 全局文件索引表 (Sidecar File_ID 主键契约)
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS library_items (
            file_id TEXT PRIMARY KEY,               -- 持久化唯一标识 (UUID/Base36)
            meta_file_id TEXT DEFAULT '',           -- .RapidNotes.json 自身 File_ID 凭据
            file_path TEXT NOT NULL UNIQUE COLLATE NOCASE, -- 绝对物理路径
            file_name TEXT NOT NULL,                -- 文件名（含后缀）
            base_name TEXT NOT NULL,                -- 纯文件名（不含后缀，搜索用）
            extension TEXT NOT NULL COLLATE NOCASE, -- 后缀（如 png、md）
            file_size INTEGER DEFAULT 0,
            is_dir INTEGER DEFAULT 0,
            is_empty INTEGER DEFAULT 0,
            is_hidden INTEGER DEFAULT 0,
            is_managed INTEGER DEFAULT 0,
            encrypted INTEGER DEFAULT 0,
            pinned INTEGER DEFAULT 0,
            rating INTEGER DEFAULT 0,               -- 评级 0~5
            color_hex TEXT DEFAULT '',              -- 标记颜色
            auto_color TEXT DEFAULT '',             -- 自动调色板特征 Hex
            width INTEGER DEFAULT 0,
            height INTEGER DEFAULT 0,
            thumb_status INTEGER DEFAULT 0,
            note TEXT DEFAULT '',                   -- 备注
            url TEXT DEFAULT '',                    -- 关联网址
            sha256 TEXT DEFAULT '',
            created_at INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT 0,
            added_at INTEGER DEFAULT 0,
            sync_state INTEGER DEFAULT 0,
            synced_at INTEGER DEFAULT 0
        );
        -- 为物理路径及智能分类的高频过滤字段建立 B-Tree 索引
        CREATE INDEX IF NOT EXISTS idx_items_meta_file_id ON library_items(meta_file_id);
        CREATE INDEX IF NOT EXISTS idx_items_rating ON library_items(rating);
        CREATE INDEX IF NOT EXISTS idx_items_ext ON library_items(extension);
        CREATE INDEX IF NOT EXISTS idx_items_color ON library_items(color_hex);
        CREATE INDEX IF NOT EXISTS idx_items_updated ON library_items(updated_at DESC);

        -- ===================================================================
        -- 4. 主色调调色板拆分表
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS item_palettes (
            file_id TEXT NOT NULL,
            idx INTEGER NOT NULL,
            color_hex TEXT DEFAULT '',
            ratio REAL DEFAULT 0,
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
        -- 6. 回收站表（补全高频检索与恢复索引）
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
        -- 7. 驱动器与智能分类元数据
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
    if (sqlite3_exec(db, kNativeSchema, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        logSqlError("Native schema creation", db);
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_close_v2(db);
        m_lastError = "原生库建表失败";
        return false;
    }

    if (currentVersion != kSchemaVersion) {
        sqlite3_exec(db, QString("PRAGMA user_version = %1;").arg(kSchemaVersion).toUtf8().constData(),
                     nullptr, nullptr, nullptr);
    }

    m_nativeDb = db;
    logStartup("原生连接就绪: " + dbPath);
    return true;
}

QString DatabaseManager::getAppDir() {
    return QCoreApplication::applicationDirPath();
}

QString DatabaseManager::getGlobalDbPath() {
    return QDir::toNativeSeparators(getAppDir() + "/.RapidNotes/inspiration.db");
}

QString DatabaseManager::getBackupDir() {
    QString dir = QDir::toNativeSeparators(getAppDir() + "/.RapidNotes/backups");
    QDir().mkpath(dir);
    return dir;
}

sqlite3* DatabaseManager::getGlobalDb() {
    // 原生句柄由本类自己持有（exe 内的 sqlite3.c），绝不再返回 Qt 驱动里另一份 SQLite 的句柄
    return m_nativeDb;
}

void DatabaseManager::logSqlError(const char* location, sqlite3* db, const QString& sql) {
    const char* errmsg = db ? sqlite3_errmsg(db) : "Unknown error (null db)";
    if (!sql.isEmpty()) {
        QString truncatedSql = sql.left(120);
        qWarning() << QString("[DatabaseError][%1] %2 | SQL: %3").arg(location, errmsg, truncatedSql);
    } else {
        qWarning() << QString("[DatabaseError][%1] %2").arg(location, errmsg);
    }
}

void DatabaseManager::shutdown() {
    QMutexLocker locker(&m_mutex);
    if (m_autoSaveTimer) m_autoSaveTimer->stop();
    if (!m_isInitialized && !m_nativeDb && !m_db.isOpen()) return;
    m_isInitialized = false;

    {
        std::lock_guard<std::mutex> g(m_globalDbMutex);
        if (m_nativeDb) {
            sqlite3_wal_checkpoint_v2(m_nativeDb, nullptr, SQLITE_CHECKPOINT_FULL, nullptr, nullptr);
            sqlite3_close_v2(m_nativeDb);
            m_nativeDb = nullptr;
        }
    }

    QString connName = m_db.connectionName();
    if (m_db.isOpen()) {
        QSqlQuery cp(m_db);
        cp.exec("PRAGMA wal_checkpoint(TRUNCATE);");
        m_db.close();
    }
    m_db = QSqlDatabase();
    if (!connName.isEmpty()) QSqlDatabase::removeDatabase(connName);

    // 退出时执行一次快速归档备份
    backupDatabaseLatest();
    qDebug() << "[DB] 数据库已安全关闭（Qt 连接 + 原生连接）。";
}

void DatabaseManager::beginBatch() {
    QMutexLocker locker(&m_mutex);
    m_isBatchMode = true;
    m_cachedTrialStatus = getTrialStatus(true); // 预先校验并缓存
    if (m_db.isOpen()) {
        m_db.transaction();
    }
}

void DatabaseManager::endBatch() {
    QMutexLocker locker(&m_mutex);
    if (m_db.isOpen()) {
        m_db.commit();
        qDebug() << "[DB] 批量模式结束：事务已毫秒级提交";
    }
    m_isBatchMode = false;
    m_cachedTrialStatus.clear();
    
    // [FIX] 性能与一致性的平衡：
    // 1. 写 license.dat 文件极快(毫秒级)，必须同步执行，防止下一次操作触发“数据一致性”冲突界面。
    saveTrialToFile(getTrialStatus(false));

    // 2. 刷新数据库并备份
    markDirty(); // 标记脏数据，触发后台 7 秒自动保存
    qDebug() << "[DB] 授权文件已同步，数据库刷新已排队进入后台任务";
}

void DatabaseManager::rollbackBatch() {
    QMutexLocker locker(&m_mutex);
    if (m_db.isOpen()) {
        m_db.rollback();
    }
    m_isBatchMode = false;
    m_cachedTrialStatus.clear();
}
