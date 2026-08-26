# QuarkMeta 系统顶层架构与自研代码文件规划

## 1. 架构理念与全局设计规范

`QuarkMeta` 是一个高性能的桌面级文件管理与元数据处理系统。本文档作为应用的高级设计理念、顶层架构规划与全局规范的唯一记录载体。

### 纯洁性保护规范
1. **职责绝对单一**：本文档仅且只能记录应用的高级设计理念、顶层架构规划与全局规范。
2. **严禁写入实施细节**：具体的代码修改点、Search/Replace Diff 替换块、代码行号、调试命令等，绝对禁止写入本文档。
3. **实施方案物理隔离**：所有具体的代码修改与实施方案，必须且只能创建在 `QuarkMeta Architecture/Implementation Plan/` 目录下（采用简洁英文小写命名）。

### 收藏夹（FavoritePanel）物理路径唯一性与 Drop 防重顶层规范
1. **物理路径绝对唯一性红线**：收藏夹面板（FavoritePanel）作为第二栏固定快捷通道，其保存与呈现的每一个条目必须且仅能对应一个唯一的本地物理路径，严禁允许重复收藏相同的物理文件夹或文件。
2. **强路径归一化约束**：在任何路径进入收藏夹模型（Model）之前，必须强制进行物理路径标准化与格式归一化（包含盘符大写规范、分隔符统一及消除相对路径符号），杜绝因路径表示格式差异导致的防重判断失效。
3. **视图 Drop 插入拦截与校验**：接管与拦截视图层的默认拖放插入行为，防止 Qt 原生 Model/View 拖放逻辑绕过模型层防重校验规则直接插入节点。
4. **磁盘持久化双重去重校验**：在磁盘配置持久化读取（Load）与落盘（Save）环节，必须维持强去重幂等机制，防止由于异常退出或跨模块调用写入重复配置项。
5. **上下文菜单收藏状态动态双向切换红线**：右键呼出上下文菜单（包括 `ContentPanel` 内容区及 `QuickLookWindow` 预览窗口）时，系统必须实时校验选中项的收藏状态。当项目未被收藏时显示“添加至收藏夹”；当项目已被收藏时，菜单项必须动态切换为“取消收藏”（或“从收藏夹中移除”），严禁出现硬编码写死单一静态菜单项的架构缺陷。

### 全局操作反馈 Toast (UndoToastOverlay) 停留时长规范
1. **统一 7 秒停留时长红线**：全局所有基于 `UndoToastOverlay` 弹出的轻量级操作反馈、状态通知与可撤销 Snackbar 提示，默认停留时间统一固定为 **7 秒（7000 ms）**，确保用户具备充足的提示阅读与撤销交互响应时间窗口。

### 地址栏面包屑（BreadcrumbBar）矢量图标分隔符顶层规范
1. **硬编码文本符号剔除红线**：地址栏面包屑控件（BreadcrumbBar）文件夹层级之间严禁使用低字号文本字符 `">"` 或硬编码符号作为分隔符，杜绝模糊与字体抗锯齿渲染发虚现象。
2. **矢量 SVG 箭头渲染标准**：层级分隔符必须统一采用标准的矢量 SVG 箭头图标（如 `chevron_right` 图标，图形尺寸 12x12），搭配高对比度前景色（如 `#AAAAAA`），确保在深色背景下保持极佳的清晰度与高质感 UI 风格。
3. **面包屑层级右键菜单快捷收藏规范**：地址栏面包屑中的每一个层级文件夹按钮必须支持右键上下文菜单。右键点击特定层级文件夹时，系统必须基于该文件夹当前在 `FavoritePanel` 中的收藏状态，动态提供“添加至收藏夹”或“取消收藏”选项，实现快捷路径的无缝收藏管理。

