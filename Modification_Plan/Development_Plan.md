# Development Plan —— 物理监控自动同步、右键菜单高级排序与三大视图/双尺寸调节移植计划

## 1. 物理监控与侧边栏分类树自动同步需求
### 1.1 核心需求
在停用 USN Journal 后，当用户在内容面板中执行“迁移”将文件夹移动至 `ArcMeta.Library_[盘符]`，或者以其他物理方式在托管库/自定义监控文件夹内创建、拷贝、移入新子目录时，`NativeFolderWatcher` 能够实时检测并在 SQLite 数据库中构建对应的 1:1 分类树记录（即向 `categories` 表里创建分类记录），使新文件夹在无需重启主程序的情况下，立即在侧边栏中刷新呈现。

### 1.2 解决方案概述
在 `NativeFolderWatcher::handleNotification` 中检测到目录级变动（`info.isDir()`）时，通过 `QtConcurrent::run` 异步拉起级联入库同步引擎 `AutoImportManager::instance().handleRecursiveIngestion(fullPath)`。这不仅会自动注册所有子文件和路径，还会递归在 SQLite 的 `categories` 表中补全 1:1 分类结构，最后触发 `"__RELOAD_ALL__"` 信号驱动侧边栏重载，实现完美实时刷新。


## 2. 内容容器右键“排序”主菜单与子选项需求
### 2.1 核心需求
无论在“列表模式”还是“卡片网格模式”下，内容容器中都应该提供统一的交互入口来让用户随意切换数据的排序方式。
系统需要在内容容器右键菜单（空白处及选中项目）中，新增一个主选项为“排序”的二级子菜单。

### 2.2 “排序”二级子菜单定义
- **排序属性**（单选联动，带 check 勾选状态）：
  - **名称**
  - **创建日期**
  - **修改日期**
  - **扩展名**
  - **大小**
  - **尺寸**
  - **评分**
- **分隔线**
- **排序方向**（单选联动，带 check 勾选状态）：
  - **升序**
  - **降序**

### 2.3 解决方案概述
1. **状态维护**：
   in `ContentPanel` 中维护 `SortType m_sortType` 和 `Qt::SortOrder m_sortOrder`。
2. **菜单注入**：
   在右键菜单弹出逻辑中，通过 `QActionGroup` 注入“排序”选项组并同步勾选状态。
3. **底层重排重写**：
   重写 `FilterProxyModel::lessThan` 算法。在排除了第一和第二排序权重后，直接根据 `m_sortType` 属性，提取并对比两个 `ItemRecord` 的对应物理/元数据信息。


## 3. 移植 FERREX-META 三大视图模式与双尺寸调节需求
### 3.1 核心需求
当前内容容器需要完美移植并支持 `FERREX-META` 版本中的全量 3 种视图模式，以及 2 种高度灵敏的卡片/图标尺寸调节机制。

### 3.2 移植功能点定义
1. **三种显示视图模式**：
   - **列表视图 (`ListResultView`)**：标准的多列排版。
   - **等高合理排版视图 (`JustifiedResultView`)**：高度固定，宽度按比例自适应。
   - **网格卡片视图 (`GridResultView`)**：规则的正方形卡片网格布局。
2. **两种卡片/图标尺寸调节方式**：
   - **滑动条直接交互（拖拽 / 点击轨道定位）**：支持在 UI 的工具栏/控制栏增加一个 QSlider 滑动条，范围限制在 `32px` 到 `256px`。用户不仅可以拖拽手柄，还可以直接左键单击轨道任意位置，滑动条通过 `QStyle::sliderValueFromPosition` 瞬间计算并跳转至定位值。
   - **Ctrl + 鼠标滚轮物理缩放**：当鼠标悬停在内容容器任何视图的视口上，按住键盘 `Ctrl` 键滚动鼠标滚轮时，能以 `10px` 为步进动态调节卡片/图标大小，并双向联动更新 QSlider 的滑块位置，获得极致平滑的动态视觉反馈。

### 3.3 解决方案概述
- 将 `FERREX-META/src/ui/` 中的 `IScanResultView.h`、`ListResultView.h/cpp`、`JustifiedResultView.h/cpp`、`GridResultView.h/cpp` 作为三大视图内核进行移植，并融入当前程序。
- 在工具栏或标题栏新增 `m_sizeSlider` (QSlider)，设置其范围为 `32` 到 `256`，在值发生变动时通知当前激活视图更新 `setIconSize`。
- 在 `MainWindow` 或 `ContentPanel` 的 `eventFilter` 中拦截 `QEvent::Wheel`。一旦检测到 `wheelEvent->modifiers() & Qt::ControlModifier`，根据滚轮方向将 `m_sizeSlider` 的值以 `+10` / `-10` 步进进行调整，通过单值驱动所有视图 and Delegates 实现统一的卡片和图标大小动态重载。


