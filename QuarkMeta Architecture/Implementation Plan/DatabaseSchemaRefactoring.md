# Implementation Plan - Database Schema Refactoring (File_ID Primary Key & Tag Vocabulary Isolation)

## Pre-Draft Evidence (Gate 1)

### 1. Read Files List
- `src/meta/DatabaseManager.h` (Read using `read_file`)
- `src/meta/DatabaseManager.cpp` (Read using `run_in_bash_session`)
- `src/meta/SmartFolderDao.h` (Read using `read_file`)
- `src/meta/SmartFolderDao.cpp` (Read using `run_in_bash_session`)
- `src/meta/LibraryDao.h` (Read using `read_file`)
- `src/meta/LibraryDao.cpp` (Read using `run_in_bash_session`)
- `src/meta/MetaIndexService.h` (Read using `read_file`)
- `src/meta/MetaIndexService.cpp` (Read using `run_in_bash_session`)
- `Memories.md` Section 14 (Read using `read_file`)

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
};
```

From `src/meta/SmartFolderDao.h`:
```cpp
class SmartFolderDao {
public:
    static int countMatchingItems(int folderId);
    static QList<QString> getMatchingItemPaths(int folderId, int limit = 1000);
};
```

### 3. SSOT Search Log
- Searched `library_items` and `file_tags` references across `src/meta/`:
  - `DatabaseManager.cpp`: DDL schema definitions for `tags`, `library_items`, `file_tags`.
  - `SmartFolderDao.cpp`: Subqueries on `file_tags` and `library_items` for filtering rules.
- Result: No extra redundant tables or bypass queries found outside `DatabaseManager` and `SmartFolderDao`.

### 4. Memories.md Key Rules Alignment
- **Memories.md Section 14.2**: `library_items` MUST use `file_id TEXT PRIMARY KEY` as the unique key for cross-directory resilience, with `file_path` as a unique index.
- **Memories.md Section 14.4**: `tags` table MUST remain a pure global vocabulary dictionary without direct foreign keys or耦合 with business entities. All M:N associations MUST route through `file_tags` / `tag_group_items`.

---

## 1. Overview
This implementation plan refactors the `global.db` database schema and index tables to comply with `Memories.md` Section 14:
1. **`library_items` Schema Refactoring**: Updates `library_items` table schema in `DatabaseManager.cpp` to use `file_id TEXT PRIMARY KEY` instead of auto-incrementing integer `id`. `file_path` is maintained as `TEXT NOT NULL UNIQUE` with a dedicated B-tree index (`idx_items_path`).
2. **`file_tags` M:N Association Refactoring**: Updates `file_tags` table schema to reference `file_id TEXT NOT NULL` with foreign key constraint `FOREIGN KEY (file_id) REFERENCES library_items(file_id) ON DELETE CASCADE ON UPDATE CASCADE`.
3. **`tags` Vocabulary Isolation**: Enforces `tags` table strictly as a pure global dictionary table without direct business entity coupling.
4. **`SmartFolderDao` Query Adaptation**: Updates SQL subqueries in `SmartFolderDao.cpp` to query `file_id` in `file_tags` instead of `id` / `item_id`.

---

## 2. Modified Files List
- `src/meta/DatabaseManager.cpp`
- `src/meta/SmartFolderDao.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/meta/DatabaseManager.cpp`

```cpp
<<<<<<< SEARCH
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
=======
        -- ===================================================================
        -- 3. 全局文件索引表 (Sidecar File_ID 主键契约)
        -- ===================================================================
        CREATE TABLE IF NOT EXISTS library_items (
            file_id TEXT PRIMARY KEY,               -- 持久化唯一标识 (UUID/Base36)
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
>>>>>>> REPLACE
```

---

### 3.2 `src/meta/SmartFolderDao.cpp`

```cpp
<<<<<<< SEARCH
        else if (rule.type == SmartRuleType::Tag) {
            // 利用 file_tags 多对多关联表实现秒级查询
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("id NOT IN (SELECT item_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            } else {
                whereConditions << QString("id IN (SELECT item_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            }
        }
=======
        else if (rule.type == SmartRuleType::Tag) {
            // 利用 file_tags 多对多关联表实现秒级查询 (以 file_id 解耦匹配)
            if (rule.op == SmartRuleOperator::NotContains) {
                whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            } else {
                whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                   .arg(rule.value.trimmed());
            }
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Delete temporary test database if present (`global.db`) to allow DDL re-initialization.
2. Compile the application using standard build scripts or CMake.
3. Launch QuarkMeta and verify database initialization outputs without SQLite schema errors.
4. Verify that `library_items` table is created with `file_id TEXT PRIMARY KEY` and `file_tags` uses `file_id`.
5. Execute a Smart Folder tag query to verify SQL statement validity.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Database Connection SSOT**: All database DDLs use `DatabaseManager::instance().getGlobalDb()` and `getGlobalMutex()`.
- **No Parallel Implementations**: Existing DDL statements in `DatabaseManager.cpp` are refactored in-place, eliminating obsolete `item_id` / `id` schema references.

---

## 6. Header API Signature Verification

| Calling Class | Method Called | Header Declaration Location | Verified Signature |
|---|---|---|---|
| `DatabaseManager` | `DatabaseManager::instance()` | `src/meta/DatabaseManager.h:37` | `static DatabaseManager& instance();` |
| `DatabaseManager` | `getGlobalDb()` | `src/meta/DatabaseManager.h:47` | `sqlite3* getGlobalDb();` |
| `SmartFolderDao` | `DatabaseManager::instance().getGlobalDb()` | `src/meta/DatabaseManager.h:47` | `sqlite3* getGlobalDb();` |

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/meta/DatabaseManager.cpp` includes `"DatabaseManager.h"` and `<sqlite3.h>`.
- `src/meta/SmartFolderDao.cpp` includes `"SmartFolderDao.h"` and `"DatabaseManager.h"`.
- All required symbols (`sqlite3`, `DatabaseManager`, `SmartFolderDao`) have complete type definitions.

---

## 8. Compliance Self-Check Table

| Item | Source Clause | Status | Location in Plan |
|---|---|---|---|
| Filename mapping 1:1 with core class/module | 3.1-2 | Compliant | `DatabaseSchemaRefactoring.md` |
| Isolated in physical plan directory | 3.1-1 | Compliant | `QuarkMeta Architecture/Implementation Plan/` |
| All SEARCH blocks exist in codebase | 3.2 | Compliant | Section 3 |
| CMake alignment for Q_OBJECT classes | 3.3 | Not Applicable | No new Q_OBJECT classes added |
| Chapters 1~9 complete | 3.4 | Compliant | Sections 1~9 |
| External member calls matched with headers | 3.5-1/2 | Compliant | Section 6 |
| Header inclusion chain verified | 3.5-3 | Compliant | Section 7 |
| Reused SSOT entry points | 2.4 | Compliant | Section 5 |
| No inline setStyleSheet | 4.1 | Compliant | Section 1 (DB DDL only) |
| No platform hacks | 4.2 | Compliant | Section 1 |
| Aggregated broadcast for batch operations | 4.3 | Not Applicable | DB Schema refactoring |
| Zero value alteration | 4.4 | Compliant | Section 9 |
| Preserved user interaction design | 1.2 / Memories.md | Compliant | Section 1 |
| No files modified outside scope | 5.3 | Compliant | Section 2 |

---

## 9. Zero-Value-Alteration Check
- **Not Applicable**: This implementation plan refactors database DDL schemas and SQL subquery strings only. No UI numerical parameters (colors, px, margins, dimensions) are modified or moved.
