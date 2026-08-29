# QuarkMeta 系统顶层架构与自研代码文件规划

## 1. 架构理念与全局设计规范

`QuarkMeta` 是一个高性能的桌面级文件管理与元数据处理系统。本文档作为应用的高级设计理念、顶层架构规划与全局规范的唯一记录载体。

### 纯洁性保护规范
1. **职责绝对单一**：本文档仅且只能记录应用的高级设计理念、顶层架构规划与全局规范。
2. **严禁写入实施细节与具体样式度量**：具体的代码修改点、Diff 替换块、代码行号，以及像素级尺寸、颜色色值等纯 UI 样式参数，绝对禁止写入本文档。具体视觉与度量参数全权剥离托管于独立的 `UI_DESIGN_SPEC.md`。
3. **实施方案物理隔离**：所有具体的代码修改与实施方案，必须且只能创建在 `QuarkMeta Architecture/Implementation Plan/` 目录下（采用简洁英文小写命名）。

### 收藏夹（FavoritePanel）global.db 数据库持久化与 SVG 图标/颜色动态切换顶层规范
1. **global.db 数据库持久化红线**：收藏夹数据完全废除 JSON 文件存储，统一收拢持久化存储于 `global.db` 数据库（`favorites` 表）。支持物理路径、显示名称、SVG 图标键名 (`icon_key`)、颜色 HEX (`color_hex`) 与排序号 (`sort_order`) 的 $O(1)$ 极速读写。
2. **物理路径绝对唯一性与归一化红线**：每一个收藏条目在入库前必须强制执行 `QDir::cleanPath` 路径归一化，由 SQL `path TEXT UNIQUE` 数据库约束拦截重复项，严禁允许重复收藏相同的物理路径。
3. **文件夹/文件双轨渲染与实心文件夹图标规范**：
   - **文件夹项 (`QFileInfo::isDir() == true`)**：必须统一使用 `UiHelper::getIcon(iconKey, color, 18)` 渲染视网膜屏高保真矢量 SVG 图标，默认使用实心文件夹图标（`folder_filled`）及金色（`#FDB70A`）。支持右键菜单自定义图标与颜色。
   - **文件项 (`QFileInfo::isDir() == false`)**：**严格禁止**使用矢量 SVG 图标与自定义颜色，必须且只能通过 `ShellIconManager::getFileIcon(path)` 渲染系统原生文件图标与缩略图。
4. **文件夹右键上下文菜单精简规范（纯图标无文字）**：
   - 收藏夹文件夹项的右键“切换图标”与“切换色标”菜单，**菜单项中必须仅显示图标，严禁显示任何纯文本文字**，保持视觉极简与利落。
   - 收藏夹文件项的右键菜单**严禁显示**“切换图标”与“切换色标”子菜单，仅保留“取消收藏”操作，实现严格的类型安全防护。

### 全局操作反馈 Toast (UndoToastOverlay) 停留时长规范
1. **统一停留时长红线**：全局所有基于 `UndoToastOverlay` 弹出的轻量级操作反馈与状态通知提示，默认停留时间统一固定为 7000ms（7 秒），确保用户具备充足的操作与撤销交互响应窗口。

### 地址栏面包屑（BreadcrumbBar）矢量图标分隔符顶层规范
1. **硬编码文本符号剔除红线**：地址栏面包屑控件（BreadcrumbBar）文件夹层级之间严禁使用低字号文本字符 `">"` 或硬编码符号作为分隔符。
2. **矢量 SVG 箭头渲染标准**：层级分隔符必须统一采用标准的矢量 SVG 箭头图标，确保在深色背景下保持清晰度与 UI 风格一致。
3. **面包屑层级右键菜单快捷收藏规范**：地址栏面包屑文件夹层级支持右键上下文菜单，动态提供“添加至收藏夹”或“取消收藏”选项。

