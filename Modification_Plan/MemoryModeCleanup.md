# QuarkMeta 彻底清除内存托管库模式代码无脑实施方案

## 1. 方案背景与清理目标
QuarkMeta 已独立为纯磁盘目录直连应用，原 ArcMeta 的内存托管库（SQLite 镜像数据库、分类树、绑定关联系数、托管库扫描）在纯磁盘直连模式下属于 100% 的僵尸代码与冗余开销。
本方案提供一份**按步骤、精准定位文件路径与代码行号**的无脑清理指南，指导彻底剥离内存托管库模式代码。

---

## 2. 涉及清理与重构的文件清单

| 变动类型 | 文件路径 | 清理/重构内容 |
| :--- | :--- | :--- |
| **彻底删除** | `src/meta/CategoryRepo.h / .cpp` | SQLite 分类数据库表 CRUD 与内存缓存（约 1200 行僵尸代码） |
| **彻底删除** | `src/core/CategoryLoadService.h / .cpp` | 分类数据加载与递归资产拉取服务 |
| **彻底删除** | `src/core/CategoryDropProcessor.h / .cpp` | 拖拽至分类的关联处理逻辑 |
| **精简重构** | `src/ui/MainWindow.cpp` | 清除 `isMirrorSource`、`isInsideManagedLibrary` 判断及分类胶囊渲染 |
| **精简重构** | `src/ui/ContentPanel.cpp` | 清除 `isMirrorSource()` 分支、分类拖拽绑定与内存逻辑回收站 |
| **精简重构** | `src/ui/BatchRenameDialog.cpp / .h` | 移除 `m_isMirrorSource` 变量与分支逻辑 |
| **精简重构** | `src/ui/FilterPanel.h` | 移除 `m_isMirrorSource` 隐藏限制 |
| **精简重构** | `src/core/CoreController.cpp` | 移除 `CategoryRepo::initialize()` 及数据库托管库初始化 |
| **精简重构** | `src/core/BasicCommands.h` | 移除命令集中对 `CategoryRepo` 的数据库写入调用 |
| **精简重构** | `src/meta/MetadataManager.cpp` | 移除 `isInsideManagedLibrary` 与 `CategoryRepo` 的绑盘回调 |
| **精简重构** | `CMakeLists.txt` | 移除 `CategoryRepo` / `CategoryLoadService` / `CategoryDropProcessor` |

---

## 3. 分步骤无脑清理指南

### 步骤一：彻底删除僵尸源文件
在项目中物理删除以下 6 个专门用于内存托管分类的文件：
1. `src/meta/CategoryRepo.h`
2. `src/meta/CategoryRepo.cpp`
3. `src/core/CategoryLoadService.h`
4. `src/core/CategoryLoadService.cpp`
5. `src/core/CategoryDropProcessor.h`
6. `src/core/CategoryDropProcessor.cpp`

---

### 步骤二：清理 `CMakeLists.txt` 构建配置
在 `CMakeLists.txt` 的 `SOURCES` 列表中删除以下 6 行记录：
```cmake
# 删除以下几行：
src/meta/CategoryRepo.cpp
src/meta/CategoryRepo.h
src/core/CategoryLoadService.cpp
src/core/CategoryLoadService.h
src/core/CategoryDropProcessor.cpp
src/core/CategoryDropProcessor.h
```

---

### 步骤三：清理 `MainWindow.cpp` 中的托管库逻辑
**文件**：`src/ui/MainWindow.cpp`
1. **删除头文件**：删除 `Line 36` `#include "../meta/CategoryRepo.h"`。
2. **清理选中元数据刷新逻辑（Line 438-450）**：
   - 将 `bool isDiskMode = !m_contentPanel->isMirrorSource() && !MetadataManager::isInsideManagedLibrary(...)` 直接简写为 `bool isDiskMode = true;`。
   - 删除整段 `if (!isDiskMode)` 内的 `CategoryRepo::getItemCategoryIds` 与 `setCategoryPills` 分类胶囊绘制代码。
3. **清理标签与分类解绑定（Line 788附近）**：
   - 彻底删除 `CategoryRepo::removeItemFromCategory` 信号响应槽函数。
4. **清理筛选器数据源通知（Line 1644-1646）**：
   - 删除 `MetadataManager::isInsideManagedLibrary` 调用，固定向筛选器传递 `isMirror = false`。
5. **清理盘符栏与托管库路径创建（Line 1899, 1943, 2109）**：
   - 移除 `QuarkMeta.Library_[盘符]` 默认托管库自动新建逻辑。

---

### 步骤四：清理 `ContentPanel.cpp` 中的托管库与分类绑定逻辑
**文件**：`src/ui/ContentPanel.cpp`
1. **删除头文件**：删除 `#include "../meta/CategoryRepo.h"`。
2. **重构 `isMirrorSource()` 函数（Line 2499-2506）**：
   - 将 `bool ContentPanel::isMirrorSource() const` 函数返回值直接强行返回 `false`：
     ```cpp
     bool ContentPanel::isMirrorSource() const {
         return false; // 纯磁盘模式，彻底锁定为 false
     }
     ```
3. **清理常用分类下拉列表（Line 1525-1536）**：
   - 移除 `CategoryRepo::getCachedRecentlyUsed` 与 `CategoryRepo::getCachedAll` 调用。
4. **清理右键菜单绑定分类与拖拽关联（Line 1790-1815, Line 2088）**：
   - 彻底删除右键菜单中“绑定至分类”相关 Action 及对 `CategoryRepo::addItemToCategory` 的调用。
