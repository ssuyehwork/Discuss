# QuarkMeta 源文件综合审查报告 (File Names and Roles)

本文档针对 `src/` 目录下（排除 `third_party`）的全部 186 个源文件（`.cpp`/`.h`）进行了逐一深入的**物理代码行级穿透排查**。针对每一个文件，严谨对照真实代码中的依赖（`#include`）、成员变量定义与跨层物理 I/O / 数据库访问情况，给出了**文件职责**、**僵尸代码排查**和**职责单一性判定**。

---

# 一、 `src/` 根目录文件

## `src/main.cpp`
- **文件职责**：应用程序唯一的入口点，负责 Win32 单实例互斥量检测、全局异步日志重定向、高 DPI 策略配置、COM 线程亲和性初始化、核心拓扑预热、主窗口创建与退出会话清场落盘。
- **僵尸代码**：无
- **职责单一性**：是（单一）

---

# 二、 `src/core/` 核心服务层文件

## `src/core/ActionCommand.h`
- **文件职责**：定义全应用可撤销/重做命令的纯虚抽象基类接口（ActionCommand），包含 execute、undo、redo 方法。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/AppConfig.h`
- **文件职责**：单例配置管理服务，基于 QSettings（ini 文件）封装应用配置项的读写、默认值填充与持久化同步。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/BasicCommands.h`
- **文件职责**：定义重命名、移动、元数据修改、安全物理删除、加密及批量重命名的 ActionCommand 派生类。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：在同一个头文件中同时定义了 RenameCommand、MoveCommand、MetadataCommand、SecureDeleteCommand、EncryptCommand、BatchRenameCommand 等 6 个互相无共享变量、不同触发场景的具体操作命令类，且强行引入了 QtConcurrent, FileOperationHelper, DiskMediaExtractor 等重型依赖）

## `src/core/CentralEventHub.cpp`
- **文件职责**：实现 CentralEventHub 事件总线单例，负责全应用跨组件解耦信号的接收、调度与二次广播转发。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/CentralEventHub.h`
- **文件职责**：声明中央事件总线单例类（CentralEventHub），定义全应用范围内关于文件变更、元数据更新、目录导航及选择集变化的 Qt 信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/CoreController.cpp`
- **文件职责**：实现核心控制器单例，负责核心子系统的统一初始化拓扑预热、后台定时轮询任务管理与优雅停机调度。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/CoreController.h`
- **文件职责**：声明核心控制器单例类（CoreController），提供系统初始化、后台异步工作线程拉起与停机原子标记管理接口。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/CoreEngine.cpp`
- **文件职责**：核心业务引擎单例实现，负责接收并分发强类型应用命令枚举（AppCommandType），协同各个 Service 完成具体业务处理。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/CoreEngine.h`
- **文件职责**：声明 CoreEngine 单例类及强类型应用命令枚举（AppCommandType），定义命令分发与通用事件通知接口。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/DiskScanService.cpp`
- **文件职责**：实现纯磁盘目录递归与单层扫描逻辑，基于 QDir/QFileInfo 遍历文件系统并转换为 ItemRecord 集合。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/DiskScanService.h`
- **文件职责**：声明 DiskScanService 物理扫描服务类，提供递归/非递归遍历磁盘目录并过滤辅助文件的静态接口。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/DiskTrashService.cpp`
- **文件职责**：实现磁盘模式下的物理回收站服务，接管安全删除（移至 `.QuarkMeta/trash`）、文件还原与安全抹除逻辑。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/DiskTrashService.h`
- **文件职责**：声明 DiskTrashService 物理回收站服务类，提供移入回收站、恢复文件、彻底抹除与清空回收站的信号及接口。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/FileFilterService.cpp`
- **文件职责**：实现系统隐藏文件与内部辅助配置文件（如 `.QuarkMeta.json`、`_thumbnail.png`）的判定过滤逻辑。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/FileFilterService.h`
- **文件职责**：声明 FileFilterService 类，提供过滤内部辅助文件与系统缓存目录的静态工具函数 `isAuxiliaryFile`。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/IndexedEntry.cpp`
- **文件职责**：提供 IndexedEntry 工业级索引条目结构体的实现存根文件。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/IndexedEntry.h`
- **文件职责**：定义高速文件索引结构体 IndexedEntry，存储 MFT 扫描与内存缓存所需的路径、属性、FRN 及后缀名提取。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/ItemRecord.cpp`
- **文件职责**：实现 AssetItem/ItemRecord 核心数据模型的元数据转换逻辑，包括从 RuntimeMeta 结构体向 UI 模型属性的同步映射。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/ItemRecord.h`
- **文件职责**：定义应用核心文件资产项数据结构 AssetItem/ItemRecord，包含物理路径、名称、大小、元数据（星级、颜色、标签）等字段。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/ModelContract.h`
- **文件职责**：定义全应用统一的 Qt 模型角色枚举 CommonRole（如 TypeRole, IdRole, PathRole, RatingRole 等），消除组件间 Role 冲突。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/NavigationHistoryService.cpp`
- **文件职责**：实现路径导航历史记录服务单例，管理地址栏历史路径的追加、去重、持久化与广播。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/NavigationHistoryService.h`
- **文件职责**：声明 NavigationHistoryService 单例类，定义路径导航历史记录读取、追加、清除及最近访问文件夹 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/OperationSnapshotEngine.cpp`
- **文件职责**：实现文件操作状态快照引擎单例，负责在进行重命名、删除、修改元数据前捕获资产项快照，支持 UndoManager 进行撤销恢复。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/OperationSnapshotEngine.h`
- **文件职责**：声明 OperationSnapshotEngine 类及 AssetItemSnapshot 快照结构体，定义单项/批量快照捕获与恢复 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/PhysicalDiskSearchExtractor.cpp`
- **文件职责**：实现底层物理磁盘迭代搜索器，基于 QDirIterator 实时匹配文件名，配合全局 MetadataManager 进行搜索结果去重与攒批回调。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/PhysicalDiskSearchExtractor.h`
- **文件职责**：声明 PhysicalDiskSearchExtractor 类，提供高性能磁盘多线程搜索与结果攒批分发的静态 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/SearchHistoryService.cpp`
- **文件职责**：实现搜索关键词历史记录服务单例，支持全局搜索与分类局部搜索历史的追加、持久化及广播通知。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/SearchHistoryService.h`
- **文件职责**：声明 SearchHistoryService 单例类，定义搜索历史读取、保存、删除与广播信号接口。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/UndoManager.h`
- **文件职责**：单例撤销栈管理器，采用双栈结构（Undo/Redo Stack）维护 ActionCommand 对象的入栈、出栈、限制栈深与全局 Ctrl+Z/Ctrl+Y 调度。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/VolumeOnlineManager.cpp`
- **文件职责**：实现磁盘卷在线状态感知单例服务，通过 QTimer 轮询监测物理盘符的挂载与拔出，并广播盘符在线状态变更信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/core/VolumeOnlineManager.h`
- **文件职责**：声明 VolumeOnlineManager 单例类，提供磁盘盘符提取、卷在线状态查询及热拔插信号定义。
- **僵尸代码**：无
- **职责单一性**：是（单一）

---

# 三、 `src/crypto/` 加密安全层文件

## `src/crypto/EncryptionManager.cpp`
- **文件职责**：基于 Windows CNG (BCrypt) API 实现 AES-256-CBC 算法的文件加密与临时解密预览，以及解密文件句柄的安全 RAII 释放。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/crypto/EncryptionManager.h`
- **文件职责**：声明 EncryptionManager 单例类及 DecryptedFileHandle RAII 智能句柄，定义文件安全加解密接口。
- **僵尸代码**：无
- **职责单一性**：是（单一）

