# Implementation Plan: LibraryDatabaseSchemaFix.md

- **Base Commit Hash**: 57252f1
- **Prerequisite Plans**: None
- **Supersedes**: None

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷描述
应用程序在运行中，无法将库成员索引数据正确写入 SQLite 数据库 `library_items` 表，导致智能文件夹查询匹配到的数量显示为 `0` 且点击进入后内容视图为空。

### 1.2 绝对根本原因分析 (Root Cause Analysis)
经过对 `src/meta/DatabaseManager.cpp` 与 `src/meta/LibraryItemDao.cpp` 源码的深入比对与排查，锁定根本原因如下：

1. **`library_items` 表结构与 DAO 读写列不一致（直接致错因）**：
   `DatabaseManager.cpp` 原有建表 SQL 中 `library_items` 仅包含了 13 个列，缺少 `is_dir`, `is_empty`, `is_hidden`, `is_managed`, `encrypted`, `pinned`, `auto_color`, `width`, `height`, `thumb_status`, `sha256`, `added_at`, `sync_state`, `synced_at` 等列，且完全没有创建 `item_palettes` 派生表；而 `LibraryItemDao::upsertItems` 试图写入 27 个列，`LibraryItemDao::getItemsByPaths` 试图读取 25 个列。因 SQLite 表结构缺少对应列，导致预备语句 `sqlite3_prepare_v2` 执行失败。
2. **错误吞没 (Error Swallowing)**：
   在 `LibraryItemDao.cpp` 与 `SmartFolderDao.cpp` 中，`sqlite3_prepare_v2` 失败后直接返回 `false` / `0` / 空列表，未向日志（`qWarning`）输出任何 `sqlite3_errmsg` 错误详情，导致数据库写库失败静默无感知。
3. **自愈机制不完善 (Flawed Self-Healing Mechanism)**：
   原 `loadDb` 中采用 `SELECT meta_file_id FROM library_items LIMIT 1` 进行简单探测。旧版本表只要包含 `meta_file_id` 列即判定通过，而后续的 `CREATE TABLE IF NOT EXISTS` 不会补全缺失的列，导致旧库文件永远无法修正列缺失问题。

### 1.3 拟定重构方案与设计目标
1. **服从 `LibraryItemDao` 规范**：以 `LibraryItemDao` 27 列读写 SQL 为 SSOT 权威真理源，更新 `DatabaseManager.cpp` 中的 `library_items` 建表 SQL 为完整的 27 列，并在 `file_path` 列上声明 `COLLATE NOCASE` 约束；
2. **新增 `item_palettes` 派生表**：在 `DatabaseManager.cpp` 中增加 `item_palettes` 表定义（包含主键 `PRIMARY KEY (file_id, idx)` 与外键级联）；
3. **版本号升级管理 (`PRAGMA user_version`)**：
   - 定义常量 `kSchemaVersion = 1`；
   - 废除原有的探测和 `DROP` 逻辑；
   - 读取 `PRAGMA user_version`：若不等于 1，在单个事务中按顺序 `DROP TABLE IF EXISTS item_palettes; DROP TABLE IF EXISTS file_tags; DROP TABLE IF EXISTS library_items;`，提交后重建这 3 张派生表（`tags` 字典表与其它表 100% 完整保留），成功后写入 `PRAGMA user_version = 1`；
   - 连续再次启动时，由于版本已为 1，不会执行任何 `DROP` 或重构。
4. **失败信息可见性 (`logSqlError`)**：
   - 在 `DatabaseManager` 中提供公有静态辅助函数 `logSqlError(const char* location, sqlite3* db, const char* sql)`，使用 `qWarning()` 格式化输出位置名、`sqlite3_errmsg` 及可选 SQL 前 120 字符；
   - 在 `LibraryItemDao.cpp` 和 `SmartFolderDao.cpp` 的所有 `sqlite3_prepare_v2` 失败点及 `upsertItems` 的 `sqlite3_step` 失败点调用该辅助函数。

---

## Chapter 2: Modified Files List（影响文件清单）

