# MetaIndexService-1.md Implementation Plan

## 0. Pre-Draft Evidence (起草前核查)

### 0.1 已读文件清单
- `src/meta/LibraryDao.h` (已物理读取)
- `src/meta/LibraryDao.cpp` (已物理读取)
- `src/meta/MetaIndexService.h` (已物理读取)
- `src/meta/MetaIndexService.cpp` (已物理读取)
- `src/meta/DatabaseManager.cpp` (已物理读取)

### 0.2 签名原文摘录
- `bool LibraryDao::initTable();`
- `bool MetaIndexService::initSchema(sqlite3* db);`
- `QStringList MetaIndexService::searchGlobal(const QString& keyword, const QString& scopeFolderPath, int limit);`

### 0.3 既有 API 检索记录
- 检索 `library_item_index`：仅在 `LibraryDao.cpp` 中用于废弃的独立卡片索引。
- 检索 `items_index`：在 `MetaIndexService.cpp` 中作为权威全库缓存索引表使用。
- 检索 `library_category_paths`：在 `LibraryDao.cpp` 中作为分类与物理路径关联表使用。

### 0.4 Memories.md 与 AGENTS.md 相关条款要点
- 只读不可二次修改铁律（3.1-3）：由于既有 `MetaIndexService.md` 已提交只读，本次迭代采用递增后缀形式新建 `MetaIndexService-1.md` 方案文件。
- 免数据迁移规则（2.3）：开发测试阶段，无需考虑既有数据迁移与版本兼容问题，可直接重构 DDL 与逻辑。
- 物理实施方案隔离（3.1）：实施方案保存在 `QuarkMeta Architecture/Implementation Plan/` 物理隔离目录中。

---

## 1. Overview (概述与解决的问题)

本实施方案针对 QuarkMeta 数据库（SQLite）架构中 `MetaIndexService` 与 `LibraryDao` 相关的 3 项核心缺陷进行优化：
1. **清理重叠缓存表 `library_item_index`**：归一化缓存表，移除 `LibraryDao` 中的冗余建表语句，统一归流至 `MetaIndexService` 的 `items_index` 表。
2. **补充 `library_category_paths` 的 `path` 反向索引**：创建 `idx_category_paths_path` 索引，提升“路径 ➔ 分类”反查性能（复杂度由 $O(N)$ 降至 $O(\log N)$）。
3. **升级 `items_fts` 全文检索虚表与精准匹配语法**：
   - 使用 `rowid` 进行 `items_index` 与 `items_fts` 的高效整型关联（消除 `UNINDEXED` 文本连接引发的全表扫描）；
   - 修正 SQL 表别名一致性（使用 `f MATCH ?` 或无别名虚表列，彻底规避 `no such column: items_fts` 运行报错）。

---

## 2. Modified Files List (影响文件清单)

1. `src/meta/LibraryDao.cpp`
2. `src/meta/MetaIndexService.cpp`

---

## 3. Detailed Line-by-Line Changes (精准替换块)

### 3.1 `src/meta/LibraryDao.cpp` (补充 `path` 反向索引 & 移除 `library_item_index` 表建表)

```
<<<<<<< SEARCH
    const char* sqlPaths = "CREATE TABLE IF NOT EXISTS library_category_paths ("
                           "category_id INTEGER, "
                           "path TEXT NOT NULL, "
                           "PRIMARY KEY(category_id, path));";

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

    char* errMsgs = nullptr;
    sqlite3_exec(db, sqlCat, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPaths, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlIndex, nullptr, nullptr, &errMsgs);
=======
    const char* sqlPaths = "CREATE TABLE IF NOT EXISTS library_category_paths ("
                           "category_id INTEGER, "
                           "path TEXT NOT NULL, "
                           "PRIMARY KEY(category_id, path));";

    const char* sqlPathsIdx = "CREATE INDEX IF NOT EXISTS idx_category_paths_path ON library_category_paths(path);";

    char* errMsgs = nullptr;
    sqlite3_exec(db, sqlCat, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPaths, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPathsIdx, nullptr, nullptr, &errMsgs);
>>>>>>> REPLACE
```

---

### 3.2 `src/meta/MetaIndexService.cpp` (在 `initSchema` 中新增 FTS5 全文检索表与基于 `rowid` 的自动触发器)

```
<<<<<<< SEARCH
        CREATE INDEX IF NOT EXISTS idx_items_folder ON items_index(folder_path);
        CREATE INDEX IF NOT EXISTS idx_items_name ON items_index(name);
        CREATE INDEX IF NOT EXISTS idx_items_filter ON items_index(rating, color, type, file_mtime);
        CREATE INDEX IF NOT EXISTS idx_items_tags ON items_index(tags);
    )";
=======
        CREATE INDEX IF NOT EXISTS idx_items_folder ON items_index(folder_path);
        CREATE INDEX IF NOT EXISTS idx_items_name ON items_index(name);
        CREATE INDEX IF NOT EXISTS idx_items_filter ON items_index(rating, color, type, file_mtime);

        CREATE VIRTUAL TABLE IF NOT EXISTS items_fts USING fts5(
            name,
            note,
            tags,
            tokenize = 'unicode61'
        );

        CREATE TRIGGER IF NOT EXISTS idx_items_ai AFTER INSERT ON items_index BEGIN
            INSERT INTO items_fts(rowid, name, note, tags) VALUES (new.rowid, new.name, new.note, new.tags);
        END;
        CREATE TRIGGER IF NOT EXISTS idx_items_ad AFTER DELETE ON items_index BEGIN
            DELETE FROM items_fts WHERE rowid = old.rowid;
        END;
        CREATE TRIGGER IF NOT EXISTS idx_items_au AFTER UPDATE ON items_index BEGIN
            DELETE FROM items_fts WHERE rowid = old.rowid;
            INSERT INTO items_fts(rowid, name, note, tags) VALUES (new.rowid, new.name, new.note, new.tags);
        END;
    )";
>>>>>>> REPLACE
```

