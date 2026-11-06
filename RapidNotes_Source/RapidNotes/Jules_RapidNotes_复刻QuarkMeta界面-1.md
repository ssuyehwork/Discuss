# 任务：把 QuarkMeta 的主界面复制到 RapidNotes，再按 RapidNotes 现有数据库做适配

QuarkMeta 管理文件，RapidNotes 管理文本笔记。界面一致，只有数据来源不同。

## 最高规则

1. **先复制，后适配**。先把下面"复制范围"里的 QuarkMeta 源文件整文件原样复制，保持类名与逻辑，让界面与交互完整跑起来。再按"适配"部分替换数据来源。
2. **UI 与样式参数一个数值都不许改**：尺寸、间距、颜色、objectName、样式表、图标、快捷键全部照搬。不得重写、优化或"简化"任何界面。
3. **交互方式不许改**。例如重命名是行内编辑，就保持行内编辑，不得改成弹窗。
4. **禁止另起炉灶**。不得自己新写 PanelLayoutManager、PanelMediator、AddressBar、LibraryPanel 等同名类，必须使用复制来的那一份。
5. **不要猜，不要擅自删，也不要擅自发明**。凡是 RapidNotes 数据库里没有对应数据的界面项，保持界面原样且不报错不崩溃，在最后的汇报里逐项列出，由用户决定。除第三节明确列出的两个新增列外，不得自行新增表或列。

## 一、复制范围

**复制：**
- 收藏栏容器 SidebarContainerWidget（只保留"库"这一个选项卡）及 LibraryPanel
- 内容面板 ContentPanel 及其全部控制器与卡片相关类（含星级、手动颜色标记、多窗格、行内重命名）
- 元数据面板 MetaPanel
- 筛选器面板 FilterPanel 及其全部分组类
- 标题栏与标签页（TitleBarWidget、TabBarWidget）、导航栏（NavBarWidget、AddressBar、BreadcrumbBar）、搜索框（SearchController 与历史）、状态栏、悬浮进度条
- 以上依赖的类：PanelLayoutManager、PanelMediator、AppShortcutController、NavigationService、CoreEngine（命令入口）、CentralEventHub、ModelContract、FilterStateModel、ScanStatsEngine、ContentStatsWorker、ColorPicker.h 等，**整文件复制，不拆分**（ColorPicker.h 里的 ColorStripPicker 就是手动颜色标记）

**不复制：**
- 导航面板 NavPanel
- 收藏夹、标记两个选项卡（FavoritePanel、MarkedPanel），以及盘符栏 DriveBarWidget
- 图像颜色相关：自动提取色值、调色板、自动取色的筛选（AutoColorGroup、取色面板）、MediaExtractorPipeline。**手动颜色标记完整保留**
- 磁盘直连与文件操作：DiskItemModel、磁盘扫描、ColumnView、回收站、文件冲突对话框、剪贴板粘贴、资源管理器相关菜单
- QuarkMeta 自己的存储层：MetadataManager、MetaMemoryCache、.QuarkMeta.json 侧车、LibraryDao、library_category_paths。RapidNotes 的数据库是唯一数据来源，不得再造第二份缓存或副本

**按用户要求的界面调整：**
- 状态栏视图模式按钮只保留网格、列表；标题栏视图菜单同样只留网格、列表
- 地址栏只显示当前库里的分类（面包屑"库 → 分类 → 子分类"），物理路径作废，不再有可输入路径的编辑态，双击保留历史列表
- 面板由 5 个变为 4 个：库（侧栏）、内容、元数据、筛选

## 二、启动入口

保持现状：程序启动仍显示 QuickWindow，点击其工具栏 `btnWindow` 唤起 MainWindow。保留 RapidNotes 现有的 `MainWindow::instance()` 与 `showAndActivate()`；关闭行为、托盘沿用 RapidNotes 现状，不带入 QuarkMeta 的 TrayController 与关闭逻辑。

## 三、适配（按 RapidNotes 现有数据库）

数据库访问只允许经过一层新建的薄接口（例如 NoteRepository），界面层不得直接 include DatabaseManager。

**1. 条目标识**：界面里原本用文件路径做键的地方（PathRole、信号里的 QStringList、CoreEngine 命令的 targetPaths）统一使用字符串键 `note://<笔记id>`，所有 QFileInfo、QDir 判断一并清除。

**2. 新增的两列（仅此两列）**
- `notes` 表新增 `color_label TEXT DEFAULT ''`：专门存手动颜色标记。现有 `color` 列保持原状，一律不动，不得复用。
- `categories` 表新增 `icon_key TEXT DEFAULT 'Data-Classification.svg'`：分类图标。

**3. 分类（库侧栏）**
- 来源 `categories` 表（is_deleted=0）：parent_id、name、color、sort_order、is_pinned。系统节点：全部数据（-1）、未分类（-2）。
- 各分类数量只调用一次 `getCounts()`，禁止对每个分类单独查询。沿用 RapidNotes 现有口径：只统计直属笔记。
- 排序、拖拽重排、置顶（快速访问）、重命名、新建、删除、预设标签，对接 DatabaseManager 现有的对应接口。
- 地址栏与标签页的"路径"格式使用 `library://<分类id>`。面包屑按 parent_id 链生成，每个节点携带自己的分类 id 与名称（结构化，不要字符串拼接再拆分）。

