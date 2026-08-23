# QuarkMeta 架构现状全景蓝图 (Blueprint Architecture.md)

> **重要说明与阅读规范**：
> 本文档为 QuarkMeta 项目**唯一权威的架构现状记录**。本文档的每一条结论、运行逻辑、线程上下文与代码状态均经过项目实际源码（`src/` 目录下除 `third_party` 外的全部源文件与头文件）的全局交叉检索与精准验证，并附有明确的代码行号与分支判定依据。
> **任何开发者或 AI 助手在修改项目代码或规划重构方案前，必须优先阅读本文档，严禁凭猜测、看名字联想或依据过时注释判断架构现状！**

---

## 一、数据持久化架构 (Data Persistence Architecture)

### 1.1 磁盘离散元数据与 SSOT（唯一事实源）
- **存储载体与位置**：在纯磁盘直连模式下，每一个物理文件夹或文件的星级（`rating`）、颜色标记（`color`）、标签列表（`tags`）、置顶状态（`pinned`）、物理尺寸（`width`/`height`）、备注（`note`）、关联网址（`url`）、缩略图提取状态（`thumb_status`）、旋转角度（`rotation`）以及别名/扩展元数据，**完全且唯一**存储于该物理文件所在目录下的离散 `.QuarkMeta.json` JSON 文件中。
  - **依据**：`src/meta/QuarkMetaJson.cpp`（第 16-140 行），`QuarkMetaJson::readFolderMeta` 与 `QuarkMetaJson::updateItemMeta` 直接读取和写入各级目录下的 `.QuarkMeta.json`；`src/meta/MetadataManager.cpp`（第 830, 1007, 1032, 1057, 1082, 1107 行），对单个文件的元数据更新全部调用 `QuarkMetaJson::updateItemMeta`。
- **内存元数据缓存（Source of Truth 与 Cache 边界）**：
  - `MetadataManager` 内部维护了 `m_folderCache`（即 `std::unordered_map<std::wstring, FolderCacheEntry>`），仅作为 UI 浏览与筛选时的**运行时内存缓存（Runtime Cache）**。
  - **数据流与 SSOT 铁律**：磁盘上的 `.QuarkMeta.json` 文件是绝对的**数据落盘唯一事实源（SSOT）**。当切换或读取目录时，`MetadataManager::loadFolderCacheFromDisk` 调用 `QuarkMetaJson::readFolderMeta` 从 `.QuarkMeta.json` 加载元数据填充内存缓存；当 UI 修改属性时，首先写入磁盘 `.QuarkMeta.json`，随后同步更新 `m_folderCache` 内存缓存并通知视图。
  - **依据**：`src/meta/MetadataManager.cpp`（第 2008-2015 行：`Pure disk mode: bypass writing non-root item metadata to global.db's metadata table`）；`src/meta/MetadataManager.h`（第 278-310 行）。

### 1.2 全局 SQLite 数据库 (`global.db`) 的职责限定
- **存储路径**：AppData/Local/QuarkMeta/meta/global.db（Win）或对应的系统 AppData 路径。
  - **依据**：`src/meta/DatabaseManager.cpp`（第 241 行）。
- **`global.db` 数据表清单与权责划界**：
  1. `tag_groups`：全局标签与标签组字典表，管理系统中所有可用的标签名称、颜色及所属分类组。
     - **依据**：`src/meta/TagRepository.cpp`（第 52-195 行）。
  2. `disk_trash`：物理回收站隔离盒映射表，存储被删项目的 UUID `file_id`、原始文件路径 `original_path`、隔离盒相对路径 `trash_path`、原始创建时间戳 `created_at`（用于还原碰撞权威判定）及删除时间戳 `deleted_at`。
     - **依据**：`src/meta/DiskTrashRepo.cpp`（第 15-110 行）；`src/core/DiskTrashService.cpp`（第 35-180 行）。
  3. `drive_metadata`：根目录与盘符元数据表，专门存储盘符根目录（如 `C:/`、`D:/`）自身的体积、卷标等磁盘级元数据（盘符根目录下无法或不便创建 `.QuarkMeta.json` 时备用）。
     - **依据**：`src/meta/DriveMetaDao.cpp`（第 10-85 行）。
  4. `search_history`：全局搜索历史关键词记录表。
     - **依据**：`src/core/SearchHistoryService.cpp`（第 20-95 行）。
  5. `navigation_history`：全局目录导航历史记录表。
     - **依据**：`src/core/NavigationHistoryService.cpp`（第 15-80 行）。
  6. `system_stats`：仅保留 `key = 'tag_migration_completed'` 单条系统状态标记，用于记录标签迁移完成状态。
     - **依据**：`src/meta/TagRepository.cpp`（第 169, 195 行）。

### 1.3 历史“全面转向 SQLite 存储”架构决策的废除与当前现状
- **历史决策分析**：项目早期曾尝试将全盘所有文件的元数据集中写入 `global.db` 的 `metadata` 数据表及 `metadata_fts` 全文索引表。
- **当前现状**：该决策在纯磁盘模式重构中被彻底废除。`global.db` 中的 `metadata` 数据表在新建库中已不再写入任何普通文件的元数据。任何尝试向 `global.db` 写入非根目录文件元数据的代码已被清空或重定向至 `.QuarkMeta.json`。
  - **依据**：`src/meta/MetadataManager.cpp`（第 2008 行注释与函数体逻辑）；`src/meta/DatabaseManager.cpp`（第 245-290 行，建表逻辑中仅保留核心系统表）。

---

## 二、双轨历史残留清查 (Dual-Track Historical Residual Inventory)

QuarkMeta 成功从 ArcMeta 双轨机制（托管库模式 + 磁盘直连模式）剥离为纯磁盘直连独立应用。以下为代码库中经过全局引用检索核对的残留痕迹分类：

