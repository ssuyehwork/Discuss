# 任务：把 QuarkMeta 的主界面复制到 RapidNotes，再按 RapidNotes 现有数据库做适配

QuarkMeta 管理文件，RapidNotes 管理文本笔记。两者界面一致，只有数据来源不同。

## 最高规则

1. **先复制，后适配**。第一步把下面"保留清单"里的 QuarkMeta 源文件原样复制进 RapidNotes，保持类名与逻辑，先让界面和交互完整跑起来。第二步才按"适配清单"替换数据来源。
2. **UI 与样式参数一个数值都不许改**：尺寸、间距、颜色、objectName、样式表、图标、动画、快捷键全部照搬。你不得用自己的想法重写、优化或"简化"任何界面。
3. **交互方式不许改**。例如重命名是行内编辑，就必须保持行内编辑，不得改成弹窗。
4. **禁止另起炉灶**。不得自己新写 PanelLayoutManager、PanelMediator、AddressBar、LibraryPanel 等同名类，必须使用复制来的那一份。
5. 不打补丁、不在界面层夹带数据库逻辑、不重复存储同一份数据。遇到需要决策的地方，不要猜，在最后的汇报里列出来。

## 持久化隔离（硬规则，违反即不通过）

RapidNotes 和 QuarkMeta 是两个独立应用，**任何持久化数据都不得共享**。

1. 存储位置本身必须独立，不是只给配置键加个前缀。涉及：配置（AppConfig / QSettings）、窗口几何、分栏状态、标签页状态、面板显隐、搜索历史、标签词库、数据库、缓存目录、临时文件、注册表键。
2. 复制来的 AppConfig 及所有读写配置、历史、缓存的类，必须指向 RapidNotes 自己的存储（沿用 RapidNotes 现有的配置组织名与应用名，或 RapidNotes 自己的目录），代码里不得出现 QuarkMeta 的目录、文件名、组织名、注册表键。
3. 拖放用的 mime 类型 `application/x-quarkmeta-*`（标签页、窗格移动等）全部改成 RapidNotes 专属名称，防止两个应用之间互相拖拽串扰。
4. 窗口标题、Logo 图标、对象名里的 "QuarkMeta" 改为 RapidNotes 对应项（Logo 沿用 RapidNotes 现有的），样式数值不变。
5. 完成后全局搜索 "QuarkMeta"、"quarkmeta"、".QuarkMeta"，代码、配置、路径、资源名中不得残留（命名空间与类名除外，且类名不得与 RapidNotes 现有类冲突）。

## 启动入口

保持现状：程序启动仍显示 QuickWindow，点击其工具栏 `btnWindow` 唤起 MainWindow。保留 RapidNotes 现有的 `MainWindow::instance()` 与 `showAndActivate()`；关闭行为、托盘沿用 RapidNotes 现状，不带入 QuarkMeta 的 TrayController 和关闭逻辑。

## 一、保留清单（原样复制）

- MainWindow（外壳、无边框、状态栏 32px、悬浮进度条、分割条 5px、边距 5px）
- TitleBarWidget、TabBarWidget（标签页保留）
- NavBarWidget、AddressBar、BreadcrumbBar、SearchController、SearchHistoryPanel
- SidebarContainerWidget（只保留"库"这一个选项卡）、LibraryPanel
- ContentPanel 及其全部控制器（含多窗格：ContentPaneSplitManager、PaneActivationTracker 等）、ContentHeaderWidget
- 卡片：ThumbnailDelegate、CardPainterHelper、CardLayoutEngine、TreeItemDelegate、RenameCapableDelegate、DropJustifiedView、DropTreeView、SectionProxyModel、FilterProxyModel
- MetaPanel
- FilterPanel 及其分组类
- PanelLayoutManager、PanelMediator、AppShortcutController
- NavigationService、CoreEngine（命令入口）、CentralEventHub、ModelContract、FilterStateModel、ScanStatsEngine、ContentStatsWorker

## 二、删除清单

- 导航面板 NavPanel、盘符栏 DriveBarWidget、收藏夹 FavoritePanel、标记 MarkedPanel，以及它们在 MainWindow、PanelLayoutManager、PanelMediator、状态栏按钮里的全部引用。面板由 5 个变为 4 个：库（侧栏）、内容、元数据、筛选。
- 状态栏按钮只保留：排序方向、网格、列表、重置分栏、库、内容面板、元数据、筛选器。删除分栏、自适应、导航、收藏栏按钮。标题栏视图菜单只保留网格、列表。
- 所有磁盘直连与文件操作：DiskItemModel、DiskScanService、loadDirectory、ColumnView、回收站、DiskIoService、FileCollisionDialog、ClipboardService、资源管理器相关菜单、物理路径与"打开位置""复制路径"、文件类型/大小/尺寸等文件属性行。
- 图像颜色：自动取色、调色板、ColorPill、MediaExtractorPipeline、AutoColorGroup、FilterState 中的 colors/colorHue/colorPrecision/minColorArea。**只保留手动颜色标记**。
- 缩略图与图片属性：缩略图提取、纵横比、图像比例分组、缩略图状态分组、文件大小分组、重复状态分组。
- 智能文件夹整个分组。
- MetaPanel 的"关联网址"区块（RapidNotes 没有该字段）。
- MetadataManager、MetaMemoryCache、.QuarkMeta.json 侧车、QuarkMetaJsonStore、LibraryDao、library_category_paths 等 QuarkMeta 自己的存储层。RapidNotes 的数据库是唯一的数据来源，不得再造第二份缓存或副本。

## 三、适配清单（按 RapidNotes 现有数据库）

数据库访问只允许经过一层新建的薄接口（例如 NoteRepository），界面层不得直接 include DatabaseManager。

