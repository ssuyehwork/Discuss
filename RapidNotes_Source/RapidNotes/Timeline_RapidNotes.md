# RapidNotes 架构演进时间线与代码修改记录 (Timeline)

## 记录日期：2025-05-18

### 核心演进目标
1. 依据最新架构设计理念，彻底废除 RapidNotes 中的文件级伴随侧车 JSON（`.QuarkMeta.json` 与 `.RapidNotes.json`）存储机制，全面解绑对物理目录中 JSON 文件的读写与缓存维护，所有数据操作与元数据管理彻底移交给本地数据库 `inspiration.db`。
2. 修复从旧工程合并/重构到 RapidNotes 过程中的命名空间对齐、头文件路径错位、信号槽参数不匹配以及公共对话框组件（`FramelessMessageBox` / `BatchProgressDialog`）接口缺失等编译错误。

---

### 修改与删除文件清单

#### 1. 物理侧车代码移除（删除）
- **`src/meta/QuarkMetaJson.h`**：[已删除] 废弃原 QuarkMeta 侧车 JSON 抽象定义文件。
- **`src/meta/QuarkMetaJson.cpp`**：[已删除] 废弃原 QuarkMeta 侧车 JSON 解析与序列化实现。

#### 2. 核心与生命周期服务（修改）
- **`src/core/PermanentDeleteService.cpp`**：
  - 移除了对 `QuarkMetaJson.h` 的引用。
  - 删除了文件永久删除流程中对 `RapidNotesJson::removeItemMetadata` 的调用。
- **`src/core/DiskTrashService.cpp`**：
  - 移除了对 `RapidNotesJson.h` 的引用。
  - 删除了回收站恢复与放入时对 `RapidNotesJson::roamItemMetadata` 的调用。
- **`src/core/KeyboardHook.h`**：
  - 修复 `enterPressedInOtherApp` 信号参数声明，赋予默认值 `(bool ctrl = false, bool shift = false, bool alt = false)`，解决 `MessageCaptureHandler::onEnterPressed` 槽函数连接时的静态断言失败 (C2338/C2660/C2672)。

#### 3. 元数据与同步服务（修改）
- **`src/meta/LibrarySyncService.h`** / **`src/meta/LibrarySyncService.cpp`**：
  - 移除了对 `QuarkMetaJsonStore.h` 与 `RapidNotesJsonStore.h` 的依赖。
  - 彻底移除了 `handleJsonChangeEvent` 方法及其关联逻辑，避免在收到元数据变更时回写侧车 JSON。
- **`src/meta/MetadataManager.cpp`**：
  - 移除了对 `RapidNotesJsonStore.h` 的依赖。
  - 清除了所有 `updateItemMeta`、`addTagsBatch`、`setRatingBatch`、`setColorBatch`、`setNoteBatch`、`setURLBatch`、`setPinnedBatch`、`removeTagsBatch`、`migrateFolderCache`、`renameItem` 中向侧车 JSON 同步持久化的调用。
  - 重写了 `getFolderMeta` 与 `setFolderMeta`，解除对目录级侧车 JSON 的读取和落盘。
- **`src/meta/ItemMetaMapper.h`**：
  - 将对 `RapidNotesJson.h` 的依赖重构为对底层通用结构 `MetadataDefs.h` 的依赖。
- **`src/meta/MetaIndexService.cpp`**：
  - 移除了对 `RapidNotesJson.h` 和 `RapidNotesJsonStore.h` 的包含。
  - 清除了扫描物理文件夹时解析与合并侧车 JSON 的代码。
- **`src/meta/DiskNavigatorService.h`** / **`src/meta/DiskNavigatorService.cpp`**：
  - 移除了对 `RapidNotesJson.h` 的引用。
  - 文件夹与条目元数据获取流程全部重构为安全内存/数据库占位逻辑，杜绝解析 `.RapidNotes.json`。
- **`src/meta/MetaCacheDecorator.cpp`**：
  - 移除了对 `RapidNotesJsonStore.h` 的依赖。
  - 解除了 `folderCacheMap` 向侧车 JSON 的读写代理。

