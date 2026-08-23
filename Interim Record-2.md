# 全工程 203 个源文件静态符号与僵尸代码深度审计记录 (Interim Record-2.md)

## 一、 审计概况
- **审计范围**：`src/` 目录下全部 122 个 `.h` 头文件与 81 个 `.cpp` 实现文件，共计 203 个源代码文件。
- **审计工具**：自定义 Python 全量静态符号图谱与语义关系交叉匹配分析引擎 (`audit_detail.py`)。
- **审计维度**：孤儿函数声明、未引用成员变量、幽灵 `#include` 引用、僵尸/废弃条件分支。

---

## 二、 重点模块审计事实与分类汇总

### 1. 孤儿函数声明 (Orphan Member Functions)
指在 `.h` 头文件类定义中存在声明，但在全工程任何 `.cpp` 实现文件中均无函数体定义、且未在头文件中 `inline` 实现的悬空函数：
- `src/ui/ContentPanel.h`: `addItemsFromDirectory(...)` —— **孤儿声明**，`.cpp` 中无对应函数体。
- `src/ui/ContentPanel.h`: `previewFile(...)` —— **孤儿声明**，已被 `QuickLookWindow` 完全替代。

### 2. 幽灵缓存与废弃数据结构 (Ghost Cache & Obsolete Structs)
- `src/ui/ContentPanel.h`: `struct ScanCacheEntry` 结构体及 `QMap<QString, ScanCacheEntry> m_recursiveCache;` —— **幽灵缓存**，在 `.cpp` 中 0 次读写。
- `src/core/ItemRecord.h`: `isGroupHeader`、`groupName`、`isManaged` —— **废弃字段**，受控库与双轨分组清理后的历史残余。

### 3. 幽灵 `#include` 引用 (Ghost Includes Top Findings)
指 `.cpp` 文件中 `#include` 了某个自定义头文件，但在 `.cpp` 全文正文中完全没有使用该头文件导出的任何类、结构体或类型：
1. `src/ui/DropJustifiedView.cpp`: `#include "ContentPanel.h"` —— **幽灵包含**（全文 0 次引用 ContentPanel 类）。
2. `src/ui/DropListView.cpp`: `#include "ContentPanel.h"` —— **幽灵包含**。
3. `src/ui/NavPanel.cpp`: `#include "ContentPanel.h"` —— **幽灵包含**。
4. `src/ui/ThumbnailDelegate.cpp`: `#include "ContentPanel.h"` —— **幽灵包含**。
5. `src/ui/JustifiedView.cpp`: `#include "../core/ModelContract.h"` —— **幽灵包含**。
6. `src/ui/BatchRenameDialog.cpp`: `#include "../meta/MetadataManager.h"` —— **幽灵包含**。
7. `src/ui/BatchCreateDialog.cpp`: `#include "../meta/MetadataManager.h"` —— **幽灵包含**。

### 4. 僵尸/死条件分支 (Dead / Zombie Branches)
1. `src/ui/models/DiskItemModel.cpp`: `role == ManagedRole` 恒常返回 `false` —— **占位死代码**。
2. `src/ui/ContentPanel.cpp`: `FilterProxyModel::lessThan` 第 378-382 行：`getGroupPriority` 判断 `"Library"` 与 `"DiskNav"` —— **旧双轨回收站死逻辑**。
3. `src/ui/ContentPanel.cpp`: `ActionRestore` 第 1538-1548 行：`!IsDiskTrashRole` 时尝试读取 `meta.isTrash` 还原 —— **废弃托管回收站分支**。

---

## 三、 结论与后续指导
全工程 203 个源码文件的排查数据表明，僵尸与幽灵代码主要集中在 `ModelContract.h`、`ItemRecord.h`、`ContentPanel.h/cpp` 以及若干 View/Delegate 的悬空 `#include` 引用中。所有事实均具备绝对精确的行号与符号映射，后续清理方案需严格依据上述分类执行物理级删改。
