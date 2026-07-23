# ArcMeta 职责单一原则 (SRP) 与模块化架构负债审查建档清单 (ARCHITECTURE_DEBT.md)

本文件作为 ArcMeta 项目的核心架构负债账本，系统性地扫描并记录了代码库中所有违反“职责单一原则 (SRP)”与“模块化标准”的类、函数和模块。本账本是后续重构工作的唯一权威依据。已发现的违规项在重构并验证通过前，其状态将持续保持为“待处理”。

---

## 优先级：高

### ## [001] src/meta/MetadataManager.cpp :: MetadataManager

- 状态：待处理
- 判定类型：2.1 God Object / 2.3 数据层与业务层混杂
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 内存缓存中心：在内存中维护 `m_cache` 哈希表，存储数百万条 RuntimeMeta 镜像并进行高频读写。
  2. 物理指纹提取器：利用 Windows Win32 API 提取文件的 128-bit File ID (FRN) 及其物理大小、属性、时间戳（`fetchWinApiMetadataDirect`）。
  3. 倒排索引索引器：维护文件名、文件夹名及文件后缀名的反向索引（`m_fileNameToFids`，`m_folderNameToFids`，`m_extensionToFids`）。
  4. 数据持久化执行器：通过 SQLite API 执行 SQL 数据拼接，在后台线程中异步或批量将内存镜像写入物理分库中（`persistAsync` / `persistBatchAsync`）。
  5. 双轴颜色特征量化分析：调用多媒体算法分析图像宽高并尝试进行显著色与色板提取及聚类（`tryExtractColor` 等）。
- 代码证据：
  - `MetadataManager::fetchWinApiMetadataDirect`：在第 2145 行直接调用 Win32 原生 API `GetFileInformationByHandleEx` 获取物理文件系统底层的 FID 资源标识。
  - `MetadataManager::persistAsync`：直接使用 `sqlite3_prepare_v2` 与拼接 SQL `INSERT INTO metadata ...` 绕过持久层直接写库。
  - `MetadataManager::searchInCache`：在第 1750~1802 行中，直接持有全局 `m_mutex` 读写大锁，对内存中数百万条记录执行单线程 $O(N)$ 线性大循环，这在百万级数据下导致主线程高频读锁阻塞假死。
- 拆分方案：
  - 新建 `PhysicalMetadataExtractor` (物理特征提取器)：专职负责调用 Windows Win32 原生 API 提取 128-bit FID、文件物理时间戳与基础属性。
  - 新建 `MetadataRepository` (持久化仓储)：专职负责将内存中的 RuntimeMeta 同步拼装为 SQL 语句写入物理 SQLite 数据库。
  - 新建 `MediaFeatureAnalyzer` (多媒体分析器)：抽取色板提取、聚类及宽高尺寸识别等重型 CPU 密集型任务。
  - 原类收敛为：`MetadataCache` (内存高速缓存镜像)，仅负责高速、线程安全的 RuntimeMeta 读写缓存。
  - 依赖解耦方式：基于信号槽与异步通道将 `PhysicalMetadataExtractor` 及 `MediaFeatureAnalyzer` 的提取结果，通过统一的 `MetadataService` 控制流提交给 `MetadataCache` 更新，再由 `MetadataRepository` 通过批处理事务线程排队刷入数据库。
- 历史重构备注：之前历史重构增加了 FTS5 分流和 Trigram 检索，但核心 God Object 依旧存在，无法用一句话说清职责，属于无效历史重构。
- 优先级：高 (百万级数据下，searchInCache 的 $O(N)$ 读大锁在扫描和高频写时会导致界面硬性假死甚至崩溃)

---

### ## [002] src/ui/ContentPanel.cpp :: ContentPanel

- 状态：待处理
- 判定类型：2.1 God Object / 2.2 绘制/渲染层职责过载
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. UI 视口承载与展示：管理网格视图（`m_gridView`）、列表视图（`m_treeView`）的几何排版和尺寸滑杆逻辑。
  2. 物理文件 I/O 扫描：在 `loadDirectory` 内部，通过 `QThreadPool` 发起后台大循环物理 QDirIterator 目录 DFS 扫描，承担了物理文件探测重任。
  3. 系统事件全局拦截：在 `eventFilter` 中对键盘空格、F2、退格、多选、复制、剪切、粘贴、Ctrl+滚轮事件以及 Rating 点击命中范围执行全量拦截和坐标转化。
  4. 历史足迹记录：记录和落盘用户最近访问的托管文件夹（`recordRecentVisitedFolder`）。
  5. 物理迁移调度：直接在 `ActionAddToCategory` 右键菜单分支和拖拽逻辑中，判定盘符漂移并调用 `ImportHelper::importPaths` 执行大文件物理移动、复制、重命名等。