#### 4. 工具与辅助模块（修改）
- **`src/util/ShellHelper.cpp`**：
  - 移除了对 `RapidNotesJson.h` 与 `RapidNotesJsonStore.h` 的包含。
  - 移除了 `roamBatchMetadata`（批量漫游）、`flushAllDirtyBuffers`（脏缓冲写盘）以及 `migrateItemMetadata` 对侧车 JSON 的调用。
- **`src/util/DiskMediaExtractor.cpp`**：
  - 移除了对 `RapidNotesJsonStore.h` 的包含。
  - 移除了利用侧车 JSON 缓存标记解析失败缩略图状态的逻辑。
  - 移除了深度解析文件分辨率尺寸后向侧车 JSON 同步写盘的代码。

#### 5. UI 与视图模型层（修改）
- **`src/ui/LibraryPanel.cpp`**：
  - 移除了对 `RapidNotesJsonStore.h` 的无效包含。
- **`src/ui/models/DiskItemModel.h`** / **`src/ui/models/DiskItemModel.cpp`**：
  - 移除了对 `RapidNotesJson.h` 和 `RapidNotesJsonStore.h` 的头文件引用。
  - 移除了异步预加载图片尺寸后向侧车 JSON 同步刷写的逻辑。
- **`src/ui/models/ItemModelBase.h`**：
  - 纠正了 `ItemRecord.h` 头文件包含路径（从绝对虚拟路径 `src/core/ItemRecord.h` 修正为相对路径 `../../core/ItemRecord.h`），修复 C1083 错误。
- **`src/ui/ActivationDialog.cpp`** / **`src/ui/CategoryPasswordDialog.cpp`**：
  - 修正了对基类 `FramelessDialog` 成员变量的访问（将 `m_btnPin` 修正为 `m_pinBtn`），修复 C2065 错误。
- **`src/ui/ToolTipOverlay.h`**：
  - 补充全局命名空间类型别名 `using ToolTipOverlay = RapidNotes::ToolTipOverlay;`，根除跨模块未限定命名空间导致的 C2653 与 C3861 错误。
- **`src/ui/dialogs/FramelessMessageBox.h`** / **`src/ui/dialogs/FramelessMessageBox.cpp`**：
  - 补充继承自 `FramelessDialog` 的实体对话框构造重载，提供 `confirmed` 与 `cancelled` 信号，并导出全局类型别名，全面兼容 `main.cpp` 与 `QuickWindow.cpp` 中的 `dlg.exec()` 实体调用。
- **`src/ui/BatchProgressDialog.h`**：
  - 完善多参构造函数 `(title, label, min, max, parent)`，补充 `setLabelText()`、`wasCanceled()` 以及取消中断事件监听，修复 `FileStorageHelper.cpp` 的 C2039/C2661 错误。

---

## 记录日期：2025-05-19

### 核心演进目标
1. 解决 MSVC 编译报出的 C1083、C2039、C2065、C3861、C2923、C2955 错误，确保跨模块依赖链、接口和类型完全闭合。
2. 践行《Memories_RapidNotes.md》里程碑 1 和里程碑 2 规范，物理清理废弃的列视图（ColumnView）残留代码，将物理盘符栏（DriveBarWidget）切换为纯笔记体系的快捷设置扩展栏（SettingsBarWidget）。
3. 迁移项目时间线文档 `Timeline_RapidNotes.md` 到 RapidNotes 根目录下，与架构设计文档保持同级。

---

### 修改与删除文件清单与原因说明

#### 1. 文档架构规范化（移动）
- **`src/Timeline_RapidNotes.md` -> `Timeline_RapidNotes.md`**：
  - **修改原因**：根据项目架构治理规范，将时间线文档从子目录 `src/` 迁移至 RapidNotes 项目根目录，与 `Memories_RapidNotes.md`、`README.md` 等顶层真理源文档平级，便于查阅与维护。

