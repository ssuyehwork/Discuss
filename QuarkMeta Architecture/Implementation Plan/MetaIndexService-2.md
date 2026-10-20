# MetaIndexService Implementation Plan

## 1. Pre-Draft Evidence List (第1关：起草前核查)

### 1.1 Read Files List (已读文件清单)
- `src/meta/MetaIndexService.h` (物理读取)
- `src/meta/MetaIndexService.cpp` (物理读取)

### 1.2 Function Signatures Excerpt (签名原文摘录)
- `src/meta/MetaIndexService.h`:
  ```cpp
  static bool initSchema(sqlite3* db);
  ```

### 1.3 SSOT API Search Log (既有 API 检索记录)
- 检索关键词：`idx_items_type` / `CREATE INDEX`
- 结果说明：`MetaIndexService.cpp` 中初始化 `items_index` 表结构，原索引包括 `idx_items_folder`, `idx_items_name`, `idx_items_filter`，缺失独立的 `type` 单列索引。

### 1.4 Memories.md & AGENTS.md Relevant Rules
- 物理隔离铁律（3.1节）：所有实施方案统一存放在 `QuarkMeta Architecture/Implementation Plan/` 物理隔离目录。
- 只读不可二次修改（3.1-3）：由于 `MetaIndexService.md` 和 `MetaIndexService-1.md` 已存在并提交，按规定新建递增后缀文件 `MetaIndexService-2.md`。
- 严禁直接改动源码（1.0节）：本方案仅作为实施方案存在，待用户批准后方可执行。

---

## 2. Overview（概述与解决的问题）

在 `items_index` 数据集中，文件扩展名/后缀名保存在 `type` 字段中。用户确认需要为 `type` 字段单独创建单列独立索引，以解决按文件格式/类型筛选卡片时由于单列索引缺失导致的全表扫描（Full Table Scan）性能隐患。

本方案在 `MetaIndexService::initSchema` 中为 `items_index(type)` 增加独立单列索引 `idx_items_type`。

---

## 3. Modified Files List（影响文件清单）

- `src/meta/MetaIndexService.cpp`

---

## 4. Detailed Line-by-Line Changes（精准替换块）

### 4.1 `src/meta/MetaIndexService.cpp`

<<<<<<< SEARCH
        CREATE INDEX IF NOT EXISTS idx_items_folder ON items_index(folder_path);
        CREATE INDEX IF NOT EXISTS idx_items_name ON items_index(name);
        CREATE INDEX IF NOT EXISTS idx_items_filter ON items_index(rating, color, type, file_mtime);
=======
        CREATE INDEX IF NOT EXISTS idx_items_folder ON items_index(folder_path);
        CREATE INDEX IF NOT EXISTS idx_items_name ON items_index(name);
        CREATE INDEX IF NOT EXISTS idx_items_type ON items_index(type);
        CREATE INDEX IF NOT EXISTS idx_items_filter ON items_index(rating, color, type, file_mtime);
>>>>>>> REPLACE

---

## 5. Build & Verification Steps（编译命令与验证方法）

1. **构建验证**：
   在 MSVC / CMake 环境下进行编译，确保无语法错误。
2. **SQLite 查询计划验证**：
   使用 SQLite 客户端连接数据库，运行：
   `EXPLAIN QUERY PLAN SELECT * FROM items_index WHERE type = 'png';`
   确认输出包含：`USING INDEX idx_items_type (type=?)`，验证按后缀名检索已成功命中单列索引。

---

## 6. SSOT API Reuse & Anti-Redundancy Self-Check（自查说明）

- 仅在 `initSchema` DDL 中补充单列索引定义，未新增任何冗余 C++ 函数。

---

## 7. Header API Signature Verification（物理头文件签名核查表）

- `MetaIndexService::initSchema(sqlite3* db)` -> `static bool initSchema(sqlite3* db);` (100% 映射一致)

---

## 8. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

- 不涉及头文件包含链修改。

---

## 9. Compliance Self-Check Table（第 2 关：交付前自检）

| 检查项 | 来源条款 | 状态 | 说明/对应位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名：`MetaIndexService-2.md`（按 3.1-3 递增后缀命名） |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | 物理目录：`QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 与 `src/meta/MetaIndexService.cpp` 136-139 行匹配 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 未新建类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 章节无缺失 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 见第 7 章 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 不涉及 | 未改动 `#include` |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 仅优化 DDL |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 不涉及 UI 样式 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 操作 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不涉及广播 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 无数值迁移 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 严格响应用户明确指令 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅针对 `MetaIndexService.cpp` |

---

## 10. Zero-Value-Alteration Check（数值零篡改核对）

- 不涉及（本方案不涉及 UI 样式或数值参数迁移）。
