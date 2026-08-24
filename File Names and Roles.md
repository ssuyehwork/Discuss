# QuarkMeta 项目代码文件与职责清单 (File Names and Roles)

本文档记录 QuarkMeta 项目中自身源码目录下所有真实存在的代码文件及其明确、可验证的职责描述与深度僵尸代码/幽灵代码/架构违规项排查结果（已排除第三方 LibTIFF）。

---

### ` src/core/ActionCommand.h `
- **文件职责**：定义 Action/Command 撤销恢复基类 `ActionCommand` 及其派生命令（如移动、重命名、属性修改、安全删除操作类）。
- **僵尸代码**：无

### ` src/core/AppConfig.h `
- **文件职责**：应用全局配置管理类 `AppConfig`，基于 QSettings 读写应用程序级别的参数与偏好设置。
- **僵尸代码**：无

### ` src/core/BasicCommands.h `
- **文件职责**：实现基础的 ActionCommand 操作，如 `MoveCommand`、`RenameCommand`、`MetadataCommand` 与 `SecureDeleteCommand`。
- **僵尸代码**：无

### ` src/core/CentralEventHub.cpp `
- **文件职责**：中央消息事件总线（传声筒），解耦 UI 与底层业务，通过 Qt 信号槽分发系统级增量事件。
- **僵尸代码**：无

### ` src/core/CentralEventHub.h `
- **文件职责**：中央消息事件总线（传声筒），解耦 UI 与底层业务，通过 Qt 信号槽分发系统级增量事件。
- **僵尸代码**：无

### ` src/core/CoreController.cpp `
- **文件职责**：应用程序高层核心控制器，连接业务引擎 CoreEngine 与 UI 视图控制器。
- **僵尸代码**：无

### ` src/core/CoreController.h `
- **文件职责**：应用程序高层核心控制器，连接业务引擎 CoreEngine 与 UI 视图控制器。
- **僵尸代码**：无

### ` src/core/CoreEngine.cpp `
- **文件职责**：中央业务大脑 `CoreEngine`，统筹处理业务决策、指令封装执行及调度后台服务。
- **僵尸代码**：无

### ` src/core/CoreEngine.h `
- **文件职责**：中央业务大脑 `CoreEngine`，统筹处理业务决策、指令封装执行及调度后台服务。
- **僵尸代码**：无

### ` src/core/DiskScanService.cpp `
- **文件职责**：磁盘异步扫描服务，负责遍历扫描本地物理目录并生成文件记录。
- **僵尸代码**：无

### ` src/core/DiskScanService.h `
- **文件职责**：磁盘异步扫描服务，负责遍历扫描本地物理目录并生成文件记录。
- **僵尸代码**：无

### ` src/core/DiskTrashService.cpp `
- **文件职责**：回收站业务服务层，负责将文件移入基于 File_ID 的隔离盒回收站以及逆向还原操作。
- **僵尸代码**：无

### ` src/core/DiskTrashService.h `
- **文件职责**：回收站业务服务层，负责将文件移入基于 File_ID 的隔离盒回收站以及逆向还原操作。
- **僵尸代码**：无

### ` src/core/FileFilterService.cpp `
- **文件职责**：文件过滤服务，提供基于扩展名、星级、颜色标记、名称及时间条件的匹配过滤逻辑。
- **僵尸代码**：无

### ` src/core/FileFilterService.h `
- **文件职责**：文件过滤服务，提供基于扩展名、星级、颜色标记、名称及时间条件的匹配过滤逻辑。
- **僵尸代码**：无

### ` src/core/IndexedEntry.cpp `
- **文件职责**：索引条目数据结构与相关辅助操作封装。
- **僵尸代码**：无

### ` src/core/IndexedEntry.h `
- **文件职责**：索引条目数据结构与相关辅助操作封装。
- **僵尸代码**：无

### ` src/core/ItemRecord.cpp `
- **文件职责**：磁盘文件/目录的核心元数据记录结构体 `ItemRecord`，承载路径、尺寸、星级、颜色、标签、时间及缩略图状态等。
- **僵尸代码**：无

### ` src/core/ItemRecord.h `
- **文件职责**：磁盘文件/目录的核心元数据记录结构体 `ItemRecord`，承载路径、尺寸、星级、颜色、标签、时间及缩略图状态等。
- **僵尸代码**：无