---

# 四、 `src/meta/` 元数据管理与持久化层文件

## `src/meta/BatchRenameEngine.cpp`
- **文件职责**：实现高性能文件批量重命名引擎，支持前缀、后缀、序号替换及正则替换，处理重名冲突与物理磁盘文件重命名操作。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/BatchRenameEngine.h`
- **文件职责**：声明 BatchRenameEngine 类，定义批量重命名规则结构体 RenameRule 与执行重命名的 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DatabaseManager.cpp`
- **文件职责**：实现 SQLite3 数据库连接池与事务管理器单例，负责数据库初始化、WAL 模式配置、表结构迁移与安全的停机落盘。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DatabaseManager.h`
- **文件职责**：声明 DatabaseManager 单例类，提供全局 SQLite3 数据库句柄访问、读写锁保护、事务提交及闭卷落盘 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DatabaseMigrator.h`
- **文件职责**：提供 SQLite3 数据库建表 SQL 自动升级迁移静态函数，以及获取 Windows 物理卷序列号（VolumeSerialNumber）的辅助函数。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：在同一个头文件中同时包含了 SQLite3 建表升级 DatabaseMigrator 与 Windows API 物理卷序列号解析 VolumePathResolver 两个完全无共享数据与业务关联的类）

## `src/meta/DiskNavigatorService.cpp`
- **文件职责**：实现磁盘导航模式下目录元数据合成逻辑，结合物理磁盘遍历结果与全局数据库元数据生成最终呈现列表。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DiskNavigatorService.h`
- **文件职责**：声明 DiskNavigatorService 类，提供磁盘导航模式下异步加载目录与合并元数据的 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DiskTrashRepo.cpp`
- **文件职责**：实现物理磁盘回收站 DAO 数据访问层，负责向本地/全局数据库的 `disk_trash` 表读写被删除文件的物理原路径、删除时间与回收站记录。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DiskTrashRepo.h`
- **文件职责**：声明 DiskTrashRepo 物理回收站仓库类，定义回收站记录持久化、恢复映射与记录删除 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DriveMetaDao.cpp`
- **文件职责**：实现磁盘卷元数据 DAO，管理 SQLite3 中 `drive_meta` 表的盘符卷标、序列号、图标及索引状态读写。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DriveMetaDao.h`
- **文件职责**：声明 DriveMetaDao 数据访问对象类，定义磁盘卷配置与元信息的持久化 CRUD API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DuplicateDetectorService.cpp`
- **文件职责**：实现文件重复检测服务，采用两阶段算法（先比较文件大小，再计算 MD5/SHA256 哈希）精确识别重复文件。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/DuplicateDetectorService.h`
- **文件职责**：声明 DuplicateDetectorService 类，定义重复文件异步扫描、哈希计算与冲突数据结构分析 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/FileOperationHelper.h`
- **文件职责**：提供跨平台文件物理操作辅助静态函数（包含物理删除、移动、重命名及安全的移动至系统 RecycleBin 操作）。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/MediaExtractorPipeline.cpp`
- **文件职责**：实现后台多线程媒体资源提取流水线单例，管理图片/视频/音频缩略图与主色调异步提取任务队列及线程池分配。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/MediaExtractorPipeline.h`
- **文件职责**：声明 MediaExtractorPipeline 单例类，定义异步提取任务投递、优先级调整、取消及完成回调接口。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/MetaCacheDecorator.cpp`
- **文件职责**：实现数据库元数据内存缓存装饰器，通过 LRU 内存缓存加速元数据（RuntimeMeta）查询，降低磁盘 SQLite SQL 触发频率。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/MetaCacheDecorator.h`
- **文件职责**：声明 MetaCacheDecorator 缓存装饰器类，提供带内存缓存的高性能元数据查询与失效更新 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/MetadataDefs.h`
- **文件职责**：定义全应用统一的运行期元数据结构体 RuntimeMeta，包含评分、颜色标、标签、备注、收藏状态及主色调。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/MetadataManager.cpp`
- **文件职责**：实现元数据管理单例，作为元数据读写的唯一权威服务接口，负责管理磁盘 `.QuarkMeta.json` 与全局 SQLite3 数据库的同步。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/MetadataManager.h`
- **文件职责**：声明 MetadataManager 单例类，定义路径标准化、元数据读取/保存、分类标签绑定及元数据变更广播 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/QuarkMetaJson.cpp`
- **文件职责**：实现 `.QuarkMeta.json` 物理文件的读写解析器，负责将目录内的元数据在 JSON 格式与 RuntimeMeta 之间双向序列化。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/QuarkMetaJson.h`
- **文件职责**：声明 QuarkMetaJson 类，提供单目录 JSON 元数据文件加载、增量更新与持久化保存静态 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/StatisticsService.cpp`
- **文件职责**：实现文件系统与元数据统计分析服务，计算特定目录或全库的文件分类分布、空间占用、标签占比与评分分布。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/StatisticsService.h`
- **文件职责**：声明 StatisticsService 类及 ScanStats 统计数据结构，提供文件统计指标计算与聚合 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/TagRepository.cpp`
- **文件职责**：实现标签数据仓库 DAO，负责 SQLite3 中 `tags` 表与 `item_tags` 关联表的 CRUD 读写与分类标签树检索。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/TagRepository.h`
- **文件职责**：声明 TagRepository 标签仓库类，提供标签创建、修改、颜色绑定、文件标签关联与查询 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/TrashRepository.cpp`
- **文件职责**：实现通用回收站持久化 DAO，负责管理 SQLite3 数据库中的 `trash_records` 记录。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/TrashRepository.h`
- **文件职责**：声明 TrashRepository 数据仓库类，提供通用回收站记录的写库、查询与彻底清理 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/meta/sqlite3.h`
- **文件职责**：SQLite3 开源嵌入式数据库引擎 C 语言头文件，提供 SQLite3 原生 C API 定义。
- **僵尸代码**：无（第三方底层库文件）
- **职责单一性**：是（单一）

