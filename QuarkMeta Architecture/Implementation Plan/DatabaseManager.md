# QuarkMeta.store Database Architecture Analysis & Optimization Plan

# DatabaseManager Implementation Plan

## 1. Pre-Draft Evidence List (第1关：起草前核查)

### 1.1 Read Files List (已读文件清单)
- `src/meta/DatabaseManager.h` & `src/meta/DatabaseManager.cpp` (物理读取)
- `src/meta/LibraryDao.h` & `src/meta/LibraryDao.cpp` (物理读取)
- `src/meta/MetaIndexService.h` & `src/meta/MetaIndexService.cpp` (物理读取)
- `src/meta/FavoriteDao.cpp` (物理读取)
- `src/meta/SmartFolderDao.cpp` (物理读取)
- `src/meta/DriveMetaDao.cpp` (物理读取)

### 1.2 Function Signatures Excerpt (签名原文摘录)
- `src/meta/DatabaseManager.h`:
  ```cpp
  static QString getGlobalDbPath();
  sqlite3* getGlobalDb();
  bool loadDb(const std::wstring& diskPath, DbConnection& conn);
  ```
- `src/meta/LibraryDao.h`:
  ```cpp
  static bool initTable();
  static bool initItemIndexTable();
  ```
- `src/meta/MetaIndexService.h`:
  ```cpp
  static bool initSchema(sqlite3* db);
  ```

### 1.3 SSOT API Search Log (既有 API 检索记录)
- 检索关键词：`QuarkMeta.store` / `CREATE TABLE` / `CREATE INDEX`
- 结果说明：所有表结构在 `DatabaseManager.cpp`、`LibraryDao.cpp` 及 `MetaIndexService.cpp` 中初始化，均建立在单数据库文件 `QuarkMeta.store` 上。

### 1.4 Memories.md & AGENTS.md Relevant Rules
- 免数据迁移规则（2.3节）：开发测试阶段无需考虑既有数据迁移与兼容性，直接优化 schema 架构。
- 物理隔离铁律（3.1节）：方案物理隔离存放在 `QuarkMeta Architecture/Implementation Plan/` 目录。
- 严禁修改源码（1.0节）：本文件仅作为实施方案存在，禁止直接改动 C++ 代码。

---

## 2. Overview（概述与深度架构分析）

用户提问：“分析 QuarkMeta.store 数据库的架构是否为多对多关联？是否存在索引设计缺陷/无效？”

通过深度扫描整个 C++ 代码库关于 `QuarkMeta.store` 的所有 DDL 建表与 DML 语句，分析结论如下：

### 2.1 数据库架构与关联关系分析（多对多 vs 一对多）
`QuarkMeta.store` 数据库内部包含两大功能板块的表集合：

1. **分类与路径管理 (Library Module)**：
   - `library_categories`: 储存分类树节点。
   - `library_category_paths`: 关联表 `(category_id INTEGER, path TEXT, PRIMARY KEY(category_id, path))`。
   - **关联判定**：**典型的多对多关联 (Many-to-Many Association)**。一个分类可以包含多个物理路径；同时，代码设计允许路径对应特定分类或全局/未分类组合（联合主键 `category_id + path`），通过 `library_category_paths` 交叉表解耦了分类与路径。
   - `library_item_index`: 储存卡片/条目元数据。`(category_id INTEGER, file_path TEXT, UNIQUE(category_id, file_path))`。也是按分类与路径多对多索引关联。

2. **标签组与标签管理 (Tag Group Module)**：
   - `tag_groups`: 储存标签分组。
   - `tag_group_items`: 关联表 `(group_id INTEGER, tag_name TEXT, PRIMARY KEY(group_id, tag_name))`。
   - **关联判定**：**典型的多对多关联 (Many-to-Many Association)**。一个标签组包含多个标签，一个标签也可归属于不同的标签组。

3. **磁盘回收站与驱动器/标签元数据 (Trash, Drive, Tags)**：
   - `disk_trash`, `drive_metadata`, `tags`: 为标准单表结构，主键分别为 `id`, `drive_path`, `name`。

4. **全局全文检索与文件夹缓存 (FTS & Meta Index Service)**：
   - `folders_index`: 记录文件夹最后扫描 mtime 及 size。
   - `items_index`: 记录项元数据。
   - `items_fts`: SQLite FTS5 虚拟表 + 触发器同步。

### 2.2 索引设计缺陷与无效索引分析

经过对全部 `CREATE INDEX` 和 SQL 查询（`SELECT`/`DELETE`/`UPDATE`）的交叉对比，发现 **4 大索引缺陷与性能隐患**：