### ` src/core/ModelContract.h `
- **文件职责**：定义数据模型与视图交互的 Role 契约枚举（如 `PathRole`, `NameRole`, `RatingRole` 等）。
- **僵尸代码**：无

### ` src/core/NavigationHistoryService.cpp `
- **文件职责**：目录导航历史记录服务，维护前进、后退的历史路径栈。
- **僵尸代码**：无

### ` src/core/NavigationHistoryService.h `
- **文件职责**：目录导航历史记录服务，维护前进、后退的历史路径栈。
- **僵尸代码**：无

### ` src/core/OperationSnapshotEngine.cpp `
- **文件职责**：文件批处理与敏感操作快照引擎，用于备份和恢复批量操作前的磁盘与元数据状态。
- **僵尸代码**：无

### ` src/core/OperationSnapshotEngine.h `
- **文件职责**：文件批处理与敏感操作快照引擎，用于备份和恢复批量操作前的磁盘与元数据状态。
- **僵尸代码**：无

### ` src/core/PhysicalDiskSearchExtractor.cpp `
- **文件职责**：物理磁盘实时搜索与提取引擎，在非数据库索引模式下直接遍历物理磁盘进行模式匹配。
- **僵尸代码**：无

### ` src/core/PhysicalDiskSearchExtractor.h `
- **文件职责**：物理磁盘实时搜索与提取引擎，在非数据库索引模式下直接遍历物理磁盘进行模式匹配。
- **僵尸代码**：无

### ` src/core/SearchHistoryService.cpp `
- **文件职责**：搜索历史关键词记录与持久化管理服务。
- **僵尸代码**：无

### ` src/core/SearchHistoryService.h `
- **文件职责**：搜索历史关键词记录与持久化管理服务。
- **僵尸代码**：无

### ` src/core/UndoManager.h `
- **文件职责**：撤销/重做历史命令管理器 `UndoManager`，管理撤销栈与重做栈。
- **僵尸代码**：无

### ` src/core/VolumeOnlineManager.cpp `
- **文件职责**：本地磁盘卷与盘符联机/脱机状态检测与监控服务。
- **僵尸代码**：无

### ` src/core/VolumeOnlineManager.h `
- **文件职责**：本地磁盘卷与盘符联机/脱机状态检测与监控服务。
- **僵尸代码**：无

### ` src/crypto/EncryptionManager.cpp `
- **文件职责**：敏感数据加密与解密管理类。
- **僵尸代码**：无

### ` src/crypto/EncryptionManager.h `
- **文件职责**：敏感数据加密与解密管理类。
- **僵尸代码**：无

### ` src/main.cpp `
- **文件职责**：应用程序主入口点，初始化 QApplication、皮肤样式、主窗口 `MainWindow` 并启动 Qt 事件循环。
- **僵尸代码**：无

### ` src/meta/BatchRenameEngine.cpp `
- **文件职责**：批量重命名规则计算引擎，支持正则、插入、替换、序号递增等规则的预览与文件名计算。
- **僵尸代码**：无

### ` src/meta/BatchRenameEngine.h `
- **文件职责**：批量重命名规则计算引擎，支持正则、插入、替换、序号递增等规则的预览与文件名计算。
- **僵尸代码**：无

### ` src/meta/DatabaseManager.cpp `
- **文件职责**：基于 SQLite 的 `global.db` 全局数据库连接与事务管理服务。
- **僵尸代码**：无

### ` src/meta/DatabaseManager.h `
- **文件职责**：基于 SQLite 的 `global.db` 全局数据库连接与事务管理服务。
- **僵尸代码**：无

### ` src/meta/DatabaseMigrator.h `
- **文件职责**：数据库 Schema 结构版本迁移辅助工具类。
- **僵尸代码**：无

### ` src/meta/DiskNavigatorService.cpp `
- **文件职责**：纯磁盘目录导航与路径有效性校验服务。
- **僵尸代码**：无

### ` src/meta/DiskNavigatorService.h `
- **文件职责**：纯磁盘目录导航与路径有效性校验服务。
- **僵尸代码**：无

### ` src/meta/DiskTrashRepo.cpp `
- **文件职责**：回收站数据库仓储类，读写 `global.db` 中的 `disk_trash` 表记录。
- **僵尸代码**：无

