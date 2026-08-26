# QuarkMeta 系统顶层架构与编译代码规划

## 1. 架构理念与全局设计规范

`QuarkMeta` 是一个高性能的桌面级文件管理与元数据处理系统。本文档作为应用的高级设计理念、顶层架构规划与全局规范的唯一记录载体。

### 纯洁性保护规范
1. **职责绝对单一**：本文档仅且只能记录应用的高级设计理念、顶层架构规划与全局规范。
2. **严禁写入实施细节**：具体的代码修改点、Search/Replace Diff 替换块、代码行号、调试命令等，绝对禁止写入本文档。
3. **实施方案物理隔离**：所有具体的代码修改与实施方案，必须且只能创建在 `QuarkMeta Architecture/Implementation Plan/` 目录下（采用简洁英文小写命名）。

---

## 2. `src` 目录参与编译的代码架构规划 (Compiled Code Architecture)

根据 CMake 构建系统 (`CMakeLists.txt`) 的显式注册配置，`src` 目录下共有 **238 个** 代码文件参与实际构建与编译。以下按照系统模块划分其顶层架构组织：

### 2.1 应用程序入口 (Root Entry) [1]
- `src/main.cpp` — 应用程序主入口，负责初始化 Qt 环境、应用程序生命周期及主界面拉起。

### 2.2 核心逻辑与控制层 (Core Module - `src/core/`) [36]
负责底层事件调度、磁盘扫描、检索、撤销/重做管理以及操作命令封装。

- `src/core/ActionCommand.h`
- `src/core/AppConfig.h`
- `src/core/BasicCommands.h`
- `src/core/CentralEventHub.h`
- `src/core/CentralEventHub.cpp`
- `src/core/CoreController.h`
- `src/core/CoreController.cpp`
- `src/core/CoreEngine.h`
- `src/core/CoreEngine.cpp`
- `src/core/DiskScanService.h`
- `src/core/DiskScanService.cpp`
- `src/core/DiskTrashService.h`
- `src/core/DiskTrashService.cpp`
- `src/core/FileFilterService.h`
- `src/core/FileFilterService.cpp`
- `src/core/IndexedEntry.h`
- `src/core/IndexedEntry.cpp`
- `src/core/ItemRecord.h`
- `src/core/ItemRecord.cpp`
- `src/core/NavigationHistoryService.h`
- `src/core/NavigationHistoryService.cpp`
- `src/core/OperationSnapshotEngine.h`
- `src/core/OperationSnapshotEngine.cpp`
- `src/core/PhysicalDiskSearchExtractor.h`
- `src/core/PhysicalDiskSearchExtractor.cpp`
- `src/core/SearchHistoryService.h`
- `src/core/SearchHistoryService.cpp`
- `src/core/UndoManager.h`
- `src/core/VolumeOnlineManager.h`
- `src/core/VolumeOnlineManager.cpp`
- `src/core/commands/BatchRenameCommand.h`
- `src/core/commands/MetadataCommand.h`
- `src/core/commands/MoveCommand.h`
- `src/core/commands/RenameCommand.h`
- `src/core/commands/SecureDeleteCommand.h`
- `src/core/commands/ShellProtectionCommand.h`

### 2.3 元数据与数据持久化层 (Meta Module - `src/meta/`) [30]
负责 SQLite 数据库交互、元数据解析提取、标签库、重命名引擎以及重复文件检测。

- `src/meta/BatchRenameEngine.h`
- `src/meta/BatchRenameEngine.cpp`
- `src/meta/DatabaseManager.h`
- `src/meta/DatabaseManager.cpp`
- `src/meta/DatabaseMigrator.h`
- `src/meta/DiskNavigatorService.h`
- `src/meta/DiskNavigatorService.cpp`
- `src/meta/DiskTrashRepo.h`
- `src/meta/DiskTrashRepo.cpp`
- `src/meta/DriveMetaDao.h`
- `src/meta/DriveMetaDao.cpp`
- `src/meta/DuplicateDetectorService.h`
- `src/meta/DuplicateDetectorService.cpp`
- `src/meta/MediaExtractorPipeline.h`
- `src/meta/MediaExtractorPipeline.cpp`
- `src/meta/MetaCacheDecorator.h`
- `src/meta/MetaCacheDecorator.cpp`
- `src/meta/MetadataDefs.h`
- `src/meta/MetadataManager.h`
- `src/meta/MetadataManager.cpp`
- `src/meta/QuarkMetaJson.h`
- `src/meta/QuarkMetaJson.cpp`
- `src/meta/sqlite3.h`
- `src/meta/sqlite3.c`
- `src/meta/StatisticsService.h`
- `src/meta/StatisticsService.cpp`
- `src/meta/TagRepository.h`
- `src/meta/TagRepository.cpp`
- `src/meta/TrashRepository.h`
- `src/meta/TrashRepository.cpp`

