# MetaIndexService-1.md - 数据库派生索引表与复合 SQL 索引架构实施方案

## 【第 1 关：起草前核查（Pre-Draft Evidence）】

### 1. 已读文件清单
- `src/meta/MetaIndexService.h` (已用 read_file 物理读取)
- `src/meta/MetaIndexService.cpp` (已用 read_file 物理读取)
- `src/meta/SmartFolderDao.h` (已用 read_file 物理读取)

### 2. 签名原文摘录
- `MetaIndexService.h`:
  ```cpp
  static MetaIndexService& instance();
  void init();
  bool initSchema(sqlite3* db);
  QStringList searchGlobal(const QString& keyword, const QString& scopeFolderPath = "", int limit = 1000);
  ```

### 3. 既有 API 检索记录
- 检索关键词：`initSchema`, `items_index`, `CREATE INDEX`
- 检索结果：
  - `MetaIndexService::initSchema` 中目前仅创建了基础的单列索引 `idx_items_folder` (folder_path) 和 `idx_items_name` (name)。
  - 缺乏针对星级（`rating`）、颜色标记（`color`）、文件类型（`type`）、修改时间（`file_mtime`）的复合 SQL 索引（B-Tree Index），导致在海量数据下通过数据库进行智能文件夹匹配与复杂条件筛选时，数据库只能进行全表扫描（Full Table Scan）。

### 4. Memories.md 与 AGENTS.md 相关条款
- **五道工程硬锁**：保持 `.h` 接口与既有契约 100% 兼容。
- **真理源 (SSOT)**：元数据权威源仍然是本地隐藏侧车 JSON，数据库专职负责毫秒级派生 SQL 索引过滤。
- **只读不可二次修改铁律**：按 3.1 规则创建递增后缀方案文件 `MetaIndexService-1.md`。

---

## 1. Overview（概述与解决的问题）

### 解决的问题
当素材库拥有十万/百万级文件时，筛选器条件过滤与智能文件夹（Smart Folder）若在 C++ 内存中逐行匹配或在数据库中缺乏索引，会导致极高的内存消耗与 UI 线程卡顿。

### 核心重构与设计方案
1. **在派生数据库 (`index.db`) 建立高频多维复合 SQL 索引**：
   在 `MetaIndexService::initSchema` 中添加针对 `rating`、`color`、`type`、`file_mtime` 及标签的复合 B-Tree 索引（`CREATE INDEX IF NOT EXISTS`）。
2. **支持毫秒级条件下沉**：
   支持 SQLite 引擎利用 B-Tree 索引在毫秒级内完成 `WHERE rating >= ? AND color = ? AND type = ?` 的过滤，彻底避免全表扫描与内存逐行遍历。

---

## 2. Modified Files List（影响文件清单）

1. `src/meta/MetaIndexService.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 修改 `src/meta/MetaIndexService.cpp`
在 `MetaIndexService::initSchema` 中增加复合 SQL 索引定义：

```
<<<<<<< SEARCH
        CREATE INDEX IF NOT EXISTS idx_items_folder ON items_index(folder_path);
        CREATE INDEX IF NOT EXISTS idx_items_name ON items_index(name);
=======
        CREATE INDEX IF NOT EXISTS idx_items_folder ON items_index(folder_path);
        CREATE INDEX IF NOT EXISTS idx_items_name ON items_index(name);
        CREATE INDEX IF NOT EXISTS idx_items_filter ON items_index(rating, color, type, file_mtime);
        CREATE INDEX IF NOT EXISTS idx_items_tags ON items_index(tags);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译核查**：
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --config Debug
   ```
2. **数据库索引验证**：
   - 启动 QuarkMeta；
   - 查看生成的 `index.db`，验证 `idx_items_filter` 和 `idx_items_tags` 成功创建；
   - 触发海量素材筛选与智能文件夹检索，确认 SQL 查询走 B-Tree 索引，毫秒级返回结果。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 入口复用自查）

- 本次修改是否复用了既有 SSOT 官方 API：是，复用了 `MetaIndexService::initSchema` 标准 Schema 构建通道。
- 是否存在私自另起炉灶行为：否，完全基于既有的 `items_index` 派生索引表增加辅助索引。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用函数 | 声明所在头文件 | 头文件中的精准原样签名 | 核查状态 |
|---|---|---|---|
| `initSchema` | `src/meta/MetaIndexService.h` | `bool initSchema(sqlite3* db);` | 100% 物理原生匹配 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

| 修改/引用的文件 | 修改/增加的 `#include` 行 | 核查理由与完整类型保障 |
|---|---|---|
| `src/meta/MetaIndexService.cpp` | 无 | 仅在既有 SQL Schema 文本块中扩展 `CREATE INDEX` 语句 |

---

## 8. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 核对结论 (满足/不满足/不涉及) | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 核心修改类为 `MetaIndexService`，映射文件名 `MetaIndexService-1.md` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含足够上下文 | 3.2 | 满足 | 章节 3.1 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册，且有配套 .cpp | 3.3 | 不涉及 | 本次不新建类 |
| 第 1~9 章齐全 | 3.4 | 满足 | 章节 1~9 完整覆盖 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 章节 6 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 章节 7 |
| 已复用既有 SSOT 入口，无另起炉灶；旧分散实现已列入删除 | 2.4 | 满足 | 章节 5 |
| 无内联 `setStyleSheet`，样式走 style.qss | 4.1 | 不涉及 | 不修改样式 |
| 无平台级 Hack、无无范围全局事件过滤器 | 4.2 | 满足 | 无 Hack 行为 |
| 批量操作为一次聚合广播 | 4.3 | 不涉及 | 不包含批量数据变动 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 无数值迁移 |
| 未改动用户既有设计理念与交互方式（无脑补） | 1.2 / Memories.md | 满足 | 完全符合与用户达成的共识逻辑 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅改动 `MetaIndexService.cpp` |

---

## 9. Zero-Value-Alteration Check（数值零篡改核对）

- 本方案不涉及任何 UI 尺寸、颜色、Hex、px 或边距参数更改（填写：**不涉及**）。