### 内容面板（ContentPanel）模块化拆分与高并发选中性能防卡死顶层规范
1. **上帝类拆分解耦红线**：内容面板（ContentPanel）必须遵循单一职责原则，严禁过度堆砌跨领域业务。主面板仅保留视图栈管理与子控制器调度职责，右键上下文菜单构建、物理文件系统操作（复制/剪切/粘贴/删除/重命名）以及选择状态与统计运算必须物理解耦拆分为独立控制器模块。
2. **高并发选中索引计算防卡死与多视图兼容规范**：在处理选中项获取（如 `getSelectedIndexes()`）时，必须兼顾高并发防卡死与视图模式兼容性。在列表视图（QTreeView）下获取 `selectedRows(0)`；在网格视图（QListView/JustifiedView）下仅提取 `column == 0` 的首列单元格索引，严禁因直接返回空列表导致面板间联动中断（如元数据面板无数据），杜绝在成千上万条记录场景下因遍历多列生成巨量 QModelIndex 导致主 UI 线程假死卡顿。

### 核心解耦与单一职责架构顶层规范 (MainWindow, FilterPanel, MetaPanel, MetadataManager)
1. **主窗口 (MainWindow) 拆分规范**：主窗口仅允许承载顶层 UI 布局构建与 QSS 样式加载。全局快捷键捕获与事件分发必须解耦至 `GlobalShortcutController`；多面板（ContentPanel、MetaPanel、FavoritePanel）之间的联动逻辑必须解耦至中介者 `PanelMediator`。
2. **筛选面板 (FilterPanel) 拆分规范**：筛选面板仅保留纯 UI 控件渲染职责。筛选状态管理（`FilterState`）解耦至 `FilterStateModel`；后台文件数量聚合与分类统计解耦至 `ScanStatsEngine`。
3. **属性面板 (MetaPanel) 拆分与 UI 布局架构规范**：属性面板（MetaPanel）头部标题必须保持“元数据属性”并使用 `#4a90e2` 数据库风格；纵向布局必须严格从上至下保持物理顺序：1. 顶部预览与调色板 -> 2. 文件名编辑框 -> 3. 备注说明 -> 4. 关联网址 -> 5. 星级评级与颜色标记（必须配备 `no_color` 清除 ⊘ 图标与全矢量 SVG 图标）-> 6. 标签管理（使用纯矢量 `add` 按钮，严禁使用 `[+]` 文本符号）-> 7. 基础属性（包含加密状态）-> 8. 物理路径（包含“复制路径”与“打开位置”独立物理按钮）。
4. **元数据中心 (MetadataManager) 门面模式规范**：元数据中心作为对外统一门面（Facade），不再直接混合磁盘 IO 与数据库存取。`.QuarkMeta.json` 序列化由 `QuarkMetaJsonStore` 承载；SQLite `global.db` 持久化由 `MetaDbRepository` 承载；内存 LRU 缓存由 `MetaMemoryCache` 承载。

---

## 2. 自研代码文件职责与功能深度剖析 (Self-Developed Source File Responsibilities)

根据 CMake 构建系统 (`CMakeLists.txt`) 的显式注册配置，以下为 `src` 目录下 **200 个** 项目自主研发核心代码文件的详细职责说明（已彻底剔除所有第三方库及第三方嵌入式源码文件）：

## ` src/core/ActionCommand.h `
- **职责数量**：2
- **文件职责**：定义全局可撤销/重做的操作命令抽象基类 ActionCommand；提供 Command 模式统一接口规范。

## ` src/core/AppConfig.h `
- **职责数量**：3
- **文件职责**：工业级全局配置管理单例 AppConfig；物理隔离 QSettings 并解决配置读取身份分裂问题；统一组织与应用配置项。

## ` src/core/BasicCommands.h `
- **职责数量**：3
- **文件职责**：封装系统基础原子操作命令；包含新建文件夹、单文件物理删除、剪切/粘贴等命令逻辑；支持撤销/重做堆栈。

## ` src/core/CentralEventHub.cpp `
- **职责数量**：2
- **文件职责**：全局解耦事件总线 CentralEventHub 的实现；负责模块间的异步信号广播、监听绑定与事件分发。

## ` src/core/CentralEventHub.h `
- **职责数量**：2
- **文件职责**：定义全局强类型应用程序事件枚举与事件结构体；提供事件总线结构规范。

## ` src/core/CoreController.cpp `
- **职责数量**：3
- **文件职责**：核心中控类 CoreController 的实现；连接 UI 视图与底层 Core/Meta 服务；为前端提供统一的异步通知与中控调度接口。

## ` src/core/CoreController.h `
- **职责数量**：2
- **文件职责**：核心中控类 CoreController 的头文件；负责协调底层服务初始化与全局状态管理。

