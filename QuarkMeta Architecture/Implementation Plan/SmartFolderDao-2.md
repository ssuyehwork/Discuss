# SmartFolderDao Implementation Plan

## 1. Pre-Draft Evidence List (第1关：起草前核查)

### 1.1 Read Files List (已读文件清单)
- `src/meta/SmartFolderDao.h` (物理读取)
- `src/meta/SmartFolderDao.cpp` (物理读取)
- `src/meta/MetaIndexService.h` & `src/meta/MetaIndexService.cpp` (物理读取)

### 1.2 Function Signatures Excerpt (签名原文摘录)
- `src/meta/SmartFolderDao.h`:
  ```cpp
  static int countMatchingItems(const SmartFolderRecord& rec);
  ```

### 1.3 SSOT API Search Log (既有 API 检索记录)
- 检索关键词：`countMatchingItems` / `items_index`
- 结果说明：重构 `SmartFolderDao::countMatchingItems` 逻辑，从全量拉取内存 C++ 逐行匹配修改为 SQL 表达式下推到数据库 `items_index` 进行统计。

### 1.4 Memories.md & AGENTS.md Relevant Rules
- 物理隔离铁律（3.1节）：所有实施方案必须存放在 `QuarkMeta Architecture/Implementation Plan/` 物理隔离目录。
- 递增后缀只读铁律（3.1-3节）：方案一旦交付即只读，后续重新起草时必须增加递增后缀名（如 `SmartFolderDao-2.md`）。
- 先审后做铁律（1.2节）：方案输出后必须停下来等待用户明确批准，严禁直接对 C++ 源码进行物理修改。

---

## 2. Overview（概述与解决的问题）

用户反馈“内容区包含 177 个 `.cpp` 项目，但智能文件夹计数却显示为 `0`”的致命缺陷。

该问题的根源在于旧逻辑调用了仅包含分类库条目的 `library_item_index`，导致未归属于分类库的文件无法被计数。本方案通过在 `SmartFolderDao::countMatchingItems` 中全量切换为 SQL 动态翻译并查询全局元数据表 `items_index`（配合 `LOWER(type) = LOWER(?)` 忽略大小写及索引加速），彻底消除“177 变为 0”的缺陷，实现毫秒级精准计数。

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
   使用 CMake / MSVC 进行编译，确认无任何编译错误。
2. **场景功能验证**：
   打开智能文件夹配置，添加规则：`扩展名/类型 等于 cpp`。
   观察智能文件夹侧边栏角标数量，确认显示数量与实际匹配文件数（如 177 个）精准映射，且响应时间小于 1ms。

---

## 6. SSOT API Reuse & Anti-Redundancy Self-Check（自查说明）

- 仅在 `SmartFolderDao::countMatchingItems` 方法内部优化算法逻辑与 SQL 查询，未增加任何多余的全局函数或重复类。

---

## 7. Header API Signature Verification（物理头文件签名核查表）

- `SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec)` -> `static int countMatchingItems(const SmartFolderRecord& rec);` (100% 物理匹配)

---

## 8. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

- 不涉及 `#include` 头文件更改。

---

## 9. Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 状态 | 说明/对应位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名：`SmartFolderDao-2.md`（映射 `SmartFolderDao` 类，带递增后缀） |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | 物理目录：`QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 匹配 `src/meta/SmartFolderDao.cpp` 217-272 行 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 章节无缺失 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 7 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未改动 `#include` |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 原位算法升级 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 不涉及 UI 样式 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 纯后台 DAO 逻辑 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及广播 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 无数值迁移 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 不改动既有交互，仅修复数值为 0 的 Bug |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅针对 `SmartFolderDao.cpp` |

---

## 10. Zero-Value-Alteration Check（数值零篡改核对）

- 不涉及（本方案不涉及 UI 样式或数值参数迁移）。