### 2.1 纯命名残留（名实不符：逻辑已纯净，仅函数名/变量名未改）
1. **`DiskMediaExtractor::getCapsuleThumbnail` / `getCapsuleThumbnailReadOnly`**
   - **名实分析**：名字中带有历史的 `Capsule`（胶囊）字样，但其实际实现代码已经完全重构为：调用 `QImageReader`、`WindowsShellThumbnailProvider`、`FormatDecoders`（Ghostscript/TIFF/WebP）直接提取普通物理磁盘文件的 512px / 2048px 缩略图。与胶囊解包或解压无关。
   - **依据**：`src/util/DiskMediaExtractor.cpp`（第 73-180 行）；`src/meta/MediaExtractorPipeline.cpp`（第 244 行）；`src/ui/models/DiskItemModel.cpp`（第 398 行）。
2. **`ModelContract::ManagedRole` 与 `ItemRecord::isManaged`**
   - **名实分析**：在双轨时代，`ManagedRole` 意为“项目是否已被导入托管库数据库”。在当前磁盘模式中，`ItemRecord.cpp` 第 24 行将其计算逻辑改为 `r.isManaged = meta.hasUserOperations();`（即项目是否有用户设置的评级、标签、颜色等操作）。UI 渲染层（如 `ThumbnailDelegate.cpp` 第 141 行与 `CardPainterHelper.cpp` 第 114 行）借用此标记判断是否需要绘制 UI 状态指示图标（角标）。
   - **依据**：`src/core/ItemRecord.cpp`（第 24 行）；`src/ui/ThumbnailDelegate.cpp`（第 141-145 行）；`src/ui/CardPainterHelper.cpp`（第 94-114 行）。

### 2.2 真实存在但已确认不可达的死分支 (Dead Code Branches)
1. **`BatchRenameCommand` 中的 `m_isCapsule` 分支**
   - **不可达依据**：`src/core/BasicCommands.h`（第 222-397 行）保留了 `if (isCapsule)` 的重命名逻辑分支。但全系统中调用 `BatchRenameCommand` 的唯一入口 `src/ui/BatchRenameDialog.cpp` 第 338 行明确写死 `bool isCapsule = false;`，且 `BatchRenameDialog` 界面完全没有胶囊模式复选框。该 `if (isCapsule)` 分支物理不可达。
   - **依据**：`src/core/BasicCommands.h`（第 245, 316, 376 行）；`src/ui/BatchRenameDialog.cpp`（第 338, 380 行）。

### 2.3 涉及历史数据兼容/纯只读预览的保留代码
1. **`.arc` 胶囊文件夹磁盘纯只读直通预览分支**
   - **保留依据**：`src/meta/MetadataManager.cpp`（第 521 行）保留了对历史 `.arc` 胶囊文件夹的只读预览判断。当用户在磁盘中双击点击历史留存的 `.arc` 文件夹时，系统视其为普通目录直通浏览其内部主体文件，不强行解包也不写盘。符合架构规范第 6 章规定，暂时保留。
   - **依据**：`src/meta/MetadataManager.cpp`（第 521 行）。
2. **`system_stats` 数据表中的 `tag_migration_completed` 标记检测**
   - **保留依据**：`src/meta/TagRepository.cpp`（第 169, 195 行）保留了从老版本数据库升级时执行标签迁移的单次标记查询与写入。此逻辑用于保证历史数据库升级时的平滑性，目前属于兼容性保留。
   - **依据**：`src/meta/TagRepository.cpp`（第 169, 195 行）。

---

## 三、模块职责边界图 (Module Responsibility Boundaries Map)

### 3.1 核心模块职责映射矩阵

| 模块层级 | 类名 / 模块名 | 核心单一职责 | 严禁越界行为 |
| :--- | :--- | :--- | :--- |
| **中央调度** | `CoreController` | 全局生命周期、线程池调度、全局搜索触发 | 严禁直接进行 UI 控件重绘或直接读写磁盘 JSON |
| **调度中枢** | `CoreEngine` | 业务指令编排、Command 提交处理 | 严禁持有 UI 视图句柄 |
| **事件总线** | `CentralEventHub` | 纯增量事件订阅与通知分发 (`emit`) | 严禁包含任何业务计算逻辑或数据缓存 |
| **持久化层** | `QuarkMetaJson` | `.QuarkMeta.json` 离散文件的读写与原子迁移 | 严禁访问 `global.db` SQLite 句柄 |
| **元数据中枢**| `MetadataManager` | 管理 `m_folderCache` 内存缓存，调度 `QuarkMetaJson` | 严禁直接操作 UI 界面控件 |
| **数据库服务**| `DatabaseManager` | `global.db` 的 SQLite 线程安全连接池与事务管理 | 严禁写入普通文件属性元数据 |
| **视图模型** | `DiskItemModel` | 为 UI 视图提供 `QAbstractTableModel` 接口数据 | 严禁同步阻塞主线程做耗时 I/O 提取 |
| **渲染层** | `ThumbnailDelegate` / `CardPainterHelper` | 绘制缩略图网格卡片、文件图标与状态标记 | 严禁在 `paint()` 函数内部发起磁盘 I/O 读写 |

### 3.2 已确认的重复实现或职责越界排查
1. **缩略图提取入口重叠（已厘清）**：
   - `DiskMediaExtractor` 负责同步/轻量级文件头与缩略图解码（底层解码器）；
   - `MediaExtractorPipeline` 负责后台多线程任务队列排队与并发控制；
   - `DiskItemModel` 拥有专用独立线程池 `thumbnailPool()` (MaxThreads=2, LowestPriority) 调度 `MediaExtractorPipeline` 与 `FormatDecoders`。
   - **结论**：三者分层明确，`DiskMediaExtractor` 为解码基础类，`MediaExtractorPipeline` 为中枢调度，`DiskItemModel` 为 UI 模型管道，无非法越界。

### 3.3 已经清理干净的正确标准样板
- **样板一：撤销/重构指令体系 (`ActionCommand`)**
  - `MoveCommand` / `RenameCommand` / `MetadataCommand` / `SecureDeleteCommand` 严格继承 `ActionCommand` 接口，物理操作与 `.QuarkMeta.json` 键值变更完全绑定原子化，完美作为 Action 扩展样板。
