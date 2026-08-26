# QuarkMeta 系统顶层架构与编译代码规划

## 1. 架构理念与全局设计规范

`QuarkMeta` 是一个高性能的桌面级文件管理与元数据处理系统。本文档作为应用的高级设计理念、顶层架构规划与全局规范的唯一记录载体。

### 纯洁性保护规范
1. **职责绝对单一**：本文档仅且只能记录应用的高级设计理念、顶层架构规划与全局规范。
2. **严禁写入实施细节**：具体的代码修改点、Search/Replace Diff 替换块、代码行号、调试命令等，绝对禁止写入本文档。
3. **实施方案物理隔离**：所有具体的代码修改与实施方案，必须且只能创建在 `QuarkMeta Architecture/Implementation Plan/` 目录下（采用简洁英文小写命名）。

---

## 2. `src` 目录参与编译的代码架构与逐文件职责规划 (Compiled Code File Responsibilities)

根据 CMake 构建系统 (`CMakeLists.txt`) 的显式注册配置，`src` 目录下共有 **238 个** 代码文件参与实际构建与编译。以下逐文件梳理其承载的核心职责与功能定位：

### 2.1 应用程序入口 (Root Entry) [1]
- **`src/main.cpp`** — 应用程序主入口。负责初始化 Qt 环境（高 DPI 缩放、QApplication 示例拉起）、全局样式表加载、日志拦截注册以及 MainWindow 主界面的构建与显示。

### 2.2 核心逻辑与控制层 (Core Module - `src/core/`) [36]
负责底层事件调度、磁盘扫描、检索、撤销/重做管理以及操作命令封装。

- **`src/core/ActionCommand.h`** — 定义全局可撤销/重做的操作命令抽象基类 `ActionCommand`。
- **`src/core/AppConfig.h`** — 应用程序工业级全局配置管理单例 `AppConfig`，物理隔离 QSettings 并统一配置项。
- **`src/core/BasicCommands.h`** — 封装系统基础原子命令（新建文件夹、单文件删除、文件剪切/粘贴等）。
- **`src/core/CentralEventHub.h` / `.cpp`** — 全局强类型解耦事件总线 `CentralEventHub`，负责模块间的异步信号广播与事件分发。
- **`src/core/CoreController.h` / `.cpp`** — 核心中控类 `CoreController`，作为 UI 与底层 Core/Meta 服务交互的总枢纽。
- **`src/core/CoreEngine.h` / `.cpp`** — 核心逻辑引擎 `CoreEngine`，负责处理资产流转、应用命令调度与任务执行。
- **`src/core/DiskScanService.h` / `.cpp`** — 纯磁盘文件系统深度导航扫描服务 `DiskScanService`，支持并行文件系统遍历。
- **`src/core/DiskTrashService.h` / `.cpp`** — 磁盘导航模式下的物理回收站服务 `DiskTrashService`，接管双轨物理隔离删除与还原。
- **`src/core/FileFilterService.h` / `.cpp`** — 全局文件与目录过滤服务 `FileFilterService`，排除系统隐藏文件及日志缓存。
- **`src/core/IndexedEntry.h` / `.cpp`** — 内存级磁盘条目索引结构 `IndexedEntry`，支持极速 MFT 扫描与高速搜索。
- **`src/core/ItemRecord.h` / `.cpp`** — 统一文件/资产数据结构 `ItemRecord`，封装路径、尺寸、修改时间、元数据及 UI 标记。
- **`src/core/NavigationHistoryService.h` / `.cpp`** — 路径导航历史纪录服务 `NavigationHistoryService`，管理路径的前进/后退堆栈。
- **`src/core/OperationSnapshotEngine.h` / `.cpp`** — 批量操作快照引擎 `OperationSnapshotEngine`，记录批量重命名与分类的状态快照。
- **`src/core/PhysicalDiskSearchExtractor.h` / `.cpp`** — 物理磁盘文件搜索与提取引擎 `PhysicalDiskSearchExtractor`，实现攒批限速与异步搜索。
- **`src/core/SearchHistoryService.h` / `.cpp`** — 搜索历史持久化与检索服务 `SearchHistoryService`。
- **`src/core/UndoManager.h`** — 全局撤销/重做栈管理器 `UndoManager`，采用双栈结构响应全局 Ctrl+Z/Ctrl+Y 操作。
- **`src/core/VolumeOnlineManager.h` / `.cpp`** — 物理在线盘符监听与托管服务 `VolumeOnlineManager`，感知热插拔与盘符状态。
- **`src/core/commands/BatchRenameCommand.h`** — 批量文件重命名撤销命令封装。
- **`src/core/commands/MetadataCommand.h`** — 文件元数据/标签修改撤销命令封装。
- **`src/core/commands/MoveCommand.h`** — 文件物理移动/归类撤销命令封装。
- **`src/core/commands/RenameCommand.h`** — 单文件重命名撤销命令封装。
- **`src/core/commands/SecureDeleteCommand.h`** — 文件粉碎/安全彻底删除命令封装。
- **`src/core/commands/ShellProtectionCommand.h`** — 系统 Shell 文件保护与安全防误删命令封装。

