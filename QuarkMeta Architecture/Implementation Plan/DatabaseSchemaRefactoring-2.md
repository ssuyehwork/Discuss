# Implementation Plan - Database Schema Refactoring Iteration 2 (Pre-existing DB Schema Auto-Healing Fix)

## Pre-Draft Evidence (Gate 1)

### 1. Read Files List
- `src/meta/DatabaseManager.h` (Read using `read_file`)
- `src/meta/DatabaseManager.cpp` (Read using `run_in_bash_session`)
- `Memories.md` Section 2.3 (Read using `read_file`)
- `AGENTS.md` Section 3.1 & Section 3.7 (Read using `read_file`)

### 2. Header Method Signatures Excerpts
From `src/meta/DatabaseManager.h`:
```cpp
class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager& instance();
    bool init();
    void shutdown();
    sqlite3* getGlobalDb();
    std::mutex& getGlobalMutex() { return m_globalDbMutex; }
    QRecursiveMutex* dbMutex() { return &m_dbMutex; }
    static QString getAppDir();
    static QString getGlobalDbPath();
private:
    bool loadDb(const std::wstring& diskPath, DbConnection& conn);
    void closeDb(DbConnection& conn);
};
```

### 3. SSOT Search Log
- Inspected `DatabaseManager::loadDb()` error path in `src/meta/DatabaseManager.cpp`:
  - Lines 98-100: Sets `PRAGMA foreign_keys = ON;`.
  - Lines 130-170: Executes `schema` script containing `CREATE TABLE IF NOT EXISTS library_items (...)` and `CREATE TABLE IF NOT EXISTS file_tags (...)`.
  - If a pre-existing `.QuarkMeta/QuarkMeta.store` file exists with the old `library_items` schema (having `id INTEGER` instead of `file_id TEXT`), `CREATE TABLE IF NOT EXISTS library_items` skips execution, while `file_tags` foreign key constraint `FOREIGN KEY (file_id) REFERENCES library_items(file_id)` fails with `SQLITE_ERROR`.
  - `sqlite3_exec` fails, causing `loadDb()` to return `false`, which aborts `DatabaseManager::init()` and prevents application startup.

### 4. Memories.md Key Rules Alignment
- **Memories.md Section 2.3**: Development/test phase rule explicitly states no complex data migration logic is needed. When schema changes occur, obsolete tables can be dropped and recreated directly.

---

## 1. Overview
### 1.1 Problem Statement & Root Cause Attribution
When launching the application after refactoring `library_items` schema to use `file_id TEXT PRIMARY KEY`, existing installations failed to start:

**Root Cause Attribution**:
1. On machines with pre-existing `.QuarkMeta/QuarkMeta.store` database files, the `library_items` table was previously created with column `id INTEGER PRIMARY KEY` and `file_tags` with `item_id INTEGER`.
2. When `DatabaseManager::loadDb()` executes `CREATE TABLE IF NOT EXISTS library_items (...)`, SQLite sees that `library_items` already exists and skips table creation.
3. Next, `CREATE TABLE IF NOT EXISTS file_tags` attempts to execute with constraint `FOREIGN KEY (file_id) REFERENCES library_items(file_id)`.
4. Because the existing `library_items` table lacks column `file_id`, SQLite fails with `foreign key mismatch` or column error, causing `sqlite3_exec()` to return `SQLITE_ERROR`.
5. `loadDb()` logs `表结构创建失败`, closes the database connection, and returns `false`, causing `DatabaseManager::init()` to fail and the main application startup to abort.

### 1.2 Solution
In `DatabaseManager::loadDb()`, insert an auto-healing schema check prior to executing `schema`:
- Execute `SELECT file_id FROM library_items LIMIT 1;`.
- If `sqlite3_prepare_v2` fails (indicating `library_items` either does not exist or possesses the obsolete schema missing `file_id`), execute `DROP TABLE IF EXISTS file_tags; DROP TABLE IF EXISTS library_items;`.
- This ensures any pre-existing database file cleanly drops obsolete schema tables and creates fresh `file_id` primary key tables without startup errors.