- **样板二：离散元数据原子更新 (`QuarkMetaJson`)**
  - 静态函数 `QuarkMetaJson::updateItemMeta` 采用 Lambda 闭包传参，自动处理“加载 JSON -> 内存修改 -> 序列化落盘”的全流程，原子且安全。

---

## 四、当前正在推进但尚未完成的架构变更 (Ongoing Architecture Changes)

### 4.1 筛选面板 UI 纯净化与复合控件彻底清退（进行中，完成度 95%）
- **状态描述**：架构规范第 14 章要求彻底清退筛选面板（`FilterPanel`）中的色相滑块、容差滑块、面积占比滑块及 12 色矩阵等复合控件，统一为标准纵向复选框列表。
- **验证证据**：`src/ui/FilterPanel.cpp` 已删除复杂 Slider 实例，恢复标准 `StyledCheckBox` 分组列表，但部分历史信号连接及样式表变量仍有轻微清理残留。

### 4.2 标签管理视图（`TagManagerView`）废弃与对话框（`TagManagerDialog`）全面接管（完成度 100%）
- **状态描述**：`TagManagerView` 已完全物理剔除，全局标签字典维护统一由 `TagManagerDialog` 与 `TagSelectorOverlay` 搭配完成。
- **验证证据**：`src/ui/TagManagerDialog.cpp`（第 1-300 行）；`src/ui/TagSelectorOverlay.cpp`（第 1-250 行）。

---

## 五、已知的技术债与未决问题 (Known Technical Debt & Unresolved Issues)

1. **`getCapsuleThumbnail` 系列方法的统一重命名**：
   - **技术债描述**：`DiskMediaExtractor` 中的 `getCapsuleThumbnail`、`getCapsuleThumbnailReadOnly` 等静态函数名字带有 `Capsule` 历史遗产，但功能已完全用于磁盘模式缩略图提取，极易引发新开发者或 AI 的混淆误判。
   - **未决卡点**：涉及 `MediaExtractorPipeline`、`DiskItemModel`、`QuickLookWindow`、`DuplicateDetectorService` 等十余个文件的调用点，需安排一次原子重命名。
2. **`BasicCommands.h` 中 `BatchRenameCommand` 僵尸参数 `isCapsule` 的清理**：
   - **技术债描述**：`BatchRenameCommand` 构造函数仍保留 `bool isCapsule` 参数与多处 `if (isCapsule)` 判定。
   - **未决卡点**：需移除构造函数参数，清理内部 dead code 判定。

---

## 六、逐文件说明：用途 / 运行逻辑 / 时序与完整启动序列 (Per-File Specification & Startup Timeline)

### 6.0 系统的完整启动序列时间线 (Startup Timeline)

从 `src/main.cpp` 的 `main()` 函数开始，系统启动序列按时间线推进如下：

```
[时间线 T0: main() 入口]
  │
  ├── 1. 单实例互斥量检测 (CreateMutexA / QLockFile) ── (主线程 / 同步阻塞)
  │      └── 依据: src/main.cpp (第 62-75 行)。若已有实例运行，立刻 return 0 退出。
  │
  ├── 2. 日志系统初始化 (qInstallMessageHandler) ── (主线程 / 同步阻塞)
  │      └── 依据: src/main.cpp (第 79 行)。安装 customMessageHandler，将日志重定向至 Logger RingBuffer。
  │
  ├── 3. QApplication 实例化与高 DPI 设置 ── (主线程 / 同步阻塞)
  │      └── 依据: src/main.cpp (第 82-84 行)。设置 HighDpiScaleFactorRoundingPolicy 及全局 QPalette 蓝色框选样式。
  │
  ├── 4. COM 组件环境初始化 (CoInitializeEx) ── (主线程 / Win平台同步)
  │      └── 依据: src/main.cpp (第 106 行)。初始化 COINIT_APARTMENTTHREADED 线程模型，为 Windows Shell 缩略图提取做准备。
  │
  ├── 5. 核心组件拓扑预热 (CoreController::initializeCoreComponents) ── (主线程 / 同步阻塞)
  │      ├── 步骤 5.1: DatabaseManager::instance() ── 初始化 global.db SQLite 数据库句柄及建表 (src/core/CoreController.cpp 第 25 行)。
  │      ├── 步骤 5.2: MetadataManager::instance() ── 预热元数据缓存结构 (src/core/CoreController.cpp 第 28 行)。
  │      └── 步骤 5.3: MediaExtractorPipeline::instance() ── 初始化后台提图线程池与 Ghostscript 信号 (src/core/CoreController.cpp 第 31 行)。
  │
  ├── 6. 实例化主窗口 (MainWindow w) ── (主线程 / 栈上局部作用域 RAII)
  │      └── 依据: src/main.cpp (第 116 行)。构建五栏式 UI 面板布局 (NavPanel, FavoritePanel, ContentPanel, MetaPanel, FilterPanel)。
  │
  ├── 7. 启动系统后台扫描 (CoreController::instance().startSystem()) ── (主线程发起 -> 异步投递至 QThreadPool)
  │      ├── 7.1 主线程发起 startSystem()；
  │      └── 7.2 扔进 QThreadPool::globalInstance() 后台线程执行 (src/core/CoreController.cpp 第 50-78 行)，更新系统准备状态，不阻塞 UI 渲染。
  │
  ├── 8. 调度首帧延迟显示 (QTimer::singleShot(0, [&w](){ w.show(); })) ── (主线程 Event Loop 第一个 Tick)
  │      └── 依据: src/main.cpp (第 124-126 行)。避开首帧信号洪暴，平滑展示主界面。
  │
  └── 9. 进入 Qt 事件主循环 (return a.exec()) ── (主线程 / 阻塞等待事件)
```

---