### ` src/meta/DiskTrashRepo.h `
- **文件职责**：回收站数据库仓储类，读写 `global.db` 中的 `disk_trash` 表记录。
- **僵尸代码**：无

### ` src/meta/DriveMetaDao.cpp `
- **文件职责**：根目录/本地盘符元数据 Data Access Object，持久化盘符级别元数据至 `global.db`。
- **僵尸代码**：无

### ` src/meta/DriveMetaDao.h `
- **文件职责**：根目录/本地盘符元数据 Data Access Object，持久化盘符级别元数据至 `global.db`。
- **僵尸代码**：无

### ` src/meta/DuplicateDetectorService.cpp `
- **文件职责**：重复文件检测服务，支持快速哈希/全量哈希对比判定重复文件。
- **僵尸代码**：无

### ` src/meta/DuplicateDetectorService.h `
- **文件职责**：重复文件检测服务，支持快速哈希/全量哈希对比判定重复文件。
- **僵尸代码**：无

### ` src/meta/FileOperationHelper.h `
- **文件职责**：磁盘文件/目录常规物理 I/O 操作（复制、移动、强删）辅助函数。
- **僵尸代码**：无

### ` src/meta/MediaExtractorPipeline.cpp `
- **文件职责**：媒体文件元数据提取流水线，异步调度解码器提取分辨率、颜色分布与 EXIF 信息。
- **僵尸代码**：无

### ` src/meta/MediaExtractorPipeline.h `
- **文件职责**：媒体文件元数据提取流水线，异步调度解码器提取分辨率、颜色分布与 EXIF 信息。
- **僵尸代码**：无

### ` src/meta/MetaCacheDecorator.cpp `
- **文件职责**：元数据内存缓存装饰器，提供快速访问与缓存失效机制。
- **僵尸代码**：无

### ` src/meta/MetaCacheDecorator.h `
- **文件职责**：元数据内存缓存装饰器，提供快速访问与缓存失效机制。
- **僵尸代码**：无

### ` src/meta/MetadataDefs.h `
- **文件职责**：元数据通用枚举定义（如 Rating、ColorLabel、SortOrder 等）。
- **僵尸代码**：无

### ` src/meta/MetadataManager.cpp `
- **文件职责**：离散元数据统一入口管理器，协调磁盘 `.QuarkMeta.json` 与 `global.db` 盘符表。
- **僵尸代码**：无

### ` src/meta/MetadataManager.h `
- **文件职责**：离散元数据统一入口管理器，协调磁盘 `.QuarkMeta.json` 与 `global.db` 盘符表。
- **僵尸代码**：无

### ` src/meta/QuarkMetaJson.cpp `
- **文件职责**：磁盘离散 JSON 元数据管理类，负责读写各目录下 `.QuarkMeta.json` 文件中的元数据条目。
- **僵尸代码**：无

### ` src/meta/QuarkMetaJson.h `
- **文件职责**：磁盘离散 JSON 元数据管理类，负责读写各目录下 `.QuarkMeta.json` 文件中的元数据条目。
- **僵尸代码**：无

### ` src/meta/StatisticsService.cpp `
- **文件职责**：全局/目录统计服务类，计算资产与回收站计数汇总。
- **僵尸代码**：无

### ` src/meta/StatisticsService.h `
- **文件职责**：全局/目录统计服务类，计算资产与回收站计数汇总。
- **僵尸代码**：无

### ` src/meta/TagRepository.cpp `
- **文件职责**：全局标签管理仓储类，增删改查 `global.db` 中的 `tag_groups` 与标签元数据。
- **僵尸代码**：无

### ` src/meta/TagRepository.h `
- **文件职责**：全局标签管理仓储类，增删改查 `global.db` 中的 `tag_groups` 与标签元数据。
- **僵尸代码**：无

### ` src/meta/TrashRepository.cpp `
- **文件职责**：旧版回收站仓储持久化适配类（包含历史 `trash_items` 逻辑）。
- **僵尸代码**：无

### ` src/meta/TrashRepository.h `
- **文件职责**：旧版回收站仓储持久化适配类（包含历史 `trash_items` 逻辑）。
- **僵尸代码**：无