### 内容面板（ContentPanel）物理双独立容器 (Dual Containers) 顶层规范
1. **文件夹与文件物理双轨分流红线**：`ContentPanel` 内容呈现区必须强制划分为“上方文件夹独立容器”与“下方文件独立容器”的物理上下结构，禁止将文件夹与文件混合放置于单个视图容器中。
2. **数据模型物理分流**：上方容器仅绑定 `FolderProxyModel`（`isDir == true`），下方容器仅绑定 `FileProxyModel`（`isDir == false`），从数据源头彻底切断两者的排版混杂。
3. **动态高度收缩与独占拉伸规范**：上方文件夹容器采用高度自适应（`0` 权重），无文件夹或显示关闭时容器高度自动缩为 `0` 彻底隐形；下方文件容器独占弹性拉伸权重（`1` 权重），支撑大部分视图区域。
4. **置顶互不跨界红线**：文件夹置顶仅作用于上方文件夹容器内部，文件置顶仅作用于下方文件容器内部，绝不允许文件置顶超越文件夹。

### 内容面板（ContentPanel）模块化拆分与高并发选中性能防卡死顶层规范
1. **上帝类拆分解耦红线**：内容面板（ContentPanel）必须遵循单一职责原则，主面板仅保留视图栈管理与子控制器调度职责，右键菜单构建、物理文件系统操作（复制/剪切/粘贴/删除/重命名）与统计运算物理解耦拆分为独立控制器。
2. **右键上下文菜单独立控制器解耦规范 (ContentContextMenu)**：内容面板的巨型右键菜单构建、事件路由及菜单动作派发逻辑彻底从 `ContentPanel` 剥离，完全收拢至独立的 `ContentContextMenu` 控制组件，`ContentPanel` 仅保留 3 行纯调度调用，实现视图容器与上下文菜单业务逻辑的彻底解耦。
3. **按键响应与 Hitbox 点击拦截独立控制器解耦规范 (ContentKeyHandler)**：内容面板的滚轮缩放、快捷键（Ctrl+0~5 评分、Alt+D 置顶、Alt+1~9 色标、F2/Delete/空格预览/Ctrl+C/X/V）以及委托 Hitbox 点击拦截等重度事件过滤逻辑彻底剥离并收拢至 `ContentKeyHandler`，`ContentPanel::eventFilter` 仅保留统一的事件入口路由转发。
4. **高并发选中索引计算防卡死与多视图兼容规范**：在处理选中项获取时（如 `getSelectedIndexes()`），在列表视图（QTreeView）下获取 `selectedRows(0)`；在网格视图下仅提取 `column == 0` 的首列单元格索引，杜绝在成千上万条记录场景下因遍历多列导致主 UI 线程卡顿。
4. **视图编辑触发器纯化红线**：视图层编辑触发器（`EditTriggers`）必须限制为仅响应 `EditKeyPressed`（如 F2 按键），绝对禁止将 `DoubleClicked` 注册为编辑触发器。

### 高并发选区响应防抖与信号广播熔断保护顶层规范
1. **防抖时间窗口红线**：选区变更通知必须引入防抖定时器机制，防止在多选、框选或全选过程中连续高频触发选区变更事件。
2. **大批量选区信号广播熔断保护红线**：当选中项目总数超越阈值（如 50 项）时，系统启动选区广播熔断机制，精简为仅传输首个选中项进行元数据预览，彻底消除全选/取消全选场景下的假死与卡顿。

### 搜索中枢与 11 维多维条件过滤代理独立解耦顶层规范 (FilterProxyModel)
1. **多维条件过滤与 View 解耦红线**：高级 11 维多维条件过滤算法、搜索关键词加权匹配、加权排序权重及重复项过滤完全解耦并物理收揽于独立的 `FilterProxyModel`（位于 `src/ui/models/FilterProxyModel.h/cpp`），内容面板（`ContentPanel`）仅承载 UI 视图容器与控件展现职责，绝对禁止在 `ContentPanel` 内部内嵌代理模型类定义或手写过滤比对算式。
2. **筛选状态 SSOT 归一与胶水拼接消灭红线**：`FilterState` 作为筛选与搜索状态的唯一真理源 (SSOT)，统一管理搜索关键词（`keyword`）与各维度筛选条件。严禁在 `ContentPanel` 或 `PanelMediator` 中编写中间状态备份/恢复补丁（如手写 `preservedShowFolders`）或在信号分发时强行搜刮与拼接文本框状态。