### 6.1 `src/main.cpp`
- **用途**：应用程序入口文件，负责单实例检测、全局应用程序（`QApplication`）配置、COM 环境初始化、核心组件拓扑预热、主窗口创建与退出生理清场。
- **关键函数与逻辑**：
  - `main(int argc, char *argv[])`：程序入口。先执行 `CreateMutexA` 单实例检测，失败即退；接着安装自定义日志句柄，创建 `QApplication`；调用 `CoreController::initializeCoreComponents()` 预热核心中枢；在栈上创建 `MainWindow w`；调用 `CoreController::startSystem()`；通过 `QTimer::singleShot(0)` 延迟显示主窗口并调用 `a.exec()`。
  - `customMessageHandler(...)`：Qt 消息重定向函数，将 `qDebug`/`qWarning` 无阻塞投递到 `Logger` 异步 RingBuffer 写入磁盘。
  - `onApplicationAboutToQuit(HANDLE hMutex)`：退出生理清场函数。请求 `CoreController` 停机、取消 `MediaExtractorPipeline` 后台任务、等待线程池 200ms 排空、安全关闭 `DatabaseManager` SQLite 数据库。
- **时序与上下文**：
  - **执行时机**：应用启动与退出时。
  - **线程/并发**：运行于主线程（UI 线程），`onApplicationAboutToQuit` 中包含 200ms 线程池等待限时。

---

### 6.2 `src/core/` 核心控制层

#### `src/core/ActionCommand.h`
- **用途**：撤销/重做（Undo/Redo）操作指令基类接口定义。
- **关键函数**：`execute()`（执行指令）、`undo()`（撤销指令）、`redo()`（重做指令）、`title()`（指令名称）。
- **时序/上下文**：由主线程上的 UI 操作触发并提交给 `UndoManager`，在主线程同步执行。

#### `src/core/AppConfig.h`
- **用途**：全局 QSettings 配置读写单例类，隔离 QuarkMeta 独立注册表/INI 路径。
- **关键函数**：`getValue(key, defaultVal)`、`setValue(key, val)`、`sync()`。
- **时序/上下文**：可在任意线程调用，内部带 `QMutex` 互斥锁保护。

#### `src/core/BasicCommands.h`
- **用途**：实现具体的操作指令类，包括 `MoveCommand`（移动）、`RenameCommand`（重命名）、`MetadataCommand`（元数据修改）、`BatchRenameCommand`（批量重命名）。
- **关键函数**：
  - `MoveCommand::execute()`：物理移动文件，并调用 `QuarkMetaJson::migrateItemMetadata` 同步迁移离散元数据。
  - `MetadataCommand::execute()/undo()`：对比修改前后的 OldState 与 NewState 快照，调用 `QuarkMetaJson::updateItemMeta` 原子落盘。
- **时序/上下文**：由用户在 UI（如右键菜单、元数据面板）触发，在主线程提交，耗时物理文件迁移内部派发给 `QtConcurrent` 执行。

#### `src/core/CentralEventHub.cpp / .h`
- **用途**：全系统唯一纯消息事件分发总线单例（传声筒），无任何数据逻辑。
- **关键函数与信号**：
  - `emitFolderMetadataChanged(folderPath)`：发射目录元数据变更信号。
  - `emitItemMetadataChanged(filePath)`：发射单个文件元数据变更信号。
- **时序/上下文**：主线程单例，所有 UI 面板（`MetaPanel`, `ContentPanel`）订阅其信号实现局部增量刷新（铁律三）。

#### `src/core/CoreController.cpp / .h`
- **用途**：系统顶层中央神经控制器，管理系统生命周期、全局搜索触发与物理磁盘搜索调度。
- **关键函数**：
  - `initializeCoreComponents()`：按拓扑顺序预热 `DatabaseManager`、`MetadataManager`、`MediaExtractorPipeline`。
  - `performSearch(keyword, scopeSource, categoryId, parentPath)`：触发异步搜索，第一阶段检索 `MetadataManager` 缓存，第二阶段调用 `PhysicalDiskSearchExtractor` 进行物理磁盘 I/O 扫描。
- **时序/上下文**：主线程单例，`performSearch` 内部使用 `QtConcurrent::run` 丢入后台线程池执行。

#### `src/core/CoreEngine.cpp / .h`
- **用途**：中央大脑与业务逻辑编排器，接收 UI 提交的 Command 并负责全局标签组的管理逻辑。
- **关键函数**：
  - `executeCommand(cmd)`：接收并执行 ActionCommand，成功后推入 `UndoManager` 栈。
  - `addTag(tagName, color)` / `removeTag(tagName)`：首先原子更新 `global.db` 的 `tag_groups` 表，随后通知 `CentralEventHub`。
- **时序/上下文**：主线程单例，由 UI 用户交互同步调用。

#### `src/core/DiskScanService.cpp / .h`
- **用途**：磁盘目录递归遍历与扫描服务类。
- **关键函数**：`scanDirectory(path)`：递归收集指定目录下的所有文件项生成 `ItemRecord` 列表。
- **时序/上下文**：在后台 worker 线程由 `DiskItemModel` 异步调用。