1. **缺陷一：多对多关联表 `library_category_paths` 缺失逆向索引（导致 DELETE/查重 慢扫描）**
   - **现状**：表定义为 `PRIMARY KEY(category_id, path)`，SQLite 会自动创建索引 `(category_id, path)`。代码中又额外写了 `CREATE INDEX IF NOT EXISTS idx_category_paths_path ON library_category_paths(path);`。
   - **诊断**：写了 `idx_category_paths_path` 虽覆盖了按 `path` 查询，但在 `removePathsFromCategory` 中执 SQL：
     `DELETE FROM library_category_paths WHERE category_id = ? AND path = ?;`
     由于 `PRIMARY KEY(category_id, path)` 的最左前缀是 `category_id`，按 `path` 查找需要用到 `idx_category_paths_path`。但 `idx_category_paths_path` 仅对 `path` 单列建索引。
   - **更严重的问题在 `library_item_index` 表**：
     `library_item_index` 建立了 `UNIQUE(category_id, file_path)`。但在 `LibraryDao.cpp` 中存在大量 `DELETE FROM library_item_index WHERE file_path = ?;`（按文件路径擦除）以及按 `file_path` 单独查询。由于没有 `file_path` 的单列索引或逆向联合索引，当数据量大时，**对 `library_item_index` 的单文件擦除将触发全表扫描**！

2. **缺陷二：`items_index` 表高频复合查询索引失效（最左前缀匹配规则违背）**
   - **现状**：`MetaIndexService.cpp` 中建有复合索引：
     `CREATE INDEX IF NOT EXISTS idx_items_filter ON items_index(rating, color, type, file_mtime);`
   - **诊断**：在 UI 交互中，用户极少同时对 `rating + color + type + file_mtime` 进行组合过滤。更多场景是按 `folder_path + type`，或按 `color` 单独筛选，或按 `file_mtime` 排序。该 4 列复合索引过度苛刻，绝大多数单个或两个条件的 SELECT 查询**完全无法命中该复合索引**，属于**伪有效/低效索引**。

3. **缺陷三：`tags` 表索引冗余（SQLite Primary Key 自动索引重叠）**
   - **现状**：`DatabaseManager.cpp` 中定义：
     `name TEXT PRIMARY KEY` 以及 `CREATE INDEX IF NOT EXISTS idx_tags_last_used ON tags(last_used DESC);`
   - **诊断**：`name` 已是 PRIMARY KEY，SQLite 会自动创建 UNIQUE 索引。按 `last_used DESC` 建索引适合历史排序，此项设计正常；但 `tags` 表缺少 `use_count` 索引，若按使用频率排序则无法走索引。

4. **缺陷四：外键约束 (Foreign Key) 物理缺失**
   - **现状**：所有多对多关联表（如 `tag_group_items` 中的 `group_id`，`library_category_paths` 中的 `category_id`）均未声明 `REFERENCES` 级联外键。
   - **诊断**：当前数据一致性全靠 C++ 业务层手写 `DELETE` 手动清理（如 `removeCategoryById` 中用递归 CTE 手动删除关联路径和索引）。一旦 C++ 异常崩溃，极易在多对多关联表中留下孤立的无主垃圾数据（Orphan Rows）。

---

## 3. Modified Files List（影响文件清单）

- `src/meta/DatabaseManager.cpp`（优化全局表 DDL 索引）
- `src/meta/LibraryDao.cpp`（优化分类与条目索引 DDL）
- `src/meta/MetaIndexService.cpp`（优化 FTS 及条目过滤索引 DDL）

---

## 4. Detailed Line-by-Line Changes（精准替换块）

### 4.1 `src/meta/DatabaseManager.cpp`

<<<<<<< SEARCH
        CREATE TABLE IF NOT EXISTS tag_group_items (
            group_id INTEGER,
            tag_name TEXT,
            PRIMARY KEY (group_id, tag_name)
        );
