### src
[01] src/main.cpp
- **模块归属**：外壳与应用启动层 (Native Shell & Lifecycle)
- **职责数量**：6
- **职责 1**：初始化 QApplication 框架并配置高 DPI 缩放策略
- **职责 2**：执行 Win32/跨平台单实例互斥量哨兵检测
- **职责 3**：设置全局 QPalette 暗黑主题调色板
- **职责 4**：初始化 Win32 COM 环境亲和性
- **职责 5**：初始化与启动中控控制器 CoreController
- **职责 6**：挂接 aboutToQuit 信号并执行四阶段 Clean Shutdown 优雅清场闭卷
- **持有的核心状态/字段**：HANDLE hMutex（单实例句柄）、QApplication a（全局应用实例）
- **异味与风险诊断**：
  - 【造轮子判定】：无
  - 【打补丁判定】：无
  - 【归属判定】：纯粹（属于程序入口与全局生命周期接管）

### src/core
[02] src/core/CentralEventHub.cpp
- **模块归属**：全局解耦事件总线 (Event Bus)
- **职责数量**：2
- **职责 1**：注册 QuarkMeta::AppEvent 元类型
- **职责 2**：发射 eventOccurred 信号解耦广播 AppEvent
- **持有的核心状态/字段**：static CentralEventHub s_instance（单例静态实例引用）
- **异味与风险诊断**：
  - 【造轮子判定】：无
  - 【打补丁判定】：无
  - 【归属判定】：纯粹（属于全局事件解耦总线实现）

[03] src/core/CentralEventHub.h
- **模块归属**：全局解耦事件总线 (Event Bus)
- **职责数量**：2
- **职责 1**：定义 AppEventType 强类型事件枚举与 AppEvent 统一数据包结构体
- **职责 2**：声明 CentralEventHub 单例接口与 publishEvent 广播入口
- **持有的核心状态/字段**：无成员变量（纯无状态消息传输中枢）
- **异味与风险诊断**：
  - 【造轮子判定】：无
  - 【打补丁判定】：无
  - 【归属判定】：纯粹（属于事件定义与总线声明）

[04] src/core/CoreEngine.cpp
- **模块归属**：业务决策与调度指挥中心 (Domain Core)
- **职责数量**：3
- **职责 1**：路由与分发 AppCommand 业务命令（星级/颜色/标签/置顶/备注/网址/访问历史）
- **职责 2**：校验命令合法性并调度 MetadataManager 与 TagLexiconService 执行真实写盘
- **职责 3**：操作完成后构造 AppEvent 并驱动 CentralEventHub 进行全网广播
- **持有的核心状态/字段**：static CoreEngine s_instance（静态单例对象）
- **异味与风险诊断**：
  - 【造轮子判定】：部分命令（如 handleSetRating、handleSetColor）直接在本类循环广播，与 Command 命令模式层存在潜在职责边界混淆
  - 【打补丁判定】：无
  - 【归属判定】：纯粹（属于领域核心决策与指挥大脑）

[05] src/core/CoreEngine.h
- **模块归属**：业务决策与调度指挥中心 (Domain Core)
- **职责数量**：2
- **职责 1**：定义 AppCommandType 强类型命令枚举与 AppCommand 命令数据包结构
- **职责 2**：定义 CancellationToken 线程安全取消令牌与 CoreEngine 单例接口声明
- **持有的核心状态/字段**：std::atomic<bool> m_canceled（位于 CancellationToken 中，持有取消状态原子标志）
- **异味与风险诊断**：
  - 【造轮子判定】：无
  - 【打补丁判定】：无
  - 【归属判定】：纯粹（属于命令接口与取消令牌定义）
[06] src/core/CoreController.cpp
[07] src/core/CoreController.h
[08] src/core/PhysicalDiskSearchExtractor.cpp
[09] src/core/PhysicalDiskSearchExtractor.h
[10] src/core/IndexedEntry.cpp
[11] src/core/IndexedEntry.h
[12] src/core/ItemRecord.cpp
[13] src/core/ItemRecord.h
[14] src/core/DiskScanService.cpp
[15] src/core/DiskScanService.h
[16] src/core/FileFilterService.cpp
[17] src/core/FileFilterService.h
[18] src/core/VolumeOnlineManager.cpp
[19] src/core/VolumeOnlineManager.h
[20] src/core/DiskTrashService.cpp
[21] src/core/DiskTrashService.h
[22] src/core/OperationSnapshotEngine.h
[23] src/core/OperationSnapshotEngine.cpp
[24] src/core/TrashService.h
[25] src/core/TrashService.cpp
[26] src/core/PermanentDeleteService.h
[27] src/core/PermanentDeleteService.cpp
[28] src/core/ClipboardService.h
[29] src/core/ClipboardService.cpp
[30] src/core/DeviceWatcher.h
[31] src/core/DeviceWatcher.cpp
[32] src/core/TaskProgressService.h
[33] src/core/TaskProgressService.cpp
[34] src/core/AppConfig.h
[35] src/core/NavigationService.cpp
[36] src/core/NavigationService.h
[37] src/core/NavigationHistoryService.cpp
[38] src/core/NavigationHistoryService.h
[39] src/core/SearchHistoryService.cpp
[40] src/core/SearchHistoryService.h
[41] src/core/BatchRenameService.h
[42] src/core/BatchRenameService.cpp
[43] src/core/TagLexiconService.h
[44] src/core/TagLexiconService.cpp
[45] src/core/ActionCommand.h
[46] src/core/UndoManager.h
[47] src/core/BasicCommands.h