---

## 2. Modified Files List
- `src/meta/DatabaseManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/meta/DatabaseManager.cpp`

```cpp
<<<<<<< SEARCH
    sqlite3_busy_timeout(conn.diskDb, 25000);
    sqlite3_exec(conn.diskDb, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA temp_store = MEMORY;", nullptr, nullptr, nullptr);

    const char* schema = R"(
=======
    sqlite3_busy_timeout(conn.diskDb, 25000);
    sqlite3_exec(conn.diskDb, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(conn.diskDb, "PRAGMA temp_store = MEMORY;", nullptr, nullptr, nullptr);

    // 自动自愈：探测预存库中 library_items 是否具备 file_id 列，若为旧表则擦除以触发全新建表
    sqlite3_stmt* checkStmt = nullptr;
    if (sqlite3_prepare_v2(conn.diskDb, "SELECT file_id FROM library_items LIMIT 1;", -1, &checkStmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS file_tags;", nullptr, nullptr, nullptr);
        sqlite3_exec(conn.diskDb, "DROP TABLE IF EXISTS library_items;", nullptr, nullptr, nullptr);
    } else {
        sqlite3_finalize(checkStmt);
    }

    const char* schema = R"(
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile `src/meta/DatabaseManager.cpp`.
2. Launch QuarkMeta on an environment with a pre-existing `.QuarkMeta/QuarkMeta.store` file.
3. Verify that `DatabaseManager::loadDb()` executes auto-healing table cleanup, initializes schema without SQLite errors, and successfully starts the main application.
4. Check that `library_items` and `file_tags` are created with `file_id` primary key and foreign key constraints.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Schema Initialization SSOT**: All database DDL execution and auto-healing logic is consolidated inside `DatabaseManager::loadDb()`.

---

## 6. Header API Signature Verification

| Calling Class | Method Called | Header Declaration Location | Verified Signature |
|---|---|---|---|
| `DatabaseManager` | `loadDb` | `src/meta/DatabaseManager.h:74` | `bool loadDb(const std::wstring& diskPath, DbConnection& conn);` |

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/meta/DatabaseManager.cpp` includes `"DatabaseManager.h"` and `<sqlite3.h>`.
- The added `sqlite3_prepare_v2`, `sqlite3_finalize`, and `sqlite3_exec` calls are standard C APIs from `<sqlite3.h>`.

---

## 8. Compliance Self-Check Table

| Item | Source Clause | Status | Location in Plan |
|---|---|---|---|
| Filename mapping 1:1 with core class/module | 3.1-2 | Compliant | `DatabaseSchemaRefactoring-2.md` |
| Isolated in physical plan directory | 3.1-1 | Compliant | `QuarkMeta Architecture/Implementation Plan/` |
| All SEARCH blocks exist in codebase | 3.2 | Compliant | Section 3 |
| CMake alignment for Q_OBJECT classes | 3.3 | Not Applicable | No new classes added |
| Chapters 1~9 complete | 3.4 | Compliant | Sections 1~9 |
| External member calls matched with headers | 3.5-1/2 | Compliant | Section 6 |
| Header inclusion chain verified | 3.5-3 | Compliant | Section 7 |
| Reused SSOT entry points | 2.4 | Compliant | Section 5 |
| No inline setStyleSheet | 4.1 | Compliant | Section 1 |
| No platform hacks | 4.2 | Compliant | Section 1 |
| Aggregated broadcast for batch operations | 4.3 | Not Applicable | Schema initialization fix |
| Zero value alteration | 4.4 | Compliant | Section 9 |
| Preserved user interaction design | 1.2 / Memories.md | Compliant | Section 1 |
| No files modified outside scope | 5.3 | Compliant | Section 2 |

---

## 9. Zero-Value-Alteration Check
- **Not Applicable**: This plan addresses database initialization auto-healing logic only. No UI numerical parameters are modified.