### ` src/meta/sqlite3.c `
- **文件职责**：嵌入式 SQLite 3 数据库引擎开源实现与头文件。
- **僵尸代码**：无

### ` src/meta/sqlite3.h `
- **文件职责**：嵌入式 SQLite 3 数据库引擎开源实现与头文件。
- **僵尸代码**：无

### ` src/meta/sqlite3ext.h `
- **文件职责**：嵌入式 SQLite 3 数据库引擎开源实现与头文件。
- **僵尸代码**：无

### ` src/ui/AddressBar.cpp `
- **文件职责**：地址栏路径输入与显示控件。
- **僵尸代码**：无

### ` src/ui/AddressBar.h `
- **文件职责**：地址栏路径输入与显示控件。
- **僵尸代码**：无

### ` src/ui/AddressHistoryPanel.cpp `
- **文件职责**：地址栏历史下拉菜单面板。
- **僵尸代码**：无

### ` src/ui/AddressHistoryPanel.h `
- **文件职责**：地址栏历史下拉菜单面板。
- **僵尸代码**：无

### ` src/ui/BatchCreateDialog.cpp `
- **文件职责**：批量创建文件夹/文件对话框。
- **僵尸代码**：无

### ` src/ui/BatchCreateDialog.h `
- **文件职责**：批量创建文件夹/文件对话框。
- **僵尸代码**：无

### ` src/ui/BatchProgressDialog.h `
- **文件职责**：批量后台任务进度显示对话框。
- **僵尸代码**：无

### ` src/ui/BatchRenameDialog.cpp `
- **文件职责**：批量重命名交互对话框，提供规则配置与实时重命名效果对比预览。
- **僵尸代码**：无

### ` src/ui/BatchRenameDialog.h `
- **文件职责**：批量重命名交互对话框，提供规则配置与实时重命名效果对比预览。
- **僵尸代码**：无

### ` src/ui/BreadcrumbBar.cpp `
- **文件职责**：面包屑可点击路径导航条控件。
- **僵尸代码**：无

### ` src/ui/BreadcrumbBar.h `
- **文件职责**：面包屑可点击路径导航条控件。
- **僵尸代码**：无

### ` src/ui/CardPainterHelper.cpp `
- **文件职责**：缩略图卡片绘制辅助工具类，负责星级、标签、选中态的高性能绘制。
- **僵尸代码**：无

### ` src/ui/CardPainterHelper.h `
- **文件职责**：缩略图卡片绘制辅助工具类，负责星级、标签、选中态的高性能绘制。
- **僵尸代码**：无

### ` src/ui/ColorAlgorithmEngine.cpp `
- **文件职责**：颜色聚类与主色提取算法引擎（Lab 颜色空间对比）。
- **僵尸代码**：无

### ` src/ui/ColorAlgorithmEngine.h `
- **文件职责**：颜色聚类与主色提取算法引擎（Lab 颜色空间对比）。
- **僵尸代码**：无

### ` src/ui/ColorPicker.cpp `
- **文件职责**：颜色选择与色相环交互控件。
- **僵尸代码**：无

### ` src/ui/ColorPicker.h `
- **文件职责**：颜色选择与色相环交互控件。
- **僵尸代码**：无

### ` src/ui/ContentPanel.cpp `
- **文件职责**：第三栏核心主视图区（内容展示区），支持缩略图网格、列表及自适应瀑布流展示。
- **僵尸代码**：无

### ` src/ui/ContentPanel.h `
- **文件职责**：第三栏核心主视图区（内容展示区），支持缩略图网格、列表及自适应瀑布流展示。
- **僵尸代码**：无

### ` src/ui/CreateRuleRow.cpp `
- **文件职责**：批量创建模板规则配置行 UI 控件。
- **僵尸代码**：无

### ` src/ui/CreateRuleRow.h `
- **文件职责**：批量创建模板规则配置行 UI 控件。
- **僵尸代码**：无

### ` src/ui/DiskBatchRenameService.cpp `
- **文件职责**：磁盘批量重命名 UI 交互服务适配器。
- **僵尸代码**：无

### ` src/ui/DiskBatchRenameService.h `
- **文件职责**：磁盘批量重命名 UI 交互服务适配器。
- **僵尸代码**：无