### src/core/commands
[48] src/core/commands/RenameCommand.h
[49] src/core/commands/MoveCommand.h
[50] src/core/commands/MetadataCommand.h
[51] src/core/commands/SecureDeleteCommand.h
[52] src/core/commands/ShellProtectionCommand.h
[53] src/core/commands/BatchRenameCommand.h

### src/crypto
[54] src/crypto/EncryptionManager.cpp
[55] src/crypto/EncryptionManager.h

### src/meta
[56] src/meta/QuarkMetaJson.cpp
[57] src/meta/QuarkMetaJson.h
[58] src/meta/DiskNavigatorService.cpp
[59] src/meta/DiskNavigatorService.h
[60] src/meta/StatisticsService.cpp
[61] src/meta/StatisticsService.h
[62] src/meta/MetaCacheDecorator.cpp
[63] src/meta/MetaCacheDecorator.h
[64] src/meta/DiskTrashRepo.cpp
[65] src/meta/DiskTrashRepo.h
[66] src/meta/DriveMetaDao.cpp
[67] src/meta/DriveMetaDao.h
[68] src/meta/TrashRepository.cpp
[69] src/meta/TrashRepository.h
[70] src/meta/DatabaseManager.cpp
[71] src/meta/DatabaseManager.h
[72] src/meta/ExtensionColorDao.h
[73] src/meta/ExtensionColorDao.cpp
[74] src/meta/FavoriteDao.h
[75] src/meta/FavoriteDao.cpp
[76] src/meta/DatabaseMigrator.h
[77] src/meta/BatchRenameEngine.cpp
[78] src/meta/BatchRenameEngine.h
[79] src/meta/DuplicateDetectorService.h
[80] src/meta/DuplicateDetectorService.cpp
[81] src/meta/MetadataDefs.h
[82] src/meta/QuarkMetaJsonStore.h
[83] src/meta/QuarkMetaJsonStore.cpp
[84] src/meta/MetaDbRepository.h
[85] src/meta/MetaDbRepository.cpp
[86] src/meta/MetaMemoryCache.h
[87] src/meta/MetaMemoryCache.cpp
[88] src/meta/MetadataManager.cpp
[89] src/meta/MetadataManager.h
[90] src/meta/MediaExtractorPipeline.cpp
[91] src/meta/MediaExtractorPipeline.h