#### `src/core/DiskTrashService.cpp / .h`
- **用途**：基于 File_ID 隔离盒的磁盘回收站核心逻辑服务类。
- **关键函数**：
  - `moveToTrash(filePath)`：生成 UUID `file_id`，创建 `<盘符>:\.QuarkMeta\disk_trash\{file_id}\` 隔离盒，原封不动将文件移入盒内，并向 `global.db` 的 `disk_trash` 表插入记录。
  - `restoreFromTrash(fileId)`：提取原始路径与原始创建时间戳，若发生同名碰撞，依据创建时间戳权威对比，较早创建的占原名，较晚创建的重命名为 `A-1.txt`。
- **时序/上下文**：主线程发起，文件移动过程带物理锁与路径校验。

#### `src/core/FileFilterService.cpp / .h`
- **用途**：文件类型扩展名匹配与分类过滤服务。
- **关键函数**：`matchCategory(extension, categoryId)`：判断文件扩展名是否符合指定文件类型分类（图片/视频/文档/音频等）。
- **时序/上下文**：纯计算类，无状态，任意线程安全同步调用。

#### `src/core/IndexedEntry.cpp / .h`
- **用途**：磁盘扫描项的高效索引内存结构体封装。
- **时序/上下文**：纯数据结构。

#### `src/core/ItemRecord.cpp / .h`
- **用途**：单个磁盘文件/文件夹在内存中的核心记录结构体。
- **关键函数**：`ItemRecord::fromPath(path, meta)`：结合物理 `QFileInfo` 与 `ItemMeta` 构建完整运行时 `ItemRecord`。第 24 行 `r.isManaged = meta.hasUserOperations();`。
- **时序/上下文**：数据结构封装，在模型与元数据管理类之间传递。

#### `src/core/ModelContract.h`
- **用途**：定义 Qt Model/View 架构所需的扩展 Role 枚举（如 `PathRole`、`RatingRole`、`ColorRole`、`ManagedRole` 等）。
- **时序/上下文**：头文件枚举定义。

#### `src/core/NavigationHistoryService.cpp / .h`
- **用途**：目录导航前进/后退历史栈服务，持久化记录至 `global.db` 的 `navigation_history` 表。
- **关键函数**：`navigateBack()`、`navigateForward()`、`recordNavigation(path)`。
- **时序/上下文**：主线程 UI 导航时触发。

#### `src/core/OperationSnapshotEngine.cpp / .h`
- **用途**：批量操作（如批量重命名、批量改元数据）的前置快照引擎，为 Undo 提供崩溃恢复数据。
- **关键函数**：`createSnapshot(paths)`、`restoreSnapshot(snapshotId)`。
- **时序/上下文**：批量 Action 派发前在主线程同步生成。

#### `src/core/PhysicalDiskSearchExtractor.cpp / .h`
- **用途**：物理磁盘直接目录树扫描搜索提取器。
- **关键函数**：`performDiskSearch(...)`：在指定目录下逐级递归扫描物理文件，匹配关键词，并定期通过 Callback 分批回传结果。
- **时序/上下文**：运行于 `QtConcurrent` 后台线程池，内部检查 `isAborted` 标记支持秒级打断。

#### `src/core/SearchHistoryService.cpp / .h`
- **用途**：搜索历史记录持久化服务，读写 `global.db` 的 `search_history` 表。
- **关键函数**：`addHistory(keyword)`、`getRecentHistory(limit)`。
- **时序/上下文**：主线程搜索触发时调用。

#### `src/core/UndoManager.h`
- **用途**：全局撤销/重做命令栈管理器（单例）。
- **关键函数**：`pushCommand(cmd)`、`undo()`、`redo()`、`clearHistoryForPath(path)`。
- **时序/上下文**：主线程单例，持有 `std::vector<std::unique_ptr<ActionCommand>>` 历史栈。

#### `src/core/VolumeOnlineManager.cpp / .h`
- **用途**：Windows 盘符热插拔与在线状态监听管理器。
- **关键函数**：`getOnlineVolumes()`、`handleDeviceChange()`。
- **时序/上下文**：主线程单例，响应 `WM_DEVICECHANGE` 系统消息。

---

### 6.3 `src/crypto/` 加密层

#### `src/crypto/EncryptionManager.cpp / .h`
- **用途**：文件安全加密/解密管理器（AES-256 算法）。
- **关键函数**：`encryptFile(src, dest, key)`、`decryptFileToTemp(src, key)`。
- **时序/上下文**：用户手动触发，文件加解密在 `QtConcurrent` 后台 worker 线程进行。

---

### 6.4 `src/meta/` 元数据持久化与数据库层

#### `src/meta/BatchRenameEngine.cpp / .h`
- **用途**：批量重命名正则表达式与规则解析引擎。
- **关键函数**：`generateNewNames(oldNames, pattern, ruleParams)`：依据通配符、序号、替换规则生成目标文件名列表。
- **时序/上下文**：纯逻辑计算类，在 `BatchRenameDialog` 中主线程实时计算预览。

#### `src/meta/DatabaseManager.cpp / .h`
- **用途**：全局 SQLite 数据库 `global.db` 唯一连接管理者。
- **关键函数**：`instance()`、`getDatabase()`（支持递归互斥锁）、`shutdown()`（优雅关闭连接）。
- **时序/上下文**：全局单例，带有 `QRecursiveMutex` 保证多线程并发 SQL 执行安全。

#### `src/meta/DatabaseMigrator.h`
- **用途**：SQLite 数据库 Schema 升级迁移辅助类。
- **关键函数**：`migrateSchema(db)`：检测数据库版本并补全表字段。
- **时序/上下文**：系统启动时在 `DatabaseManager` 初始化中同步调用。

#### `src/meta/DiskNavigatorService.cpp / .h`
- **用途**：磁盘目录导航辅助服务，包装 `QuarkMetaJson` 为导航层提供元数据映射。
- **关键函数**：`getFolderMeta(path)`：调用 `QuarkMetaJson::readFolderMeta`。
- **时序/上下文**：主线程或后台扫描线程同步调用。

#### `src/meta/DiskTrashRepo.cpp / .h`
- **用途**：`global.db` 中 `disk_trash` 表的数据访问对象（DAO）。
- **关键函数**：`insertTrashRecord(record)`、`deleteTrashRecord(fileId)`、`queryAllTrashRecords()`。
- **时序/上下文**：由 `DiskTrashService` 调用，受 `DatabaseManager` 锁保护。

#### `src/meta/DriveMetaDao.cpp / .h`
- **用途**：`global.db` 中 `drive_metadata` 表的数据访问对象。
- **关键函数**：`saveDriveMeta(driveRoot, meta)`、`getDriveMeta(driveRoot)`。
- **时序/上下文**：盘符根目录加载时在主线程调用。

#### `src/meta/DuplicateDetectorService.cpp / .h`
- **用途**：文件重复项检测服务，基于文件体积与 Hash 计算。
- **关键函数**：`detectDuplicatesAsync(folderPath, callback)`：后台扫描文件夹中相同体积文件并计算 SHA-256。
- **时序/上下文**：派发至后台 `QtConcurrent` 线程池执行，完成后回调主线程。

#### `src/meta/FileOperationHelper.h`
- **用途**：底层物理文件复制、删除、剪切辅助静态工具类。
- **关键函数**：`copyDirectoryRecursively(src, dest)`。
- **时序/上下文**：在 worker 线程中同步执行文件拷贝。

#### `src/meta/MediaExtractorPipeline.cpp / .h`
- **用途**：多媒体特征与缩略图提取中枢队列单例。
- **关键函数**：
  - `submitTask(filePath, priority)`：将提取任务提交至后台并发队列。
  - `cancelAll()`：停机时熔断所有提取任务。
- **时序/上下文**：全局单例，内部维护任务队列与 worker 线程。

#### `src/meta/MetaCacheDecorator.cpp / .h`
- **用途**：`QuarkMetaJson` 对象的二级内存装饰器与缓存池。
- **关键函数**：`getJsonForDir(dirPath)`：防止频繁重复创建 JSON 解析器。
- **时序/上下文**：内存装饰器，任意线程安全访问。

#### `src/meta/MetadataDefs.h`
- **用途**：定义元数据基础数据结构体（`ItemMeta`、`FolderMeta` 等）。
- **时序/上下文**：头文件数据结构定义。

#### `src/meta/MetadataManager.cpp / .h`
- **用途**：全系统元数据核心中枢，管理 `m_folderCache` 内存缓存，调度 `QuarkMetaJson` 读写落盘。
- **关键函数**：
  - `loadFolderCacheFromDisk(folderPath)`：读取磁盘 `.QuarkMeta.json` 填充内存缓存。
  - `setRating(path, rating)` / `setColor(path, color)` / `setTags(path, tags)`：调用 `QuarkMetaJson::updateItemMeta` 写入磁盘并更新内存缓存，广播 `CentralEventHub` 信号。
- **时序/上下文**：主线程单例，读写接口原子化。

#### `src/meta/QuarkMetaJson.cpp / .h`
- **用途**：`.QuarkMeta.json` 磁盘文件的精准解析与写入控制器。
- **关键函数**：
  - `readFolderMeta(folderPath)`：解析指定目录下的 `.QuarkMeta.json` 返回 `std::unordered_map<std::wstring, ItemMeta>`。
  - `updateItemMeta(filePath, updater)`：原子加载、修改并保存指定文件的属性条目。
  - `migrateItemMetadata(oldPath, newPath)`：文件移动/重命名时原子迁移 JSON 中的 Key 值。
- **时序/上下文**：静态/局部工具类，在文件操作与元数据修改时同步执行磁盘 JSON I/O。

#### `src/meta/StatisticsService.cpp / .h`
- **用途**：统计当前目录下文件评级、颜色、比例分布的统计计算服务。
- **关键函数**：`calculateStats(itemList)`：遍历当前项目计算 `ScanStats`。
- **时序/上下文**：在 `ContentPanel` 加载目录后于主线程计算并同步给 `FilterPanel`。

#### `src/meta/TagRepository.cpp / .h`
- **用途**：`global.db` 中 `tag_groups` 表与全局标签主词典的数据访问对象。
- **关键函数**：`getAllTagGroups()`、`addTag(name, color)`、`removeTag(name)`。
- **时序/上下文**：主线程调用，受 SQLite 事务锁保护。

#### `src/meta/TrashRepository.cpp / .h`
- **用途**：回收站仓储高层封装类，包装 `DiskTrashRepo`。
- **关键函数**：`getTrashItems()`、`emptyTrash()`。
- **时序/上下文**：主线程 UI 回收站对话框调用。

---

### 6.5 `src/ui/` 界面渲染与交互层

#### `src/ui/AddressBar.cpp / .h`
- **用途**：顶部地址路径输入框与 Breadcrumb 切换按钮。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/AddressHistoryPanel.cpp / .h`
- **用途**：地址栏历史下拉弹出面板。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/BatchCreateDialog.cpp / .h`
- **用途**：批量新建文件夹/文件对话框。
- **时序/上下文**：主线程 Modal 模态对话框。

#### `src/ui/BatchProgressDialog.h`
- **用途**：批量文件处理进度展示对话框。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/BatchRenameDialog.cpp / .h`
- **用途**：批量重命名规则设置与实时预览对话框。
- **关键逻辑**：固定硬编码 `bool isCapsule = false;`，提交重命名命令给 `UndoManager`。
- **时序/上下文**：主线程 Modal 模态对话框。