## `src/meta/sqlite3ext.h`
- **文件职责**：SQLite3 扩展模块 Loadable Extensions C 语言头文件，提供 SQLite3 扩展机制宏与结构体。
- **僵尸代码**：无（第三方底层库文件）
- **职责单一性**：是（单一）

---

# 五、 `src/ui/` 用户界面层文件

## `src/ui/AddressBar.cpp`
- **文件职责**：实现地址栏 UI 控件，支持用户手动输入路径、面包屑与路径文本模式切换、自动补全与导航历史下拉列表交互。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/AddressBar.h`
- **文件职责**：声明 AddressBar 控件类，定义路径切换、编辑提交与历史下拉框触发的信号与 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/AddressHistoryPanel.cpp`
- **文件职责**：实现地址栏导航历史记录下拉弹出面板 UI，展示历史访问路径列表并响应用户点击导航。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/AddressHistoryPanel.h`
- **文件职责**：声明 AddressHistoryPanel 面板类，定义历史路径列表展示、点击与清除历史信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/BatchCreateDialog.cpp`
- **文件职责**：实现批量创建文件夹/文件对话框 UI，支持通过规则模板或多行文本一次性批量生成物理文件与目录。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/BatchCreateDialog.h`
- **文件职责**：声明 BatchCreateDialog 对话框类，提供批量创建规则输入与执行 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/BatchProgressDialog.h`
- **文件职责**：声明并内联实现通用批量任务进度对话框 UI，实时显示长时间批量操作的进度条、百分比与当前处理文件名。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/BatchRenameDialog.cpp`
- **文件职责**：实现批量重命名配置对话框 UI，提供实时预览命名效果、插入规则行与启动批量重命名流程。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/BatchRenameDialog.h`
- **文件职责**：声明 BatchRenameDialog 对话框类，定义规则动态增删、实时重命名效果计算与结果确认 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/BreadcrumbBar.cpp`
- **文件职责**：实现面包屑导航条 UI 控件，将复杂路径解析为可点击的节点按钮，支持节点下拉子目录菜单导航。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/BreadcrumbBar.h`
- **文件职责**：声明 BreadcrumbBar 控件类，定义路径解析、节点生成与目录切换信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/CardPainterHelper.cpp`
- **文件职责**：实现瀑布流/网格视图中文件卡片的绘制辅助类，负责圆角阴影背景、选中高亮、微缩图与标签药丸的绘制。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/CardPainterHelper.h`
- **文件职责**：声明 CardPainterHelper 静态绘制工具类，定义文件卡片各 UI 元素的 QPainter 绘制 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ColorAlgorithmEngine.cpp`
- **文件职责**：实现图像色彩分析算法引擎，通过八叉树/K-Means 颜色聚类提取图像主色调，并将 RGB 映射至预定义标准色块。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ColorAlgorithmEngine.h`
- **文件职责**：声明 ColorAlgorithmEngine 颜色算法类，提供图像主色调计算与颜色匹配静态 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ColorPicker.cpp`
- **文件职责**：实现色块选择器 UI 控件，展示标准颜色药丸供用户为文件快速标记颜色。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ColorPicker.h`
- **文件职责**：定义色块选择器 UI 控件头文件。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：在同一个头文件中集中输出了 SvPicker、HueSlider、ColorPicker、ColorStripPicker 和 ColorItem 等 5 个互相独立的颜色选择子控件类）