### 严谨三阶哈希验重与 UI 线程 0 阻塞顶层规范 (DuplicateDetectorService)
1. **三阶哈希判重红线**：文件判重严格采用三阶流水线算法：一阶依物理尺寸粗滤 -> 二阶做 FastHash（首尾分块哈希） -> 三阶做全量 SHA-256 校验。彻底杜绝误判。
2. **UI 主线程零 I/O 判重保护红线**：哈希计算与全量判重集合生成全权移交至 `QtConcurrent` 后台工作线程处理。

### 多步连续 Undo/Redo 事务快照与撤销栈顶层规范 (OperationSnapshotEngine & UndoManager)
1. **正反向双向闭环红线**：撤销指令（`GeneralSnapshotUndoCommand`）必须同时封装正向操作（`doAction`）与逆向回滚（`undoAction`），确保全系统批量改名、移入回收站、收藏夹切换等快照事务 100% 具备完美的 `Ctrl + Z`（撤销）与 `Ctrl + Y`（重做）双向可逆能力。
2. **单一严格时序栈规范**：全系统所有物理文件与元数据变更统一收拢入 `UndoManager` 线程安全单例栈中按时间线性排列，容量控制为 50 步，杜绝多撤销栈并存导致的时序错乱。
3. **销毁路径自动清洗规范**：当文件被物理粉碎或永久删除时，系统必须自动触发 `UndoManager::removeCommandsForPath` 清洗撤销栈中涉及该路径的 Command，防止针对已销毁路径的回滚造成系统崩溃。

### 批量重命名服务归一化与两阶段安全重命名顶层规范 (BatchRenameService)
1. **重命名管道与物理 I/O 归一收敛红线**：全系统批量重命名、移动与复制规则计算（固定文本、序列、日期、原文件名、元数据变量）、同名冲突校验、Windows NTFS 大小写不敏感两阶段 UUID 中转安全重命名、缩略图与元数据 (.QuarkMeta.json) 全量同步漫游统一由 `BatchRenameService` 承载，视图层（`BatchRenameDialog`）与计算引擎（`BatchRenameEngine`）严禁就地执行同步 `std::filesystem::rename` 物理写盘。
2. **原子撤销与双重撤销消除红线**：批量重命名操作必须生成单一次原子的 `BatchRenameCommand` 并推入全局 `UndoManager`，彻底消除视图层手动二次推入撤销快照引起的双重撤销冲突，撤销完成反馈 Toast 统一固定为 7 秒 (7000ms) 停留机制。

### 全局标签词库服务与磁盘 I/O 完全解耦顶层规范 (TagLexiconService)
1. **词库维护与文件标注解耦红线**：全局标签词库（词条 CRUD、分组管理、颜色与拼音/前缀联想补全）统一由 `TagLexiconService` 承载，仅对 SQLite `global.db` 数据库（`tags` 与 `tag_groups` 表）进行持久化更新，严禁在重命名或删除全局词条时执行全盘 `.QuarkMeta.json` 磁盘扫描遍历。
2. **底层并发访问与事务安全红线**：`TagLexiconService` 底层必须严格对接 `DatabaseManager` 的原生 `sqlite3*` 句柄与全局并发锁，统一采用 `SqlTransaction` RAII 事务和 `sqlite3_wal_checkpoint_v2` WAL 检查点，杜绝混合使用 Qt `QSqlDatabase` 引起的数据库锁冲突（`SQLITE_BUSY`）。
3. **极速前缀联想与零阻塞补全规范**：搜索输入框与标签选择弹窗（`TagSelectorOverlay`）的自动补全列表由 `TagLexiconService::querySuggestions` 提供毫秒级内存/索引检索，保障 UI 主线程无延迟响应。

