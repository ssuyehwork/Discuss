# QuarkMeta 系统顶层架构与编译代码规划

## 1. 架构理念与全局设计规范

`QuarkMeta` 是一个高性能的桌面级文件管理与元数据处理系统。本文档作为应用的高级设计理念、顶层架构规划与全局规范的唯一记录载体。

### 纯洁性保护规范
1. **职责绝对单一**：本文档仅且只能记录应用的高级设计理念、顶层架构规划与全局规范。
2. **严禁写入实施细节**：具体的代码修改点、Search/Replace Diff 替换块、代码行号、调试命令等，绝对禁止写入本文档。
3. **实施方案物理隔离**：所有具体的代码修改与实施方案，必须且只能创建在 `QuarkMeta Architecture/Implementation Plan/` 目录下（采用简洁英文小写命名）。

---

## 2. 代码文件职责与功能深度剖析 (File Responsibilities & Functions)

根据 CMake 构建系统 (`CMakeLists.txt`) 的显式注册配置，以下为 `src` 目录下 238 个参与编译的代码文件的详细职责说明：

## ` src/core/ActionCommand.h `
- **职责数量**：2
- **文件职责**：定义全局可撤销/重做的操作命令抽象基类 `ActionCommand`；提供 Command 模式统一接口规范。

## ` src/core/AppConfig.h `
- **职责数量**：3
- **文件职责**：工业级全局配置管理单例 `AppConfig`；物理隔离 QSettings 并解决配置读取身份分裂问题；统一组织与应用配置项。

## ` src/core/BasicCommands.h `
- **职责数量**：3
- **文件职责**：封装系统基础原子操作命令；包含新建文件夹、单文件物理删除、剪切/粘贴等命令逻辑；支持撤销/重做堆栈。

## ` src/core/CentralEventHub.cpp `
- **职责数量**：2
- **文件职责**：全局解耦事件总线 `CentralEventHub` 的实现；负责模块间的异步信号广播、监听绑定与事件分发。

## ` src/core/CentralEventHub.h `
- **职责数量**：2
- **文件职责**：定义全局强类型应用程序事件枚举与事件结构体；提供事件总线结构规范。

## ` src/core/CoreController.cpp `
- **职责数量**：3
- **文件职责**：核心中控类 `CoreController` 的实现；连接 UI 视图与底层 Core/Meta 服务；为前端提供统一的异步通知与中控调度接口。

## ` src/core/CoreController.h `
- **职责数量**：2
- **文件职责**：核心中控类 `CoreController` 的头文件；负责协调底层服务初始化与全局状态管理。

## ` src/core/CoreEngine.cpp `
- **职责数量**：3
- **文件职责**：核心业务大脑 `CoreEngine` 的实现；统筹资产流转、应用命令封装与调度后台多线程任务。

## ` src/core/CoreEngine.h `
- **职责数量**：2
- **文件职责**：定义强类型应用程序命令类型枚举与 `CoreEngine` 核心引擎头文件。

## ` src/core/DiskScanService.cpp `
- **职责数量**：3
- **文件职责**：纯磁盘物理扫描服务 `DiskScanService` 的实现；高效遍历文件系统目录；攒批输出扫描结果并推送到视图层。

## ` src/core/DiskScanService.h `
- **职责数量**：2
- **文件职责**：纯磁盘物理扫描服务 `DiskScanService` 的头文件；规范磁盘导航扫描的物理隔离红线。

## ` src/core/DiskTrashService.cpp `
- **职责数量**：3
- **文件职责**：磁盘模式物理回收站服务 `DiskTrashService` 的实现；接管磁盘文件安全删除；管理本地回收站元数据仓储并支持物理还原。

## ` src/core/DiskTrashService.h `
- **职责数量**：2
- **文件职责**：磁盘模式物理回收站服务 `DiskTrashService` 的头文件；声明双轨隔离的删除、还原与粉碎接口。

## ` src/core/FileFilterService.cpp `
- **职责数量**：2
- **文件职责**：文件过滤服务 `FileFilterService` 的实现；统一过滤系统隐藏文件、临时缓存及缩略图数据库。