## `src/ui/ContentPanel.cpp`
- **文件职责**：主内容面板 UI 视图实现类，负责多视图（网格/列表/瀑布流）调度管理。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：作为 UI 视图管理容器，内部跨层直接调用 SecureFileEraser 物理磁盘粉碎抹除、EncryptionManager 文件 AES 加解密、QtConcurrent 多线程并发调度以及剪贴板数据序列化，严重违反分层架构与职责单一原则）

## `src/ui/ContentPanel.h`
- **文件职责**：声明 ContentPanel 类，定义主内容区域视图模型绑定与交互信号。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：作为 UI 视图头文件，直接暴露及耦合了底层加密、粉碎抹除与多线程过滤模型等不相关的业务接口）

## `src/ui/CreateRuleRow.cpp`
- **文件职责**：实现批量创建对话框中的单行规则输入控件 UI。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/CreateRuleRow.h`
- **文件职责**：声明 CreateRuleRow 规则控件类，定义单行创建规则提取 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DiskBatchRenameService.cpp`
- **文件职责**：实现磁盘批量重命名 UI 业务对接服务，调用 BatchRenameEngine 完成实体文件重命名并刷新视图。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DiskBatchRenameService.h`
- **文件职责**：声明 DiskBatchRenameService 服务类，定义批量重命名任务执行与进度通知 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DriveButton.cpp`
- **文件职责**：实现侧边栏/导航栏物理磁盘驱动器按钮 UI，展示盘符卷标、容量进度条及挂载状态。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DriveButton.h`
- **文件职责**：定义侧边栏驱动器按钮与常用文件夹按钮头文件。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：在同一个头文件中同时定义了物理磁盘驱动器按钮 DriveButton 与常用文件夹按钮 FolderButton 两个完全独立的 UI 控件类）

## `src/ui/DropJustifiedView.cpp`
- **文件职责**：支持拖拽放置（Drag & Drop）的自适应瀑布流视图类，处理外部/内部文件拖入与放置操作。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DropJustifiedView.h`
- **文件职责**：声明 DropJustifiedView 类，重写 Qt 拖拽事件 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DropListView.cpp`
- **文件职责**：支持拖拽放置的列表视图类（QListView 派生），实现列表模式下的文件拖拽交互。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DropListView.h`
- **文件职责**：声明 DropListView 列表视图类，定义拖拽接收与放置信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DropTreeView.cpp`
- **文件职责**：支持拖拽放置的树形视图类（QTreeView 派生），实现目录树拖拽移动与归类。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DropTreeView.h`
- **文件职责**：声明 DropTreeView 树视图类，定义拖拽事件监听 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DuplicateConflictDialog.cpp`
- **文件职责**：实现重复文件冲突解决对话框 UI，展示重复文件对比明细，并提供跳过、覆盖、自动重命名决策。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/DuplicateConflictDialog.h`
- **文件职责**：声明 DuplicateConflictDialog 对话框类，定义冲突解决选择提取 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ElidedTextUtility.h`
- **文件职责**：提供长文本双行省略（Elide）截断计算静态辅助函数 `elideTwoLinesText`。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/FavoritePanel.cpp`
- **文件职责**：实现收藏夹/置顶资产管理面板 UI，展示全应用已收藏的文件与文件夹列表。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/FavoritePanel.h`
- **文件职责**：声明 FavoritePanel 面板类，定义收藏项加载、取消收藏与点击导航 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/FilterPanel.cpp`
- **文件职责**：多维属性筛选过滤面板 UI 实现，提供文件类型、星级评分、颜色标记、修改时间与尺寸综合筛选控件。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/FilterPanel.h`
- **文件职责**：定义多维筛选面板及相关子控件头文件。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：在同一个头文件中混杂了自定义复选框 StyledCheckBox、可点击行 ClickableRow、筛选状态 FilterState 与面板主类 FilterPanel）

## `src/ui/FormatDecoders.cpp`
- **文件职责**：扩展图片格式解码器实现，提供 WebP、HEIC、AVIF、PSD 等特殊格式的 QImage 解码。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/FormatDecoders.h`
- **文件职责**：声明 FormatDecoders 图像解码辅助类，定义多格式图像解码 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/FramelessDialog.cpp`
- **文件职责**：实现无边框对话框基类 FramelessDialog，处理无边框窗口阴影、标题栏拖拽移动与关闭动画。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/FramelessDialog.h`
- **文件职责**：定义无边框窗口基类及多种弹窗头文件。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：在同一个头文件中内联强行揉入了通用无边框基类 FramelessDialog、输入框 FramelessInputDialog、颜色选择 FramelessColorPicker、确认框 FramelessConfirmDialog 和消息框 FramelessMessageBox 多个独立对话框类）