## ` src/core/CoreEngine.cpp `
- **职责数量**：3
- **文件职责**：核心业务大脑 CoreEngine 的实现；统筹资产流转、应用命令封装与调度后台多线程任务。

## ` src/core/CoreEngine.h `
- **职责数量**：2
- **文件职责**：定义强类型应用程序命令类型枚举与 CoreEngine 核心引擎头文件。

## ` src/core/DiskScanService.cpp `
- **职责数量**：3
- **文件职责**：纯磁盘物理扫描服务 DiskScanService 的实现；高效遍历文件系统目录；攒批输出扫描结果并推送到视图层。

## ` src/core/DiskScanService.h `
- **职责数量**：2
- **文件职责**：纯磁盘物理扫描服务 DiskScanService 的头文件；规范磁盘导航扫描的物理隔离红线。

## ` src/core/DiskTrashService.cpp `
- **职责数量**：3
- **文件职责**：磁盘模式物理回收站服务 DiskTrashService 的实现；接管磁盘文件安全删除；管理本地回收站元数据仓储并支持物理还原。

## ` src/core/DiskTrashService.h `
- **职责数量**：2
- **文件职责**：磁盘模式物理回收站服务 DiskTrashService 的头文件；声明双轨隔离的删除、还原与粉碎接口。

## ` src/core/FileFilterService.cpp `
- **职责数量**：2
- **文件职责**：文件过滤服务 FileFilterService 的实现；统一过滤系统隐藏文件、临时缓存及缩略图数据库。

## ` src/core/FileFilterService.h `
- **职责数量**：2
- **文件职责**：文件过滤服务 FileFilterService 的头文件；定义配置与辅助文件排除规则。

## ` src/core/IndexedEntry.cpp `
- **职责数量**：2
- **文件职责**：内存级磁盘条目结构 IndexedEntry 的逻辑实现与辅助转换方法。

## ` src/core/IndexedEntry.h `
- **职责数量**：2
- **文件职责**：内存级磁盘条目结构 IndexedEntry 的声明；定义 MFT 高速扫描与条目索引字段。

## ` src/core/ItemRecord.cpp `
- **职责数量**：2
- **文件职责**：统一文件/资产记录结构体 ItemRecord 的方法实现与数据格式化处理。

## ` src/core/ItemRecord.h `
- **职责数量**：2
- **文件职责**：统一文件/资产记录结构体 ItemRecord 的声明；包含路径、尺寸、修改时间及元数据扩展字段。

## ` src/core/NavigationHistoryService.cpp `
- **职责数量**：3
- **文件职责**：路径导航历史纪录服务 NavigationHistoryService 的实现；记录用户访问路径；维护历史堆栈并广播路径变更信号。

## ` src/core/NavigationHistoryService.h `
- **职责数量**：2
- **文件职责**：路径导航历史纪录服务 NavigationHistoryService 的头文件；管理前进/后退堆栈。

## ` src/core/OperationSnapshotEngine.cpp `
- **职责数量**：3
- **文件职责**：批量操作快照引擎 OperationSnapshotEngine 的实现；记录批量重命名与归类前后的原子快照；支持一键全盘恢复。

## ` src/core/OperationSnapshotEngine.h `
- **职责数量**：2
- **文件职责**：批量操作快照引擎 OperationSnapshotEngine 的头文件；定义状态快照数据结构。

## ` src/core/PhysicalDiskSearchExtractor.cpp `
- **职责数量**：3
- **文件职责**：物理磁盘搜索提取器 PhysicalDiskSearchExtractor 的实现；通过 QDirIterator 执行高并发磁盘搜索；支持攒批限速推送到 UI。

## ` src/core/PhysicalDiskSearchExtractor.h `
- **职责数量**：2
- **文件职责**：物理磁盘搜索提取器 PhysicalDiskSearchExtractor 的头文件；定义搜索参数与接口。

## ` src/core/SearchHistoryService.cpp `
- **职责数量**：2
- **文件职责**：搜索历史服务 SearchHistoryService 的实现；持久化存储搜索关键词历史；提供模糊匹配与热词推荐。

## ` src/core/SearchHistoryService.h `
- **职责数量**：2
- **文件职责**：搜索历史服务 SearchHistoryService 的头文件；管理搜索关键词历史记录。