#### `src/ui/BreadcrumbBar.cpp / .h`
- **用途**：面包屑导航栏控件，支持分级点击跳转。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/CardPainterHelper.cpp / .h`
- **用途**：缩略图网格卡片绘制辅助类（绘制阴影、边框、选中态、评级星标、角标指示符）。
- **关键函数**：`drawStatusIndicators(...)`：依据 `isManaged` 与 `isPinned` 绘制卡片角标。
- **时序/上下文**：主线程 `paintEvent` / `Delegate::paint` 中同步渲染。

#### `src/ui/ColorAlgorithmEngine.cpp / .h`
- **用途**：RGB 与 HSL 颜色转换及主色调匹配计算引擎。
- **时序/上下文**：纯算法计算类，任意线程安全调用。

#### `src/ui/ColorPicker.cpp / .h`
- **用途**：颜色选择器 UI 控件（SV 盘与 Hue 滑块）。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/ContentPanel.cpp / .h`
- **用途**：第 3 栏核心内容展示面板，管理 `DiskItemModel`、`FilterProxyModel` 及 `ThumbnailDelegate`，支持网格/列表视图切换与右键菜单（右键菜单“删除”选项严格置底）。
- **关键函数**：
  - `loadDirectory(path)`：切换当前浏览目录，重置 `DiskItemModel` 根路径。
  - `recalculateAndEmitStats()`：计算目录统计信息并向 `FilterPanel` 发射 `directoryStatsReady` 信号。
- **时序/上下文**：主线程核心面板，所有视图交互由此分发。