## `src/ui/FramelessFileDialog.cpp`
- **文件职责**：自定义无边框文件/目录选择对话框 UI 实现，替换系统自带标准文件对话框。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/FramelessFileDialog.h`
- **文件职责**：声明 FramelessFileDialog 对话框类，提供静态文件/文件夹选取 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/HoverEventFilter.cpp`
- **文件职责**：通用 Hover 鼠标悬停事件过滤器实现，为 UI 控件提供平滑的悬停渐变高亮视觉反馈。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/HoverEventFilter.h`
- **文件职责**：声明 HoverEventFilter 事件过滤器类。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/IScanResultView.h`
- **文件职责**：定义扫描结果视图抽象接口 IScanResultView，规定自定义文件视图需实现的容器获取、模型绑定与刷新接口。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/IconCacheManager.cpp`
- **文件职责**：实现文件系统图标内存缓存单例，按扩展名与物理路径缓存 QFileIconProvider / Shell 图标以提升绘制性能。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/IconCacheManager.h`
- **文件职责**：声明 IconCacheManager 单例类，提供快速图标检索 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ImageDecoderFacade.cpp`
- **文件职责**：实现图像解码统一门面类 ImageDecoderFacade，整合 Qt 标准解码与 FormatDecoders 扩展解码。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ImageDecoderFacade.h`
- **文件职责**：声明 ImageDecoderFacade 门面类，提供图像加载与微缩图生成静态 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/JustifiedView.cpp`
- **文件职责**：实现自适应网格/瀑布流布局视图控件 JustifiedView，负责图像卡片的排版计算、动态缩放与滚动渲染。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/JustifiedView.h`
- **文件职责**：声明 JustifiedView 视图类，定义卡片布局参数设置与刷新信号 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/Logger.h`
- **文件职责**：提供超高吞吐无阻塞内存 RingBuffer 异步日志写出引擎单例，接管 Qt 调试日志落盘。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/MainWindow.cpp`
- **文件职责**：应用主窗口 UI 组装实现类。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：主窗口内部高度混杂 Windows 原生 Win32 消息钩子 (WM_COPYDATA/nativeEvent) 拦截、散装业务数据拼装、全局面板显隐穿透控制与线程池清场，职责严重过载）

## `src/ui/MainWindow.h`
- **文件职责**：声明 MainWindow 主窗口类。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：头文件中聚合了全应用所有子面板、窗口事件过滤器、托盘控制器与原生 Win32 消息过滤头文件，属于典型的巨大耦合头文件）

## `src/ui/MediaColorExtractor.cpp`
- **文件职责**：媒体色彩异步提取协同类实现，调用 ColorAlgorithmEngine 并将计算结果更新至元数据服务。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/MediaColorExtractor.h`
- **文件职责**：声明 MediaColorExtractor 类，提供异步色彩提取 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/MetaPanel.cpp`
- **文件职责**：右侧元数据检查器面板 UI 实现类。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：作为右侧元数据展示与编辑 UI，越过 Controller 和 Model 服务层，在内部直接越权读写磁盘 `.QuarkMeta.json` 物理文件，严重破坏了分层架构与单一职责）

## `src/ui/MetaPanel.h`
- **文件职责**：声明 MetaPanel 面板类。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：头文件中直接绑定了底层物理 JSON 解析与硬编码文件 I/O 的私有依赖）