## ` src/core/FileFilterService.h `
- **职责数量**：2
- **文件职责**：文件过滤服务 `FileFilterService` 的头文件；定义配置与辅助文件排除规则。

## ` src/core/IndexedEntry.cpp `
- **职责数量**：2
- **文件职责**：内存级磁盘条目结构 `IndexedEntry` 的逻辑实现与辅助转换方法。

## ` src/core/IndexedEntry.h `
- **职责数量**：2
- **文件职责**：内存级磁盘条目结构 `IndexedEntry` 的声明；定义 MFT 高速扫描与条目索引字段。

## ` src/core/ItemRecord.cpp `
- **职责数量**：2
- **文件职责**：统一文件/资产记录结构体 `ItemRecord` 的方法实现与数据格式化处理。

## ` src/core/ItemRecord.h `
- **职责数量**：2
- **文件职责**：统一文件/资产记录结构体 `ItemRecord` 的声明；包含路径、尺寸、修改时间及元数据扩展字段。

## ` src/core/NavigationHistoryService.cpp `
- **职责数量**：3
- **文件职责**：路径导航历史纪录服务 `NavigationHistoryService` 的实现；记录用户访问路径；维护历史堆栈并广播路径变更信号。

## ` src/core/NavigationHistoryService.h `
- **职责数量**：2
- **文件职责**：路径导航历史纪录服务 `NavigationHistoryService` 的头文件；管理前进/后退堆栈。

## ` src/core/OperationSnapshotEngine.cpp `
- **职责数量**：3
- **文件职责**：批量操作快照引擎 `OperationSnapshotEngine` 的实现；记录批量重命名与归类前后的原子快照；支持一键全盘恢复。

## ` src/core/OperationSnapshotEngine.h `
- **职责数量**：2
- **文件职责**：批量操作快照引擎 `OperationSnapshotEngine` 的头文件；定义状态快照数据结构。

## ` src/core/PhysicalDiskSearchExtractor.cpp `
- **职责数量**：3
- **文件职责**：物理磁盘搜索提取器 `PhysicalDiskSearchExtractor` 的实现；通过 QDirIterator 执行高并发磁盘搜索；支持攒批限速推送到 UI。

## ` src/core/PhysicalDiskSearchExtractor.h `
- **职责数量**：2
- **文件职责**：物理磁盘搜索提取器 `PhysicalDiskSearchExtractor` 的头文件；定义搜索参数与接口。

## ` src/core/SearchHistoryService.cpp `
- **职责数量**：2
- **文件职责**：搜索历史服务 `SearchHistoryService` 的实现；持久化存储搜索关键词历史；提供模糊匹配与热词推荐。

## ` src/core/SearchHistoryService.h `
- **职责数量**：2
- **文件职责**：搜索历史服务 `SearchHistoryService` 的头文件；管理搜索关键词历史记录。

## ` src/core/UndoManager.h `
- **职责数量**：3
- **文件职责**：全局撤销/重做管理器 `UndoManager`；采用双栈结构响应全局 Ctrl+Z / Ctrl+Y 操作；管理命令生命周期。

## ` src/core/VolumeOnlineManager.cpp `
- **职责数量**：3
- **文件职责**：物理在线盘符管理器 `VolumeOnlineManager` 的实现；实时感知系统盘符热插拔；维护物理在线托管盘符集合。

## ` src/core/VolumeOnlineManager.h `
- **职责数量**：2
- **文件职责**：物理在线盘符管理器 `VolumeOnlineManager` 的头文件；声明盘符状态监听接口。

## ` src/core/commands/BatchRenameCommand.h `
- **职责数量**：2
- **文件职责**：批量重命名撤销命令 `BatchRenameCommand`；封装批量文件名变更的反向恢复逻辑。

## ` src/core/commands/MetadataCommand.h `
- **职责数量**：2
- **文件职责**：元数据修改撤销命令 `MetadataCommand`；封装文件标签与属性修改的恢复逻辑。