### ` src/ui/DriveButton.cpp `
- **文件职责**：侧边栏本地盘符/快捷目录项按钮控件。
- **僵尸代码**：无

### ` src/ui/DriveButton.h `
- **文件职责**：侧边栏本地盘符/快捷目录项按钮控件。
- **僵尸代码**：无

### ` src/ui/DropJustifiedView.cpp `
- **文件职责**：支持拖拽 Drop 交互的自适应瀑布流视图控件。
- **僵尸代码**：无

### ` src/ui/DropJustifiedView.h `
- **文件职责**：支持拖拽 Drop 交互的自适应瀑布流视图控件。
- **僵尸代码**：无

### ` src/ui/DropListView.cpp `
- **文件职责**：支持拖拽 Drop 交互的列表视图控件。
- **僵尸代码**：无

### ` src/ui/DropListView.h `
- **文件职责**：支持拖拽 Drop 交互的列表视图控件。
- **僵尸代码**：无

### ` src/ui/DropTreeView.cpp `
- **文件职责**：支持拖拽 Drop 交互的树形视图控件。
- **僵尸代码**：无

### ` src/ui/DropTreeView.h `
- **文件职责**：支持拖拽 Drop 交互的树形视图控件。
- **僵尸代码**：无

### ` src/ui/DuplicateConflictDialog.cpp `
- **文件职责**：文件覆盖/重命名冲突解决对话框。
- **僵尸代码**：无

### ` src/ui/DuplicateConflictDialog.h `
- **文件职责**：文件覆盖/重命名冲突解决对话框。
- **僵尸代码**：无

### ` src/ui/ElidedTextUtility.h `
- **文件职责**：文本省略号截断格式化绘制工具类。
- **僵尸代码**：无

### ` src/ui/FavoritePanel.cpp `
- **文件职责**：第二栏：收藏夹独占栏 UI 控件，展示用户添加的常用快捷文件夹。
- **僵尸代码**：无

### ` src/ui/FavoritePanel.h `
- **文件职责**：第二栏：收藏夹独占栏 UI 控件，展示用户添加的常用快捷文件夹。
- **僵尸代码**：无

### ` src/ui/FilterPanel.cpp `
- **文件职责**：第五栏：条件筛选栏 UI 控件，提供按颜色、星级、类型等维度的多维过滤与实时统计。
- **僵尸代码**：无

### ` src/ui/FilterPanel.h `
- **文件职责**：第五栏：条件筛选栏 UI 控件，提供按颜色、星级、类型等维度的多维过滤与实时统计。
- **僵尸代码**：无

### ` src/ui/FormatDecoders.cpp `
- **文件职责**：各类图像格式（如 TIFF/WEBP/PSD 等）解码与 QImage 转换工具。
- **僵尸代码**：无

### ` src/ui/FormatDecoders.h `
- **文件职责**：各类图像格式（如 TIFF/WEBP/PSD 等）解码与 QImage 转换工具。
- **僵尸代码**：无

### ` src/ui/FramelessDialog.cpp `
- **文件职责**：自定义无边框对话框基类及配套弹窗（ FramelessMessageBox / FramelessInputDialog 等）。
- **僵尸代码**：无

### ` src/ui/FramelessDialog.h `
- **文件职责**：自定义无边框对话框基类及配套弹窗（ FramelessMessageBox / FramelessInputDialog 等）。
- **僵尸代码**：无

### ` src/ui/FramelessFileDialog.cpp `
- **文件职责**：自定义风格的无边框文件/目录选择对话框。
- **僵尸代码**：无

### ` src/ui/FramelessFileDialog.h `
- **文件职责**：自定义风格的无边框文件/目录选择对话框。
- **僵尸代码**：无

### ` src/ui/HoverEventFilter.cpp `
- **文件职责**：鼠标悬停事件过滤器，用于 UI 动态亮色或动画效果。
- **僵尸代码**：无

### ` src/ui/HoverEventFilter.h `
- **文件职责**：鼠标悬停事件过滤器，用于 UI 动态亮色或动画效果。
- **僵尸代码**：无

### ` src/ui/IScanResultView.h `
- **文件职责**：扫描结果展示视图的通用抽象接口类。
- **僵尸代码**：无