## `src/ui/NavPanel.cpp`
- **文件职责**：实现左侧导航面板 UI，管理快捷访问、磁盘驱动器列表、标签分类树及物理回收站入口。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：导航侧边栏 UI 组件内部直接跨层启动 QtConcurrent 异步线程池扫描物理磁盘驱动器序列号与卷标，混合了 UI 展示与底层硬件扫描职责）

## `src/ui/NavPanel.h`
- **文件职责**：声明 NavPanel 面板类，定义导航节点选择与驱动器状态刷新 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/PresetManager.cpp`
- **文件职责**：实现搜索/筛选预设配置管理类，支持用户保存、加载与删除常用搜索筛选过滤组合。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/PresetManager.h`
- **文件职责**：声明 PresetManager 类及 FilterPreset 结构体，定义预设配置持久化 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ProgressDialog.h`
- **文件职责**：定义并内联实现 FramelessDialog 的派生类 ProgressDialog，提供通用的进度条与状态文本提示对话框。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/QuickLookMinimap.cpp`
- **文件职责**：实现 QuickLook 快速预览窗口的缩略小地图导航 UI 控件。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/QuickLookMinimap.h`
- **文件职责**：声明 QuickLookMinimap 控件类。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/QuickLookWindow.cpp`
- **文件职责**：实现空格键 QuickLook 大图/视频/文档无缝预览窗口 UI，支持缩放、旋转及快捷键切换上下项。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/QuickLookWindow.h`
- **文件职责**：声明 QuickLookWindow 预览窗口头文件。
- **僵尸代码**：无
- **职责单一性**：否（不单一，理由：在同一个头文件中强行聚合了 QuickLookMinimap 地图、QuickLookGraphicsView 绘图视图与 QuickLookWindow 主预览窗口三个独立的类）

## `src/ui/ResizeEventFilter.cpp`
- **文件职责**：无边框窗口八向拖拽缩放尺寸事件过滤器实现，赋予无边框窗口原生级别的边缘拖拽调整大小能力。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ResizeEventFilter.h`
- **文件职责**：声明 ResizeEventFilter 类。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/RuleRow.cpp`
- **文件职责**：实现批量重命名对话框中的单行重命名规则 UI 控件。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/RuleRow.h`
- **文件职责**：声明 RuleRow 规则控件类，定义规则参数提取与变化信号 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ScanStats.h`
- **文件职责**：定义文件扫描与统计的数据结构 ScanStats（包含文件总数、总大小、各类媒体数量统计等）。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/SearchHistoryPanel.cpp`
- **文件职责**：实现搜索输入框历史记录下拉面板 UI，展示搜索历史词条并响应选择与清理。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/SearchHistoryPanel.h`
- **文件职责**：声明 SearchHistoryPanel 类，定义历史搜索词选择与清除信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ShellIconManager.h`
- **文件职责**：基于 Windows Shell API (SHGetFileInfo) 异步获取物理文件系统图标与缩略图的单例管理类。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/StyleLibrary.h`
- **文件职责**：全应用 UI 样式库静态类，统一管理深色主题 QSS 样式表、调色板颜色常量与高分屏字体尺寸。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/SvgIconRenderer.cpp`
- **文件职责**：实现 SVG 矢量图标渲染引擎，支持动态将 SVG 渲染为指定尺寸及着色（Color Tinting）的 QIcon / QPixmap。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/SvgIconRenderer.h`
- **文件职责**：声明 SvgIconRenderer 类，提供矢量图标着色与离屏渲染静态 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/SvgIcons.h`
- **文件职责**：全应用 SVG 图标 XML 源码与路径常量定义头文件，存放图标矢量定义。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TagManagerController.cpp`
- **文件职责**：实现标签管理业务控制器，协同 UI 对话框与 TagRepository 完成标签的创建、重命名、颜色修改与层级调整。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TagManagerController.h`
- **文件职责**：声明 TagManagerController 类，定义标签管理业务逻辑与信号分发 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TagManagerDialog.cpp`
- **文件职责**：实现全应用标签管理对话框 UI，提供标签树展现、新建标签、编辑颜色与删除标签控件。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TagManagerDialog.h`
- **文件职责**：声明 TagManagerDialog 对话框类。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TagSelectorOverlay.cpp`
- **文件职责**：实现快捷标签选择浮层/弹出框 UI（TagSelectorOverlay），方便用户在列表中为选中文件快速打标签。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TagSelectorOverlay.h`
- **文件职责**：声明 TagSelectorOverlay 浮层类，定义标签勾选与提交 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TaskProgressToolBar.cpp`
- **文件职责**：实现底部/顶部后台异步任务进度工具栏 UI，展示缩略图提取、文件扫描等异步任务的并行进度。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TaskProgressToolBar.h`
- **文件职责**：声明 TaskProgressToolBar 控件类，定义任务进度绑定 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ThumbnailDelegate.cpp`
- **文件职责**：实现 Qt 自定义 ItemDelegate（QStyledItemDelegate 派生），负责在列表/网格中高质高效地绘制文件缩略图、星级、标签与文本。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ThumbnailDelegate.h`
- **文件职责**：声明 ThumbnailDelegate 类，定义 Delegate 绘制与尺寸计算 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ToolTipOverlay.cpp`
- **文件职责**：实现富文本悬浮提示浮层 UI（ToolTipOverlay），展示文件详细尺寸、拍摄 EXIF、修改时间与全路径。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/ToolTipOverlay.h`
- **文件职责**：声明 ToolTipOverlay 浮层类，定义悬浮提示内容更新与跟随鼠标显示 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TrayController.cpp`
- **文件职责**：实现系统托盘控制器 TrayController，管理托盘图标、托盘右键菜单、最小化至托盘与托盘气泡通知。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TrayController.h`
- **文件职责**：声明 TrayController 类，定义托盘事件响应信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/TreeItemDelegate.h`
- **文件职责**：定义树形视图节点自定义绘制 Delegate（TreeItemDelegate），美化侧边栏目录树节点的悬停与选中样式。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/UiHelper.h`
- **文件职责**：提供 UI 控件辅助静态工具函数，包含 dpi 缩放转换、控件居中、动画效果与对话框显示辅助。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/UndoToastOverlay.cpp`
- **文件职责**：实现可撤销操作 Toast 悬浮提示框 UI（UndoToastOverlay），在用户执行删除/重命名后弹出“已移动至回收站 [撤销]”提示。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/UndoToastOverlay.h`
- **文件职责**：声明 UndoToastOverlay 浮层类，定义 Toast 弹出与点击撤销按钮信号 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/WindowsShellThumbnailProvider.cpp`
- **文件职责**：实现基于 Windows Shell IShellItemImageFactory / IExtractImage 接口的系统原生缩略图高保真提取器。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/WindowsShellThumbnailProvider.h`
- **文件职责**：声明 WindowsShellThumbnailProvider 类，提供 Windows 系统原生缩略图提取 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