## ` src/core/UndoManager.h `
- **职责数量**：3
- **文件职责**：全局撤销/重做管理器 UndoManager；采用双栈结构响应全局 Ctrl+Z / Ctrl+Y 操作；管理命令生命周期。

## ` src/core/VolumeOnlineManager.cpp `
- **职责数量**：3
- **文件职责**：物理在线盘符管理器 VolumeOnlineManager 的实现；实时感知系统盘符热插拔；维护物理在线托管盘符集合。

## ` src/core/VolumeOnlineManager.h `
- **职责数量**：2
- **文件职责**：物理在线盘符管理器 VolumeOnlineManager 的头文件；声明盘符状态监听接口。

## ` src/core/commands/BatchRenameCommand.h `
- **职责数量**：2
- **文件职责**：批量重命名撤销命令 BatchRenameCommand；封装批量文件名变更的反向恢复逻辑。

## ` src/core/commands/MetadataCommand.h `
- **职责数量**：2
- **文件职责**：元数据修改撤销命令 MetadataCommand；封装文件标签与属性修改的恢复逻辑。

## ` src/core/commands/MoveCommand.h `
- **职责数量**：2
- **文件职责**：文件移动撤销命令 MoveCommand；封装文件/目录跨路径归类移动的反向恢复逻辑。

## ` src/core/commands/RenameCommand.h `
- **职责数量**：2
- **文件职责**：单文件重命名撤销命令 RenameCommand；封装单个文件名修改的撤销/重做逻辑。

## ` src/core/commands/SecureDeleteCommand.h `
- **职责数量**：2
- **文件职责**：文件粉碎/彻底删除命令 SecureDeleteCommand；封装不可逆数据覆写与彻底删除逻辑。

## ` src/core/commands/ShellProtectionCommand.h `
- **职责数量**：2
- **文件职责**：系统 Shell 文件保护命令 ShellProtectionCommand；防止误删系统关键目录与保护受限制文件。

## ` src/crypto/EncryptionManager.cpp `
- **职责数量**：3
- **文件职责**：文件加解密管理器 EncryptionManager 的实现；调用 Windows BCrypt 对敏感情报/资产进行加密落盘；支持密码校验。

## ` src/crypto/EncryptionManager.h `
- **职责数量**：2
- **文件职责**：文件加解密管理器 EncryptionManager 的头文件；声明对称加解密算法与密钥管理规范。

## ` src/main.cpp `
- **职责数量**：3
- **文件职责**：应用程序主入口。负责初始化 Qt 高 DPI 环境与 QApplication 实例、加载全局 QSS 样式表与日志拦截注册、构建并拉起 MainWindow 主界面。

## ` src/meta/BatchRenameEngine.cpp `
- **职责数量**：3
- **文件职责**：表达式重命名引擎 BatchRenameEngine 的实现；计算批量文件的预览新路径；校验重名冲突与非法字符。

## ` src/meta/BatchRenameEngine.h `
- **职责数量**：2
- **文件职责**：表达式重命名引擎 BatchRenameEngine 的头文件；定义正则替换与序列生成规则。

## ` src/meta/DatabaseManager.cpp `
- **职责数量**：3
- **文件职责**：数据库管理器 DatabaseManager 的实现；管理 SQLite3 数据库连接与线程安全读写；控制事务提交与回滚。

## ` src/meta/DatabaseManager.h `
- **职责数量**：2
- **文件职责**：数据库管理器 DatabaseManager 的头文件；声明连接池与事务管理接口。

## ` src/meta/DatabaseMigrator.h `
- **职责数量**：2
- **文件职责**：数据库版本迁移器 DatabaseMigrator；检测 Schema 版本并自动执行数据库表结构升级脚本。

## ` src/meta/DiskNavigatorService.cpp `
- **职责数量**：2
- **文件职责**：磁盘元数据导航服务 DiskNavigatorService 的实现；将物理磁盘条目与 SQLite 元数据进行快速映射关联。

## ` src/meta/DiskNavigatorService.h `
- **职责数量**：2
- **文件职责**：磁盘元数据导航服务 DiskNavigatorService 的头文件；声明磁盘元数据绑定接口。