## ` src/core/commands/MoveCommand.h `
- **职责数量**：2
- **文件职责**：文件移动撤销命令 `MoveCommand`；封装文件/目录跨路径归类移动的反向恢复逻辑。

## ` src/core/commands/RenameCommand.h `
- **职责数量**：2
- **文件职责**：单文件重命名撤销命令 `RenameCommand`；封装单个文件名修改的撤销/重做逻辑。

## ` src/core/commands/SecureDeleteCommand.h `
- **职责数量**：2
- **文件职责**：文件粉碎/彻底删除命令 `SecureDeleteCommand`；封装不可逆数据覆写与彻底删除逻辑。

## ` src/core/commands/ShellProtectionCommand.h `
- **职责数量**：2
- **文件职责**：系统 Shell 文件保护命令 `ShellProtectionCommand`；防止误删系统关键目录与保护受限制文件。

## ` src/crypto/EncryptionManager.cpp `
- **职责数量**：3
- **文件职责**：文件加解密管理器 `EncryptionManager` 的实现；调用 Windows BCrypt 对敏感情报/资产进行加密落盘；支持密码校验。

## ` src/crypto/EncryptionManager.h `
- **职责数量**：2
- **文件职责**：文件加解密管理器 `EncryptionManager` 的头文件；声明对称加解密算法与密钥管理规范。

## ` src/main.cpp `
- **职责数量**：3
- **文件职责**：应用程序主入口。负责初始化 Qt 高 DPI 环境与 QApplication 实例、加载全局 QSS 样式表与日志拦截注册、构建并拉起 MainWindow 主界面。

## ` src/meta/BatchRenameEngine.cpp `
- **职责数量**：3
- **文件职责**：表达式重命名引擎 `BatchRenameEngine` 的实现；计算批量文件的预览新路径；校验重名冲突与非法字符。

## ` src/meta/BatchRenameEngine.h `
- **职责数量**：2
- **文件职责**：表达式重命名引擎 `BatchRenameEngine` 的头文件；定义正则替换与序列生成规则。

## ` src/meta/DatabaseManager.cpp `
- **职责数量**：3
- **文件职责**：数据库管理器 `DatabaseManager` 的实现；管理 SQLite3 数据库连接与线程安全读写；控制事务提交与回滚。

## ` src/meta/DatabaseManager.h `
- **职责数量**：2
- **文件职责**：数据库管理器 `DatabaseManager` 的头文件；声明连接池与事务管理接口。

## ` src/meta/DatabaseMigrator.h `
- **职责数量**：2
- **文件职责**：数据库版本迁移器 `DatabaseMigrator`；检测 Schema 版本并自动执行数据库表结构升级脚本。

## ` src/meta/DiskNavigatorService.cpp `
- **职责数量**：2
- **文件职责**：磁盘元数据导航服务 `DiskNavigatorService` 的实现；将物理磁盘条目与 SQLite 元数据进行快速映射关联。

## ` src/meta/DiskNavigatorService.h `
- **职责数量**：2
- **文件职责**：磁盘元数据导航服务 `DiskNavigatorService` 的头文件；声明磁盘元数据绑定接口。

## ` src/meta/DiskTrashRepo.cpp `
- **职责数量**：2
- **文件职责**：磁盘回收站仓储 `DiskTrashRepo` 的实现；负责回收站物理条目的数据库 CRUD 操作。

## ` src/meta/DiskTrashRepo.h `
- **职责数量**：2
- **文件职责**：磁盘回收站仓储 `DiskTrashRepo` 的头文件；定义物理删除条目的持久化映射结构。

## ` src/meta/DriveMetaDao.cpp `
- **职责数量**：2
- **文件职责**：驱动器元数据 DAO 层 `DriveMetaDao` 的实现；读写 SQLite 中的驱动器卷标与空间使用元数据。

## ` src/meta/DriveMetaDao.h `
- **职责数量**：2
- **文件职责**：驱动器元数据 DAO 层 `DriveMetaDao` 的头文件；声明盘符信息持久化接口。