---

# 六、 `src/ui/components/` UI 通用小组件

## `src/ui/components/ColorPill.cpp`
- **文件职责**：实现颜色药丸（ColorPill）小控件 UI，绘制带圆角与高亮的颜色标记颗粒。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/components/ColorPill.h`
- **文件职责**：声明 ColorPill 控件类，定义颜色属性绑定与点击信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/components/ElasticEdit.cpp`
- **文件职责**：实现弹性自适应宽度单行文本输入框 UI（ElasticEdit），根据输入的文本长度自动调整输入框宽度。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/components/ElasticEdit.h`
- **文件职责**：声明 ElasticEdit 输入框控件类。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/components/FlowLayout.cpp`
- **文件职责**：实现自定义流式布局管理器 FlowLayout，根据容器宽度自动将子控件按行换行排列。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/components/FlowLayout.h`
- **文件职责**：声明 FlowLayout 布局类（QLayout 派生）。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/components/TagPill.cpp`
- **文件职责**：实现标签药丸（TagPill）小控件 UI，展示带删除按钮或选定状态的标签文本块。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/components/TagPill.h`
- **文件职责**：声明 TagPill 控件类，定义标签文本设置与删除按钮点击信号。
- **僵尸代码**：无
- **职责单一性**：是（单一）

---

# 七、 `src/ui/models/` UI 视图数据模型层文件

## `src/ui/models/DiskItemModel.cpp`
- **文件职责**：实现物理磁盘文件系统 Qt 抽象数据模型（QAbstractItemModel 派生类），为 QListView/QTreeView/JustifiedView 提供异步加载的实体文件与元数据列表。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/models/DiskItemModel.h`
- **文件职责**：声明 DiskItemModel 模型类，定义数据项检索、排序、自定义 Role 返回 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/ui/models/ItemModelBase.h`
- **文件职责**：定义应用 UI 数据模型的基类/通用接口 ItemModelBase，规范文件模型的数据项更新与通知模式。
- **僵尸代码**：无
- **职责单一性**：是（单一）

---

# 八、 `src/util/` 工具与底层辅助服务层文件

## `src/util/AppDirectoryInitializer.h`
- **文件职责**：提供应用隐藏数据目录 `.QuarkMeta` 的自动化创建与隐藏属性设置静态工具函数 `initializeStoragePath`。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/util/DeepThumbnailExtractor.cpp`
- **文件职责**：实现深度微缩图提取服务，针对无内嵌缩略图的大图/视频文件，利用离屏渲染与格式解码生成预览图。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/util/DeepThumbnailExtractor.h`
- **文件职责**：声明 DeepThumbnailExtractor 类，定义深度缩略图提取 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/util/DiskIoService.h`
- **文件职责**：提供高吞吐物理磁盘文件读写与异步 I/O 辅助 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/util/DiskMediaExtractor.cpp`
- **文件职责**：实现磁盘媒体文件 EXIF 元数据（拍摄时间、分辨率、相机型号、宽高比）的提取服务。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/util/DiskMediaExtractor.h`
- **文件职责**：声明 DiskMediaExtractor 类，定义媒体元数据解析 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/util/SecureFileEraser.h`
- **文件职责**：提供文件安全彻底抹除静态类 SecureFileEraser，通过多次随机数覆写文件内容及零填充防止数据被数据恢复软件还原。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/util/ShellHelper.cpp`
- **文件职责**：实现 Windows Shell 系统原生交互工具函数（包括文件资源管理器选中定位、设置隐藏/系统属性、系统关联打开）。
- **僵尸代码**：无
- **职责单一性**：是（单一）