### ` src/ui/IconCacheManager.cpp `
- **文件职责**：系统图标与类型图标内存缓存管理服务。
- **僵尸代码**：无

### ` src/ui/IconCacheManager.h `
- **文件职责**：系统图标与类型图标内存缓存管理服务。
- **僵尸代码**：无

### ` src/ui/ImageDecoderFacade.cpp `
- **文件职责**：图像解码统一门面类，路由不同格式解码器生成缩略图与 preview 图像。
- **僵尸代码**：无

### ` src/ui/ImageDecoderFacade.h `
- **文件职责**：图像解码统一门面类，路由不同格式解码器生成缩略图与 preview 图像。
- **僵尸代码**：无

### ` src/ui/JustifiedView.cpp `
- **文件职责**：自适应对齐瀑布流视图控件 (Justified Layout View)。
- **僵尸代码**：无

### ` src/ui/JustifiedView.h `
- **文件职责**：自适应对齐瀑布流视图控件 (Justified Layout View)。
- **僵尸代码**：无

### ` src/ui/Logger.h `
- **文件职责**：UI 异步线程安全日志记录与落盘工具 `Logger`。
- **僵尸代码**：无

### ` src/ui/MainWindow.cpp `
- **文件职责**：主窗口界面类，组合 5 栏视图布局、顶栏菜单、工具栏及快捷键响应机制。
- **僵尸代码**：无

### ` src/ui/MainWindow.h `
- **文件职责**：主窗口界面类，组合 5 栏视图布局、顶栏菜单、工具栏及快捷键响应机制。
- **僵尸代码**：无

### ` src/ui/MediaColorExtractor.cpp `
- **文件职责**：媒体文件颜色提取工具类。
- **僵尸代码**：无

### ` src/ui/MediaColorExtractor.h `
- **文件职责**：媒体文件颜色提取工具类。
- **僵尸代码**：无

### ` src/ui/MetaPanel.cpp `
- **文件职责**：第四栏：元数据属性栏 UI 控件，实时展示与编辑选中项的星级、颜色、标签、备注等属性。
- **僵尸代码**：无

### ` src/ui/MetaPanel.h `
- **文件职责**：第四栏：元数据属性栏 UI 控件，实时展示与编辑选中项的星级、颜色、标签、备注等属性。
- **僵尸代码**：无

### ` src/ui/NavPanel.cpp `
- **文件职责**：第一栏：目录导航栏 UI 控件，展示此电脑、本地盘符与系统目录树。
- **僵尸代码**：无

### ` src/ui/NavPanel.h `
- **文件职责**：第一栏：目录导航栏 UI 控件，展示此电脑、本地盘符与系统目录树。
- **僵尸代码**：无

### ` src/ui/PresetManager.cpp `
- **文件职责**：用户 UI 预设与规则模板配置管理服务。
- **僵尸代码**：无

### ` src/ui/PresetManager.h `
- **文件职责**：用户 UI 预设与规则模板配置管理服务。
- **僵尸代码**：无

### ` src/ui/ProgressDialog.h `
- **文件职责**：操作进度条通用弹窗控件。
- **僵尸代码**：无

### ` src/ui/QuickLookMinimap.cpp `
- **文件职责**：空格键快速预览窗口的右侧/下方小地图导览控件。
- **僵尸代码**：无

### ` src/ui/QuickLookMinimap.h `
- **文件职责**：空格键快速预览窗口的右侧/下方小地图导览控件。
- **僵尸代码**：无

### ` src/ui/QuickLookWindow.cpp `
- **文件职责**：空格键 QuickLook 大图/媒体快速预览高帧率无边框窗口。
- **僵尸代码**：无

### ` src/ui/QuickLookWindow.h `
- **文件职责**：空格键 QuickLook 大图/媒体快速预览高帧率无边框窗口。
- **僵尸代码**：无

### ` src/ui/ResizeEventFilter.cpp `
- **文件职责**：窗口/控件大小改变事件过滤器。
- **僵尸代码**：无

### ` src/ui/ResizeEventFilter.h `
- **文件职责**：窗口/控件大小改变事件过滤器。
- **僵尸代码**：无

### ` src/ui/RuleRow.cpp `
- **文件职责**：重命名规则列表中单条规则的 UI 配置控件。
- **僵尸代码**：无