- 代码证据：
  - `ContentPanel::loadDirectory`：在第 1520~1560 行直接拉起后台线程，执行 DFS 式的本地磁盘物理遍历，严重穿透到文件系统。
  - `ContentPanel::eventFilter`：在第 850~1020 行混合了快捷键、星级 Hitbox 点击测试、颜色 QSS 重建等多项杂乱职责。
- 拆分方案：
  - 新建 `DirectoryScanner` (物理目录扫描器)：专职负责后台磁盘遍历及扫描结果组装。
  - 新建 `ShortcutHandler` (快捷键处理器)：解耦按键及手势事件过滤。
  - 新建 `FileTransferController` (文件传输控制器)：接管复制、剪切、粘贴、拖拽、以及迁移等物理大 I/O 调度。
  - 原类收敛为：`ContentPanel` (纯视图管理器)，仅负责管理子视图 QStackedWidget 和顶层标题栏/滑杆布局。
  - 依赖解耦方式：视图层仅通过 `DirectoryScanner` 抛出的 `resultsReady` 信号更新 Model；物理操作通过发出信号，交由上层 Service 处理。
- 历史重构备注：此前针对视图进行了重构并引入了虚拟模型，但 ContentPanel 依然是上帝类，对物理扫描、迁移大 I/O 和事件过滤高度耦合，职责依旧极其臃肿。
- 优先级：高 (在库外大物理磁盘中，全盘物理 DFS 扫描极度依赖物理硬盘性能，主线程容易被 I/O 等待锁死)

---

### ## [003] src/ui/CategoryModel.cpp :: CategoryModel

- 状态：待处理
- 判定类型：2.3 数据层与业务层混杂
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 视图数据绑定：将“我的分类”及系统分类结构转化为 Qt 模型单元格数据，为侧边栏 QTreeView 提供数据绑定。
  2. 物理重命名：在 `setData` 单元格被编辑时，通过 `QFile::rename` 直接修改物理磁盘中托管库文件夹的物理名称。
  3. 数据库持久化：在 `setData` 单元格中，通过后台线程触发 `CategoryRepo::update` 将修改刷写到底层 SQLite 中。
- 代码证据：
  - `CategoryModel::setData`：在第 350~380 行，当判断为分类编辑时，在子线程内直接调用了 `QFile::rename(oldPath, newPath)` 这一底层的物理重命名命令。
- 拆分方案：
  - 新建 `CategoryService` (分类业务控制器)：负责处理分类的更名、删除和合并等高级事务，包括驱动磁盘重命名和调用仓储持久化。
  - 原类收敛为：`CategoryModel` (纯模型层)，在数据被修改时抛出业务更名信号（如 `renameRequested`），不直接在 `setData` 内部干涉物理 I/O 和持久化逻辑。
  - 依赖解耦方式：Controller 监听 Model 的编辑请求，执行底层 `CategoryService` 业务，处理完成后触发 Model reload。
- 历史重构备注：为了解决更名引起的锁冲突，此前将更名移入了 `QtConcurrent::run` 后台，但这反而进一步加深了 Model 穿透修改物理磁盘的 SRP 严重缺陷。
- 优先级：高 (Model 的 `setData` 是 Qt UI 机制高频调用的地方，在里面现场执行重型物理 I/O 与持久化，极易导致模型状态不一致、崩溃和难以排查的竞态死锁)

---

### ## [004] src/ui/TagManagerView.cpp :: TagManagerView

- 状态：待处理
- 判定类型：2.2 绘制/渲染层职责过载
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 标签视图渲染：绘制三栏标签组布局、流式布局（FlowLayout）和分组字母标题。
  2. 数据库直写：在 `addTagToGroup`、`deleteGroup` 内部，通过 `QtConcurrent::run` 在后台直接拼写 SQL，穿透到具体的物理分库，绕过仓储层进行原子 SQL 直写。
- 代码证据：
  - `TagManagerView::addTagToGroup` 等：在第 340~425 行等多处，直接在 QWidget UI 内部调用了后台线程来通过 `TagRepository` 间接（或内部直接通过 sqlite API）直写底层连接，甚至在多处硬编码了获取 C 盘分库连接：`DatabaseManager::instance().getMemoryDb(L"C")`（第 338、358 行等）。
