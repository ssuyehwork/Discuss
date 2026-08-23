# 全工程全模块物理文件逐一深度排查记录 (Interim Record-3.md)

## 一、 模块划分与排查总量统计
- **根目录 (`src/`)**: `main.cpp` (1 文件)
- **核心逻辑模块 (`src/core/`)**: 31 个文件
- **元数据与数据库模块 (`src/meta/`)**: 31 个文件
- **加密模块 (`src/crypto/`)**: 2 个文件
- **通用工具模块 (`src/util/`)**: 9 个文件
- **UI 界面与控件模块 (`src/ui/`)**: 112 个文件
- **第三方与底层组件 (`src/third_party/`)**: 17 个文件
- **总计**：203 个 C++ 源码文件（122 个 `.h` 头文件，81 个 `.cpp` 实现文件）。

---

## 二、 模块级深度排查事实汇总

### 1. `src/core/` 模块（核心中枢）
- **`ModelContract.h`**: 残留 `IdRole`、`ManagedRole`、`RegistrationProgressRole`、`IsGroupHeaderRole`、`GroupNameRole`（废弃 Role 占位）。
- **`ItemRecord.h` / `ItemRecord.cpp`**: 残留 `isManaged`、`isGroupHeader`、`groupName`（受控库与双轨残留字段）。

### 2. `src/meta/` 模块（数据库与元数据）
- **`DatabaseMigrator.h` / `.cpp`**: 定义了 `DatabaseMigrator` 与 `VolumePathResolver` 类，但在全局架构中未被实例化或调用（属于独立工具留存）。
- **`StatisticsService.h` / `.cpp`**: 存在关于旧版用户分类计数（`userCategoryCounts`）的统计逻辑残余。

### 3. `src/ui/` 模块（界面与面板）
- **`ContentPanel.h` / `.cpp`**:
  - `addItemsFromDirectory`（孤儿函数声明，无 cpp 定义）；
  - `ScanCacheEntry` / `m_recursiveCache`（幽灵缓存结构与变量，0 次读写）；
  - `m_textPreview` / `m_imagePreview` 及 `previewFile(...)`（悬空内嵌预览，已完全被 `QuickLookWindow` 替代）；
  - `m_btnLayersBlue` 按钮（旧版显示子分类残留）；
  - `FilterProxyModel::lessThan` 中的 `Library` 与 `DiskNav` 字符串组权重比较（双轨死逻辑）；
  - `ActionRestore` 中的 `!IsDiskTrashRole` 托管回收站分支（死分支）。
- **`DropJustifiedView.cpp`** / **`DropListView.cpp`** / **`NavPanel.cpp`** / **`ThumbnailDelegate.cpp`**:
  - `#include "ContentPanel.h"`（幽灵 `#include`，实现中未引用 ContentPanel）。
- **`ProgressDialog.h`**: 定义了 `ProgressDialog` 类，全系统中 0 次引用实例化。

---

## 三、 审计结论
全工程 203 个源码文件的模块级排查结果表明，项目在纯磁盘独立直连模式演进后，UI 层（`ContentPanel` / `Views`）与模型契约层（`ModelContract` / `ItemRecord`）存在最显著的僵尸 Role、幽灵 include、孤儿函数以及旧版逻辑分类/双轨回收站代码残留。