### src/ui
[92] src/ui/BatchRenameDialog.cpp
[93] src/ui/BatchRenameDialog.h
[94] src/ui/BatchProgressDialog.h
[95] src/ui/DuplicateConflictDialog.h
[96] src/ui/DuplicateConflictDialog.cpp
[97] src/ui/PresetManager.cpp
[98] src/ui/PresetManager.h
[99] src/ui/UndoToastOverlay.cpp
[100] src/ui/UndoToastOverlay.h
[101] src/ui/BreadcrumbBar.cpp
[102] src/ui/BreadcrumbBar.h
[103] src/ui/ColorPicker.cpp
[104] src/ui/ColorPicker.h
[105] src/ui/ContentPanel.cpp
[106] src/ui/ContentPanel.h
[107] src/ui/DriveButton.cpp
[108] src/ui/DriveButton.h
[109] src/ui/DropJustifiedView.cpp
[110] src/ui/DropJustifiedView.h
[111] src/ui/DropListView.cpp
[112] src/ui/DropListView.h
[113] src/ui/DropTreeView.cpp
[114] src/ui/DropTreeView.h
[115] src/ui/IScanResultView.h
[116] src/ui/FilterStateModel.h
[117] src/ui/FilterStateModel.cpp
[118] src/ui/ScanStatsEngine.h
[119] src/ui/ScanStatsEngine.cpp
[120] src/ui/FilterPanel.cpp
[121] src/ui/FilterPanel.h
[122] src/ui/FramelessDialogBase.h
[123] src/ui/FramelessDialog.cpp
[124] src/ui/FramelessDialog.h
[125] src/ui/FramelessFileDialog.cpp
[126] src/ui/FramelessFileDialog.h
[127] src/ui/JustifiedView.cpp
[128] src/ui/JustifiedView.h
[129] src/ui/ElidedTextUtility.h
[130] src/ui/CardPainterHelper.cpp
[131] src/ui/CardPainterHelper.h
[132] src/ui/ThumbnailDelegate.cpp
[133] src/ui/ThumbnailDelegate.h
[134] src/ui/Logger.h
[135] src/ui/AppShortcutController.h
[136] src/ui/AppShortcutController.cpp
[137] src/ui/PanelMediator.h
[138] src/ui/PanelMediator.cpp
[139] src/ui/MainWindow.cpp
[140] src/ui/MainWindow.h
[141] src/ui/SearchController.h
[142] src/ui/SearchController.cpp
[143] src/ui/TrayController.cpp
[144] src/ui/TrayController.h
[145] src/ui/HoverEventFilter.cpp
[146] src/ui/HoverEventFilter.h
[147] src/ui/FramelessWindowHelper.cpp
[148] src/ui/FramelessWindowHelper.h
[149] src/ui/PanelLayoutManager.cpp
[150] src/ui/PanelLayoutManager.h
[151] src/ui/AddressBar.cpp
[152] src/ui/AddressBar.h
[153] src/ui/MetaPreviewWidget.h
[154] src/ui/MetaPreviewWidget.cpp
[155] src/ui/MetaRatingColorWidget.h
[156] src/ui/MetaRatingColorWidget.cpp
[157] src/ui/MetaTagSection.h
[158] src/ui/MetaTagSection.cpp
[159] src/ui/MetaInfoSection.h
[160] src/ui/MetaInfoSection.cpp
[161] src/ui/MetaPanel.cpp
[162] src/ui/MetaPanel.h
[163] src/ui/TagSelectorOverlay.h
[164] src/ui/TagSelectorOverlay.cpp
[165] src/ui/TaskProgressToolBar.h
[166] src/ui/TaskProgressToolBar.cpp
[167] src/ui/NavPanel.cpp
[168] src/ui/NavPanel.h
[169] src/ui/FavoritePanel.cpp
[170] src/ui/FavoritePanel.h
[171] src/ui/QuickLookWindow.cpp
[172] src/ui/QuickLookWindow.h
[173] src/ui/QuickLookMinimap.cpp
[174] src/ui/QuickLookMinimap.h
[175] src/ui/RuleRow.cpp
[176] src/ui/RuleRow.h
[177] src/ui/CreateRuleRow.cpp
[178] src/ui/CreateRuleRow.h
[179] src/ui/BatchCreateDialog.cpp
[180] src/ui/BatchCreateDialog.h
[181] src/ui/TagManagerDialog.cpp
[182] src/ui/TagManagerDialog.h
[183] src/ui/SearchHistoryPanel.cpp
[184] src/ui/SearchHistoryPanel.h
[185] src/ui/AddressHistoryPanel.cpp
[186] src/ui/AddressHistoryPanel.h
[187] src/ui/TagManagerController.cpp
[188] src/ui/TagManagerController.h
[189] src/ui/FolderButton.h
[190] src/ui/FolderButton.cpp
[191] src/ui/QuickLookGraphicsView.h
[192] src/ui/QuickLookGraphicsView.cpp
[193] src/ui/CardLayoutEngine.h
[194] src/ui/CardLayoutEngine.cpp
[195] src/ui/RowLayoutEngine.h
[196] src/ui/RowLayoutEngine.cpp
[197] src/ui/RatingBarLayout.h
[198] src/ui/RatingBarLayout.cpp
[199] src/ui/ToolTipOverlay.cpp
[200] src/ui/ToolTipOverlay.h
[201] src/ui/TreeItemDelegate.h
[202] src/ui/ThemeManager.h
[203] src/ui/ThemeManager.cpp
[204] src/ui/UiHelper.h
[205] src/ui/IconCacheManager.h
[206] src/ui/IconCacheManager.cpp
[207] src/ui/ShellIconManager.h
[208] src/ui/SvgIconRenderer.h
[209] src/ui/SvgIconRenderer.cpp
[210] src/ui/WindowsShellThumbnailProvider.h
[211] src/ui/WindowsShellThumbnailProvider.cpp
[212] src/ui/FormatDecoders.h
[213] src/ui/FormatDecoders.cpp
[214] src/ui/ImageDecoderFacade.h
[215] src/ui/ImageDecoderFacade.cpp