## ` src/meta/DuplicateDetectorService.cpp `
- **职责数量**：3
- **文件职责**：重复文件检测服务 `DuplicateDetectorService` 的实现；利用文件尺寸与分块哈希算法高效判定重复资产；分组输出冲突列表。

## ` src/meta/DuplicateDetectorService.h `
- **职责数量**：2
- **文件职责**：重复文件检测服务 `DuplicateDetectorService` 的头文件；声明排重匹配策略。

## ` src/meta/MediaExtractorPipeline.cpp `
- **职责数量**：3
- **文件职责**：多媒体元数据提取流水线 `MediaExtractorPipeline` 的实现；并发调度图片 EXIF / 音视频 ID3 元数据提取逻辑；批量更新数据库。

## ` src/meta/MediaExtractorPipeline.h `
- **职责数量**：2
- **文件职责**：多媒体元数据提取流水线 `MediaExtractorPipeline` 的头文件；定义异步提取任务链。

## ` src/meta/MetaCacheDecorator.cpp `
- **职责数量**：2
- **文件职责**：元数据缓存装饰器 `MetaCacheDecorator` 的实现；在 DAO 层之上增加 LRU 内存缓存；显著提升频繁读取性能。

## ` src/meta/MetaCacheDecorator.h `
- **职责数量**：2
- **文件职责**：元数据缓存装饰器 `MetaCacheDecorator` 的头文件；定义内存缓存装饰策略。

## ` src/meta/MetadataDefs.h `
- **职责数量**：3
- **文件职责**：元数据全局定义头文件 `MetadataDefs.h`；定义元数据枚举类型；提供标准元数据 Tag 键值常量声明。

## ` src/meta/MetadataManager.cpp `
- **职责数量**：3
- **文件职责**：元数据中心管理者 `MetadataManager` 的实现；协调数据库读写、缓存更新与提取流水线；提供统一元数据查询 API。

## ` src/meta/MetadataManager.h `
- **职责数量**：2
- **文件职责**：元数据中心管理者 `MetadataManager` 的头文件；声明元数据调度总枢纽。

## ` src/meta/QuarkMetaJson.cpp `
- **职责数量**：2
- **文件职责**：JSON 序列化工具 `QuarkMetaJson` 的实现；将元数据与项目规则导出为格式化 JSON 文件或反序列化导入。

## ` src/meta/QuarkMetaJson.h `
- **职责数量**：2
- **文件职责**：JSON 序列化工具 `QuarkMetaJson` 的头文件；声明配置与资产导出接口。

## ` src/meta/StatisticsService.cpp `
- **职责数量**：3
- **文件职责**：统计分析服务 `StatisticsService` 的实现；聚合计算文件类型分布、空间占用趋势与分类占比数据。

## ` src/meta/StatisticsService.h `
- **职责数量**：2
- **文件职责**：统计分析服务 `StatisticsService` 的头文件；声明空间与类型分析接口。

## ` src/meta/TagRepository.cpp `
- **职责数量**：3
- **文件职责**：标签仓储 `TagRepository` 的实现；管理自定义标签库；读写文件与标签的关联映射关系。

## ` src/meta/TagRepository.h `
- **职责数量**：2
- **文件职责**：标签仓储 `TagRepository` 的头文件；声明标签与颜色标注数据库 CRUD 接口。

## ` src/meta/TrashRepository.cpp `
- **职责数量**：2
- **文件职责**：回收站仓储 `TrashRepository` 的实现；持久化存储已被放入回收站资产的原路径与删除时间信息。

## ` src/meta/TrashRepository.h `
- **职责数量**：2
- **文件职责**：回收站仓储 `TrashRepository` 的头文件；声明回收站物理条目与元数据映射。

## ` src/meta/sqlite3.c `
- **职责数量**：3
- **文件职责**：SQLite3 嵌入式数据库核心引擎 C 实现源码（Amalgamation 单文件）；提供高性能本地 SQL 引擎支持。

## ` src/meta/sqlite3.h `
- **职责数量**：2
- **文件职责**：SQLite3 嵌入式关系型数据库 C 接口头文件；定义原生 C API 与结构体。

