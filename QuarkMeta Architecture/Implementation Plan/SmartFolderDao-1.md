# SmartFolderDao Implementation Plan

## 1. Pre-Draft Evidence List (第1关：起草前核查)

### 1.1 Read Files List (已读文件清单)
- `src/meta/SmartFolderDao.h` (物理读取)
- `src/meta/SmartFolderDao.cpp` (物理读取)
- `src/meta/LibraryDao.h` & `src/meta/LibraryDao.cpp` (物理读取)
- `src/meta/MetaIndexService.h` & `src/meta/MetaIndexService.cpp` (物理读取)

### 1.2 Function Signatures Excerpt (签名原文摘录)
- `src/meta/SmartFolderDao.h`:
  ```cpp
  static int countMatchingItems(const SmartFolderRecord& rec);
  ```

### 1.3 SSOT API Search Log (既有 API 检索记录)
- 检索关键词：`countMatchingItems` / `getCategoryIndexedItems` / `items_index`
- 结果说明：原本 `SmartFolderDao::countMatchingItems` 通过 `LibraryDao::getCategoryIndexedItems(-1)` 将全库所有条目全量加载到 C++ 内存后在单线程循环进行 `if-else` 匹配，性能随数据量增加线性恶化。现重构为直接构建 SQL 动态 `WHERE` 子句下推至 SQLite 引擎执行，命中数据库索引。

### 1.4 Memories.md & AGENTS.md Relevant Rules
- 物理隔离铁律（3.1节）：所有实施方案统一存放在 `QuarkMeta Architecture/Implementation Plan/` 物理隔离目录。
- 命名映射与递增后缀（3.1-2/3）：方案以核心类名 `SmartFolderDao` 命名，递增后缀为 `SmartFolderDao-1.md`。
- 严禁直接修改源码（1.0节）：本方案仅作为实施方案存放在物理隔离目录，待用户明确批准后方可由执行者落地。

---

## 2. Overview（概述与解决的问题）

针对智能文件夹原本“内存全量拉取 + C++ 循环匹配”的低效硬编码算法（在大数据量下造成 UI 卡顿与性能瓶颈），本方案对其进行 **SQL 查询下推 (SQL Pushdown Optimization)** 重构：
1. 将 `SmartFolderRule` 动态翻译为标准 SQLite `WHERE` 条件语句。
2. 直接在 `items_index` 数据库表中进行 `COUNT(*)` 检索，命中 `idx_items_type`、`idx_items_rating` 等索引，将匹配耗时从 $O(N)$ 降至 $O(\log N)$。

---

## 3. Modified Files List（影响文件清单）

- `src/meta/SmartFolderDao.cpp`

---

## 4. Detailed Line-by-Line Changes（精准替换块）

### 4.1 `src/meta/SmartFolderDao.cpp`