### 核心解耦与单一职责架构顶层规范 (MainWindow, FilterPanel, MetaPanel, MetadataManager)
1. **主窗口 (MainWindow) 拆分与壳体归一化**：主窗口仅允许承载顶层 UI 布局构建与 QSS 样式加载。无边框窗口 8 方向边缘感应、DPI 动态热区、光标切换、边缘拉伸、标题栏拖拽移动、双击最大化/还原及跨平台安全置顶全权交由 `FramelessWindowHelper` 统一收敛承载；彻底清除主窗口中的底层几何算式与裸 Win32 API 杂质；应用内局域快捷键解耦至声明式 `AppShortcutController` (`QShortcut(Qt::WindowShortcut)`)；多面板联动解耦至 `PanelMediator`。
2. **筛选面板 (FilterPanel) 拆分规范**：筛选面板仅保留 UI 控件渲染职责。筛选状态管理解耦至 `FilterStateModel`；后台文件数量聚合解耦至 `ScanStatsEngine`。
3. **属性面板 (MetaPanel) 拆分与纯 View 状态机与 Delta 打标顶层规范**：属性面板作为纯粹 Presentation View，严禁就地调用写盘与数据库存取代码，100% 仅对外发射标准 Qt 信号由 Controller 统一路由。多选打标强约束采用 Delta 差集增量计算，仅针对变动 Tag 发射单点增删信号，严禁全量覆盖清空文件私有标签；多选状态下强制禁用单文件重命名编辑框；针对加密文件与回收站项目触发全量只读守卫；FlowLayout 布局控件严格遵循 Qt 父子对象与 `deleteLater` 内存生命周期管理，彻底根除悬空指针隐患。
4. **元数据中心 (MetadataManager) 门面模式规范**：元数据中心作为对外统一门面（Facade），不再直接混合磁盘 I/O 与数据库存取。序列化由 `QuarkMetaJsonStore` 承载；SQLite 持久化由 `MetaDbRepository` 承载；内存缓存由 `MetaMemoryCache` 承载。
5. **元数据持久化脏缓冲合并落盘规范 (QuarkMetaJsonStore)**：`QuarkMetaJsonStore` 引入脏目录缓冲（Dirty Buffer Merge）与 50ms 自动防抖机制，同目录连续元数据修改先在内存中高效合流，防抖期满后执行 1 次原子落盘，且应用退出时触发强制刷盘（`flushAllDirtyBuffers`），彻底消除磁盘高频写盘震荡。

### 系统外壳工具职责纯粹化与两阶段安全重命名规范 (ShellHelper)
1. **0 重度写盘/删除越权代码红线**：`ShellHelper` 彻底剥离文件传输、删除与移动等业务逻辑，全权委托为 `TrashService` 与 `DiskIoService`；仅保留 Windows 外壳原生关联操作（Explorer 定位高亮、属性框呼出、隐藏属性设置及字节格式化）。
2. **Windows 两阶段 UUID 安全重命名规范**：`ShellHelper::renameItem` 统一接入 `FileOperationHelper::safeRename`，采用临时 UUID 独立中转文件名，彻底根治 Windows NTFS 文件系统大小写不敏感导致的重命名失败缺陷，并自动保证 `.QuarkMeta.json` 元数据、磁盘 Hash 缩略图与内存/SQLite 索引的全量漫游与同步。

### 对话框与悬浮遮罩纯 UI 职责及纯 SVG 图标化顶层规范 (TagManagerDialog & TagSelectorOverlay)
1. **词库服务 1:1 契约绝对对齐红线**：所有标签管理对话框（`TagManagerDialog`）与悬浮选择遮罩（`TagSelectorOverlay`）必须 100% 绑定 `TagLexiconService` 标准接口（如 `getAllTagGroups()`、`getAllTagNames()`、`TagGroup`、`moveTagToGroup()`），严禁调用任何废弃的非标方法。
2. **纯 SVG 图标渲染与文本符号/Emoji 0 容忍红线**：全界面严禁使用硬编码文本字符或 Emoji 符号（如 `"📁 "`、`"• "`、`"+ "` 等），所有分类、层级与按钮图标强制统一使用 `UiHelper::getIcon(...)` 加载标准的矢量 SVG 图标，保障视网膜屏高保真视觉呈现。
3. **悬浮遮罩视口碰撞防护与失焦自闭环规范**：悬浮遮罩 (`TagSelectorOverlay`) 必须包含屏幕视口边界碰撞检测，支持在靠近屏幕边缘时自动反折缩进；并在焦点转移时安全触发 `closeOverlay()` 优雅自闭环销毁。