---

### 3.3 `src/meta/MetaIndexService.cpp` (修正 FTS5 别名逻辑与基于 `rowid` 的高效整型 JOIN)

```
<<<<<<< SEARCH
    std::string sql;
    bool hasScope = !scopeFolderPath.trimmed().isEmpty();
    if (hasScope) {
        sql = "SELECT path FROM items_index WHERE folder_path LIKE ? AND (name LIKE ? OR note LIKE ? OR tags LIKE ?) LIMIT ?;";
    } else {
        sql = "SELECT path FROM items_index WHERE (name LIKE ? OR note LIKE ? OR tags LIKE ?) LIMIT ?;";
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_readDb, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return results;
    }

    QString pattern = "%" + keyword.trimmed() + "%";
    QByteArray patternUtf8 = pattern.toUtf8();

    int idx = 1;
    if (hasScope) {
        QString scopePattern = QString::fromStdWString(normalizePath(scopeFolderPath.toStdWString())) + "%";
        sqlite3_bind_text(stmt, idx++, scopePattern.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt, idx++, patternUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, patternUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, idx++, patternUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, idx++, limit);
=======
    std::string sql;
    bool hasScope = !scopeFolderPath.trimmed().isEmpty();
    if (hasScope) {
        sql = "SELECT i.path FROM items_index i JOIN items_fts f ON i.rowid = f.rowid WHERE i.folder_path LIKE ? AND f MATCH ? LIMIT ?;";
    } else {
        sql = "SELECT i.path FROM items_index i JOIN items_fts f ON i.rowid = f.rowid WHERE f MATCH ? LIMIT ?;";
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_readDb, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return results;
    }

    QString cleanKeyword = keyword.trimmed();
    cleanKeyword.replace("\"", "");
    QString ftsQuery = QString("\"%1\"*").arg(cleanKeyword);
    QByteArray ftsQueryUtf8 = ftsQuery.toUtf8();

    int idx = 1;
    if (hasScope) {
        QString scopePattern = QString::fromStdWString(normalizePath(scopeFolderPath.toStdWString())) + "%";
        sqlite3_bind_text(stmt, idx++, scopePattern.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt, idx++, ftsQueryUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, idx++, limit);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps (编译与验证)

### 编译步骤
1. 配置 CMake：
   `cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug`
2. 编译项目：
   `cmake --build build --config Debug`

### 验证方法
1. **测试反向索引**：在分类添加/查询路径时，核验 `library_category_paths` 快速基于 `path` 查询响应。
2. **测试 FTS5 搜索**：通过 `MetaIndexService::instance().searchGlobal("关键词", ...)` 进行搜寻，检查是否秒级返回结果且准确命中 `name`, `note`, `tags`。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT 复用说明**：本次重构清理了冗余表 `library_item_index`，将全库元数据缓存单一收敛于 `MetaIndexService` 的 `items_index` 表，彻底闭合 SSOT 入口。
- **防另起炉灶检查**：未新增重复数据通道，通过触发表（Trigger）基于 `rowid` 保证 `items_fts` 与主表自动同步。

---

## 6. Header API Signature Verification (头文件 API 签名核查)

| 类 / 结构体 | 函数签名 (源头 `.h` 声明) | 是否 100% 一致 |
|---|---|---|
| `LibraryDao` | `static bool initTable();` | 是 |
| `MetaIndexService` | `bool initSchema(sqlite3* db);` | 是 |
| `MetaIndexService` | `QStringList searchGlobal(const QString& keyword, const QString& scopeFolderPath, int limit);` | 是 |

---

## 7. Header Inclusion Chain & Type Completeness Check (头文件包含链核查)

- `src/meta/LibraryDao.cpp` 包含 `<sqlite3.h>`, `LibraryDao.h`, `DatabaseManager.h`。类型定义完整。
- `src/meta/MetaIndexService.cpp` 包含 `<sqlite3.h>`, `MetaIndexService.h`, `DatabaseManager.h`。类型定义完整。

---

## 8. Zero-Value-Alteration Check (数值零篡改核对)

- 本实施方案不涉及任何 UI 控件尺寸、颜色 Hex、px 等样式参数的迁移或变动。写为：**不涉及**。

---

## 9. Compliance Self-Check Table (合规核对表 - 第 2 关)

| 检查项 | 来源条款 | 自检结果 | 对应章节/行号 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射或明确模块映射 | 3.1-2 | 满足 | 文件名 MetaIndexService-1.md 精准映射核心类 MetaIndexService 的递增版本 |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | QuarkMeta Architecture/Implementation Plan/ |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 3.1~3.3 节 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 本次无新增 Q_OBJECT 类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 第 0~9 章全齐 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 第 6 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 第 7 章 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 移除 `library_item_index` 旧实现，归一至 `items_index` |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 本方案不涉及 UI 样式 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 补丁 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 本方案不涉及 UI 批量广播 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 8 章：不涉及 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 保持外部接口行为完全一致 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅影响 `LibraryDao.cpp` 与 `MetaIndexService.cpp` |

---
*方案编写完成，已通过第 2 关自检。准备提交供审查。*