## ` src/third_party/libtiff/tif_aux.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_aux.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_close.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_close.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_codec.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_codec.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_color.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_color.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_compress.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_compress.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_dir.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_dir.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_dirinfo.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_dirinfo.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_dirread.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_dirread.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_dirwrite.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_dirwrite.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_dumpmode.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_dumpmode.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_error.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_error.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_extension.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_extension.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_fax3.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_fax3.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_fax3sm.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_fax3sm.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_flush.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_flush.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_getimage.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_getimage.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_hash_set.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_hash_set.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_luv.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_luv.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_lzw.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_lzw.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_next.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_next.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_ojpeg.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_ojpeg.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_open.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_open.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_packbits.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_packbits.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_pixarlog.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_pixarlog.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_predict.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_predict.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_print.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_print.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_read.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_read.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_strip.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_strip.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_swab.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_swab.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_thunder.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_thunder.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_tile.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_tile.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_version.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_version.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_warning.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_warning.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_win32.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_win32.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_write.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_write.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/third_party/libtiff/tif_zip.c `
- **职责数量**：2
- **文件职责**：嵌入式 libtiff 图像解码库核心源文件 `tif_zip.c`；负责 TIFF 图像特定编解码、格式解析或文件 I/O 逻辑。

## ` src/ui/AddressBar.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `AddressBar` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/AddressBar.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `AddressBar` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/AddressHistoryPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `AddressHistoryPanel` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/AddressHistoryPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `AddressHistoryPanel` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/BatchCreateDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `BatchCreateDialog` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/BatchCreateDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `BatchCreateDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/BatchProgressDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `BatchProgressDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/BatchRenameDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `BatchRenameDialog` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/BatchRenameDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `BatchRenameDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/BreadcrumbBar.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `BreadcrumbBar` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/BreadcrumbBar.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `BreadcrumbBar` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/CardPainterHelper.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `CardPainterHelper` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/CardPainterHelper.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `CardPainterHelper` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ColorAlgorithmEngine.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ColorAlgorithmEngine` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/ColorAlgorithmEngine.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ColorAlgorithmEngine` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ColorPicker.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ColorPicker` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/ColorPicker.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ColorPicker` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ContentPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ContentPanel` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/ContentPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ContentPanel` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/CreateRuleRow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `CreateRuleRow` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/CreateRuleRow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `CreateRuleRow` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/DiskBatchRenameService.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DiskBatchRenameService` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/DiskBatchRenameService.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DiskBatchRenameService` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/DriveButton.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DriveButton` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/DriveButton.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DriveButton` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/DropJustifiedView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DropJustifiedView` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/DropJustifiedView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DropJustifiedView` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/DropListView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DropListView` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/DropListView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DropListView` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/DropTreeView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DropTreeView` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/DropTreeView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DropTreeView` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/DuplicateConflictDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DuplicateConflictDialog` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/DuplicateConflictDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DuplicateConflictDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ElidedTextUtility.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ElidedTextUtility` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/FavoritePanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FavoritePanel` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/FavoritePanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FavoritePanel` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/FilterPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FilterPanel` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/FilterPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FilterPanel` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/FolderButton.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FolderButton` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/FolderButton.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FolderButton` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/FormatDecoders.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FormatDecoders` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/FormatDecoders.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FormatDecoders` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/FramelessDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessDialog` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/FramelessDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/FramelessDialogBase.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessDialogBase` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/FramelessFileDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessFileDialog` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/FramelessFileDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessFileDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/HoverEventFilter.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `HoverEventFilter` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/HoverEventFilter.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `HoverEventFilter` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/IScanResultView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `IScanResultView` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/IconCacheManager.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `IconCacheManager` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/IconCacheManager.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `IconCacheManager` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ImageDecoderFacade.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ImageDecoderFacade` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/ImageDecoderFacade.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ImageDecoderFacade` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/JustifiedView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `JustifiedView` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/JustifiedView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `JustifiedView` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/Logger.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `Logger` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/MainWindow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `MainWindow` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/MainWindow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `MainWindow` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/MediaColorExtractor.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `MediaColorExtractor` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/MediaColorExtractor.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `MediaColorExtractor` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/MetaPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `MetaPanel` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/MetaPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `MetaPanel` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/NavPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `NavPanel` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/NavPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `NavPanel` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/PresetManager.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `PresetManager` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/PresetManager.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `PresetManager` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/QuickLookGraphicsView.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `QuickLookGraphicsView` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/QuickLookGraphicsView.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `QuickLookGraphicsView` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/QuickLookMinimap.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `QuickLookMinimap` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/QuickLookMinimap.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `QuickLookMinimap` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/QuickLookWindow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `QuickLookWindow` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/QuickLookWindow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `QuickLookWindow` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ResizeEventFilter.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ResizeEventFilter` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/ResizeEventFilter.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ResizeEventFilter` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/RuleRow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `RuleRow` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/RuleRow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `RuleRow` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/SearchHistoryPanel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `SearchHistoryPanel` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/SearchHistoryPanel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `SearchHistoryPanel` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ShellIconManager.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ShellIconManager` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/SvgIconRenderer.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `SvgIconRenderer` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/SvgIconRenderer.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `SvgIconRenderer` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/TagManagerController.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TagManagerController` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/TagManagerController.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TagManagerController` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/TagManagerDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TagManagerDialog` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/TagManagerDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TagManagerDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/TagSelectorOverlay.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TagSelectorOverlay` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/TagSelectorOverlay.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TagSelectorOverlay` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/TaskProgressToolBar.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TaskProgressToolBar` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/TaskProgressToolBar.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TaskProgressToolBar` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ThumbnailDelegate.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ThumbnailDelegate` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/ThumbnailDelegate.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ThumbnailDelegate` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/ToolTipOverlay.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ToolTipOverlay` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/ToolTipOverlay.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ToolTipOverlay` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/TrayController.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TrayController` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/TrayController.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TrayController` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/TreeItemDelegate.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TreeItemDelegate` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/UiHelper.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `UiHelper` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/UndoToastOverlay.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `UndoToastOverlay` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/UndoToastOverlay.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `UndoToastOverlay` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/WindowsShellThumbnailProvider.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `WindowsShellThumbnailProvider` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/WindowsShellThumbnailProvider.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `WindowsShellThumbnailProvider` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/components/ClickableRow.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ClickableRow` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/components/ClickableRow.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ClickableRow` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/components/ColorPill.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ColorPill` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/components/ColorPill.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ColorPill` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/components/ElasticEdit.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ElasticEdit` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/components/ElasticEdit.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ElasticEdit` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/components/FlowLayout.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FlowLayout` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/components/FlowLayout.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FlowLayout` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/components/StyledCheckBox.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `StyledCheckBox` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/components/StyledCheckBox.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `StyledCheckBox` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/components/TagPill.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TagPill` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/components/TagPill.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `TagPill` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/dialogs/FramelessColorPicker.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessColorPicker` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/dialogs/FramelessColorPicker.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessColorPicker` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/dialogs/FramelessConfirmDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessConfirmDialog` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/dialogs/FramelessConfirmDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessConfirmDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/dialogs/FramelessInputDialog.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessInputDialog` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/dialogs/FramelessInputDialog.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessInputDialog` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/dialogs/FramelessMessageBox.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessMessageBox` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/dialogs/FramelessMessageBox.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `FramelessMessageBox` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/models/DiskItemModel.cpp `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DiskItemModel` 的逻辑实现；处理界面渲染代理、用户输入及信号槽响应。