### ` src/ui/RuleRow.h `
- **文件职责**：重命名规则列表中单条规则的 UI 配置控件。
- **僵尸代码**：无

### ` src/ui/ScanStats.h `
- **文件职责**：存储扫描与筛选维度汇总数据（缩略图、颜色、星级、类型统计）的结构体 `ScanStats`。
- **僵尸代码**：无

### ` src/ui/SearchHistoryPanel.cpp `
- **文件职责**：搜索框历史下拉选择面板。
- **僵尸代码**：无

### ` src/ui/SearchHistoryPanel.h `
- **文件职责**：搜索框历史下拉选择面板。
- **僵尸代码**：无

### ` src/ui/ShellIconManager.h `
- **文件职责**：Windows Shell 系统文件图标关联提取服务。
- **僵尸代码**：无

### ` src/ui/StyleLibrary.h `
- **文件职责**：应用 QSS 样式表与配色常量定义库。
- **僵尸代码**：无

### ` src/ui/SvgIconRenderer.cpp `
- **文件职责**：SVG 矢量图标渲染与颜色替换工具类。
- **僵尸代码**：无

### ` src/ui/SvgIconRenderer.h `
- **文件职责**：SVG 矢量图标渲染与颜色替换工具类。
- **僵尸代码**：无

### ` src/ui/SvgIcons.h `
- **文件职责**：内嵌 SVG 内联字符串资源库。
- **僵尸代码**：无

### ` src/ui/TagManagerController.cpp `
- **文件职责**：标签管理对话框的控制器业务逻辑适配层。
- **僵尸代码**：无

### ` src/ui/TagManagerController.h `
- **文件职责**：标签管理对话框的控制器业务逻辑适配层。
- **僵尸代码**：无

### ` src/ui/TagManagerDialog.cpp `
- **文件职责**：全局标签管理与编辑对话框。
- **僵尸代码**：无

### ` src/ui/TagManagerDialog.h `
- **文件职责**：全局标签管理与编辑对话框。
- **僵尸代码**：无

### ` src/ui/TagSelectorOverlay.cpp `
- **文件职责**：悬浮式标签选择器 Popover 控件，支持快捷勾选与搜索添加标签。
- **僵尸代码**：无

### ` src/ui/TagSelectorOverlay.h `
- **文件职责**：悬浮式标签选择器 Popover 控件，支持快捷勾选与搜索添加标签。
- **僵尸代码**：无

### ` src/ui/TaskProgressToolBar.cpp `
- **文件职责**：底层后台异步任务进度条工具栏控件。
- **僵尸代码**：无

### ` src/ui/TaskProgressToolBar.h `
- **文件职责**：底层后台异步任务进度条工具栏控件。
- **僵尸代码**：无

### ` src/ui/ThumbnailDelegate.cpp `
- **文件职责**：缩略图视图渲染 Delegate（委托），负责网格卡片的绘制与文件名编辑。
- **僵尸代码**：无

### ` src/ui/ThumbnailDelegate.h `
- **文件职责**：缩略图视图渲染 Delegate（委托），负责网格卡片的绘制与文件名编辑。
- **僵尸代码**：无

### ` src/ui/ToolTipOverlay.cpp `
- **文件职责**：自定义悬浮 ToolTip 消息气泡控件。
- **僵尸代码**：无

### ` src/ui/ToolTipOverlay.h `
- **文件职责**：自定义悬浮 ToolTip 消息气泡控件。
- **僵尸代码**：无

### ` src/ui/TrayController.cpp `
- **文件职责**：系统托盘图标及托盘右键菜单控制器。
- **僵尸代码**：无

### ` src/ui/TrayController.h `
- **文件职责**：系统托盘图标及托盘右键菜单控制器。
- **僵尸代码**：无

### ` src/ui/TreeItemDelegate.h `
- **文件职责**：目录树节点自定义绘制与高亮 Delegate。
- **僵尸代码**：无

### ` src/ui/UiHelper.h `
- **文件职责**：UI 辅助绘制工具与 DPI 缩放换算静态函数库。
- **僵尸代码**：无

### ` src/ui/UndoToastOverlay.cpp `
- **文件职责**：底部 Ctrl+Z 撤销提示浮层弹窗 Toast。
- **僵尸代码**：无