#### `src/ui/CreateRuleRow.cpp / .h` & `src/ui/RuleRow.cpp / .h`
- **用途**：批量重命名规则配置单行 UI 控件。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/DiskBatchRenameService.cpp / .h`
- **用途**：磁盘模式批量重命名异步执行服务。
- **关键函数**：`executeBatchRenameAsync(oldPaths, newPaths, callback)`。
- **时序/上下文**：主线程发起，后台线程执行实际 `QFile::rename`。

#### `src/ui/DriveButton.cpp / .h`
- **用途**：第一栏目录导航栏中的盘符与快捷文件夹按钮控件。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/DropJustifiedView.cpp / .h` & `src/ui/DropListView.cpp / .h` & `src/ui/DropTreeView.cpp / .h`
- **用途**：支持拖拽（Drag & Drop）物理文件移动的自适应网格/列表/树形视图扩展类。
- **关键逻辑**：捕获 `dropEvent`，构建 `MoveCommand` 并提交给 `CoreEngine`。
- **时序/上下文**：主线程 UI 拖拽事件处理。

#### `src/ui/DuplicateConflictDialog.cpp / .h`
- **用途**：文件重命名或移动遇到同名碰撞时的冲突解决对话框（替换/跳过/保留两者）。
- **时序/上下文**：主线程 Modal 模态对话框。

#### `src/ui/FavoritePanel.cpp / .h`
- **用途**：第 2 栏独占收藏夹面板，展示常用快捷文件夹列表。
- **关键函数**：`addFavorite(path)`、`removeFavorite(path)`、`refreshFavorites()`。
- **时序/上下文**：主线程 UI 控件，配置持久化至 `AppConfig`。

#### `src/ui/FilterPanel.cpp / .h`
- **用途**：第 5 栏条件筛选面板，统一为纵向复选框列表形式，提供评级、颜色、文件类型及“无缩略图(失败/跳过)”过滤。
- **关键函数**：`populateStats(stats)`：接收 `ContentPanel` 发出的统计信号更新统计数字。
- **时序/上下文**：主线程 UI 面板，通过信号联动 `FilterProxyModel`。

#### `src/ui/FormatDecoders.cpp / .h`
- **用途**： Ghostscript (PDF/EPS/PS) 安全渲染及 Specialized Format (TIFF, WebP) 解码器。
- **关键函数**：`renderGhostscriptSafely(pdfPath, size, token)`：在全局互斥锁与 50ms `CancellationToken` 轮询保护下调用 Ghostscript 渲染 PDF 第一页。
- **时序/上下文**：运行于后台提图线程池，严禁阻塞主线程。

#### `src/ui/FramelessDialog.cpp / .h` & `src/ui/FramelessFileDialog.cpp / .h`
- **用途**：系统无边框自定义窗口与无边框文件选择对话框基类。
- **时序/上下文**：主线程 UI 窗口。

#### `src/ui/HoverEventFilter.cpp / .h` & `src/ui/ResizeEventFilter.cpp / .h`
- **用途**：UI 悬停高亮与无边框窗口尺寸调整事件过滤器。
- **时序/上下文**：主线程 Qt 事件过滤器。

#### `src/ui/IconCacheManager.cpp / .h`
- **用途**：系统文件图标（QIcon）内存二级缓存管理器。
- **关键函数**：`getIconForExtension(ext)`：避免重复提取 Shell 图标。
- **时序/上下文**：主线程单例。

#### `src/ui/ImageDecoderFacade.cpp / .h`
- **用途**：高级图像解码统一外观接口。
- **关键函数**：`decodeImage(path, targetSize)`。
- **时序/上下文**：后台 worker 线程调用。

#### `src/ui/JustifiedView.cpp / .h`
- **用途**：自适应对齐（Justified Layout）图片网格视图控件。
- **时序/上下文**：主线程 Custom View 控件。

#### `src/ui/Logger.h`
- **用途**：全系统日志引擎，内部维护 `LoggerWriterThread` 异步写出日志至 `quarkmeta_debug.log`。
- **关键函数**：`Logger::log(msg)`：极为高效的无锁内存队列投递，绝对无磁盘 I/O 阻塞。
- **时序/上下文**：任意线程安全调用，后台专用 `QThread` 写盘。

#### `src/ui/MainWindow.cpp / .h`
- **用途**：系统顶级主窗口，管理横向 5 栏式整体布局、主菜单、快捷键及窗口缩放。
- **关键逻辑**：初始化并组合 `NavPanel` (Col 1)、`FavoritePanel` (Col 2)、`ContentPanel` (Col 3)、`MetaPanel` (Col 4)、`FilterPanel` (Col 5)。
- **时序/上下文**：主线程 UI 核心。

#### `src/ui/MediaColorExtractor.cpp / .h`
- **用途**：图像主色调调色板提取工具类。
- **时序/上下文**：后台提图线程调用。

#### `src/ui/MetaPanel.cpp / .h`
- **用途**：第 4 栏元数据属性面板，展示并修改当前选中文件的星级、颜色标记、标签列表（通过按钮触发 `TagSelectorOverlay` 实时选择）、尺寸及备注。
- **关键函数**：`updateItemMeta(filePath)`：选中项切换时重新加载并刷屏 UI；属性修改时构建 `MetadataCommand` 并提交执行。
- **时序/上下文**：主线程 UI 面板，订阅 `CentralEventHub` 信号。

#### `src/ui/NavPanel.cpp / .h`
- **用途**：第 1 栏目录导航栏，展示“此电脑”、本地盘符及系统快捷目录树。
- **关键函数**：`refreshDriveList()`：获取系统当前盘符并渲染按钮。
- **时序/上下文**：主线程 UI 面板。

#### `src/ui/PresetManager.cpp / .h`
- **用途**：批量重命名预设规则管理器。
- **时序/上下文**：主线程 UI 辅助类。

#### `src/ui/QuickLookMinimap.cpp / .h` & `src/ui/QuickLookWindow.cpp / .h`
- **用途**：空格键大图极速预览（QuickLook）浮窗与小地图控件。
- **关键函数**：`showPreview(filePath)`：高分辨率解码文件并弹窗展示。
- **时序/上下文**：主线程触发，异步加载大图。