## ` src/meta/DiskTrashRepo.cpp `
- **职责数量**：2
- **文件职责**：磁盘回收站仓储 DiskTrashRepo 的实现；负责回收站物理条目的数据库 CRUD 操作。

## ` src/meta/DiskTrashRepo.h `
- **职责数量**：2
- **文件职责**：磁盘回收站仓储 DiskTrashRepo 的头文件；定义物理删除条目的持久化映射结构。

## ` src/meta/DriveMetaDao.cpp `
- **职责数量**：2
- **文件职责**：驱动器元数据 DAO 层 DriveMetaDao 的实现；读写 SQLite 中的驱动器卷标与空间使用元数据。

## ` src/meta/DriveMetaDao.h `
- **职责数量**：2
- **文件职责**：驱动器元数据 DAO 层 DriveMetaDao 的头文件；声明盘符信息持久化接口。

## ` src/meta/DuplicateDetectorService.cpp `
- **职责数量**：3
- **文件职责**：重复文件检测服务 DuplicateDetectorService 的实现；利用文件尺寸与分块哈希算法高效判定重复资产；分组输出冲突列表。

## ` src/meta/DuplicateDetectorService.h `
- **职责数量**：2
- **文件职责**：重复文件检测服务 DuplicateDetectorService 的头文件；声明排重匹配策略。

## ` src/meta/MediaExtractorPipeline.cpp `
- **职责数量**：3
- **文件职责**：多媒体元数据提取流水线 MediaExtractorPipeline 的实现；并发调度图片 EXIF / 音视频 ID3 元数据提取逻辑；批量更新数据库。

## ` src/meta/MediaExtractorPipeline.h `
- **职责数量**：2
- **文件职责**：多媒体元数据提取流水线 MediaExtractorPipeline 的头文件；定义异步提取任务链。

## ` src/meta/MetaCacheDecorator.cpp `
- **职责数量**：2
- **文件职责**：元数据缓存装饰器 MetaCacheDecorator 的实现；在 DAO 层之上增加 LRU 内存缓存；显著提升频繁读取性能。

## ` src/meta/MetaCacheDecorator.h `
- **职责数量**：2
- **文件职责**：元数据缓存装饰器 MetaCacheDecorator 的头文件；定义内存缓存装饰策略。

## ` src/meta/MetadataDefs.h `
- **职责数量**：3
- **文件职责**：元数据全局定义头文件 MetadataDefs.h；定义元数据枚举类型；提供标准元数据 Tag 键值常量声明。

## ` src/meta/MetadataManager.cpp `
- **职责数量**：3
- **文件职责**：元数据中心管理者 MetadataManager 的实现；协调数据库读写、缓存更新与提取流水线；提供统一元数据查询 API。

## ` src/meta/MetadataManager.h `
- **职责数量**：2
- **文件职责**：元数据中心管理者 MetadataManager 的头文件；声明元数据调度总枢纽。

## ` src/meta/QuarkMetaJson.cpp `
- **职责数量**：2
- **文件职责**：JSON 序列化工具 QuarkMetaJson 的实现；将元数据与项目规则导出为格式化 JSON 文件或反序列化导入。

## ` src/meta/QuarkMetaJson.h `
- **职责数量**：2
- **文件职责**：JSON 序列化工具 QuarkMetaJson 的头文件；声明配置与资产导出接口。

## ` src/meta/StatisticsService.cpp `
- **职责数量**：3
- **文件职责**：统计分析服务 StatisticsService 的实现；聚合计算文件类型分布、空间占用趋势与分类占比数据。

## ` src/meta/StatisticsService.h `
- **职责数量**：2
- **文件职责**：统计分析服务 StatisticsService 的头文件；声明空间与类型分析接口。

## ` src/meta/TagRepository.cpp `
- **职责数量**：3
- **文件职责**：标签仓储 TagRepository 的实现；管理自定义标签库；读写文件与标签的关联映射关系。

## ` src/meta/TagRepository.h `
- **职责数量**：2
- **文件职责**：标签仓储 TagRepository 的头文件；声明标签与颜色标注数据库 CRUD 接口。

## ` src/meta/TrashRepository.cpp `
- **职责数量**：2
- **文件职责**：回收站仓储 TrashRepository 的实现；持久化存储已被放入回收站资产的原路径与删除时间信息。