**4. 笔记列表与卡片**
- 用 `searchNotes` 的筛选口径（category / all / uncategorized），但**不得读取 content 与 data_blob 整列**。新增一个轻量查询，只返回：id、title、content 前 200 字摘要、tags、rating、color_label、is_pinned、item_type、file_extensions、created_at、updated_at、remark、word_count。必须分页或分批，不得一次性全量载入。
- 字段映射：星级 = `rating`；备注 = `remark`；标签 = `tags`（用 TagCodec）；置顶 = `is_pinned`；手动颜色标记 = `color_label`。
- 卡片外框、选中边框、置顶图标、星级、颜色标记胶囊、标题两行省略全部照搬。
- 卡片左上角徽章：显示笔记类型（文件类显示后缀，多后缀显示"首个 +N"，文件夹显示 DIR，链接 LINK，文本 TXT，代码 CODE，截图 IMG，批量 BATCH）。
- 卡片中心按 item_type 显示（绘制写在新增的独立函数里，不得改动 `renderDefaultIcon` 与 `drawCardCover`）：
  - 文本、代码：content 摘要，自动换行并省略，代码用等宽字体；字号与颜色沿用卡片标题的同一套数值
  - 链接：链接图标加网址文字
  - 截图：图片缩略图，来自 `data_blob`，只在后台用只读连接按需读取并解码，缓存在内存里（有大小上限），不得把缩略图写成磁盘文件
  - 文件、文件夹、批量：系统文件图标，批量类附加"N 个项目"
- 列表视图的列：名称、评分、字数、类型、创建日期、修改日期，列宽沿用。
- 行内重命名保持行内编辑，改的是笔记标题 `title`。

**5. 修改元数据（星级、色标、标签、备注、置顶、重命名）**
- 入口只有 CoreEngine 的命令；CoreEngine 不得再 include 或调用任何 UI 类。
- 写库时**只更新被修改的那一列**，不得修改 `updated_at` 与 `last_accessed_at`，不得触发整表 `noteUpdated` 刷新；刷新方式是对受影响的行发 `dataChanged`。
- 批量修改放进一个事务一次提交。不得逐条提交，不得每次写入都执行 checkpoint 或整库备份（沿用 DatabaseManager 现有的 7 秒闲置同步机制）。
- 点击一次星级只写一次库：命令执行之后不得再调用 `model->setData` 触发第二次写入。

**6. 多窗格**
- 保留 QuarkMeta 全部多窗格能力：拆分、合并标签、拖动窗格、拖标签页到内容区分屏、双击空白返回上级（上级 = 父分类）。
- 元数据变更事件必须刷新**所有**窗格里包含该笔记的行，不能只刷新"活动窗格"和"根窗格"。
- 窗格拖动不得再把 ContentPanel 裸指针写进 mime 数据，改成窗格编号并在窗格表里查找。
- 把笔记从一个窗格拖到另一个窗格 = 移动到目标窗格正在显示的分类；拖到侧栏分类节点同理。

**7. 线程**
- RapidNotes 主数据库连接只能在主线程使用。需要后台加载时，使用单独的只读连接（独立连接名），所有窗格共用同一个只读读取层，不得每个窗格各开连接。
- 切换分类时沿用 QuarkMeta 现有的 loadRequestId 方式丢弃过期结果。

**8. 搜索与筛选**
- 搜索框范围菜单两项改为"搜索：当前分类""搜索：全库"。查询直接走 SQL（匹配 title、remark、tags），不得先取全库再在内存里过滤，不得每次搜索调用建表语句。保持现有设计：不搜索正文 content。
- 筛选与统计只走 QuarkMeta 的内存过滤与统计这一条路径，不得再并行使用 DatabaseManager 的 `getFilterStats`。

**9. 迁移**：已有的真实笔记只保留文本；新增的两列都有默认值，现有数据无需改动。

## 四、持久化隔离（硬规则，违反即不通过）

RapidNotes 与 QuarkMeta 是两个独立应用，**任何持久化数据都不得共享**。

1. 存储位置本身必须独立，不是只给配置键加前缀。涉及：配置（AppConfig / QSettings）、窗口几何、分栏状态、标签页状态、面板显隐、搜索历史、标签词库、数据库、缓存目录、临时文件、注册表键。
2. 复制来的 AppConfig 及所有读写配置、历史、缓存的类，必须指向 RapidNotes 自己的存储（沿用 RapidNotes 现有的配置组织名与应用名，或 RapidNotes 自己的目录），代码里不得出现 QuarkMeta 的目录、文件名、组织名、注册表键。
3. 拖放用的 mime 类型 `application/x-quarkmeta-*` 全部改成 RapidNotes 专属名称，防止两个应用互相拖拽串扰。
4. 窗口标题、Logo 图标、对象名里的 "QuarkMeta" 改为 RapidNotes 对应项（Logo 沿用 RapidNotes 现有的），样式数值不变。
5. 完成后全局搜索 "QuarkMeta"、"quarkmeta"、".QuarkMeta"，代码、配置、路径、资源名中不得残留（命名空间与类名除外，且类名不得与 RapidNotes 现有类冲突）。

## 五、验收

1. 启动后界面与 QuarkMeta 肉眼一致，只有 4 个面板。
2. 切换分类、搜索、筛选、评级、手动色标、标签、备注、置顶、行内重命名、多窗格拆分与合并、标签页拖放全部可用。
3. 改一次星级只产生一次数据库写入，卡片位置不变。
4. 大量笔记（含图片）时切换分类不卡顿。
5. RapidNotes 与 QuarkMeta 同时运行互不影响：配置、窗口状态、历史、数据库各自独立。
6. 汇报里单独列出：RapidNotes 数据库没有对应数据的界面项、没有照搬的地方及原因、需要用户决策的地方。