**1. 条目标识**
界面里原本用文件路径做键的地方（PathRole、信号里的 QStringList、CoreEngine 命令的 targetPaths）统一使用字符串键 `note://<笔记id>`。所有 QFileInfo、QDir 判断一并清除。

**2. 分类（库侧栏）**
- 来源：`categories` 表（is_deleted=0），字段 parent_id、name、color、sort_order、is_pinned。
- 系统节点：全部数据（-1）、未分类（-2）。
- 数量：只调用一次 `getCounts()` 取得所有分类数量，禁止对每个分类单独查询。保持 RapidNotes 现有口径：只统计直属笔记，不递归子分类。
- `categories` 新增列 `icon_key`（默认 `Data-Classification.svg`），用于分类图标。
- 排序、拖拽重排、置顶（快速访问）、重命名、新建、删除、预设标签，对接 `updateCategoryOrder`、`toggleCategoryPinned`、`renameCategory`、`addCategory`、`softDeleteCategories`、`setCategoryPresetTags`。
- 地址栏与标签页的"路径"格式使用 `library://<分类id>`，沿用 QuarkMeta 已有的这套 URL。面包屑按 parent_id 链生成"库 → 分类 → 子分类"，每一节点携带自己的分类 id 与名称（结构化，不再用字符串拼接再拆分）。地址栏不再有可输入物理路径的编辑态，双击保留历史列表。

**3. 笔记列表（内容面板）**
- 用 `searchNotes` 的筛选口径（category / all / uncategorized），但**不得读取 content 与 data_blob 整列**。新增一个轻量查询，只返回：id、title、content 前 200 字摘要、tags、rating、color_label、is_pinned、item_type、created_at、updated_at、remark。必须分页或分批，不得一次性全量载入。
- 字段映射：星级 = `rating`；备注 = `remark`；标签 = `tags`（用 TagCodec）；置顶 = `is_pinned`；手动颜色标记 = 新列 `color_label`。
- **`notes` 表新增列 `color_label TEXT DEFAULT ''`**，专门存手动颜色标记。现有 `color` 列保持原状，一律不动，不得复用。
- 卡片中间区域：显示 content 的文本摘要（具体绘制写在一个新增的独立函数里，不得改动 `renderDefaultIcon` 和 `drawCardCover`）。卡片外框、选中边框、置顶图标、星级、颜色标记胶囊、标题两行省略全部照搬。原"扩展名徽章"改为显示笔记类型（item_type）。
- 列表视图的"尺寸/大小/类型"列改为：字数、类型、创建日期、修改日期，列宽沿用。
- 行内重命名保持行内编辑，改的是笔记标题 `title`。

**4. 修改元数据（星级、色标、标签、备注、置顶、重命名）**
- 入口只有 CoreEngine 的命令；CoreEngine 不得再 include 或调用任何 UI 类（PanelMediator、ContentPanel）。
- 写库时**只更新被修改的那一列**，不得修改 `updated_at` 与 `last_accessed_at`，不得触发整表 `noteUpdated` 刷新。刷新方式是对受影响的行发 `dataChanged`。
- 批量修改放进一个事务一次提交。不得逐条提交，不得每次写入都执行 checkpoint 或整库备份（沿用 DatabaseManager 现有的 7 秒闲置同步机制）。
- 点击一次星级只写一次库：事件处理里只通过命令写，不得在命令之后再调用 `model->setData` 触发第二次写入。

**5. 多窗格**
- 保留 QuarkMeta 全部多窗格能力：双击内容区空白返回上级（上级 = 父分类）、拆分、合并标签、拖动窗格、拖标签页到内容区分屏。
- 元数据变更事件必须刷新**所有**窗格里包含该笔记的行，不能只刷新"活动窗格"和"根窗格"。
- 窗格拖动不得再把 ContentPanel 裸指针写进 mime 数据，改成窗格编号并在窗格表里查找。
- 把笔记从一个窗格拖到另一个窗格 = 移动到目标窗格正在显示的分类；拖到侧栏分类节点同理。

**6. 线程**
- RapidNotes 的主数据库连接只能在主线程使用。需要后台加载时，使用单独的只读连接（独立连接名，WAL 模式下可并行读），所有窗格共用同一个只读读取层，不得每个窗格各开一个连接。
- 窗格切换分类时用 QuarkMeta 现有的 loadRequestId 方式丢弃过期结果。

**7. 搜索**
- 保留搜索框外观与搜索历史。范围菜单的两项改为"搜索：当前分类""搜索：全库"。
- 查询直接走 SQL（匹配 title、remark、tags），不得先取出全库条目再在内存里过滤，不得每次搜索调用建表语句。保持现有设计：不搜索正文 content。

**8. 筛选面板**
保留分组：颜色标记（只有手动色标）、标签、评级、笔记类型、创建日期、修改日期、备注。统计与过滤只走 QuarkMeta 的内存过滤与统计这一条路径，不得再并行使用 DatabaseManager 的 `getFilterStats`。

**9. 迁移**
已有的真实笔记不迁移文件，只保留文本；新增的列都有默认值，现有数据无需改动。

## 四、验收

1. 启动后界面与 QuarkMeta 肉眼一致，只有 4 个面板，没有导航与盘符栏。
2. 切换分类、搜索、筛选、评级、色标、标签、备注、置顶、行内重命名、多窗格拆分与合并、标签页拖放全部可用。
3. 改一次星级只产生一次数据库写入，卡片位置不变。
4. 大量笔记（含图片 blob）时切换分类不卡顿。
5. RapidNotes 与 QuarkMeta 同时运行，互不影响：配置、窗口状态、历史、数据库都各自独立。
6. 汇报里单独列出：你遇到的需要用户决策的地方、没有照搬的地方及原因。