### 2.3 元数据与数据持久化层 (Meta Module - `src/meta/`) [30]
负责 SQLite 数据库交互、元数据解析提取、标签库、重命名引擎以及重复文件检测。

- **`src/meta/BatchRenameEngine.h` / `.cpp`** — 表达式重命名与正则替换引擎 `BatchRenameEngine`，计算新文件名序列。
- **`src/meta/DatabaseManager.h` / `.cpp`** — SQLite3 数据库连接池与事务管理器 `DatabaseManager`。
- **`src/meta/DatabaseMigrator.h`** — 数据库 Schema 版本迁移与自动升级器 `DatabaseMigrator`。
- **`src/meta/DiskNavigatorService.h` / `.cpp`** — 磁盘导航与元数据映射服务 `DiskNavigatorService`。
- **`src/meta/DiskTrashRepo.h` / `.cpp`** — 回收站元数据仓储 `DiskTrashRepo`，持久化物理删除条目的原始路径信息。
- **`src/meta/DriveMetaDao.h` / `.cpp`** — 驱动器与卷标元数据 DAO 层 `DriveMetaDao`。
- **`src/meta/DuplicateDetectorService.h` / `.cpp`** — 重复文件高效检测服务 `DuplicateDetectorService`，通过文件哈希与尺寸快速判重。
- **`src/meta/MediaExtractorPipeline.h` / `.cpp`** — 多媒体元数据异步提取流水线 `MediaExtractorPipeline`。
- **`src/meta/MetaCacheDecorator.h` / `.cpp`** — 元数据内存缓存装饰器 `MetaCacheDecorator`，提供二次查询加速。
- **`src/meta/MetadataDefs.h`** — 元数据类型定义、分类常量与 EXIF/ID3 字段映射标准。
- **`src/meta/MetadataManager.h` / `.cpp`** — 全局元数据中心管理者 `MetadataManager`，调度提取、缓存与更新。
- **`src/meta/QuarkMetaJson.h` / `.cpp`** — 应用程序 JSON 导入导出与配置序列化工具 `QuarkMetaJson`。
- **`src/meta/sqlite3.h` / `.c`** — 嵌入式关系型数据库 SQLite3 引擎核心源码（Amalgamation 单文件库）。
- **`src/meta/StatisticsService.h` / `.cpp`** — 存储空间、文件类型分布及资产分析统计服务 `StatisticsService`。
- **`src/meta/TagRepository.h` / `.cpp`** — 标签与颜色标注仓储 `TagRepository`，管理标签 CRUD 与文件绑定关系。
- **`src/meta/TrashRepository.h` / `.cpp`** — 物理/逻辑回收站条目持久化仓储 `TrashRepository`。

### 2.4 数据安全与加密层 (Crypto Module - `src/crypto/`) [2]
负责数据的加解密与安全管理。

- **`src/crypto/EncryptionManager.h` / `.cpp`** — 文件加解密与敏感资产安全管理引擎 `EncryptionManager`，支持密码与密钥管理。

### 2.5 系统工具与通用服务 (Util Module - `src/util/`) [8]
提供系统 Shell 对接、深层缩略图提取及卷路径解析服务。

- **`src/util/DeepThumbnailExtractor.h` / `.cpp`** — 深层缩略图与大图异步提取工具 `DeepThumbnailExtractor`。
- **`src/util/DiskMediaExtractor.h` / `.cpp`** — 视频/音频媒体主色调与缩略图提取服务 `DiskMediaExtractor`。
- **`src/util/ShellHelper.h` / `.cpp`** — Windows OS Shell 系统原生 API 封装 `ShellHelper`（如打开文件位置、右键菜单拉起、系统关联打开）。
- **`src/util/VolumePathResolver.h` / `.cpp`** — 盘符 GUID、UNC 路径与卷标绝对路径解析器 `VolumePathResolver`。