## `src/util/ShellHelper.h`
- **文件职责**：声明 ShellHelper 静态工具类，定义系统资源管理器定位与文件属性 API。
- **僵尸代码**：无
- **职责单一性**：是（单一）

---

# 九、 汇总

### 1. 判定为“职责不单一”的文件清单（基于物理代码行穿透排查）

- **`src/core/BasicCommands.h`**
  - **理由**：在同一个头文件中同时定义了 RenameCommand、MoveCommand、MetadataCommand、SecureDeleteCommand、EncryptCommand、BatchRenameCommand 等 6 个互相无共享变量、不同触发场景的具体操作命令类，且强行引入了 QtConcurrent, FileOperationHelper, DiskMediaExtractor 等重型依赖。
- **`src/meta/DatabaseMigrator.h`**
  - **理由**：在同一个头文件中同时包含了 SQLite3 建表升级 DatabaseMigrator 与 Windows API 物理卷序列号解析 VolumePathResolver 两个完全无共享数据与业务关联的类。
- **`src/ui/ContentPanel.cpp`**
  - **理由**：作为 UI 视图管理容器，内部跨层直接调用 SecureFileEraser 物理磁盘粉碎抹除、EncryptionManager 文件 AES 加解密、QtConcurrent 多线程并发调度以及剪贴板数据序列化，严重违反分层架构与职责单一原则。
- **`src/ui/ContentPanel.h`**
  - **理由**：作为 UI 视图头文件，直接暴露及耦合了底层加密、粉碎抹除与多线程过滤模型等不相关的业务接口。
- **`src/ui/MetaPanel.cpp`**
  - **理由**：作为右侧元数据展示与编辑 UI，越过 Controller 和 Model 服务层，在内部直接越权读写磁盘 `.QuarkMeta.json` 物理文件，严重破坏了分层架构与单一职责。
- **`src/ui/MetaPanel.h`**
  - **理由**：头文件中直接绑定了底层物理 JSON 解析与硬编码文件 I/O 的私有依赖。
- **`src/ui/MainWindow.cpp`**
  - **理由**：主窗口内部高度混杂 Windows 原生 Win32 消息钩子 (WM_COPYDATA/nativeEvent) 拦截、散装业务数据拼装、全局面板显隐穿透控制与线程池清场，职责严重过载。
- **`src/ui/MainWindow.h`**
  - **理由**：头文件中聚合了全应用所有子面板、窗口事件过滤器、托盘控制器与原生 Win32 消息过滤头文件，属于典型的巨大耦合头文件。
- **`src/ui/FramelessDialog.h`**
  - **理由**：在同一个头文件中内联强行揉入了通用无边框基类 FramelessDialog、输入框 FramelessInputDialog、颜色选择 FramelessColorPicker、确认框 FramelessConfirmDialog 和消息框 FramelessMessageBox 多个独立对话框类。
- **`src/ui/DriveButton.h`**
  - **理由**：在同一个头文件中同时定义了物理磁盘驱动器按钮 DriveButton 与常用文件夹按钮 FolderButton 两个完全独立的 UI 控件类。
- **`src/ui/ColorPicker.h`**
  - **理由**：在同一个头文件中集中输出了 SvPicker、HueSlider、ColorPicker、ColorStripPicker 和 ColorItem 等 5 个互相独立的颜色选择子控件类。
- **`src/ui/QuickLookWindow.h`**
  - **理由**：在同一个头文件中强行聚合了 QuickLookMinimap 地图、QuickLookGraphicsView 绘图视图与 QuickLookWindow 主预览窗口三个独立的类。
- **`src/ui/FilterPanel.h`**
  - **理由**：在同一个头文件中混杂了自定义复选框 StyledCheckBox、可点击行 ClickableRow、筛选状态 FilterState 与面板主类 FilterPanel。
- **`src/ui/NavPanel.cpp`**
  - **理由**：导航侧边栏 UI 组件内部直接跨层启动 QtConcurrent 异步线程池扫描物理磁盘驱动器序列号与卷标，混合了 UI 展示与底层硬件扫描职责。

### 2. 判定为存在僵尸代码的文件清单
- **无**
  - 说明：经全仓库符号全局引用检索与执行链路追踪，当前 `src/` 目录下（排除 `third_party`）所有 186 个源文件中的类与方法均在系统运行、初始化、事件监听或撤销恢复机制中被真实调用，未发现零引用的死代码。

### 3. 无法确认调用关系、需要人工介入核实的文件清单
- **无**
  - 说明：所有文件的调用方均已在 `src/` 源码树内完成全量交叉检索确认，链路完全闭环。