#### 2. 对话框与头文件冗余清理（删除与修改）
- **`src/ui/SmartFolderDialog.h`**：[已删除]
  - **修改原因**：该文件为历史残留的冗余假头文件，内部引用了物理不存在的 `../core/SmartFolderRecord.h`，引发 C1083 编译错误。项目中正式的无边框智能文件夹对话框实现已规范归口于 `src/ui/dialogs/SmartFolderDialog.h` / `.cpp`。因此彻底删除该冗余文件并在 `CMakeLists.txt` 中同步注销。
- **`CMakeLists.txt`**：
  - **修改原因**：移除了已被删除的废弃头文件 `src/ui/SmartFolderDialog.h`。

#### 3. 悬浮提示与对话框基类接口契约对齐（修改）
- **`src/ui/ToolTipOverlay.h`**：
  - **修改原因**：在 `QuickPreview.h`、`QuickWindow.cpp`、`UnifiedSearchWindow.cpp` 等多处业务调用中，使用了 `ToolTipOverlay::ActivePriority` 优先级参数。但当前 `ToolTipOverlay` 缺失枚举定义与带优先级的重载，导致报出 C2039/C2065 错误。本次添加 `enum Priority { PassivePriority = 0, ActivePriority = 1 };` 及带 `Priority` 的 `showText` 重载，恢复接口兼容。
- **`src/ui/FramelessDialogBase.h` / `src/ui/FramelessDialog.cpp`**：
  - **修改原因**：`ColorPickerWindow`、`FileStorageWindow` 等派生对话框在构造时统一调用 `loadWindowSettings()` 恢复置顶状态与几何信息，但重构后的基类中漏声明了该虚方法，导致 C3861“找不到标识符”错误。本次在基类补充了 `virtual void loadWindowSettings();` 和 `virtual void saveWindowSettings();` 的声明与基于 QSettings 的标准实现。

#### 4. 列视图（ColumnView）废弃代码物理清除（修改）
- **`src/ui/ContentPanel.cpp`**：
  - **修改原因**：根据《Memories_RapidNotes.md》第 19 章里程碑 2 规范，RapidNotes 纯数据库化后界面模式收敛，仅保留“列表视图 (ListView)”和“网格视图 (GridView)”，所有列视图源码（`ColumnViewWidget`、`ColumnViewPane`）已物理删除。但在 `ContentPanel.cpp` 中仍残留了 9 处对 `ColumnView`、`m_columnView`、`ColumnViewWidget`、`ColumnViewPane` 的初始化、信号连接与分支判断，引发大量的 C2065/C2653/C2276/C2530/C3536 编译报错。本次彻底清除了所有残留的 ColumnView 逻辑分支，使内容面板与设计规范 100% 对齐。

#### 5. 任务进度状态快照类型闭合（修改）
- **`src/core/TaskProgressService.h`**：
  - **修改原因**：`StatusTextCoordinator.h` 使用了 `TextSegment`、`TaskProgressSnapshot` 结构体及 `TaskProgressService::snapshotChanged` 信号，但 `TaskProgressService.h` 中此前缺少这几个结构体定义，导致 C2923、C2955、C2065、C4430“QList: TextSegment 不是有效模板类型参数”连锁报错。本次在 `TaskProgressService.h` 中补充了 `TextSegment`（包含文本与高亮标志）、`TaskProgressSnapshot` 结构体以及 `snapshotChanged` 信号声明，彻底消除模板实例化错误。

#### 6. 盘符栏（DriveBarWidget）向设置栏（SettingsBarWidget）重构替换（修改）
- **`src/ui/MainWindow.h` / `src/ui/MainWindow.cpp`**：
  - **修改原因**：根据《Memories_RapidNotes.md》第 14.2 节与第 19 章里程碑 1 规范，QuarkMeta 原有的面向磁盘物理盘符的 `DriveBarWidget` 已被彻底剔除，并更名重构为快捷设置与扩展管理工具栏组件 `SettingsBarWidget`。原 `DriveBarWidget.h` 已不存在，导致 `MainWindow.cpp:8` 报出 C1083 错误。本次将 `MainWindow` 中的头文件包含由 `DriveBarWidget.h` 更新为 `SettingsBarWidget.h`，成员变量由 `m_driveBarWidget` 更新为 `m_settingsBarWidget`，彻底根除物理驱动器旧概念并消除编译报错。