### 2.6 第三方图像解码库 (Third Party Module - `src/third_party/libtiff/`) [36]
嵌入式 libtiff 核心图像解码组件。

- **`src/third_party/libtiff/tif_aux.c`** — TIFF 辅助解析工具与数据类型支持。
- **`src/third_party/libtiff/tif_close.c`** — TIFF句柄释放与文件关闭清理。
- **`src/third_party/libtiff/tif_codec.c`** — 编解码器注册与调度接口。
- **`src/third_party/libtiff/tif_color.c`** — YCbCr 与 RGB 颜色空间转换实现。
- **`src/third_party/libtiff/tif_compress.c`** — 压缩算法调度与缓冲区处理。
- **`src/third_party/libtiff/tif_dir.c`** — Directory (IFD) 目录项读取与修改。
- **`src/third_party/libtiff/tif_dirinfo.c`** — IFD Tag 信息注册与字典管理。
- **`src/third_party/libtiff/tif_dirread.c`** — IFD 目录与标签解包读取逻辑。
- **`src/third_party/libtiff/tif_dirwrite.c`** — IFD 目录写入与结构生成。
- **`src/third_party/libtiff/tif_dumpmode.c`** — 原始未压缩 (Dump Mode) 编码器。
- **`src/third_party/libtiff/tif_error.c`** — 错误日志与异常回调处理。
- **`src/third_party/libtiff/tif_extension.c`** — 扩展 Tag 字段定制。
- **`src/third_party/libtiff/tif_fax3.c`** — Group 3 / Group 4 传真图像解码器。
- **`src/third_party/libtiff/tif_fax3sm.c`** — 传真解码状态机。
- **`src/third_party/libtiff/tif_flush.c`** — 文件刷盘与写入缓冲区同步。
- **`src/third_party/libtiff/tif_getimage.c`** — RGBA 图像解码核心转换逻辑。
- **`src/third_party/libtiff/tif_hash_set.c`** — 内存哈希集合，加速 Tag 检索。
- **`src/third_party/libtiff/tif_luv.c`** — LogL/LogLuv 高动态范围颜色编码。
- **`src/third_party/libtiff/tif_lzw.c`** — LZW 无损压缩与解压实现。
- **`src/third_party/libtiff/tif_next.c`** — NeXT 图像格式解压。
- **`src/third_party/libtiff/tif_ojpeg.c`** — Old JPEG 格式相容性解码。
- **`src/third_party/libtiff/tif_open.c`** — TIFF 打开与模式匹配。
- **`src/third_party/libtiff/tif_packbits.c`** — PackBits 游程编码解压。
- **`src/third_party/libtiff/tif_pixarlog.c`** — Pixar Log11 编码解压。
- **`src/third_party/libtiff/tif_predict.c`** — 差分预测器。
- **`src/third_party/libtiff/tif_print.c`** — TIFF 结构控制台与调试打印。
- **`src/third_party/libtiff/tif_read.c`** — 基础 Strip / Tile 像素数据读取。
- **`src/third_party/libtiff/tif_strip.c`** — Strip 分块索引计算与管理。
- **`src/third_party/libtiff/tif_swab.c`** — 大小端字节序转换。
- **`src/third_party/libtiff/tif_thunder.c`** — ThunderScan 游程解码。
- **`src/third_party/libtiff/tif_tile.c`** — Tile 瓦片索引计算与切片读取。
- **`src/third_party/libtiff/tif_version.c`** — LibTIFF 版本信息查询。
- **`src/third_party/libtiff/tif_warning.c`** — 警告日志回调机制。
- **`src/third_party/libtiff/tif_win32.c`** — Windows Win32 平台 I/O 句柄实现。
- **`src/third_party/libtiff/tif_write.c`** — 基础 Strip / Tile 写入实现。
- **`src/third_party/libtiff/tif_zip.c`** — Deflate / ZIP 解压支持。

### 2.7 视图与用户交互界面层 (UI Module - `src/ui/`) [125]
实现基于 Qt 的自定 UI 控件、对话框、布局引擎与渲染代理。

- **导航与地址栏关联控件**：
  - `src/ui/AddressBar.h` / `.cpp` — 交互式地址栏控件 `AddressBar`，支持路径点击分节、路径输入与自动补全。
  - `src/ui/AddressHistoryPanel.h` / `.cpp` — 地址历史下拉面板 `AddressHistoryPanel`。
  - `src/ui/BreadcrumbBar.h` / `.cpp` — 面包屑路径导航栏 `BreadcrumbBar`。
  - `src/ui/NavPanel.h` / `.cpp` — 主界面左侧导航树面板 `NavPanel`，包含磁盘列表、快捷分类与收藏夹。
  - `src/ui/FavoritePanel.h` / `.cpp` — 收藏夹快捷管理面板 `FavoritePanel`。