### QuickLook 空格全屏预览与代际熔断/鹰眼小地图双向联动顶层规范 (QuickLookWindow & QuickLookMinimap)
1. **切图代际号（`previewGeneration`）原子熔断红线**：用户通过空格键呼出或方向键高频连续切换预览文件时，`QuickLookWindow` 必须通过 `std::atomic<uint64_t>` 递增代际号，秒级丢弃前序大图/文本的在途异步解码任务，确保 CPU/GPU 算力 100% 聚焦当前选中的预览文件，彻底消灭连续切图引发的卡顿与线程池阻塞。
2. **反模式全局顶层窗口搜刮彻底禁止红线**：右键菜单快捷操作（如“添加至收藏夹”）必须严格通过 Qt 信号（`favoriteRequested`）发射给 Controller/PanelMediator 统一调度，彻底禁止在 `QApplication::topLevelWidgets()` 或全局控件树中递归搜刮 `FavoritePanel` 等私有 UI 指针。
3. **平台置顶统一收拢规范**：预览窗口置顶与焦点夺取统一调用 `FramelessWindowHelper::setAlwaysOnTop(this, true)`，严禁嵌入裸 Win32 `SetWindowPos(HWND_TOPMOST)` 原生 API。
4. **鹰眼小地图（QuickLookMinimap）防重入与防震荡规范**：小地图拖拽缩略框与主视口 `centerOn` 双向联动必须设置防重入锁，消除主视口滚动与小地图矩形重绘之间的浮点震荡。

### 缩略图三级缓存流水线与代际号原子熔断顶层规范 (ThumbnailPipelineService)
1. **三级缓存降级流水线红线**：全系统多媒体缩略图的加载与渲染必须 100% 统一走 `ThumbnailPipelineService`（位于 `src/util/`）的三级缓存流水线——“一级内存 LRU（0ms 耗时直取） -> 二级磁盘持久化 Hash（PNG 固化缓存） -> 三级后台无锁解码（QtConcurrent 异步并发降采样）”。
2. **原子代际号（`generationId`）即时熔断规范**：引入 `std::atomic<uint64_t>` 代际号机制。当用户在视图中高速滚动或切换目录时，系统强制触发 `cancelAll()` 递增代际号，后台解码线程与 UI 回调函数在执行前必须进行代际号比对，一旦发现代际过期立刻毫秒级放弃任务，确保 CPU/GPU 算力 100% 聚焦当前视口可见区域。
3. **主线程 GUI 锁脱钩与 60FPS 丝滑保障红线**：后台子线程提图解码过程绝对禁止包含对 UI 绘图组件的同步等待，纯粹依赖线程安全的 `QImageReader` 与 `QImage` 进行像素运算，解码完成后通过 `QMetaObject::invokeMethod(Qt::QueuedConnection)` 投递给主 UI 线程转换为 `QPixmap` 并填入内存 LRU 缓存，保障视图 60FPS 丝滑无卡顿。

### 硬件设备监听中枢与主窗口 0 行 Win32 脱敏规范 (DeviceWatcher)
1. **原生硬件消息监听解耦红线**：全系统 Windows 磁盘与移动设备热插拔消息（`WM_DEVICECHANGE`）的底层捕获与盘符掩码（`dbcv_unitmask`）解析，必须统一由独立的 `DeviceWatcher` 服务（继承自 `QAbstractNativeEventFilter`，位于 `src/core/`）承载，并通过干净的标准 Qt 信号（`driveMounted` / `driveUnmounted`）对外广播。
2. **领域自治与防护闭环规范**：导航服务（`NavigationService`）直接订阅 `DeviceWatcher::driveUnmounted` 信号。当检测到当前正浏览的物理盘符被拔出时，自动安全回退至“此电脑”（`computer://`），无需任何外部 UI 面板或主窗口书写中间拦截代码。
3. **主窗口 Win32 脱敏与 100% 跨平台纯洁性红线**：彻底移除主窗口（`MainWindow`）中的 `nativeEvent` 虚函数覆盖、拔盘槽函数以及 `<dbt.h>`、`<windows.h>` 等原始平台头文件包含，确保主窗口达成 **0 行 Win32 API 杂质** 的 100% 跨平台纯洁性。