### 2.4 数据安全与加密层 (Crypto Module - `src/crypto/`) [2]
负责数据的加解密与安全管理。

- `src/crypto/EncryptionManager.h`
- `src/crypto/EncryptionManager.cpp`

### 2.5 系统工具与通用服务 (Util Module - `src/util/`) [8]
提供系统 Shell 对接、深层缩略图提取及卷路径解析服务。

- `src/util/DeepThumbnailExtractor.h`
- `src/util/DeepThumbnailExtractor.cpp`
- `src/util/DiskMediaExtractor.h`
- `src/util/DiskMediaExtractor.cpp`
- `src/util/ShellHelper.h`
- `src/util/ShellHelper.cpp`
- `src/util/VolumePathResolver.h`
- `src/util/VolumePathResolver.cpp`

### 2.6 第三方图像解码库 (Third Party Module - `src/third_party/libtiff/`) [36]
嵌入式 libtiff 核心图像解码组件。

- `src/third_party/libtiff/tif_aux.c`
- `src/third_party/libtiff/tif_close.c`
- `src/third_party/libtiff/tif_codec.c`
- `src/third_party/libtiff/tif_color.c`
- `src/third_party/libtiff/tif_compress.c`
- `src/third_party/libtiff/tif_dir.c`
- `src/third_party/libtiff/tif_dirinfo.c`
- `src/third_party/libtiff/tif_dirread.c`
- `src/third_party/libtiff/tif_dirwrite.c`
- `src/third_party/libtiff/tif_dumpmode.c`
- `src/third_party/libtiff/tif_error.c`
- `src/third_party/libtiff/tif_extension.c`
- `src/third_party/libtiff/tif_fax3.c`
- `src/third_party/libtiff/tif_fax3sm.c`
- `src/third_party/libtiff/tif_flush.c`
- `src/third_party/libtiff/tif_getimage.c`
- `src/third_party/libtiff/tif_hash_set.c`
- `src/third_party/libtiff/tif_luv.c`
- `src/third_party/libtiff/tif_lzw.c`
- `src/third_party/libtiff/tif_next.c`
- `src/third_party/libtiff/tif_ojpeg.c`
- `src/third_party/libtiff/tif_open.c`
- `src/third_party/libtiff/tif_packbits.c`
- `src/third_party/libtiff/tif_pixarlog.c`
- `src/third_party/libtiff/tif_predict.c`
- `src/third_party/libtiff/tif_print.c`
- `src/third_party/libtiff/tif_read.c`
- `src/third_party/libtiff/tif_strip.c`
- `src/third_party/libtiff/tif_swab.c`
- `src/third_party/libtiff/tif_thunder.c`
- `src/third_party/libtiff/tif_tile.c`
- `src/third_party/libtiff/tif_version.c`
- `src/third_party/libtiff/tif_warning.c`
- `src/third_party/libtiff/tif_win32.c`
- `src/third_party/libtiff/tif_write.c`
- `src/third_party/libtiff/tif_zip.c`

### 2.7 视图与用户交互界面层 (UI Module - `src/ui/`) [125]
实现基于 Qt 的自定 UI 控件、对话框、布局引擎与渲染代理。