## 4. [2026-07-06] 标签视图界面 SQL 裸写重构与 MVC 解耦
### 4.1 核心需求
`TagManagerView` 是标准的表现层 UI View 控件，但其多处直接执行底层 `sqlite3` API （如 `sqlite3_prepare_v2`）裸写 SQL 语句，且硬编码了对特定 C 盘数据库连接的调用。在百万级元数据大并发处理时易导致死锁或 `SQLITE_BUSY` 冲突。需求是彻底废除 `TagManagerView` 的 SQL 裸写职责，将其下沉至元数据/数据持久层管理器中。

### 4.2 解决方案概述
1. **持久层下沉**：在 `MetadataManager` 中新增或封装对标签、标签组的增删改查底层原子接口，统一管理多盘符并发下的事务与锁。
2. **MVC 分离**：重构 `TagManagerView` 内部关于标签组与标签的加载、新建、删除以及关联逻辑。UI 仅保留事件触发与 Model/视图刷新，彻底实现“哑表现”与“数据持久层”的清晰剥离。
3. **消除硬编码**：解耦所有硬编码盘符获取连接的脏逻辑，统一走数据服务层的路由解析。
4. **对应方案文档**：Modification_Plan-23.md


## 5. [2026-07-20] 账本核对审计 (fullRecount) 增量化重构与 MVC 分离
### 5.1 核心需求
`CategoryRepo::fullRecount`（判定为 FAIL 的第 3 项）不仅包含对全账本核对审计和全局静态计数器维护等非分类持久化职责（职责不单一），而且在百万记录规模下，每次启动或刷新分类时，会强行线性遍历 `MetadataManager` 数百万条记录的内存快照进行对账。这是一个明显的性能灾难（耗时高、高频持有大锁、产生不可恢复的 UI 假死）。需要对该全量审计进行**增量化重构**并从主加载链条剥离。

### 5.2 解决方案概述
1. **废除 fullRecount 的全量线性对账**：
   彻底删除或屏蔽在系统初始化时阻塞主线程调用的 `fullRecount` $O(N)$ 遍历计算。
2. **构建“内存增量对账模型”**：
   在内存中通过分类变动（如归类、去分类、批量删除）的信号和原子计数增减，维护瞬时、实时的已分类/未分类计数；全局数据库表（如 `system_stats`）中维护分类计数持久化，仅在关键原子操作中通过数据库级别增量更新（无需全表/全缓存线性重数）。
3. **剥离全局静态计数器职责**：
   将 `s_totalFileCount` 计数器和回收站数据同步从 `CategoryRepo` 剥离出去，确保其只专注于分类树与关联表的关系。
4. **对应方案文档**：Modification_Plan-24.md


## 6. [2026-07-21] DatabaseManager 纯酸化与物理 I/O 及纠偏逻辑剥离
### 6.1 核心需求
`DatabaseManager`（判定为 FAIL 的第 1 项）核心职责应仅为 SQLite 连接管理与高并发 I/O 任务调度。但其内部直接调用了 Windows 物理文件 API（`SetFileAttributesW` 设置隐藏属性）、`QFile::rename` 物理盘符纠偏以及对无效冗余历史数据库的清理、移动等物理磁盘动作。这些非数据库事务逻辑降低了其高可靠度并引入了多线程下的外部磁盘 I/O 阻塞风险，亟待拆分纯酸化。

### 6.2 解决方案概述
1. **物理隐藏下沉剥离**：
   彻底废除 `DatabaseManager::ensureHidden` 及相关 Windows.h 调用，将文件隐藏属性标记交由上层的物理搬运层 `ImportHelper` 或 `ShellHelper` 实现。
2. **盘符路由与数据库文件名自适应纠偏解耦**：
   重构 `DatabaseManager::getMemoryDb` 接口，废除其中对历史数据库物理 `rename`、冗余无效应答及清理行为。
3. **建立纯酸的底层连接架构**：
   `DatabaseManager` 纯粹地通过 wstring 绝对物理路径，加载 SQLite 连接和分配并发事务（WriteGuard / SqlTransaction），将 I/O 线程和内存备份高度内聚，确保 100% 数据库专职单一性。
4. **对应方案文档**：Modification_Plan-25.md