### 多媒体色彩提取与调色板引擎归一化顶层规范 (ColorPaletteEngine)
1. **底层工具层绝对归一收敛红线**：多媒体图像格式权威判定（标准图/矢量图/RAW 等）、主导色彩提取、5 色调色板桶量化算法、标准色标（红/橙/黄/绿/青/蓝/紫/灰等）量化映射以及 CIEDE2000 国际标准色差算法（$\Delta E$）必须 100% 物理归一化收敛至底层 `ColorPaletteEngine`（位于 `src/util/`），绝对禁止在 UI 视图层、中介者或控制器内编写手写 RGB 差值算法或色彩比对逻辑。
2. **后缀名 Badge 专属与 global.db 数据库持久化配色顶层规范**：卡片与视图层文件扩展名 Badge 标签的背景色与字体色映射计算完全交由 `ColorPaletteEngine` 统一调度。对于特定后缀名（如 `psd`、`eps`、`ai`）提供专属高辨识度配色保护；对于其他未指定扩展名，配色方案统一物理落盘持久化存储于 `global.db` 数据库（`extension_colors` 表）。系统首次遇到新扩展名时生成高对比度配色并自动刷盘固化，后续由内存 LRU 缓存与 DB 级联直取，既保障配色 100% 物理固化与高灵活性配置，又杜绝界面重绘与写盘性能损耗。
2. **纯计算与 0 UI 依赖隔离红线**：`ColorPaletteEngine` 归属于底层 Utility 计算层，严禁包含任何 `src/ui/` 目录头文件或持有 QWidget/QPainter 等 UI 绘图组件。同时支持基于文件路径（`extractPalette`）与内存 `QImage` 句柄（`extractPaletteFromImage`）的双重提取重载，保障后台多媒体提取管道（`MediaExtractorPipeline`）的高性能零卡顿处理。
3. **架构分层倒挂物理彻底清除红线**：物理废除并彻底删除原 UI 层中分层倒挂的 `MediaColorExtractor` 与 `ColorAlgorithmEngine` 旧类；`UiHelper` 仅保留平滑转发内联接口，确保既有调用的 100% 向后兼容性。

### 全局任务进度中枢与观察者视图顶层解耦规范 (TaskProgressService & TaskProgressToolBar)
1. **全局进度中枢线程安全与单例调度红线**：全系统所有后台耗时任务（如扫描、判重、提取缩略图等）的进度管理统一由领域服务 `TaskProgressService` 单例承载。内部状态操作与任务字典管理必须通过锁（`QMutex`）保护，跨线程进度变动与完成信号通知强制采用 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 切回主 UI 线程，彻底消除跨线程访问 UI 崩溃漏洞。
2. **进度工具栏 UI 纯粹观察者模式红线**：底部进度工具栏 (`TaskProgressToolBar`) 仅作为纯粹的观察者（Observer），直接订阅 `TaskProgressService` 信号，根据活动任务状态实现自动显隐与进度条更新。工具栏内部禁止包含任何任务调度、倒计时逻辑或重型计算。
3. **主窗口宿主与侵入式控制器彻底拔除红线**：彻底物理废除并删除 UI 层侵入式控制器 `TaskProgressController`。主窗口 (`MainWindow`) 仅作为界面装配容器，严禁持有任务进度控制器指针或在主窗口内手动编写进度胶水代码。

### 全局矢量图标与控件尺寸度量收拢顶层规范
1. **SVG 图标按钮 iconSize 显式约束红线**：全系统所有基于 `QPushButton` 或 `QAbstractButton` 承载矢量 SVG 图标的控件，必须强制显式绑定 `setIconSize` 尺寸约束（如 12x12 或 14x14），严禁依赖 Qt 默认 Style 的自适应拉伸，防止在不同 DPI 缩放下图标膨胀失真。
2. **微型折叠/排序箭头与操作按钮尺寸规范**：筛选面板（FilterPanel）等侧边栏控件中的分组折叠箭头、排序箭头及微型操作按钮，图标必须统一限定为微型矢量尺寸，且控件边框保持紧凑防挤压排版。
3. **星级评级控件与底色标记视觉协调规范**：元数据面板（MetaPanel）与内容视图卡片（CardPainterHelper / ThumbnailDelegate）中的五星评级组件，必须保持图标尺寸、按钮 Hitbox 与底色标记（Color Tag）的视觉居中与比例协调，杜绝图标放大贴边或与卡片边框重叠。

### 系统托盘退出生命周期与主进程优雅终结顶层规范 (TrayController)
1. **标准关闭序列与配置落盘红线**：系统托盘（TrayController）在响应右键“退出”指令时，必须发射退出信号或调用 `qApp->closeAllWindows()` 发起标准的窗口关闭流程，由主窗口 `closeEvent` 统一调度后台工作线程熔断与数据库安全落盘，严禁强杀主事件循环。