- 拆分方案：
  - 新建 `TagGroupController` (标签业务管理器)：接管标签加组、解组、重命名与删除等具体业务控制逻辑。
  - 原类收敛为：`TagManagerView` (纯绘制 UI 界面)，仅通过事件或动作通知 `TagGroupController`。
  - 依赖解耦方式：UI 通过发射信号（如 `addTagRequested`）将业务行为上抛给业务控制层，禁止在 QWidget 内进行任何多线程业务下发或硬编码特定盘符连接。
- 历史重构备注：无。
- 优先级：高 (作为 QWidget UI 类直接深度参与 SQL 持久化和盘符获取，绕过了统一的业务分发总线，在百万级多盘高频写入时会由于 SQLite_BUSY 导致闪退或标签数据一致性崩塌)

---

## 优先级：中

### ## [005] src/meta/DatabaseManager.cpp :: DatabaseManager

- 状态：待处理
- 判定类型：2.1 God Object / 2.3 数据层与业务层混杂
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 连接池生命周期管理：打开、关闭以及克隆内存/磁盘 SQLite 数据库（`loadDb` / `saveDb` / `closeDb`）。
  2. 物理文件属性控制：在 `loadDb` 中直接穿透物理边界，调用 Windows Shell API 设置数据库文件的隐藏属性（`ShellHelper::ensureHidden`）。
  3. 盘符纠偏路由：在 `getMemoryDb` 中包含“盘符漂移”时的物理路径自适应路由计算、重对账及冗余文件对账逻辑。
- 代码证据：
  - `DatabaseManager::loadDb`：在第 132 行直接硬编码调用了 `ShellHelper::ensureHidden(diskPath)` 去标记磁盘文件。
  - `DatabaseManager::getMemoryDb`：在第 305~330 行，检测到盘符变化时，自动在内存连接中触发克隆、关闭、重命名及对账重建连接。
- 拆分方案：
  - 新建 `VolumeChangeCoordinator` (盘符重定位协调器)：专职负责捕捉磁盘序列号、盘符变化，并计算纠偏后的数据库文件路径。
  - 新建 `DatabaseStorageGuard` (数据库文件系统守卫)：专职负责数据库文件在磁盘物理目录下的创建、备份及隐藏属性设置。
  - 原类收敛为：`DatabaseConnectionManager`，仅负责单纯的内存连接池生命周期开闭与读写大锁分发。
  - 依赖解耦方式：当磁盘卷发生漂移时，由 `VolumeChangeCoordinator` 统筹并重新获取 `DatabaseConnectionManager` 的连接，杜绝连接管理器反向操作盘符业务。
- 历史重构备注：之前增加的秒退架构（Plan-130）废除了 flushStep，但 `DatabaseManager` 作为连接层去反向操控 Windows 物理属性和漂移路由的基本架构缺陷依然原封不动。
- 优先级：中 (连接池高耦合了重路由和物理操作，导致盘符发生频繁插入/拔出时极易产生死锁或句柄泄露，且阻碍了核心基础设施的独立单元测试)

---

### ## [006] src/meta/CategoryRepo.cpp :: CategoryRepo

- 状态：待处理
- 判定类型：2.1 God Object / 2.3 数据层与业务层混杂
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 内存/数据库持久化 CRUD：管理分类、分类项在 SQLite 及 SCCH 缓存模式下的直接写入与读取。
  2. 启动期对账重计：在 `fullRecount` 中执行对百万级数据的全量大对账审计，遍历 `MetadataManager` 全量快照更新计数。
  3. 回收站高级业务状态转换：在 `moveToTrashBatch` 内部干预重型回收站业务。
  4. 全局计数指标维护：通过 7 个全局静态 `std::atomic<int>` 原子计数器直接对外提供跨线程状态指标统计服务。
- 代码证据：
  - `CategoryRepo::fullRecount`：在第 800~820 行，初始化时循环全量内存快照执行文件分类数目的校正。
  - `CategoryRepo::moveToTrashBatch`：包含物理回收、还原以及内存缓存标记修改。