=======
        CREATE TABLE IF NOT EXISTS tag_group_items (
            group_id INTEGER,
            tag_name TEXT,
            PRIMARY KEY (group_id, tag_name),
            FOREIGN KEY (group_id) REFERENCES tag_groups(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_tag_group_items_tag ON tag_group_items(tag_name);
>>>>>>> REPLACE

### 4.2 `src/meta/LibraryDao.cpp`

<<<<<<< SEARCH
    const char* sqlIndex = "CREATE TABLE IF NOT EXISTS library_item_index ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "category_id INTEGER NOT NULL, "
                           "file_path TEXT NOT NULL, "
                           "rating INTEGER DEFAULT 0, "
                           "manual_color TEXT DEFAULT '', "
                           "auto_color TEXT DEFAULT '', "
                           "palettes TEXT DEFAULT '', "
                           "tags TEXT DEFAULT '', "
                           "note TEXT DEFAULT '', "
                           "link TEXT DEFAULT '', "
                           "ratio INTEGER DEFAULT 0, "
                           "width INTEGER DEFAULT 0, "
                           "height INTEGER DEFAULT 0, "
                           "updated_at INTEGER, "
                           "UNIQUE(category_id, file_path));";
=======
    const char* sqlIndex = "CREATE TABLE IF NOT EXISTS library_item_index ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "category_id INTEGER NOT NULL, "
                           "file_path TEXT NOT NULL, "
                           "rating INTEGER DEFAULT 0, "
                           "manual_color TEXT DEFAULT '', "
                           "auto_color TEXT DEFAULT '', "
                           "palettes TEXT DEFAULT '', "
                           "tags TEXT DEFAULT '', "
                           "note TEXT DEFAULT '', "
                           "link TEXT DEFAULT '', "
                           "ratio INTEGER DEFAULT 0, "
                           "width INTEGER DEFAULT 0, "
                           "height INTEGER DEFAULT 0, "
                           "updated_at INTEGER, "
                           "UNIQUE(category_id, file_path));";

    const char* sqlItemPathIdx = "CREATE INDEX IF NOT EXISTS idx_library_item_file_path ON library_item_index(file_path);";
>>>>>>> REPLACE

<<<<<<< SEARCH
    sqlite3_exec(db, sqlCat, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPaths, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPathsIdx, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlIndex, nullptr, nullptr, &errMsgs);
=======
    sqlite3_exec(db, sqlCat, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPaths, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPathsIdx, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlIndex, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlItemPathIdx, nullptr, nullptr, &errMsgs);
>>>>>>> REPLACE

### 4.3 `src/meta/MetaIndexService.cpp`

<<<<<<< SEARCH
        CREATE INDEX IF NOT EXISTS idx_items_folder ON items_index(folder_path);
        CREATE INDEX IF NOT EXISTS idx_items_name ON items_index(name);
        CREATE INDEX IF NOT EXISTS idx_items_filter ON items_index(rating, color, type, file_mtime);
=======
        CREATE INDEX IF NOT EXISTS idx_items_folder ON items_index(folder_path);
        CREATE INDEX IF NOT EXISTS idx_items_name ON items_index(name);
        CREATE INDEX IF NOT EXISTS idx_items_rating ON items_index(rating);
        CREATE INDEX IF NOT EXISTS idx_items_color ON items_index(color);
        CREATE INDEX IF NOT EXISTS idx_items_type ON items_index(type);
        CREATE INDEX IF NOT EXISTS idx_items_mtime ON items_index(file_mtime DESC);
>>>>>>> REPLACE

---

## 5. Build & Verification Steps（编译命令与验证方法）

1. **构建工程**：
   在 MSVC / CMake 环境下执行编译构建，确保无语法或符号错误。
2. **执行 SQLite `EXPLAIN QUERY PLAN` 验证**：
   使用 sqlite3 命令行或 GUI 工具打开 `QuarkMeta.store`，对以下核心查询进行验证：
   - `EXPLAIN QUERY PLAN DELETE FROM library_item_index WHERE file_path = 'C:\\test.jpg';`
     确认输出提示：`USING INDEX idx_library_item_file_path (file_path=?)`（由全表扫描转为毫秒级索引查找）。
   - `EXPLAIN QUERY PLAN SELECT * FROM tag_group_items WHERE tag_name = 'Design';`
     确认输出提示：`USING INDEX idx_tag_group_items_tag (tag_name=?)`。

---

## 6. SSOT API Reuse & Anti-Redundancy Self-Check（自查说明）

- 本方案未修改任何 C++ 业务逻辑或新建冗余函数，仅针对数据库底层建表与索引 DDL 进行架构补强。

---

## 7. Header API Signature Verification（物理头文件签名核查表）

- `DatabaseManager::getGlobalDb()` -> `sqlite3* getGlobalDb();` (100% 映射一致)
- `LibraryDao::initTable()` -> `static bool initTable();` (100% 映射一致)

---

## 8. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

- 无修改 `#include` 头文件包含链。

---

## 9. Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 状态 | 说明/对应位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名：`DatabaseManager.md`（1:1 物理映射 `DatabaseManager` 类） |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | 物理目录：`QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 已与源文件逐行对比验证 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 章节完整 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 7 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未改动 `#include` |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 仅优化 DDL |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 不涉及 UI 样式 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 纯数据库架构分析与方案 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及事件广播 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 无数值迁移 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 完全基于数据库客观架构分析 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅聚焦数据库相关 DDL |

---

## 10. Zero-Value-Alteration Check（数值零篡改核对）

- 不涉及（本方案不涉及 UI 样式或数值参数迁移）。