### 标准应用局域快捷键控制器顶层规范 (AppShortcutController)
1. **彻底禁止 eventFilter 按键拦截补丁红线**：严禁采用 `eventFilter` 截获 `QKeyEvent` 原始按键的补丁做法，杜绝输入框（搜索框/重命名文本框）打字按 `Ctrl+Z` 时越权截获误触发文件大撤销的严重漏洞。
2. **声明式 QShortcut 与 WindowContext 作用域红线**：全系统快捷键统一使用 Qt 官方声明式 `QShortcut` 实现，快捷键 Context 强制指定为 `Qt::WindowShortcut`，确保仅在应用活动窗口内生效，100% 绝不上杀侵入操作系统全局钩子，且 Qt 底层自动处理输入框获焦时的焦点协调。

### 面板中介者与跨面板事件路由顶层解耦规范 (PanelMediator)
1. **彻底拔除宿主友元特权与指针依赖红线**：面板中介者（PanelMediator）仅作为独立的跨面板信号路由器，构造函数显式接收各子面板与地址栏指针，绝不保存主窗口（MainWindow）宿主指针。严禁在主窗口或任何视图头文件中使用 `friend class PanelMediator` 伪解耦破坏类封装。
2. **黑盒 Model 访问与标准契约角色红线**：中介者在处理视图选中项数据读取时，必须严格通过 Qt 标准 `QModelIndex::data(index, role)` 与标准角色接口（如 `TagsRole`、`RatingRole`、`ColorRole`、`EncryptedRole`）进行传输，绝对禁止将模型强转为具体 Model 实现类指针或跨层强行读取/遍历 Model 内部私有数据结构。
3. **QPointer 内存安全与 UI 呈现纯洁性红线**：中介者内部引用的所有 UI 组件统一采用 `QPointer<T>` 安全指针包装，防止野指针解引用风险；彻底剥离中介者内部的 HTML 文本拼接、全局屏幕坐标居中计算及 UI 动画控制算式，确保中介者职责纯净单一。

### 路径导航与历史栈服务顶层解耦规范 (NavigationService)
1. **全局路径状态与历史栈统一收敛红线**：全系统路径状态（`m_currentUrl`）、协议归一化解析（`file://`、`computer://`、`trash://`）、前进/后退双向历史栈状态机、上级路径解析以及最近访问记录持久化必须 100% 独立于 `NavigationService` 领域服务，主窗口（MainWindow）与视图层严禁持有路径历史栈变量或自行计算层级关系。
2. **UI 零感知与单向事件流广播规范**：`NavigationService` 归属于 Domain 领域层，绝对禁止包含任何 UI 视图或控制器头文件。路径变更（`currentUrlChanged`）与导航按钮可用性（`navStateChanged`）统一通过单向信号广播，由 Controller 层（如 `PanelMediator`）订阅并联动更新 UI 状态与清空临时搜索/筛选视图。
3. **虚拟协议与物理路径分级审计红线**：对于 `computer://`（此电脑）及 `trash://`（回收站）等虚拟协议，导航服务必须准确识别并拦截上级跳转逻辑，且禁止将虚拟协议注入操作系统或 SQLite 数据库的本地文件访问历史记录。

### 核心文件生命周期与剪贴板服务模块化拆分规范 (TrashService, PermanentDeleteService, ClipboardService)
1. **视图层物理 I/O 完全剥离红线**：视图层（如 `ContentPanel`）仅负责 UI 交互、控件呈现与视图刷新，严禁就地书写物理文件擦除、剪贴板 MIME 数据解析、多线程物理传输或底层回收站数据库存取逻辑。
2. **回收站服务 (TrashService) 撤销闭环规范**：回收站生命周期统一由 `TrashService` 承载，封装移入回收站、选定还原、全量还原与定向恢复，并强绑定快照撤销引擎 (`OperationSnapshotEngine`) 与轻量级反馈 (`UndoToastOverlay`) 构成原子闭环。
3. **永久删除服务 (PermanentDeleteService) 异步擦除规范**：物理粉碎与永久删除统一由 `PermanentDeleteService` 承载，涉及磁盘重度 I/O 擦除的操作必须隔离在 `QtConcurrent` 后台工作线程，并通过进度回调主线程刷新 UI，禁止阻塞主事件循环。
4. **剪贴板服务 (ClipboardService) 多态传输与智能防死循环规范**：剪贴板操作统一由 `ClipboardService` 承载，收拢复制、剪切、粘贴判定与物理传输；智能识别剪贴板图片并直接保存为本地图像文件；严格校验层级包含关系，杜绝将父目录复制/剪切入子目录引发的死循环。