- `src/ui/AddressBar.h`
- `src/ui/AddressBar.cpp`
- `src/ui/AddressHistoryPanel.h`
- `src/ui/AddressHistoryPanel.cpp`
- `src/ui/BatchCreateDialog.h`
- `src/ui/BatchCreateDialog.cpp`
- `src/ui/BatchProgressDialog.h`
- `src/ui/BatchRenameDialog.h`
- `src/ui/BatchRenameDialog.cpp`
- `src/ui/BreadcrumbBar.h`
- `src/ui/BreadcrumbBar.cpp`
- `src/ui/CardPainterHelper.h`
- `src/ui/CardPainterHelper.cpp`
- `src/ui/ColorAlgorithmEngine.h`
- `src/ui/ColorAlgorithmEngine.cpp`
- `src/ui/ColorPicker.h`
- `src/ui/ColorPicker.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/CreateRuleRow.h`
- `src/ui/CreateRuleRow.cpp`
- `src/ui/DiskBatchRenameService.h`
- `src/ui/DiskBatchRenameService.cpp`
- `src/ui/DriveButton.h`
- `src/ui/DriveButton.cpp`
- `src/ui/DropJustifiedView.h`
- `src/ui/DropJustifiedView.cpp`
- `src/ui/DropListView.h`
- `src/ui/DropListView.cpp`
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/DuplicateConflictDialog.h`
- `src/ui/DuplicateConflictDialog.cpp`
- `src/ui/ElidedTextUtility.h`
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `src/ui/FolderButton.h`
- `src/ui/FolderButton.cpp`
- `src/ui/FormatDecoders.h`
- `src/ui/FormatDecoders.cpp`
- `src/ui/FramelessDialog.h`
- `src/ui/FramelessDialog.cpp`
- `src/ui/FramelessDialogBase.h`
- `src/ui/FramelessFileDialog.h`
- `src/ui/FramelessFileDialog.cpp`
- `src/ui/HoverEventFilter.h`
- `src/ui/HoverEventFilter.cpp`
- `src/ui/IScanResultView.h`
- `src/ui/IconCacheManager.h`
- `src/ui/IconCacheManager.cpp`
- `src/ui/ImageDecoderFacade.h`
- `src/ui/ImageDecoderFacade.cpp`
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`
- `src/ui/Logger.h`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/MediaColorExtractor.h`
- `src/ui/MediaColorExtractor.cpp`
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`
- `src/ui/NavPanel.h`
- `src/ui/NavPanel.cpp`
- `src/ui/PresetManager.h`
- `src/ui/PresetManager.cpp`
- `src/ui/QuickLookGraphicsView.h`
- `src/ui/QuickLookGraphicsView.cpp`
- `src/ui/QuickLookMinimap.h`
- `src/ui/QuickLookMinimap.cpp`
- `src/ui/QuickLookWindow.h`
- `src/ui/QuickLookWindow.cpp`
- `src/ui/ResizeEventFilter.h`
- `src/ui/ResizeEventFilter.cpp`
- `src/ui/RuleRow.h`
- `src/ui/RuleRow.cpp`
- `src/ui/SearchHistoryPanel.h`
- `src/ui/SearchHistoryPanel.cpp`
- `src/ui/ShellIconManager.h`
- `src/ui/SvgIconRenderer.h`
- `src/ui/SvgIconRenderer.cpp`
- `src/ui/TagManagerController.h`
- `src/ui/TagManagerController.cpp`
- `src/ui/TagManagerDialog.h`
- `src/ui/TagManagerDialog.cpp`
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/TaskProgressToolBar.h`
- `src/ui/TaskProgressToolBar.cpp`
- `src/ui/ThumbnailDelegate.h`
- `src/ui/ThumbnailDelegate.cpp`
- `src/ui/ToolTipOverlay.h`
- `src/ui/ToolTipOverlay.cpp`
- `src/ui/TrayController.h`
- `src/ui/TrayController.cpp`
- `src/ui/TreeItemDelegate.h`
- `src/ui/UiHelper.h`
- `src/ui/UndoToastOverlay.h`
- `src/ui/UndoToastOverlay.cpp`
- `src/ui/WindowsShellThumbnailProvider.h`
- `src/ui/WindowsShellThumbnailProvider.cpp`
- `src/ui/components/ClickableRow.h`
- `src/ui/components/ClickableRow.cpp`
- `src/ui/components/ColorPill.h`
- `src/ui/components/ColorPill.cpp`
- `src/ui/components/ElasticEdit.h`
- `src/ui/components/ElasticEdit.cpp`
- `src/ui/components/FlowLayout.h`
- `src/ui/components/FlowLayout.cpp`
- `src/ui/components/StyledCheckBox.h`
- `src/ui/components/StyledCheckBox.cpp`
- `src/ui/components/TagPill.h`
- `src/ui/components/TagPill.cpp`
- `src/ui/dialogs/FramelessColorPicker.h`
- `src/ui/dialogs/FramelessColorPicker.cpp`
- `src/ui/dialogs/FramelessConfirmDialog.h`
- `src/ui/dialogs/FramelessConfirmDialog.cpp`
- `src/ui/dialogs/FramelessInputDialog.h`
- `src/ui/dialogs/FramelessInputDialog.cpp`
- `src/ui/dialogs/FramelessMessageBox.h`
- `src/ui/dialogs/FramelessMessageBox.cpp`
- `src/ui/models/DiskItemModel.h`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/models/ItemModelBase.h`