## ` src/meta/TrashRepository.h `
- **职责数量**：2
- **文件职责**：回收站仓储 TrashRepository 的头文件；声明回收站物理条目与元数据映射。

## ` src/ui/AddressBar.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 AddressBar 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/AddressBar.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 AddressBar 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/AddressHistoryPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 AddressHistoryPanel 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/AddressHistoryPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 AddressHistoryPanel 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/BatchCreateDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 BatchCreateDialog 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/BatchCreateDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 BatchCreateDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/BatchProgressDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 BatchProgressDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/BatchRenameDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 BatchRenameDialog 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/BatchRenameDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 BatchRenameDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/BreadcrumbBar.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 BreadcrumbBar 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/BreadcrumbBar.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 BreadcrumbBar 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/CardPainterHelper.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 CardPainterHelper 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/CardPainterHelper.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 CardPainterHelper 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ColorAlgorithmEngine.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ColorAlgorithmEngine 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/ColorAlgorithmEngine.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ColorAlgorithmEngine 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ColorPicker.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ColorPicker 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/ColorPicker.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ColorPicker 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ContentPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ContentPanel 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/ContentPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ContentPanel 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/CreateRuleRow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 CreateRuleRow 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/CreateRuleRow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 CreateRuleRow 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/DiskBatchRenameService.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DiskBatchRenameService 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/DiskBatchRenameService.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DiskBatchRenameService 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/DriveButton.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DriveButton 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/DriveButton.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DriveButton 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/DropJustifiedView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DropJustifiedView 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/DropJustifiedView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DropJustifiedView 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/DropListView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DropListView 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/DropListView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DropListView 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/DropTreeView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DropTreeView 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/DropTreeView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DropTreeView 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/DuplicateConflictDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DuplicateConflictDialog 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/DuplicateConflictDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DuplicateConflictDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ElidedTextUtility.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ElidedTextUtility 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/FavoritePanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FavoritePanel 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/FavoritePanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FavoritePanel 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/FilterPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FilterPanel 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/FilterPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FilterPanel 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/FolderButton.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FolderButton 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/FolderButton.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FolderButton 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/FormatDecoders.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FormatDecoders 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/FormatDecoders.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FormatDecoders 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/FramelessDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessDialog 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/FramelessDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/FramelessDialogBase.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessDialogBase 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/FramelessFileDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessFileDialog 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/FramelessFileDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessFileDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/HoverEventFilter.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 HoverEventFilter 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/HoverEventFilter.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 HoverEventFilter 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/IScanResultView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 IScanResultView 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/IconCacheManager.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 IconCacheManager 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/IconCacheManager.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 IconCacheManager 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ImageDecoderFacade.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ImageDecoderFacade 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/ImageDecoderFacade.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ImageDecoderFacade 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/JustifiedView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 JustifiedView 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/JustifiedView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 JustifiedView 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/Logger.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 Logger 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/MainWindow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 MainWindow 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/MainWindow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 MainWindow 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/MediaColorExtractor.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 MediaColorExtractor 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/MediaColorExtractor.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 MediaColorExtractor 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/MetaPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 MetaPanel 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/MetaPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 MetaPanel 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/NavPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 NavPanel 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/NavPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 NavPanel 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/PresetManager.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 PresetManager 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/PresetManager.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 PresetManager 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/QuickLookGraphicsView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 QuickLookGraphicsView 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/QuickLookGraphicsView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 QuickLookGraphicsView 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/QuickLookMinimap.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 QuickLookMinimap 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/QuickLookMinimap.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 QuickLookMinimap 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/QuickLookWindow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 QuickLookWindow 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/QuickLookWindow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 QuickLookWindow 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ResizeEventFilter.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ResizeEventFilter 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/ResizeEventFilter.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ResizeEventFilter 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/RuleRow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 RuleRow 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/RuleRow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 RuleRow 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/SearchHistoryPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 SearchHistoryPanel 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/SearchHistoryPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 SearchHistoryPanel 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ShellIconManager.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ShellIconManager 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/SvgIconRenderer.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 SvgIconRenderer 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/SvgIconRenderer.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 SvgIconRenderer 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/TagManagerController.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TagManagerController 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/TagManagerController.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TagManagerController 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/TagManagerDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TagManagerDialog 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/TagManagerDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TagManagerDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/TagSelectorOverlay.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TagSelectorOverlay 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/TagSelectorOverlay.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TagSelectorOverlay 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/TaskProgressToolBar.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TaskProgressToolBar 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/TaskProgressToolBar.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TaskProgressToolBar 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ThumbnailDelegate.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ThumbnailDelegate 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/ThumbnailDelegate.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ThumbnailDelegate 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/ToolTipOverlay.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ToolTipOverlay 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/ToolTipOverlay.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ToolTipOverlay 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/TrayController.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TrayController 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/TrayController.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TrayController 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/TreeItemDelegate.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TreeItemDelegate 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/UiHelper.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 UiHelper 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/UndoToastOverlay.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 UndoToastOverlay 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/UndoToastOverlay.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 UndoToastOverlay 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/WindowsShellThumbnailProvider.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 WindowsShellThumbnailProvider 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/WindowsShellThumbnailProvider.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 WindowsShellThumbnailProvider 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/components/ClickableRow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ClickableRow 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/components/ClickableRow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ClickableRow 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/components/ColorPill.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ColorPill 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/components/ColorPill.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ColorPill 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/components/ElasticEdit.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ElasticEdit 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/components/ElasticEdit.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ElasticEdit 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/components/FlowLayout.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FlowLayout 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/components/FlowLayout.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FlowLayout 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/components/StyledCheckBox.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 StyledCheckBox 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/components/StyledCheckBox.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 StyledCheckBox 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/components/TagPill.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TagPill 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/components/TagPill.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 TagPill 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/dialogs/FramelessColorPicker.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessColorPicker 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/dialogs/FramelessColorPicker.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessColorPicker 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/dialogs/FramelessConfirmDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessConfirmDialog 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/dialogs/FramelessConfirmDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessConfirmDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/dialogs/FramelessInputDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessInputDialog 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/dialogs/FramelessInputDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessInputDialog 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/dialogs/FramelessMessageBox.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessMessageBox 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/dialogs/FramelessMessageBox.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 FramelessMessageBox 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/models/DiskItemModel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DiskItemModel 的逻辑实现；处理界面自绘、用户事件及信号槽响应。