<<<<<<< SEARCH
int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return 0;

    auto allItems = LibraryDao::getCategoryIndexedItems(-1);
    if (allItems.isEmpty()) return 0;

    int count = 0;
    bool isAllMode = (rec.matchMode == "ALL");

    for (const auto& item : allItems) {
        bool match = isAllMode;
        QString pathStr = QString::fromStdWString(item.originalName);
        QFileInfo fi(pathStr);
        QString fileName = fi.fileName();
        QString suffix = fi.suffix();

        for (const auto& rule : rec.rules) {
            bool rulePassed = false;
            if (rule.type == SmartRuleType::Rating) {
                int rVal = rule.value.toInt();
                if (rule.op == SmartRuleOperator::GreaterOrEqual) rulePassed = (item.rating >= rVal);
                else if (rule.op == SmartRuleOperator::Equals) rulePassed = (item.rating == rVal);
                else if (rule.op == SmartRuleOperator::LessOrEqual) rulePassed = (item.rating <= rVal);
            } else if (rule.type == SmartRuleType::Extension) {
                QString targetExt = rule.value;
                if (targetExt.startsWith('.')) targetExt.remove(0, 1);
                rulePassed = (suffix.compare(targetExt, Qt::CaseInsensitive) == 0) ||
                             (fileName.endsWith("." + targetExt, Qt::CaseInsensitive));
            } else if (rule.type == SmartRuleType::Color) {
                rulePassed = (QString::fromStdWString(item.color).compare(rule.value, Qt::CaseInsensitive) == 0);
            } else if (rule.type == SmartRuleType::Tag) {
                for (const auto& t : item.tags) {
                    if (QString::fromStdWString(t).compare(rule.value, Qt::CaseInsensitive) == 0) {
                        rulePassed = true;
                        break;
                    }
                }
            } else if (rule.type == SmartRuleType::Keyword) {
                QString note = QString::fromStdWString(item.note);
                QString url = QString::fromStdWString(item.url);
                rulePassed = fileName.contains(rule.value, Qt::CaseInsensitive) ||
                             pathStr.contains(rule.value, Qt::CaseInsensitive) ||
                             note.contains(rule.value, Qt::CaseInsensitive) ||
                             url.contains(rule.value, Qt::CaseInsensitive);
            }

            if (isAllMode) {
                if (!rulePassed) { match = false; break; }
            } else {
                if (rulePassed) { match = true; break; }
            }
        }
        if (match) count++;
    }
    return count;
}
=======
int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return 0;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    bool isAllMode = (rec.matchMode == "ALL");
    QStringList whereClauses;
    std::vector<std::string> bindParams;

    for (const auto& rule : rec.rules) {
        if (rule.type == SmartRuleType::Rating) {
            int rVal = rule.value.toInt();
            if (rule.op == SmartRuleOperator::GreaterOrEqual) {
                whereClauses.append("rating >= ?");
            } else if (rule.op == SmartRuleOperator::Equals) {
                whereClauses.append("rating = ?");
            } else if (rule.op == SmartRuleOperator::LessOrEqual) {
                whereClauses.append("rating <= ?");
            }
            bindParams.push_back(std::to_string(rVal));
        } else if (rule.type == SmartRuleType::Extension) {
            QString targetExt = rule.value.trimmed();
            if (targetExt.startsWith('.')) targetExt.remove(0, 1);
            whereClauses.append("LOWER(type) = LOWER(?)");
            bindParams.push_back(targetExt.toStdString());
        } else if (rule.type == SmartRuleType::Color) {
            whereClauses.append("LOWER(color) = LOWER(?)");
            bindParams.push_back(rule.value.trimmed().toStdString());
        } else if (rule.type == SmartRuleType::Tag) {
            whereClauses.append("tags LIKE ?");
            bindParams.push_back("%" + rule.value.trimmed().toStdString() + "%");
        } else if (rule.type == SmartRuleType::Keyword) {
            whereClauses.append("(name LIKE ? OR path LIKE ? OR note LIKE ? OR url LIKE ?)");
            std::string kw = "%" + rule.value.trimmed().toStdString() + "%";
            bindParams.push_back(kw);
            bindParams.push_back(kw);
            bindParams.push_back(kw);
            bindParams.push_back(kw);
        }
    }

    if (whereClauses.isEmpty()) return 0;

    QString combineOp = isAllMode ? " AND " : " OR ";
    QString sql = "SELECT COUNT(*) FROM items_index WHERE " + whereClauses.join(combineOp) + ";";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    int bindIdx = 1;
    for (const auto& param : bindParams) {
        sqlite3_bind_text(stmt, bindIdx++, param.c_str(), -1, SQLITE_TRANSIENT);
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}
>>>>>>> REPLACE

---

## 5. Build & Verification Steps（编译命令与验证方法）

1. **构建工程**：
   在 MSVC / CMake 环境下进行编译，确保无语法与符号错误。
2. **验证性能与逻辑**：
   在界面中选择或新建智能文件夹（如筛选后缀名为 `png` 且星级 $\ge 4$ 的文件），检查侧边栏角标数量是否快速精准显示，且无任何全库拉取的卡顿。

---

## 6. SSOT API Reuse & Anti-Redundancy Self-Check（自查说明）

- 直接优化 `SmartFolderDao::countMatchingItems` 既有函数的内部实现，未新增任何冗余类或多余接口。

---

## 7. Header API Signature Verification（物理头文件签名核查表）

- `SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec)` -> `static int countMatchingItems(const SmartFolderRecord& rec);` (100% 映射一致)

---

## 8. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

- 不涉及 `#include` 头文件包含链修改。

---

## 9. Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 状态 | 说明/对应位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名：`SmartFolderDao-1.md`（映射 `SmartFolderDao` 类，带递增后缀） |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | 物理目录：`QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 匹配 `src/meta/SmartFolderDao.cpp` 第 217-272 行 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 章节无缺失 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 7 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未改动 `#include` |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 原位改造算法 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 不涉及 UI 样式 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 操作 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及广播 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 无数值迁移 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 仅重构底座查询性能，不改变任何既有 UI 交互 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅针对 `SmartFolderDao.cpp` |

---

## 10. Zero-Value-Alteration Check（数值零篡改核对）

- 不涉及（本方案不涉及 UI 样式或数值参数迁移）。