### ` src/ui/UndoToastOverlay.h `
- **文件职责**：底部 Ctrl+Z 撤销提示浮层弹窗 Toast。
- **僵尸代码**：无

### ` src/ui/WindowsShellThumbnailProvider.cpp `
- **文件职责**：Windows 平台通过 COM 接口（IShellItemImageFactory）异步提取系统文件/视频缩略图的提供者。
- **僵尸代码**：无

### ` src/ui/WindowsShellThumbnailProvider.h `
- **文件职责**：Windows 平台通过 COM 接口（IShellItemImageFactory）异步提取系统文件/视频缩略图的提供者。
- **僵尸代码**：无

### ` src/ui/components/ColorPill.cpp `
- **文件职责**：彩色小圆点/药丸状标记 UI 控件。
- **僵尸代码**：无

### ` src/ui/components/ColorPill.h `
- **文件职责**：彩色小圆点/药丸状标记 UI 控件。
- **僵尸代码**：无

### ` src/ui/components/ElasticEdit.cpp `
- **文件职责**：弹性宽度可自动扩展的单行文本输入框控件。
- **僵尸代码**：无

### ` src/ui/components/ElasticEdit.h `
- **文件职责**：弹性宽度可自动扩展的单行文本输入框控件。
- **僵尸代码**：无

### ` src/ui/components/FlowLayout.cpp `
- **文件职责**：自动折行流式布局管理器 (FlowLayout)。
- **僵尸代码**：无

### ` src/ui/components/FlowLayout.h `
- **文件职责**：自动折行流式布局管理器 (FlowLayout)。
- **僵尸代码**：无

### ` src/ui/components/TagPill.cpp `
- **文件职责**：胶囊形状的标签 UI 控件 (TagPill)。
- **僵尸代码**：无

### ` src/ui/components/TagPill.h `
- **文件职责**：胶囊形状的标签 UI 控件 (TagPill)。
- **僵尸代码**：无

### ` src/ui/models/DiskItemModel.cpp `
- **文件职责**：磁盘目录项 Qt Item Model（继承自 QAbstractListModel/QAbstractItemModel），驱动 ContentPanel 数据显示。
- **僵尸代码**：无

### ` src/ui/models/DiskItemModel.h `
- **文件职责**：磁盘目录项 Qt Item Model（继承自 QAbstractListModel/QAbstractItemModel），驱动 ContentPanel 数据显示。
- **僵尸代码**：无

### ` src/ui/models/ItemModelBase.h `
- **文件职责**：Item Model 抽象基类定义与通用哈希辅助。
- **僵尸代码**：无

### ` src/util/AppDirectoryInitializer.h `
- **文件职责**：应用初始化时创建必要本地目录结构（如缓存目录、数据目录）的初始化辅助类。
- **僵尸代码**：无

### ` src/util/DeepThumbnailExtractor.cpp `
- **文件职责**：深度图像/文件缩略图提取工具，支持多重回退提取机制。
- **僵尸代码**：无

### ` src/util/DeepThumbnailExtractor.h `
- **文件职责**：深度图像/文件缩略图提取工具，支持多重回退提取机制。
- **僵尸代码**：无

### ` src/util/DiskIoService.h `
- **文件职责**：物理磁盘 I/O 高效异步读写服务与并发线程池调度工具。
- **僵尸代码**：无

### ` src/util/DiskMediaExtractor.cpp `
- **文件职责**：磁盘媒体文件基本属性与图像特征快照提取器。
- **僵尸代码**：无

### ` src/util/DiskMediaExtractor.h `
- **文件职责**：磁盘媒体文件基本属性与图像特征快照提取器。
- **僵尸代码**：无

### ` src/util/SecureFileEraser.h `
- **文件职责**：磁盘扇区数据粉碎抹除类，实现安全粉碎删除 `SecureDeleteCommand`。
- **僵尸代码**：无

### ` src/util/ShellHelper.cpp `
- **文件职责**：Windows Shell 系统交互工具（打开系统文件管理器、显示属性对话框等）。
- **僵尸代码**：无

### ` src/util/ShellHelper.h `
- **文件职责**：Windows Shell 系统交互工具（打开系统文件管理器、显示属性对话框等）。
- **僵尸代码**：无