- **视图展示与自定义布局组件**：
  - `src/ui/ContentPanel.h` / `.cpp` — 主内容展示面板 `ContentPanel`，调度列表、树形与瀑布流模式。
  - `src/ui/JustifiedView.h` / `.cpp` — 瀑布流/等高自适应图像网格视图 `JustifiedView`。
  - `src/ui/DropJustifiedView.h` / `.cpp` — 支持文件拖拽放落的等高瀑布流视图 `DropJustifiedView`。
  - `src/ui/DropListView.h` / `.cpp` — 支持拖拽放落的列表视图 `DropListView`。
  - `src/ui/DropTreeView.h` / `.cpp` — 支持拖拽放落的树形目录视图 `DropTreeView`。
  - `src/ui/MetaPanel.h` / `.cpp` — 侧边栏文件元数据与 EXIF 信息显示面板 `MetaPanel`。
  - `src/ui/FilterPanel.h` / `.cpp` — 文件类型、尺寸与时间筛选过滤器面板 `FilterPanel`。
  - `src/ui/SearchHistoryPanel.h` / `.cpp` — 历史搜索词弹出面板 `SearchHistoryPanel`。

- **QuickLook 快速预览组件**：
  - `src/ui/QuickLookWindow.h` / `.cpp` — 按空格键拉起的高清快速预览窗口 `QuickLookWindow`。
  - `src/ui/QuickLookGraphicsView.h` / `.cpp` — 预览窗口图形缩放与交互视图 `QuickLookGraphicsView`。
  - `src/ui/QuickLookMinimap.h` / `.cpp` — 预览大图的缩略导航小地图 `QuickLookMinimap`。

- **对话框与无边框原生弹窗类**：
  - `src/ui/FramelessDialogBase.h` — 无边框通用对话框基类接口 `FramelessDialogBase`。
  - `src/ui/FramelessDialog.h` / `.cpp` — 阴影与无边框效果标准对话框 `FramelessDialog`。
  - `src/ui/FramelessFileDialog.h` / `.cpp` — 应用风格物理文件选择对话框 `FramelessFileDialog`。
  - `src/ui/BatchRenameDialog.h` / `.cpp` — 批量文件重命名规则编辑对话框 `BatchRenameDialog`。
  - `src/ui/BatchProgressDialog.h` — 批量文件处理进度显示弹窗 `BatchProgressDialog`。
  - `src/ui/BatchCreateDialog.h` / `.cpp` — 批量新建文件夹/文件生成器对话框 `BatchCreateDialog`。
  - `src/ui/DuplicateConflictDialog.h` / `.cpp` — 重复文件冲突解决与比对对话框 `DuplicateConflictDialog`。
  - `src/ui/TagManagerDialog.h` / `.cpp` — 标签管理者界面对话框 `TagManagerDialog`。
  - `src/ui/dialogs/FramelessColorPicker.h` / `.cpp` — 无边框拾色器对话框 `FramelessColorPicker`。
  - `src/ui/dialogs/FramelessConfirmDialog.h` / `.cpp` — 操作确认与提示对话框 `FramelessConfirmDialog`。
  - `src/ui/dialogs/FramelessInputDialog.h` / `.cpp` — 单行文本/数值输入对话框 `FramelessInputDialog`。
  - `src/ui/dialogs/FramelessMessageBox.h` / `.cpp` — 消息弹窗 `FramelessMessageBox`。

- **控件粒度扩展组件 (`src/ui/components/`)**：
  - `src/ui/components/ClickableRow.h` / `.cpp` — 支持全行点击与 Hover 态的列表行组件 `ClickableRow`。
  - `src/ui/components/ColorPill.h` / `.cpp` — 颜色标记小圆点组件 `ColorPill`。
  - `src/ui/components/TagPill.h` / `.cpp` — 标签药丸状圆角按钮组件 `TagPill`。
  - `src/ui/components/FlowLayout.h` / `.cpp` — 流式自适应换行布局引擎 `FlowLayout`。
  - `src/ui/components/ElasticEdit.h` / `.cpp` — 内容宽度自伸缩编辑框 `ElasticEdit`。
  - `src/ui/components/StyledCheckBox.h` / `.cpp` — 包含主题样式的复选框 `StyledCheckBox`。