5. **清理内存数据库回收站逻辑（Line 2139-2147）**：
   - 移除 `CategoryRepo::moveToTrashBatch` 数据库标记逻辑，磁盘模式一律直接走系统回收站（Shell Trash）。

---

### 步骤五：清理 `CoreController.cpp` 与 `BasicCommands.h`
1. **`src/core/CoreController.cpp`**：
   - Line 4：删除 `#include "../meta/CategoryRepo.h"`。
   - Line 34：删除 `QuarkMeta::CategoryRepo::initialize();` 调用。
   - Line 158-169：删除自动检测并向数据库插入 `QuarkMeta.Library_X` 根分类的代码。
2. **`src/core/BasicCommands.h`**：
   - Line 4：删除 `#include "../meta/CategoryRepo.h"`。
   - 检索并清除 `UndoManager` 命令类中对 `CategoryRepo::addItemToCategory` / `removeItemFromCategory` / `removeAllCategories` 的所有代码。

---

### 步骤五 (附加)：迁移核心数据结构与清理 Header 包含
为了彻底删除 `CategoryRepo.h` 与 `CategoryLoadService.h` 且**保证项目 100% 编译通过**，必须完成以下数据结构迁移与头文件清理：

1. **迁移 `StatisticsSnapshot` 结构体至 `StatisticsService.h`**：
   - 原 `StatisticsSnapshot` 定义在 `CategoryRepo.h` 中。物理删除 `CategoryRepo.h` 后，`StatisticsService` 与 `CategoryModel` 会报 `"systemCounts": 不是 "QuarkMeta::StatisticsSnapshot" 的成员` 错误。
   - **解决方案**：在 `src/meta/StatisticsService.h` 中显式定义 `StatisticsSnapshot`：
     ```cpp
     struct StatisticsSnapshot {
         // 1. 静态分类计数 (key -> count)
         QMap<QString, int> systemCounts;
         // 2. 半静态托管库计数 (categoryId -> count)
         QMap<int, int> libraryCounts;
         // 3. 全动态用户分类计数 (categoryId -> count)
         QMap<int, int> userCategoryCounts;
     };
     ```
   - 替换 `src/meta/StatisticsService.h` 中的 `#include "CategoryRepo.h"` 为上述结构体定义。

2. **清理 `ContentPanel.cpp` 中对 `CategoryLoadService` 的依赖**：
   - `ContentPanel.cpp` 原包含 `#include "../core/CategoryLoadService.h"` 并调用 `CategoryLoadService::loadTrashItems()` 与 `CategoryLoadService::loadPathItems()`。
   - **解决方案**：
     - 删除 `#include "../core/CategoryLoadService.h"`。
     - 在 `ContentPanel.cpp` 的 `loadPathItems` / `loadTrashItems` 中直接转接至 `DiskScanService` / 磁盘回收站管线，彻底阻断对 `CategoryLoadService` 的静态与动态依赖。

3. **清除其余 13 个源文件中对 `#include "CategoryRepo.h"` 的残留引用**：
   直接删除以下源文件中的 `#include "CategoryRepo.h"` 头文件及相关调用：
   1. **`src/main.cpp`** (Line 29)
   2. **`src/util/ShellHelper.cpp`** (Line 17)
   3. **`src/util/AssetImporter.cpp`** (Line 7)
   4. **`src/util/ImportHelper.cpp`** (Line 6)
   5. **`src/core/OperationSnapshotEngine.cpp`** (Line 3)
   6. **`src/core/CategoryLockManager.h`** (Line 7)
   7. **`src/meta/StatisticsService.h`** (Line 9)
   8. **`src/ui/TagManagerDialog.cpp`** (Line 4)
   9. **`src/ui/PresetTagsDialog.cpp`** (Line 4)
   10. **`src/ui/DiskBatchRenameService.cpp`** (Line 3)
   11. **`src/ui/models/DiskItemModel.cpp`** (Line 16)
   12. **`src/ui/BatchRenameDialog.cpp`** (Line 10)
   13. **`src/ui/CategoryPanel.cpp`** (若未完全移除时删除 Line 28)

---

### 步骤六：清理 `MetadataManager.cpp` 中的托管库判断
**文件**：`src/meta/MetadataManager.cpp`
1. **重构 `isInsideManagedLibrary`（Line 2424）**：
   - 直接修改该函数返回 `false`：
     ```cpp
     bool MetadataManager::isInsideManagedLibrary(const std::wstring& path) {
         Q_UNUSED(path);
         return false; // 纯磁盘直连应用无托管库概念
     }
     ```
2. **清理 `bindToLibraryRootCategory` 与 `updateCategoryColorByPath`（Line 565, 1392, 1557, 1776, 2038, 2096）**：
   - 移除所有对 `CategoryRepo::*` 方法的交叉调用。

---

### 步骤七：清理批量重命名与筛选面板中的 `m_isMirrorSource`
1. **`src/ui/BatchRenameDialog.h` / `.cpp`**：
   - 移除构造函数中的 `bool isMirrorSource` 参数，默认走磁盘模式批量重命名管线（`DiskBatchRenameService`）。
2. **`src/ui/FilterPanel.h`**：
   - 移除 `bool m_isMirrorSource = true;` 字段，解封全部筛选维度。

---

## 4. 实施后的预期效果与验证
1. **彻底消除僵尸代码**：直接剔除 1200+ 行 SQLite 分类表 CRUD 代码，减小二进制体积。
2. **运行性能大幅提升**：文件选中、浏览、删除与导航时，不再进行任何多余的数据库分类查询（零 Win32 数据库 Block）。
3. **编译纯洁度**：在 CMake 构建系统中无任何内存托管库冗余模块。