## ` src/ui/models/DiskItemModel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 DiskItemModel 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/ui/models/ItemModelBase.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 ItemModelBase 的头文件声明；定义界面属性、布局接口与交互信号。

## ` src/util/DeepThumbnailExtractor.cpp `
- **职责数量**：3
- **文件职责**：深层缩略图提取器 DeepThumbnailExtractor 的实现；利用 ImageDecoderFacade 提取大图及特定格式的高清缩略图缓存。

## ` src/util/DeepThumbnailExtractor.h `
- **职责数量**：2
- **文件职责**：深层缩略图提取器 DeepThumbnailExtractor 的头文件；声明高清缩略图生成规范。

## ` src/util/DiskMediaExtractor.cpp `
- **职责数量**：3
- **文件职责**：媒体色调与信息提取服务 DiskMediaExtractor 的实现；分析图片/音视频色彩分布；提取并计算主导调色板。

## ` src/util/DiskMediaExtractor.h `
- **职责数量**：2
- **文件职责**：媒体色调与信息提取服务 DiskMediaExtractor 的头文件；声明图像主色与色彩代理。

## ` src/util/ShellHelper.cpp `
- **职责数量**：3
- **文件职责**：Windows Shell 系统工具类 ShellHelper 的实现；调用 Win32 Shell API 拉起系统关联程序、定位文件资源管理器及呼出右键菜单。

## ` src/util/ShellHelper.h `
- **职责数量**：2
- **文件职责**：Windows Shell 系统工具类 ShellHelper 的头文件；声明 OS 原生交互 API。

## ` src/util/VolumePathResolver.cpp `
- **职责数量**：3
- **文件职责**：卷路径解析器 VolumePathResolver 的实现；解析盘符 GUID、UNC 网络路径与本地卷标路径之间的互相映射。

## ` src/util/VolumePathResolver.h `
- **职责数量**：2
- **文件职责**：卷路径解析器 VolumePathResolver 的头文件；声明盘符路径转换规范。