- `src/meta/DatabaseManager.h` (修改：新增 `logSqlError` 静态辅助函数声明)
- `src/meta/DatabaseManager.cpp` (修改：更新 27 列 `library_items` 与 `item_palettes` 模式、实现 `PRAGMA user_version` 事务重构与 `logSqlError`)
- `src/meta/LibraryItemDao.cpp` (修改：在 `prepare` 与 `step` 失败点增加 `DatabaseManager::logSqlError` 输出)
- `src/meta/SmartFolderDao.cpp` (修改：在 `prepare` 失败点增加 `DatabaseManager::logSqlError` 输出)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/DatabaseManager.h`

<<<<<<< SEARCH
    static void performDailyBackup();
    static bool tryRecoverFromBackup();

private:
=======
    static void performDailyBackup();
    static bool tryRecoverFromBackup();

    static void logSqlError(const char* location, sqlite3* db, const char* sql = nullptr);

private:
>>>>>>> REPLACE

---

### 3.2 `src/meta/DatabaseManager.cpp`

<<<<<<< SEARCH
    sqlite3_busy_timeout(conn.diskDb, 25000);
    sqlite3_exec(conn.diskDb, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA temp_store = MEMORY;", nullptr, nullptr, nullptr);

    // 自动自愈：探测预存库中 library_items 是否具备 meta_file_id 列，若为旧表则擦除以触发全新建表
    sqlite3_stmt* checkStmt = nullptr;
    if (sqlite3_prepare_v2(conn.diskDb, "SELECT meta_file_id FROM library_items LIMIT 1;", -1, &checkStmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS file_tags;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS item_palettes;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS library_items;", nullptr, nullptr, nullptr);
    } else {
        sqlite3_finalize(checkStmt);
    }

    const char* schema = R"(
=======
    sqlite3_busy_timeout(conn.diskDb, 25000);
    sqlite3_exec(conn.diskDb, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA temp_store = MEMORY;", nullptr, nullptr, nullptr);

    constexpr int kSchemaVersion = 1;
    int currentVer = 0;
    sqlite3_stmt* verStmt = nullptr;
    if (sqlite3_prepare_v2(conn.diskDb, "PRAGMA user_version;", -1, &verStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(verStmt) == SQLITE_ROW) {
            currentVer = sqlite3_column_int(verStmt, 0);
        }
        sqlite3_finalize(verStmt);
    }

    if (currentVer != kSchemaVersion) {
        SqlTransaction tx(conn.diskDb);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS item_palettes;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS file_tags;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS library_items;", nullptr, nullptr, nullptr);
        if (!tx.commit()) {
            logSqlError("loadDb.DropTransaction", conn.diskDb);
            sqlite3_close_v2(conn.diskDb);
            conn.diskDb = nullptr;
            return false;
        }
    }

    const char* schema = R"(
>>>>>>> REPLACE

<<<<<<< SEARCH
        -- ===================================================================
        -- 3. 全局文件索引表 (Sidecar File_ID 主键契约)
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS library_items (
            file_id TEXT PRIMARY KEY,               -- 持久化唯一标识 (UUID/Base36)
            meta_file_id TEXT DEFAULT '',           -- .QuarkMeta.json 自身 File_ID 凭据
            file_path TEXT NOT NULL UNIQUE,         -- 绝对物理路径（独立唯一索引列）
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
        -- 为物理路径及智能文件夹的高频过滤字段建立 B-Tree 索引
        CREATE INDEX IF NOT EXISTS idx_items_path ON library_items(file_path);
        CREATE INDEX IF NOT EXISTS idx_items_meta_file_id ON library_items(meta_file_id);
        CREATE INDEX IF NOT EXISTS idx_items_rating ON library_items(rating);
        CREATE INDEX IF NOT EXISTS idx_items_ext ON library_items(extension);
        CREATE INDEX IF NOT EXISTS idx_items_color ON library_items(color_hex);
        CREATE INDEX IF NOT EXISTS idx_items_updated ON library_items(updated_at DESC);

        -- ===================================================================
        -- 4. 文件与标签的多对多解耦关联表
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
=======
        -- ===================================================================
        -- 3. 全局文件索引表 (Sidecar File_ID 主键契约，27 列服从 LibraryItemDao)
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS library_items (
            file_id TEXT PRIMARY KEY,
            meta_file_id TEXT DEFAULT '',
            file_path TEXT NOT NULL UNIQUE COLLATE NOCASE,
            file_name TEXT NOT NULL,
            base_name TEXT NOT NULL,
            extension TEXT NOT NULL COLLATE NOCASE,
            file_size INTEGER DEFAULT 0,
            is_dir INTEGER DEFAULT 0,
            is_empty INTEGER DEFAULT 0,
            is_hidden INTEGER DEFAULT 0,
            is_managed INTEGER DEFAULT 0,
            encrypted INTEGER DEFAULT 0,
            pinned INTEGER DEFAULT 0,
            rating INTEGER DEFAULT 0,
            color_hex TEXT DEFAULT '',
            auto_color TEXT DEFAULT '',
            width INTEGER DEFAULT 0,
            height INTEGER DEFAULT 0,
            thumb_status INTEGER DEFAULT 0,
            note TEXT DEFAULT '',
            url TEXT DEFAULT '',
            sha256 TEXT DEFAULT '',
            created_at INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT 0,
            added_at INTEGER DEFAULT 0,
            sync_state INTEGER DEFAULT 0,
            synced_at INTEGER DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_items_meta_file_id ON library_items(meta_file_id);
        CREATE INDEX IF NOT EXISTS idx_items_rating ON library_items(rating);
        CREATE INDEX IF NOT EXISTS idx_items_ext ON library_items(extension);
        CREATE INDEX IF NOT EXISTS idx_items_color ON library_items(color_hex);
        CREATE INDEX IF NOT EXISTS idx_items_updated ON library_items(updated_at DESC);

        -- ===================================================================
        -- 4. 文件与标签的多对多解耦关联表及调色板表
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS file_tags (
            file_id TEXT NOT NULL,
            tag_name TEXT NOT NULL COLLATE NOCASE,
            PRIMARY KEY (file_id, tag_name),
            FOREIGN KEY (file_id) REFERENCES library_items(file_id) ON DELETE CASCADE ON UPDATE CASCADE,
            FOREIGN KEY (tag_name) REFERENCES tags(name) ON DELETE CASCADE ON UPDATE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_file_tags_tag ON file_tags(tag_name);

        CREATE TABLE IF NOT EXISTS item_palettes (
            file_id TEXT NOT NULL,
            idx INTEGER NOT NULL,
            color_hex TEXT DEFAULT '',
            ratio REAL DEFAULT 0,
            PRIMARY KEY (file_id, idx),
            FOREIGN KEY (file_id) REFERENCES library_items(file_id) ON DELETE CASCADE ON UPDATE CASCADE
        );

        -- ===================================================================
        -- 5. 回收站表（补全高频检索与恢复索引）
>>>>>>> REPLACE

<<<<<<< SEARCH
    char* errMsg = nullptr;
    if (sqlite3_exec(conn.diskDb, schema, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        qCritical() << "[DatabaseManager] 表结构创建失败:" << errMsg;
        sqlite3_free(errMsg);
        sqlite3_close_v2(conn.diskDb);
        conn.diskDb = nullptr;
        return false;
    }

    // 🚨 核心加固：立即执行 TRUNCATE 检查点，将表结构瞬间从 -wal 固化回 QuarkMeta.store 主文件！
    sqlite3_wal_checkpoint_v2(conn.diskDb, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);

    conn.diskPath = nativeWPath;
    qInfo() << "[DatabaseManager] 4 张全局表已成功固化到物理数据库:" << qDiskPath;
=======
    char* errMsg = nullptr;
    if (sqlite3_exec(conn.diskDb, schema, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        logSqlError("loadDb.CreateSchema", conn.diskDb);
        sqlite3_free(errMsg);
        sqlite3_close_v2(conn.diskDb);
        conn.diskDb = nullptr;
        return false;
    }

    if (currentVer != kSchemaVersion) {
        sqlite3_exec(conn.diskDb, "PRAGMA user_version = 1;", nullptr, nullptr, nullptr);
    }

    // 🚨 核心加固：立即执行 TRUNCATE 检查点，将表结构瞬间从 -wal 固化回 QuarkMeta.store 主文件！
    sqlite3_wal_checkpoint_v2(conn.diskDb, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);

    conn.diskPath = nativeWPath;
    qInfo() << "[DatabaseManager] 全局数据库表结构已成功固化:" << qDiskPath;
>>>>>>> REPLACE

<<<<<<< SEARCH
void DatabaseManager::closeDb(DbConnection& conn) {
=======
void DatabaseManager::logSqlError(const char* location, sqlite3* db, const char* sql) {
    if (!db) return;
    const char* errmsg = sqlite3_errmsg(db);
    if (sql && strlen(sql) > 0) {
        QString sqlShort = QString::fromUtf8(sql).left(120).trimmed();
        qWarning().noquote() << QString("[DatabaseManager] SQL失败 [%1]: %2 | SQL: %3")
                                .arg(QString::fromUtf8(location))
                                .arg(QString::fromUtf8(errmsg))
                                .arg(sqlShort);
    } else {
        qWarning().noquote() << QString("[DatabaseManager] SQL失败 [%1]: %2")
                                .arg(QString::fromUtf8(location))
                                .arg(QString::fromUtf8(errmsg));
    }
}

void DatabaseManager::closeDb(DbConnection& conn) {
>>>>>>> REPLACE

---

### 3.3 `src/meta/LibraryItemDao.cpp`

<<<<<<< SEARCH
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) {
            continue;
        }
=======
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) {
            DatabaseManager::logSqlError("LibraryItemDao.getItemsByPaths.prepare", db, sql.toUtf8().constData());
            continue;
        }
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (sqlite3_prepare_v2(db, tagSql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
            for (int j = 0; j < chunkFileIds.size(); ++j) {
                sqlite3_bind_text(stmt, j + 1, chunkFileIds[j].toUtf8().constData(), -1, SQLITE_TRANSIENT);
            }
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QString fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                QString tagName = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                if (chunkItemMap.contains(fileId)) {
                    chunkItemMap[fileId].tags.append(tagName);
                }
            }
            sqlite3_finalize(stmt);
        }
=======
        if (sqlite3_prepare_v2(db, tagSql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
            for (int j = 0; j < chunkFileIds.size(); ++j) {
                sqlite3_bind_text(stmt, j + 1, chunkFileIds[j].toUtf8().constData(), -1, SQLITE_TRANSIENT);
            }
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QString fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                QString tagName = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                if (chunkItemMap.contains(fileId)) {
                    chunkItemMap[fileId].tags.append(tagName);
                }
            }
            sqlite3_finalize(stmt);
        } else {
            DatabaseManager::logSqlError("LibraryItemDao.getItemsByPaths.tagPrepare", db, tagSql.toUtf8().constData());
        }
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (sqlite3_prepare_v2(db, palSql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
            for (int j = 0; j < chunkFileIds.size(); ++j) {
                sqlite3_bind_text(stmt, j + 1, chunkFileIds[j].toUtf8().constData(), -1, SQLITE_TRANSIENT);
            }
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QString fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                QString colorHex = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                float ratio = static_cast<float>(sqlite3_column_double(stmt, 2));
                if (chunkItemMap.contains(fileId)) {
                    chunkItemMap[fileId].palettes.push_back({QColor(colorHex), ratio});
                }
            }
            sqlite3_finalize(stmt);
        }
=======
        if (sqlite3_prepare_v2(db, palSql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
            for (int j = 0; j < chunkFileIds.size(); ++j) {
                sqlite3_bind_text(stmt, j + 1, chunkFileIds[j].toUtf8().constData(), -1, SQLITE_TRANSIENT);
            }
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QString fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                QString colorHex = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                float ratio = static_cast<float>(sqlite3_column_double(stmt, 2));
                if (chunkItemMap.contains(fileId)) {
                    chunkItemMap[fileId].palettes.push_back({QColor(colorHex), ratio});
                }
            }
            sqlite3_finalize(stmt);
        } else {
            DatabaseManager::logSqlError("LibraryItemDao.getItemsByPaths.palPrepare", db, palSql.toUtf8().constData());
        }
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (sqlite3_prepare_v2(db, queryIdSql, -1, &stmtQueryId, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, upsertSql, -1, &stmtUpsert, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, delTagsSql, -1, &stmtDelTags, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, tagDictSql, -1, &stmtTagDict, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, insTagSql, -1, &stmtInsTag, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, delPalSql, -1, &stmtDelPal, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, insPalSql, -1, &stmtInsPal, nullptr) != SQLITE_OK) {
        cleanupStmts();
        return false;
    }
=======
    if (sqlite3_prepare_v2(db, queryIdSql, -1, &stmtQueryId, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, upsertSql, -1, &stmtUpsert, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, delTagsSql, -1, &stmtDelTags, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, tagDictSql, -1, &stmtTagDict, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, insTagSql, -1, &stmtInsTag, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, delPalSql, -1, &stmtDelPal, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, insPalSql, -1, &stmtInsPal, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("LibraryItemDao.upsertItems.prepare", db);
        cleanupStmts();
        return false;
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (sqlite3_step(stmtUpsert) != SQLITE_DONE) {
            cleanupStmts();
            return false;
        }
=======
        if (sqlite3_step(stmtUpsert) != SQLITE_DONE) {
            DatabaseManager::logSqlError("LibraryItemDao.upsertItems.stepUpsert", db, upsertSql);
            cleanupStmts();
            return false;
        }
>>>>>>> REPLACE

<<<<<<< SEARCH
    const char* sql = "DELETE FROM library_items WHERE file_path = ? COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    for (const QString& path : paths) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
=======
    const char* sql = "DELETE FROM library_items WHERE file_path = ? COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    for (const QString& path : paths) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        } else {
            DatabaseManager::logSqlError("LibraryItemDao.removeItemsByPaths.prepare", db, sql);
        }
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, info.fileName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, info.completeBaseName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, info.suffix().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, oldPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
=======
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, info.fileName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, info.completeBaseName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, info.suffix().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, oldPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    } else {
        DatabaseManager::logSqlError("LibraryItemDao.updatePath.prepare", db, sql);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, cleanNew.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, cleanOld.length() + 1);
        sqlite3_bind_text(stmt, 3, cleanOld.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
=======
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, cleanNew.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, cleanOld.length() + 1);
        sqlite3_bind_text(stmt, 3, cleanOld.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    } else {
        DatabaseManager::logSqlError("LibraryItemDao.updatePathPrefix.prepare", db, sql);
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, syncState);
        sqlite3_bind_text(stmt, 2, path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
=======
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, syncState);
        sqlite3_bind_text(stmt, 2, path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    } else {
        DatabaseManager::logSqlError("LibraryItemDao.markSyncStateByPath.prepare", db, sql);
    }
>>>>>>> REPLACE

---

### 3.4 `src/meta/SmartFolderDao.cpp`

<<<<<<< SEARCH
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
=======
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    } else {
        DatabaseManager::logSqlError("SmartFolderDao.countMatchingItems.prepare", db, sql.toUtf8().constData());
    }
    return count;
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* p = sqlite3_column_text(stmt, 0);
            if (p) {
                paths.append(QString::fromUtf8(reinterpret_cast<const char*>(p)));
            }
        }
        sqlite3_finalize(stmt);
    }
    return paths;
=======
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* p = sqlite3_column_text(stmt, 0);
            if (p) {
                paths.append(QString::fromUtf8(reinterpret_cast<const char*>(p)));
            }
        }
        sqlite3_finalize(stmt);
    } else {
        DatabaseManager::logSqlError("SmartFolderDao.getMatchingPaths.prepare", db, sql.toUtf8().constData());
    }
    return paths;
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译指令
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 4.2 自检答复与运行时验收标准
1. **`library_items` 列数与 `upsertItems` 一致性**：
   - `library_items` 建表定义包含 27 列，与 `LibraryItemDao::upsertItems` 27 列的 `VALUES (?, ?, ..., ?)` 清单逐列完全匹配；`item_palettes` 派生表已正确创建。
2. **全新与旧版本数据库启动表现**：
   - 对全新或旧版本数据库首次启动，`PRAGMA user_version` 在重建或初始化后均更新设为 `1`。
3. **连续启动两次**：
   - 第二次启动时读取到 `PRAGMA user_version == 1`，条件分支命中，不做任何 `DROP` 物理操作。
4. **`tags` 表数据保留验证**：
   - 模式升级事务仅物理清理并重建派生表 `item_palettes`、`file_tags` 和 `library_items`，字典表 `tags` 及其关联数据 100% 完整保留。
5. **SQL 失败日志可见性**：
   - 当构造 `prepare` 或 `step` 语法/字段错误时，`DatabaseManager::logSqlError` 在控制台中以 `qWarning` 输出 `location`、`sqlite3_errmsg` 和 SQL 前 120 字符。
6. **`LibrarySyncService` 启动阶段首次调用位置**：
   - 首次被调用的位置在 `src/main.cpp` 第 107 行实例化 `QuarkApplication` 之后，于第 138 行调用 `QuarkMeta::CoreController::initializeCoreComponents()`；
   - 在 `src/core/CoreController.cpp` 第 27 行调用的 `QuarkMeta::MetadataManager::instance()` 的构造函数中，隐式触发了对 `LibrarySyncService::instance()` 的首次单例构造与初始化，其构造函数（`src/meta/LibrarySyncService.cpp` 第 19 行）随之启动并挂接 `triggerStartupReconciliation()` 定时任务。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **统一使用 `SqlTransaction` 事务 RAII**：版本升级逻辑中的 `DROP` 使用 `DatabaseManager.cpp` 已有的 `SqlTransaction` 守卫进行提交控制；
- **纯标准 SQLite API**：版本与模式管理全部通过标准 `PRAGMA` 与 SQLite C API 完成，零另起炉灶。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 库文件全量索引记录 | `LibraryItemDao` 27 列 SQL 清单 | SQLite `library_items` 表 | 由 `LibrarySyncService` 后台监听对账补齐并刷入 `global.db` |
| 调色板颜色提取信息 | `item_palettes` 派生表 | `ItemRecord::palettes` | 随 `LibraryItemDao::upsertItems` 保存写入 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- 新增公有静态日志方法：
  - 声明位置：`src/meta/DatabaseManager.h`：`static void logSqlError(const char* location, sqlite3* db, const char* sql = nullptr);`
  - 定义位置：`src/meta/DatabaseManager.cpp`；
  - 调用点：`LibraryItemDao.cpp` 与 `SmartFolderDao.cpp` 中的 SQL prepare/step 失败捕获分支，访问控制为 `public`。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `LibraryItemDao.cpp` 与 `SmartFolderDao.cpp` 均已包含 `"DatabaseManager.h"`，可以直接调用 `DatabaseManager::logSqlError`；
- 头文件包含链完全闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 核对项目 | 是否满足 | 说明/具体位置 |
| :---: | :--- | :---: | :--- |
| 1 | 方案放置于 `Implementation Plan/` 目录 | 满足 | `Implementation Plan/LibraryDatabaseSchemaFix.md` |
| 2 | Git Merge Diff 格式合规 | 满足 | Chapter 3 统一采用 SEARCH/REPLACE 块 |
| 3 | SEARCH 块物理匹配唯一性 | 满足 | 块内上下文在对应目标文件中物理唯一匹配 |
| 4 | 无直接修改源代码承诺 | 满足 | 方案仅存放在本物理隔离方案文件中 |
| 5 | 五道工程硬锁之【契约锁】 | 满足 | 公开接口签名冻结无违规篡改 |
| 6 | 五道工程硬锁之【依赖锁】 | 满足 | 单向依赖流，无跨层逆向下钻 |
| 7 | 五道工程硬锁之【静态红线】 | 满足 | 无违规事件过滤器与平台 Hack |
| 8 | 服从 LibraryItemDao 标准 | 满足 | `library_items` 27 列完全服从 `LibraryItemDao` |
| 9 | PRAGMA user_version 管理 | 满足 | user_version == 1 时跳过 DROP，不匹配时单次事务重建 |
| 10 | 字典表 tags 数据保留 | 满足 | 表清理仅限 `item_palettes`、`file_tags`、`library_items` 3 张派生表 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 行为与交互零篡改声明
- 智能文件夹 UI 展现与过滤控制行为 100% 零篡改；
- 库侧边栏分类展开与节点点击导航 100% 零篡改。

### 10.2 样式数值零篡改声明
- 本次重构为纯底层数据持久化模式修正，不修改任何 UI QSS 样式、物理尺寸、颜色或边距数值，参数 100% 零篡改。