- 拆分方案：
  - 新建 `SidebarCountAggregator` (侧边栏原子计数聚合器)：专职管理 `s_totalCount` 等高频、跨线程原子指标，利用缓存行对齐 (Cache Line Alignment) 防止 CPU Cache Line Bouncing 性能衰退。
  - 新建 `TrashBinManager` (回收站业务管理器)：解耦批量移入回收站、物理彻底抹除以及还原等复杂回收逻辑。
  - 新建 `CategoryDatabaseAudit` (分类数据审计类)：负责定时或启动时的全账本重计，不阻塞启动通道。
  - 原类收敛为：`CategoryRepository`，纯粹作为 Category 结构在 SCCH 和 SQLite 连接中的基础 CRUD 数据访问层（DAO）。
  - 依赖解耦方式：上层业务 Controller 调用 `TrashBinManager` 处理回收事务，业务变更完成后由 `SidebarCountAggregator` 捕获信号实现单点原子增量统计。
- 历史重构备注：虽然在物理层面转向了 SCCH 架构，但 `CategoryRepo` 承担的启动全量重计、全局指标控制和重型回收事务的职责过载并未根治，职责极其不单一。
- 优先级：中 (百万级数据量下，全账本重计和移入回收站时的全缓存遍历将使界面假死数十秒)

---

### ## [007] src/core/CoreController.cpp :: CoreController

- 状态：待处理
- 判定类型：2.1 God Object
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 系统生命周期控制：统筹整个 ArcMeta 底层初始化、异步就绪调度、监控线程唤醒。
  2. 监控边界穿透：在 `startSystem` 中直接通过物理 QDir Drives Drives 探测，并反向调用 `NativeFolderWatcher::addWatch` 深度参与了物理文件夹实时监控服务 IOCP 细节。
  3. 双轨搜索统筹：在 `performSearch` 内部，拉起线程直接启动物理磁盘的 DFS 全盘迭代，将业务中控与具体的物理 I/O DFS 检索细节深度硬绑定。
- 代码证据：
  - `CoreController::startSystem`：在第 45~75 行，直接硬编码了物理盘符监测并向 `NativeFolderWatcher` 注册 IOCP 句柄。
  - `CoreController::performSearch`：直接作为检索执行者拉起后台线程调度双轨检索，控制 `searchStarted` 和 `searchFinished` 流程。
- 拆分方案：
  - 新建 `SearchDispatchService` (检索调度服务)：专职统筹内存缓存 FTS5 trigram 模糊检索及物理磁盘扫描，对外提供统一异步检索接口。
  - 新建 `WatcherBootstrap` (监视器启动加载器)：专职负责在系统就绪后加载并向监视总线（NativeFolderWatcher / USN Watcher）注册路径。
  - 原类收敛为：`CoreController` (系统生命周期中控)，仅作为全局就绪标识、全局状态栏文字及系统初始化状态机控制。
  - 依赖解耦方式：中控类仅负责在系统就绪时触发各子系统 Service 的 `initialize` 动作。搜索和监控通过完全独立的 Manager 完成，对外只暴露信号或回调。
- 历史重构备注：之前历史重构废除了双轨制监控（移除了 USN 监听，仅保留单一 IOCP 轨），但这并不能掩盖 CoreController 去操作 IOCP 监控注册与 I/O DFS 扫描的严重穿透。
- 优先级：中 (系统生命周期类与物理检索、物理监控底座高频耦合，任何细小磁盘物理操作异常均会直接反馈导致系统初始化崩溃)

---

### ## [008] src/core/AutoImportManager.cpp :: AutoImportManager

- 状态：待处理
- 判定类型：2.1 God Object
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 变更捕获对账：负责 MFT/USN 增量物理捕获以及在 DFS 递归对账中物理校准（`syncAllManagedLibraries`）。
  2. 物理目录分级建立：在扫描对账发现不匹配时，调用 `handleRecursiveIngestion` 在内存和数据库中 1:1 动态还原和分级建立 Category 分类树。
  3. 历史记录管理器：内部通过 AppConfig 维护和管理上限为 14 条的历史访问目录记录。
- 代码证据：
  - `AutoImportManager::syncAllManagedLibraries`：混合了多盘符并行的物理 MFT 加速、USN 日志提取与应用层分类树递归建立。
  - 内部使用 static `std::recursive_mutex s_dbAccessMutex;` 这一全局大物理大锁来保护所有后台写入，严重破坏了 SQLite WAL 并发优势。