### src/ui/components
[216] src/ui/components/ElasticEdit.h
[217] src/ui/components/ElasticEdit.cpp
[218] src/ui/components/TagPill.h
[219] src/ui/components/TagPill.cpp
[220] src/ui/components/FlowLayout.h
[221] src/ui/components/FlowLayout.cpp
[222] src/ui/components/ColorPill.h
[223] src/ui/components/ColorPill.cpp
[224] src/ui/components/StyledCheckBox.h
[225] src/ui/components/StyledCheckBox.cpp
[226] src/ui/components/ClickableRow.h
[227] src/ui/components/ClickableRow.cpp

### src/ui/controllers
[228] src/ui/controllers/ContentContextMenu.h
[229] src/ui/controllers/ContentContextMenu.cpp
[230] src/ui/controllers/ContentKeyHandler.h
[231] src/ui/controllers/ContentKeyHandler.cpp
[232] src/ui/controllers/ContentSortController.h
[233] src/ui/controllers/ContentSortController.cpp
[234] src/ui/controllers/ContentDataLoader.h
[235] src/ui/controllers/ContentDataLoader.cpp
[236] src/ui/controllers/ContentFileOpsHandler.h
[237] src/ui/controllers/ContentFileOpsHandler.cpp

### src/ui/dialogs
[238] src/ui/dialogs/FramelessInputDialog.h
[239] src/ui/dialogs/FramelessInputDialog.cpp
[240] src/ui/dialogs/FramelessColorPicker.h
[241] src/ui/dialogs/FramelessColorPicker.cpp
[242] src/ui/dialogs/FramelessConfirmDialog.h
[243] src/ui/dialogs/FramelessConfirmDialog.cpp
[244] src/ui/dialogs/FramelessMessageBox.h
[245] src/ui/dialogs/FramelessMessageBox.cpp

### src/ui/models
[246] src/ui/models/FilterProxyModel.h
[247] src/ui/models/FilterProxyModel.cpp
[248] src/ui/models/ItemModelBase.h
[249] src/ui/models/DiskItemModel.h
[250] src/ui/models/DiskItemModel.cpp

### src/ui/workers
[251] src/ui/workers/ContentStatsWorker.h
[252] src/ui/workers/ContentStatsWorker.cpp

### src/util
[253] src/util/VolumePathResolver.h
[254] src/util/VolumePathResolver.cpp
[255] src/util/ColorPaletteEngine.h
[256] src/util/ColorPaletteEngine.cpp
[257] src/util/ThumbnailPipelineService.h
[258] src/util/ThumbnailPipelineService.cpp
[259] src/util/DiskMediaExtractor.h
[260] src/util/DiskMediaExtractor.cpp
[261] src/util/DeepThumbnailExtractor.h
[262] src/util/DeepThumbnailExtractor.cpp
[263] src/util/ShellHelper.cpp
[264] src/util/ShellHelper.h

### 📄 [05] src/core/CoreEngine.h
- **模块归属**：业务决策与调度指挥中心 (Domain Core)
- **核心职责**：核心业务决策大脑的接口与数据结构定义。定义 `AppCommandType` 命令枚举、`AppCommand` 数据包结构及 `CancellationToken` 异步取消令牌，声明命令执行入口 `executeCommand` 与各类属性修改私有处理句柄。

### 📄 [06] src/core/CoreController.cpp
- **模块归属**：核心中控控制器 (Core Controller)
- **核心职责**：系统中控控制器的核心逻辑实现。负责在启动阶段顺序预热底层单例与设备监听器，提供系统初始化序列 `startSystem`，以及实现基于双轨模式（内存极速检索 + 磁盘 I/O 流式补全）的异步搜索 `performSearch` 与任务中止机制。

### 📄 [07] src/core/CoreController.h
- **模块归属**：核心中控控制器 (Core Controller)
- **核心职责**：系统中控控制器的接口声明与全局状态属性定义。声明 `isIndexing` 与 `statusText` 属性，提供全局初始化 `initializeCoreComponents`、全局导航代际号管理、异步搜索 `performSearch` 接口及搜索状态信号。

### 📄 [08] src/core/PhysicalDiskSearchExtractor.cpp
- **模块归属**：磁盘搜索提取工具 (Search Extractor)
- **核心职责**：物理磁盘递归搜索引擎实现。使用 `QDirIterator` 深度遍历指定目录，执行文件名不区分大小写的关键词匹配与 `seenPaths` 内存去重，采用 50 项批次流式回调通知，支持原子标志位实时中断。