- **模型与数据委托层 (`src/ui/models/`)**：
  - `src/ui/models/ItemModelBase.h` — Qt AbstractItemModel 抽象基类封装。
  - `src/ui/models/DiskItemModel.h` / `.cpp` — 磁盘文件数据 Model 模型 `DiskItemModel`，供 ListView/TreeView 绑定。
  - `src/ui/ThumbnailDelegate.h` / `.cpp` — 网格/列表缩略图自绘 Delegate 代理 `ThumbnailDelegate`。
  - `src/ui/TreeItemDelegate.h` — 树形目录绘制代理 `TreeItemDelegate`。

- **UI 逻辑控制器、服务与辅助引擎**：
  - `src/ui/MainWindow.h` / `.cpp` — 应用主窗口类 `MainWindow`，调度各 UI 面板与交互事件。
  - `src/ui/TrayController.h` / `.cpp` — 托盘图标与后台常驻控制器 `TrayController`。
  - `src/ui/TaskProgressToolBar.h` / `.cpp` — 顶部/底部任务进度状态栏 `TaskProgressToolBar`。
  - `src/ui/TagManagerController.h` / `.cpp` — 标签 UI 业务控制器 `TagManagerController`。
  - `src/ui/DiskBatchRenameService.h` / `.cpp` — 磁盘模式下的批量重命名 UI 服务桥梁 `DiskBatchRenameService`。
  - `src/ui/ColorAlgorithmEngine.h` / `.cpp` — 主色提取算法引擎 `ColorAlgorithmEngine`，计算图片/视频的主导色彩。
  - `src/ui/ColorPicker.h` / `.cpp` — 颜色选择小部件 `ColorPicker`。
  - `src/ui/FormatDecoders.h` / `.cpp` — 自定义多媒体解码器注册表 `FormatDecoders`。
  - `src/ui/ImageDecoderFacade.h` / `.cpp` — 图像解码统一门面 `ImageDecoderFacade`（整合 Qt / LibTIFF）。
  - `src/ui/MediaColorExtractor.h` / `.cpp` — 多媒体主色调提取服务代理 `MediaColorExtractor`。
  - `src/ui/IconCacheManager.h` / `.cpp` — 内存图标高速缓存管理器 `IconCacheManager`。
  - `src/ui/ShellIconManager.h` — 系统原生文件关联图标获取管理器 `ShellIconManager`。
  - `src/ui/SvgIconRenderer.h` / `.cpp` — SVG 矢量图标高质量渲染器 `SvgIconRenderer`。
  - `src/ui/WindowsShellThumbnailProvider.h` / `.cpp` — Windows 原生 Shell 缩略图提供者 `WindowsShellThumbnailProvider`。
  - `src/ui/PresetManager.h` / `.cpp` — 规则预设管理器 `PresetManager`。
  - `src/ui/RuleRow.h` / `.cpp` / `CreateRuleRow.h` / `.cpp` — 规则编辑单行控件与新建规则控件。
  - `src/ui/TagSelectorOverlay.h` / `.cpp` — 快速标记标签悬浮浮层 `TagSelectorOverlay`。
  - `src/ui/ToolTipOverlay.h` / `.cpp` — 自定义浮动提示气泡浮层 `ToolTipOverlay`。
  - `src/ui/UndoToastOverlay.h` / `.cpp` — Ctrl+Z/Ctrl+Y 操作撤销 Toast 提示浮层 `UndoToastOverlay`。
  - `src/ui/DriveButton.h` / `.cpp` — 盘符选择快捷按钮 `DriveButton`。
  - `src/ui/FolderButton.h` / `.cpp` — 文件夹快捷按钮 `FolderButton`。
  - `src/ui/HoverEventFilter.h` / `.cpp` — 全局 Hover 悬停悬浮事件过滤器 `HoverEventFilter`。
  - `src/ui/ResizeEventFilter.h` / `.cpp` — 窗口尺寸调整事件过滤器 `ResizeEventFilter`。
  - `src/ui/CardPainterHelper.h` / `.cpp` — 网格卡片 Painter 自绘辅助器 `CardPainterHelper`。
  - `src/ui/ElidedTextUtility.h` — 文本省略号算法工具 `ElidedTextUtility`。
  - `src/ui/IScanResultView.h` — 扫描结果视图纯虚接口 `IScanResultView`。
  - `src/ui/Logger.h` — UI 界面日志与调试打印宏工具 `Logger`。
  - `src/ui/UiHelper.h` — 通用 DPI 缩放与 UI 样式工具 `UiHelper`。

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