### 无边框窗口壳体归一化顶层规范 (FramelessWindowHelper)
1. **窗口壳体交互归一收敛红线**：顶级窗口（主窗口、对话框等）的无边框交互（包含 8 方向边缘感应、DPI 动态热区计算、光标图形切换、边缘拉伸、标题栏拖拽移动、双击最大化/还原及最大化拖拽还原吸附）必须统一由 `FramelessWindowHelper` 收敛控制，严禁散落在各类主窗口或多个事件过滤器中。
2. **底层几何数学算式物理清除红线**：顶级窗口类必须保持绝对纯洁，严禁重写底层鼠标事件虚函数或内嵌复杂的边缘坐标差值算式，窗口构造函数仅保留标准装配接口（如 `FramelessWindowHelper::apply(this, titleBar)`）。
3. **平台级置顶抽象隔离红线**：跨平台置顶/取消置顶抽象统一收拢于 `FramelessWindowHelper::setAlwaysOnTop`，严禁在 UI 业务代码中直接包含或调用 Win32 原生 `SetWindowPos` API。

### 多栏布局与显隐管理顶层规范 (PanelLayoutManager)
1. **多栏布局空间生命周期收敛红线**：核心多栏分割条（`QSplitter`）的拉伸权重分配、历史分栏尺寸恢复、230px 黄金比例重置、各子面板显隐切换控制、布局右键菜单构建及配置落盘持久化全权交由 `PanelLayoutManager` 独立承载，严禁在 `MainWindow` 内部书写分栏几何算式或配置读取逻辑。
2. **内容区物理不可隐藏防御红线**：内容面板（`ContentPanel`）作为主资产呈现核心，必须在布局管理器中强行锁定为绝对可见，严禁提供将其完全隐藏的选项，防止界面出现全空白极端异常。
3. **动态最小窗口宽度保底红线**：主窗口的 `minimumWidth` 必须根据当前实际可见子面板的数量动态重算（保底基线 465px），确保面板在窄屏下不被挤压变形。

### 工业级 Clean Architecture 重构路线图与命名治理顶层规范
1. **重构演进路线图（从根底重构与五层架构对齐）**：
   - **第一阶段：窗口壳体层收拢与全局事件降权**：收拢无边框拖拽、拉伸及标题栏交互（`FramelessWindowHelper`），事件过滤器严禁无校验全局挂载。
   - **第二阶段：主窗口（上帝类）拆分与控制协调层重构**：将 `MainWindow` 瘦身为纯装配壳体，彻底剥离导航控制器 (`NavigationController`)、右键上下文菜单控制器 (`ContextMenuController`) 和盘符控制器 (`DriveBarController`)。
   - **第三阶段：Model-View 协议规范与伪解耦清理**：彻底清理 `friend class` 侵入；强约束 `model->data(index, role)` 规范；批量操作严禁循环发射事件。
   - **第四阶段：数据基础设施落盘与异步线程隔离**：确保重型磁盘 I/O 与媒体解析完全隔离在后台工作线程，UI 主线程零阻塞。
   - **第五阶段：全局接口契约冻结与历史 Legacy 清场**：彻底清理废弃代码与僵尸文件，完成最终静态规范审计。

2. **重构命名治理红线**：
   - **类名与文件名（PascalCase 1:1 映射）**：类名统一采用 PascalCase 大驼峰；文件名必须与类名 1:1 严格对应（例如 `TaskProgressController.h/cpp`），严禁临时无意义名称。
   - **函数名与变量名（camelCase 表达力约束）**：函数名与变量名统一小驼峰；私有成员强约束 `m_` 前缀；严禁动词模糊的无脑命名（如 `doIt()`、`process1()`）。
   - **接口重命名平滑过渡**：重命名不规范的 Public 接口时，必须保留或提供 inline 转发函数，绝对保证既有调用方的编译契约不受破坏。