- 拆分方案：
  - 新建 `VolumeIngestionService` (磁盘物理扫描对账器)：专职统筹 USN/MFT 物理扫描对账逻辑。
  - 新建 `HistoryAccessManager` (访问足迹管理器)：剥离对账类，专门持久化和节流控制历史 14 条足迹。
  - 原类收敛为：`AutoImportManager`，专职作为后台静默对账控制流程状态机。
  - 依赖解耦方式：对账服务捕获物理变更后，将待建立分类的需求发射信号，交由 `CategoryService` 统一单事务级安全写入，将后台互斥大锁 `s_dbAccessMutex` 拆解并细化为按分库/按路径段隔离的轻量锁。
- 历史重构备注：无。
- 优先级：中 (全局大锁会导致后台线程在 MFT 扫描数十万级数据时，前台操作同一分库标签陷入数秒的强制阻塞锁等待)

---

## 优先级：低

### ## [009] src/ui/UiHelper.h :: UiHelper

- 状态：待处理
- 判定类型：2.1 God Object
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 上帝辅助类：代理并承接了 QPainter 圆角渲染、Windows Shell COM 缩略图拉取、CIE76 显著色差提取算法、QFuture 跨线程异步图标排队通知。
- 代码证据：
  - `UiHelper` 名义上是 QSS 样式及 QPainter 工具类，实际包含并代理了 `WindowsShellThumbnailProvider::getShellThumbnail`、`MediaColorExtractor::extractPalette` 等来自不同组件底座的大量高内聚物理接口。
- 拆分方案：
  - 废除无实体、高扇出、高扇入的代理上帝类 `UiHelper`。
  - 在底层组件直接暴露干净的物理接口：UI 渲染需要图标直接调用 `SvgIconRenderer`；物理提取直接调用 `WindowsShellThumbnailProvider`；色差计算调用 `MediaColorExtractor`。
  - 依赖解耦方式：强制各模块直接引用具体的、职责纯粹的子工具类头文件，禁止通过 `UiHelper` 这一焦油坑类进行多重不相干业务的中转代理。
- 历史重构备注：此前虽然将底层实现拆解到了 `SvgIconRenderer` 等三个纯粹模块，但在 `UiHelper.h` 内部依然通过 `static inline` 这一高耦合代理形式把所有底层全部强行绑定并依赖在一起，导致历史拆解伪重构未达标。
- 优先级：低 (属于高耦合的维护性灾难。其任何微小修改或引入新依赖（如 Mingw 底层 API 变动）均会导致全项目所有模块全量重新编译，严重降低敏捷开发效率)

---

### ## [010] src/ui/ThumbnailDelegate.cpp :: ThumbnailDelegate & TreeItemDelegate

- 状态：待处理
- 判定类型：2.1 God Object / 2.2 绘制/渲染层职责过载
- 发现日期：2026-10-24
- 职责清单（穷举当前承担的所有职责）：
  1. 单元格视觉重绘：控制 View 在网格、合理自适应以及列表行模式下的 QPainter 几何圆角矩形及色卡绘制。
  2. 交互状态机编辑控制：在 `editorEvent` 及 Hitbox 测试内，拦截并转换键盘、鼠标点击，直接反向修改 Model 单元格内的星级、置顶、锁标志、颜色等底层业务元数据。
- 代码证据：
  - `ThumbnailDelegate::editorEvent` 或在点击 hitbox 拦截分支中，通过强行转换 index 重新写入 `RatingRole` 属性，直接在 Delegate 内插手了 Model 数据的持久化触发和交互状态逻辑。
- 拆分方案：
  - 新建 `ViewInteractiveHandler` (视图交互控制组件)：接管网格和列表内针对评分、色卡 Hitbox 的精确几何坐标命中测试与点击事件逻辑，交由控制器处理。
  - 原类收敛为：`GridItemPainter` / `TreeItemPainter`，仅仅作为只读的、无状态的（Stateless）QPainter 纯视觉渲染代理。
  - 依赖解耦方式：View 捕获鼠标点击，计算行和局部列位置，调用 View 内部 Controller 触发 `model->setData` 进行修改，使 Delegate 彻底回归无状态纯绘制角色。
- 历史重构备注：此前在 `ContentPanel` 中增加了一部分事件拦截，但 Delegate 内部依然包含相当比例的交互控制和业务拦截逻辑。
- 优先级：低 (主要发生于视口 Viewport 可见行范围内，其性能开销和危害不会随着全库百万级数据扩展而呈线性爆发，属于典型的 MVC 模式不彻底缺陷)

---