#### `src/ui/SearchHistoryPanel.cpp / .h`
- **用途**：搜索历史下拉推荐面板。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/SvgIconRenderer.cpp / .h` & `src/ui/SvgIcons.h`
- **用途**：矢量 SVG 图标渲染与全局 SVG 字符串图标库（铁律：UI 图标必须使用 `UiHelper::getIcon` / `SvgIconRenderer`，严禁使用 Emoji）。
- **关键函数**：`getIcon(svgKey, size, color)`。
- **时序/上下文**：主线程渲染辅助类。

#### `src/ui/TagManagerController.cpp / .h` & `src/ui/TagManagerDialog.cpp / .h`
- **用途**：全局标签字典管理控制器与模态管理对话框。
- **关键函数**：`addNewTag(name, color)`、`deleteTag(name)`：调用 `CoreEngine` 更新全局标签库。
- **时序/上下文**：主线程 Modal 模态对话框。

#### `src/ui/TagSelectorOverlay.cpp / .h`
- **用途**：元数据面板专用的标签悬浮选择器，带侧边栏分类切换、无底部提示条，实时同步持久化窗口尺寸。
- **关键函数**：`showAt(globalPos)`、`onTagToggled(tagName)`。
- **时序/上下文**：主线程 Popup 浮窗。

#### `src/ui/TaskProgressToolBar.cpp / .h`
- **用途**：底部任务进度与状态工具栏。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/ThumbnailDelegate.cpp / .h` & `src/ui/TreeItemDelegate.h`
- **用途**：缩略图网格视图与树形视图的标准 ItemDelegate 自定义渲染器。
- **关键函数**：`paint(painter, option, index)`：渲染卡片阴影、图片、文件名、评级星标与 `ManagedRole` 状态角标。
- **时序/上下文**：主线程 paintEvent 期间同步调用。

#### `src/ui/ToolTipOverlay.cpp / .h`
- **用途**：悬浮提示工具栏 Overlay。
- **时序/上下文**：主线程 UI 控件。

#### `src/ui/TrayController.cpp / .h`
- **用途**：系统托盘图标（TrayIcon）控制器。
- **时序/上下文**：主线程后台服务。

#### `src/ui/UiHelper.h`
- **用途**：UI 通用样式与图标获取静态工厂（`UiHelper::getIcon`）。
- **时序/上下文**：主线程辅助类。

#### `src/ui/UndoToastOverlay.cpp / .h`
- **用途**：撤销操作完成后的 Toast 浮动提示框。
- **时序/上下文**：主线程 UI 提示控件。

#### `src/ui/WindowsShellThumbnailProvider.cpp / .h`
- **用途**：调用 Windows IExtractImage / IShellItemImageFactory 原生 Shell API 获取系统高精缩略图。
- **关键函数**：`getShellThumbnail(filePath, size)`。
- **时序/上下文**：在后台线程调用（依赖 `CoInitializeEx` COM 环境）。

#### `src/ui/components/` 基础 UI 控件
- **`ColorPill.cpp / .h`**：颜色色块组件。
- **`ElasticEdit.cpp / .h`**：自适应高度弹性文本编辑框（备注编辑）。
- **`FlowLayout.cpp / .h`**：自适应流式布局管理器（标签流式排列）。
- **`TagPill.cpp / .h`**：胶囊标签组件。
- **时序/上下文**：主线程 UI 基础控件。

#### `src/ui/models/DiskItemModel.cpp / .h`
- **用途**：核心物理磁盘目录数据模型（`QAbstractTableModel`），负责当前目录项的载入、尺寸预提取调度（`preloadDimensionsAsync`）与缩略图异步加载。
- **关键函数**：
  - `setRootPath(path)`：切换当前物理目录，递增生成 Token 取消旧任务，从磁盘异步或缓存加载 `ItemRecord`。
  - `preloadDimensionsAsync()`：主线程收集图片，后台提取物理尺寸并写回 `.QuarkMeta.json`。
  - `thumbnailPool()`：静态专属线程池（MaxThreads=2, LowestPriority），异步解码缩略图。
- **时序/上下文**：UI 主模型，数据加载在主线程与 `thumbnailPool()` / 后台 Worker 线程配合完成。

#### `src/ui/models/ItemModelBase.h`
- **用途**：模型基类接口定义。
- **时序/上下文**：头文件接口。

---

### 6.6 `src/util/` 通用工具层

#### `src/util/AppDirectoryInitializer.h`
- **用途**：初始化应用 AppData 系统配置目录与数据文件夹。
- **时序/上下文**：主线程系统启动时调用。

#### `src/util/DeepThumbnailExtractor.cpp / .h`
- **用途**：深度缩略图提取器，提供高分辨率图生成支持。
- **时序/上下文**：后台 Worker 线程调用。

#### `src/util/DiskIoService.h`
- **用途**：磁盘 I/O 读写与文件系统基础操作包装类。
- **时序/上下文**：工具类。

#### `src/util/DiskMediaExtractor.cpp / .h`
- **用途**：底层媒体提取核心工具类，包含图片物理尺寸快速头提取（`fastExtractImageSize`）、图像解码及 `getCapsuleThumbnail` 缩略图生成器。
- **关键函数**：
  - `fastExtractImageSize(filePath)`：轻量化读取 PNG/JPG/GIF/BMP 头信息秒级返回 width/height。
  - `getCapsuleThumbnailReadOnly(filePath)`：直接渲染物理磁盘图像缩略图（名字带有 Capsule 历史遗产）。
- **时序/上下文**：任意线程安全调用，提图流水线基础支撑类。

#### `src/util/SecureFileEraser.h`
- **用途**：物理数据抹除与磁盘扇区安全粉碎擦除工具。
- **关键函数**：`shredFile(filePath)`：覆写扇区擦除物理文件。
- **时序/上下文**：由 `SecureDeleteCommand` 在后台线程调用。

#### `src/util/ShellHelper.cpp / .h`
- **用途**：Windows Shell 系统交互工具（打开系统文件资源管理器、定位文件、唤起系统右键菜单）。
- **关键函数**：`showInGraphicalShell(path)`、`openFile(path)`。
- **时序/上下文**：主线程 UI 用户点击响应时调用。

---