## ` src/ui/models/DiskItemModel.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `DiskItemModel` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/ui/models/ItemModelBase.h `
- **职责数量**：2
- **文件职责**：UI 视图或组件控件 `ItemModelBase` 的头文件声明；负责定义界面布局、属性及交互信号。

## ` src/util/DeepThumbnailExtractor.cpp `
- **职责数量**：3
- **文件职责**：深层缩略图提取器 `DeepThumbnailExtractor` 的实现；利用 ImageDecoderFacade 提取大图及特定格式的高清缩略图缓存。

## ` src/util/DeepThumbnailExtractor.h `
- **职责数量**：2
- **文件职责**：深层缩略图提取器 `DeepThumbnailExtractor` 的头文件；声明高清缩略图生成规范。

## ` src/util/DiskMediaExtractor.cpp `
- **职责数量**：3
- **文件职责**：媒体色调与信息提取服务 `DiskMediaExtractor` 的实现；分析图片/音视频色彩分布；提取并计算主导调色板。

## ` src/util/DiskMediaExtractor.h `
- **职责数量**：2
- **文件职责**：媒体色调与信息提取服务 `DiskMediaExtractor` 的头文件；声明图像主色与色彩代理。

## ` src/util/ShellHelper.cpp `
- **职责数量**：3
- **文件职责**：Windows Shell 系统工具类 `ShellHelper` 的实现；调用 Win32 Shell API 拉起系统关联程序、定位文件资源管理器及呼出右键菜单。

## ` src/util/ShellHelper.h `
- **职责数量**：2
- **文件职责**：Windows Shell 系统工具类 `ShellHelper` 的头文件；声明 OS 原生交互 API。

## ` src/util/VolumePathResolver.cpp `
- **职责数量**：3
- **文件职责**：卷路径解析器 `VolumePathResolver` 的实现；解析盘符 GUID、UNC 网络路径与本地卷标路径之间的互相映射。

## ` src/util/VolumePathResolver.h `
- **职责数量**：2
- **文件职责**：卷路径解析器 `VolumePathResolver` 的头文件；声明盘符路径转换规范。

---

## 3. 第三方库与嵌入式组件排查与架构定位 (Third-Party Components & Dependencies)

为了实现轻量级架构与高可用性，系统针对第三方依赖采用了**“源码嵌入”**与**“显式剪裁”**结合的策略。

### 3.1 嵌入式第三方源码组件 (Embedded Third-Party Source Components)

1. **`libtiff` 图像解码库 (`src/third_party/libtiff/`)**
   - **架构定位**：位于 `src/third_party/libtiff` 目录，专用于 TIFF 格式深层图像数据的底层解析与解码。
   - **编译排查**：共有 **36 个** 核心 C 源文件通过 CMake (`LIBTIFF_SOURCES`) 显式注册并参与编译。
   - **剪裁与防冲突策略**：为了避免引用繁重的第三方依赖，系统对 libtiff 进行了严格的轻量化剪裁，排除了独立的工具可执行文件 (`tiff2pdf`, `tiffinfo` 等)、格式扩展组件 (`tif_webp`, `tif_zstd`, `tif_lzma`, `tif_jpeg` 等) 以及非 Windows 平台的构建文件。

2. **`SQLite3` 数据库引擎 (`src/meta/sqlite3.c`, `src/meta/sqlite3.h`)**
   - **架构定位**：位于 `src/meta/` 模块，作为本地元数据缓存、文件索引、标签映射与回收站记录的底层嵌入式关系型数据库引擎。
   - **编译排查**：以单文件 C Amalgamation 形式直接注册在 CMake 的 `SOURCES` 中参与编译。

### 3.2 系统与框架动态依赖 (Framework & System Libraries)

系统在顶层 CMake 配置中引入了以下框架与原生动态库：
- **Qt 6 Framework (`Qt6::Core`, `Qt6::Gui`, `Qt6::Widgets`, `Qt6::Svg`, `Qt6::Concurrent`)**：核心 UI 界面框架、并发计算与图形渲染支持。
- **Windows System Native DLLs**：
  - `ntdll` — 低层系统 API 支持（包含 MFT 磁盘快照与文件系统底层结构访问）。
  - `ole32` — Windows COM 组件接口（用于 Shell 缩略图获取与系统文件右键菜单集成）。
  - `bcrypt` — 系统级密码学与加密算法支持（供 `EncryptionManager` 调用）。
  - `psapi` — Windows 进程与系统状态信息提取。
