# RapidNotes 架构治理与代码债务排查报告

> 本文档根据资深架构师标准排查输出，详细记录 `RapidNotes` 代码库中存在的**严重职责过载**、**打补丁治标不治本**、**另起炉灶与多套实现（需归一化）**、以及**严重违背单一事实来源 (SSOT)** 的全部架构隐患。

---

## 一、 顶层架构七大治理铁律 (Core Governance Principles)

1. **职责过载必拆**：任何代码文件若存在职责过载，必须按分层架构严格拆分为独立职责的 Domain / Service / Dao / View。
2. **违背 SSOT 必归一化**：凡是数据源分散、缓存与持久化不一致的，必须重构为单向真理源通道，统一 API。
3. **接口天然支持批量**：底层接口和控制器方法必须优先原生支持批量（`QList<int>` / `QStringList`），严禁单项循环导致 I/O 与广播爆炸。
4. **拒绝无脑打补丁**：严禁采用 `singleShot(0)` 延迟调用、全局标志位强压时序、平台 Hack 强抢焦点等治标不治本手段；重构前必须深挖根因。
5. **全局统一熔断机制 (`cancelAll`)**：所有耗时 I/O、后台扫描（全文检索、文件遍历、OCR、监控）必须建立统一的 `cancelAll` 熔断与中断机制。
6. **杜绝性能与计算资源浪费**：严禁采用粗暴的全量清空重建、主线程全表循环扫描、高频磁盘直写等笨拙手法。
7. **快捷键架构规范化**：全面借鉴 `QuarkMeta` 项目成熟的快捷键架构（`AppShortcutController` + `LocalShortcutInterceptor`），建立无冲突、防抢占的集中调度体系。

---

## 二、 严重职责过载 (God Objects / Excessive Responsibilities)

| 序号 | 责任过载文件/类 | 代码行数 | 承担的越界职责清单 | 拆分与治理重构方向 |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **`src/ui/QuickWindow.cpp`** (`QuickWindow`) | **4,313 行** | 1. 负责主窗口所有 QWidget 布局、QSS 样式渲染与窗口九宫格拖拉；<br>2. 承载分类树、标签流式布局、多状态工具栏的事件处理；<br>3. 直接操纵 SQLite 数据库调用 CRUD、分页与排序；<br>4. 直接读写 Windows 注册表与 `QSettings` 达 14 处；<br>5. 跨模块强行操作剪贴板、热键注册与外部进程交互；<br>6. 充斥着大量跨线程 UI 操作与 `AttachThreadInput` / Win32 强改焦点的平台级 Hack。 | **上帝类彻底解耦拆分**：<br>1. 纯化为 Dumb View 容器；<br>2. 提取 `CategoryTreeController`（分类树管理）；<br>3. 提取 `NoteListController`（笔记视图控制器）；<br>4. 提取 `QuickWindowMediator` 进行跨面板事件协调；<br>5. 数据库交互全部沉降至 Domain Service。 |
| 2 | **`src/core/DatabaseManager.cpp`** (`DatabaseManager`) | **3,668 行** | 1. 既管数据库连接池与基础 SQLite 句柄；<br>2. 又包含笔记、分类、标签、提醒、加密、统计、操作日志的所有业务逻辑；<br>3. 内部包含大量拼接复杂动态 SQL 的字符串构建逻辑；<br>4. 在后台线程与主线程之间混用锁与无隔离事务；<br>5. 甚至包含前端显示相关的分页预处理与数据格式化。 | **仓储分层（Repository Pattern）重构**：<br>1. `DatabaseManager` 纯化为仅管理连接生命周期与基础迁移（Migration）；<br>2. 拆分为独立的 Dao 层：`NoteDao`、`CategoryDao`、`TagDao`、`ReminderDao`；<br>3. 上层构建 `NoteService`、`CategoryService` 等业务领域层。 |
| 3 | **`src/ui/ScreenshotTool.cpp`** (`ScreenshotTool`) | **1,978 行** | 1. 全屏截图覆盖层绘制与半透明遮罩；<br>2. 矩形选区拖拽调整算法；<br>3. 马赛克、画笔、箭头、文字批注绘制引擎；<br>4. 贴图钉（Pin）悬浮窗管理；<br>5. 本地图片保存、剪贴板交互与快捷键监听。 | 拆分为独立的 `ScreenshotOverlay`（选区捕获）、`AnnotationEngine`（批注绘制图层）、`PinnedScreenshotWindow`（独立贴图小组件）。 |
| 4 | **`src/ui/ColorPickerWindow.cpp`** (`ColorPickerWindow`) | **1,500 行** | 1. 屏幕取色放大镜像素采集与实时渲染；<br>2. 历史颜色色板管理与持久化；<br>3. RGB / HEX / HSL 多格式转换器；<br>4. 调色板自绘与滑块控件交互；<br>5. 全局低级鼠标键盘钩子与快捷键拦截。 | 拆分为 `ScreenMagnifier`（屏幕放大取色）、`ColorHistoryManager`（历史色板数据源）与 `ColorPickerView`。 |
| 5 | **`src/ui/TodoCalendarWindow.cpp`** (`TodoCalendarWindow`) | **1,455 行** | 1. 包含整套日历网格自绘；<br>2. 待办事项提醒计算与时钟轮询；<br>3. 待办事项 CRUD 直连数据库；<br>4. 弹窗交互与历史列表管理。 | 提取 `CalendarGridView`、`TodoRepository` 与 `TodoReminderWorker`。 |
| 6 | **`src/ui/FileSearchWidget.cpp`** 与 **`KeywordSearchWidget.cpp`** | **各超过 1,000 行** | 1. 本地磁盘递归遍历与后台扫描（包含正则、扩展名、大小过滤）；<br>2. 搜索结果 UI 列表自绘与右键菜单构建；<br>3. 搜索历史弹窗、收藏夹与多选合并导出逻辑；<br>4. 文本编码嗅探与大文件关键词匹配。 | 提取 `FileSearchEngine` 与 `KeywordSearchEngine`（基础设施层），视图层仅负责进度展示与结果渲染。 |

---

## 三、 典型打补丁与时序 Hack 清单 (Patchwork & Timing Hacks)

| 序号 | 文件与行号 | 存在的问题代码 / 补丁写法 | 危害与技术债务分析 | 彻底治理方案 (Root-Cause Solution) |
| :--- | :--- | :--- | :--- | :--- |
| 1 | `src/ui/QuickWindow.cpp:1811`、`src/ui/RecentNotesWindow.cpp:537`、`src/ui/IdeaQuickPanel.cpp:417` | `AttachThreadInput(curTid, targetTid, TRUE);`<br>`SetForegroundWindow(hwnd);`<br>`QTimer::singleShot(300, [lastThread, attached]() { ... });` | **Win32 强抢焦点 Hack 补丁**：因为窗口失焦或失活逻辑混乱，采用 Windows 原生跨线程输入附加并延时 300ms 强切焦点。不仅极易引发 Windows 资源死锁，更破坏了正常的 Qt 事件循环机制。 | 彻底重写窗口激活与展示状态机，使用 Qt 规范的 `show()`、`raise()`、`activateWindow()` 以及 `Qt::WindowStaysOnTopHint`，拔除所有 Win32 强抢焦点与 `singleShot` 延迟脱敏补丁。 |
| 2 | `src/ui/QuickWindow.cpp:2886, 2896, 2918` | `QTimer::singleShot(0, [this, catId]() { ... });` | **时序竞态延迟发射补丁**：在分类切换或删除节点时，因担心 UI 控件树未完全刷新或未析构，强行使用 `singleShot(0)` 延迟调用。在高频连续点击下极易引发野指针与悬空引用崩溃。 | 建立确定的事件监听与模型信号槽机制，依靠 `QAbstractItemModel` 的原子 `beginRemoveRows` / `endRemoveRows` 流程同步刷新，彻底移除 `singleShot(0)`。 |
| 3 | `src/models/CategoryModel.cpp:289, 351` | `QTimer::singleShot(0, this, [this, persistentParent]() { ... });` | 在树模型刷新和节点更新中注入 `singleShot(0)` 强行打乱同步时序。 | 收敛为标准 Qt Model 事务模型，就地发射 `layoutAboutToBeChanged` / `layoutChanged` 或 `dataChanged` 信号。 |
| 4 | `src/core/MessageCaptureHandler.cpp:38, 52, 63` | `QTimer::singleShot(0, ...);`<br>`QTimer::singleShot(50, ...);`<br>`QTimer::singleShot(150, ...);` | **级联延时“碰运气”补丁**：连续使用 0ms、50ms、150ms 延时尝试读取剪贴板内容，试图以轮询延迟规避外部程序占用剪贴板的竞态。 | 采用事件驱动的剪贴板监听，配合最大重试次数与显式 `isClipboardLocked` 检测，彻底废除多级硬编码延时。 |
| 5 | `src/ui/FramelessDialog.cpp:172, 263, 641` | `QTimer::singleShot(50, ...);`<br>`QTimer::singleShot(100, m_edit, ...);` | 使用延迟定时器强行在窗口显示后设置输入框焦点。 | 重写 `showEvent`，在事件循环处于安全就绪态后直接调用 `setFocus()` 与 `selectAll()`。 |
| 6 | `src/ui/FilterPanel.cpp:581` | `QTimer::singleShot(100, [this]() { m_lastChangedItem = nullptr; });` | 用 100ms 延时清空内部变动指针，极度脆弱的时序假设。 | 使用确定的状态机枚举与事件过滤周期，用 RAII 守卫对象管理指针生命周期。 |

---

## 四、 另起炉灶与多套平行实现（必须强制归一化）

### 4.1 搜索功能五处“平行独立造轮子”（严重归一化对象）
在 `RapidNotes` 中，居然存在多达 **5 个独立且互不相通的搜索体系**：
1. `FileSearchWindow` + `FileSearchWidget`（独立文件搜索窗口，约 1,100 行）；
2. `KeywordSearchWindow` + `KeywordSearchWidget`（独立关键字内容搜索窗口，约 1,150 行）；
3. `UnifiedSearchWindow`（联合搜索窗口，内部又重新拼装了 `FileSearchContentWidget`，约 700 行）；
4. `IntegratedSearchWindow`（综合搜索窗口，约 200 行）；
5. `SearchAppWindow`（应用程序搜索窗口，约 800 行）。

**归一化方案**：
- **基础设施层归一化**：抽取统一的 `DiskSearchEngine`，以参数化策略区分“文件名搜索”与“内容全文匹配”，支持异步多线程与统一取消标志（`cancelAll`）；
- **UI 容器归一化**：废除分散的 `FileSearchWindow`、`KeywordSearchWindow`、`IntegratedSearchWindow`，统一收敛为单一的 `SearchWindow`（内含 Tab 切换）；
- **历史记录组件归一化**：`FileSearchHistoryPopup`、`KeywordSearchHistoryPopup`、`SearchLineEdit` 内部各自实现了 3 套几乎一模一样的历史气泡，必须统一归一化为通用的 `SearchHistoryPopup`。

### 4.2 热键与快捷键“三套体系并存”
工程中同时存在：
1. `src/core/HotkeyManager`（Win32 `RegisterHotKey` 原生系统级全局热键）；
2. `src/core/KeyboardHook`（低级键盘钩子 `WH_KEYBOARD_LL`）；
3. `src/core/ShortcutManager`（基于 QKeySequence 的局部快捷键映射）。
三者互不知道对方的存在，没有防冲突保护，也没有输入态焦点防误触拦截（导致在网页或文档中按下特定按键被截断）。

**归一化方案**：
- 全面参照 `QuarkMeta` 成熟的快捷键架构；
- 由 `HotkeyManager` 统一管理全局唤醒热键，局部快捷键统一纳入 `AppShortcutController` 与 `LocalShortcutInterceptor`，统一接入 `isEditingFocus()` 防抢占检测。

### 4.3 标签管理与选择器分散造轮子
存在 `AdvancedTagSelector`、`TagEditorWidget`、`TagManagerWindow`、`TagCapsule` 4 处并行的标签管理与绘制逻辑：
- 样式各异、标签增删事件各自维护、没有统一的标签词典单一事实来源。
- **归一化方案**：抽取统一的 `TagLexiconService`，UI 统一使用 `TagSelectorOverlay` 与 `TagPillContainer` 胶囊组件。

---

## 五、 违背单一事实来源 (SSOT) 的严重隐患

| 序号 | 违规场景 | 违背 SSOT 的具体代码与现象 | 数据不一致后果 | 治理方案 (SSOT 收口) |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **系统配置读写漫天飞舞** | 全软件有超过 **20 个不同的文件**直接实例化 `QSettings("RapidNotes", ...)` 或 `QSettings("SearchTool_Standalone", ...)`，随意读取和写入配置项。甚至同一配置键（如 `ExternalEditor`、`QuickWindow`）在不同窗口中硬编码了不同的组名。 | 配置项一旦在某窗口修改，其他窗口由于各自持有多份独立读写副本，无法感知变动；组织名与应用名混乱直接导致配置丢失或覆盖。 | 建立全局唯一的 **`AppConfig::instance()`**，所有系统参数与窗口状态必须且只能通过 `AppConfig` 的统一接口读写，废除散落在各处的裸 `QSettings`。 |
| 2 | **分类与笔记数据的内存双副本** | `CategoryModel` 与 `QuickWindow` 分别在内存中缓存了各自的分类树节点列表和未加锁分类 ID 集合；当 `DatabaseManager` 中批量更新或移动分类后，仅通过部分通知更新单边内存。 | 导致分类加锁状态不同步、分类折叠状态在重开或快速切换时恢复错误、左侧树与内容区出现脏数据。 | 确立数据库为持久化 SSOT，中间层通过唯一的 `CentralEventHub` 或领域服务广播变更；UI 视图纯化为数据的投影视图，严禁私自缓存数据副本。 |
| 3 | **搜索历史记录多处分散且格式不一致** | `FileSearchWidget`、`KeywordSearchWidget`、`UnifiedSearchWindow` 各自独立调用 `QSettings` 维护名为 `"FileSearchHistory"`、`"FileSearchFilenameHistory"`、`"KeywordSearchHistory"` 的列表。 | 历史搜索记录无法共享，清理历史只能清理当前窗口，各窗口历史记录混乱。 | 统一由 `SearchHistoryService` 统一管理搜索词典，区分分类命名空间（Category），全局统一持久化与清空。 |

---

## 六、 缺少熔断机制与计算资源浪费 (Resource Exhaustion Risks)

1. **缺少全局 `cancelAll` 熔断机制**：
   - 现有的 `ScannerThread`（文件递归搜索）与 OCR 识别，在用户频繁输入文字变动时，旧的搜索线程无法立刻响应中断，导致大量线程在后台无意义狂转、磁头争用与 CPU 持续 100%；
   - **重构要求**：建立统一的原子 `std::atomic<bool> cancelFlag` 与 `cancelAll()` 统一入口，在新搜索触发、窗口关闭、用户按下 Esc 时，0 毫秒立即熔断后台 I/O。
2. **缺乏批量操作接口导致的性能灾难**：
   - 笔记的标签赋予、移入回收站、置顶状态变更，在部分控制器中居然通过 `for` 循环逐条执行单个 SQL 查询与多次 `emit dataChanged`；
   - **重构要求**：所有 DAO 与 Service 必须提供 `updateBatch`、`softDeleteBatch`，内部采用 SQLite 显式事务（`BEGIN IMMEDIATE TRANSACTION` ... `COMMIT`），一次操作只广播一次聚合事件。

---

## 七、 总结与后续实施路线图

1. **第一阶段：基础设施与配置归口 (SSOT 治理)**
   - 引入 `AppConfig`，彻底物理清除各处的裸 `QSettings`；
   - 建立 `CentralEventHub`，打通全局状态广播通道。
2. **第二阶段：核心数据层拆分与批量化 (Dao/Service 治理)**
   - 拆解 `DatabaseManager`（3,668 行），提取 `NoteDao`、`CategoryDao`、`TagDao`；
   - 补充完善原子事务批量接口，全面消除循环单条写库与无谓刷新。
3. **第三阶段：搜索体系与标签管理大归一化**
   - 物理废除 5 个独立搜索窗口，统一归并为单一 `SearchWindow` + `DiskSearchEngine`；
   - 统一注入 `cancelAll()` 熔断机制，杜绝后台线程失控。
4. **第四阶段：上帝类 QuickWindow（4,313 行）外科手术式解耦**
   - 拆分为独立的 View、Controller 与 Mediator；
   - 彻底清除 Win32 强抢焦点与 `singleShot(0)` 级联延时补丁。

---

## 八、 严重缺乏中央大脑 (Missing Central Brain / CoreEngine & CommandDispatcher)

### 8.1 核心病灶：无中枢指挥官，视图层“各自为政、越权直操”
对比 `QuarkMeta` 成熟的工业级架构，`QuarkMeta` 拥有统一的中央大脑：
- **`CoreEngine`**：作为业务决策与调度指挥中心（SSOT）；
- **`CommandDispatcher` 与 `AppCommand`**：所有业务操作（设置星级、颜色、标签、备注、删除、改名等）被封装为不可变的标准命令对象，统一下发给 `CoreEngine` 校验并执行；
- **`CentralEventHub`**：统一广播全局领域事件，UI 面板作为纯粹的 **Dumb Views** 仅监听广播刷新，绝不自行决策或直写存储。

而在 **`RapidNotes`** 中，**完全没有中央大脑**：
1. **视图直接充当业务指挥官（越权直操底座）**：
   - `QuickWindow`、`NoteEditWindow`、`SettingsWindow`、`TagManagerWindow` 等 UI 控件直接越过业务调度层，手写代码去调用 `DatabaseManager::instance().xxx()` 执行增删改查；
   - 视图层既做“界面展示”，又做“业务决策”，还直接做“数据库 I/O”和“注册表修改”，三层越界混合；
2. **缺乏统一命令分发体系（无 `AppCommand` 抽象）**：
   - 当用户在不同入口（快捷键、右键菜单、双击列表、按钮点击）触发同一件事（例如给笔记添加标签、移入回收站）时，每个入口各自写了一套独立的调用链路；
   - 导致改动一个操作逻辑，需要同步修改四五个不同窗口的代码，极易漏改导致行为不一致；
3. **缺乏中心事件总线（跨窗口事件全靠散乱的硬连线）**：
   - 窗口 A 改动了数据，想通知窗口 B 刷新，目前全靠 `parent()` 指针硬转、全局单例指针强行相互引用、或者依靠轮询定时器与 `singleShot` 延迟发射；
   - 导致类与类之间出现极其复杂的网状耦合（Spiderweb Coupling），牵一发而动全身。

### 8.2 建设 RapidNotes 中央大脑的治理方案
1. **构建 `CoreEngine` 业务指挥大脑**：
   - 引入不可变命令结构体 `AppCommand`（枚举包含 `AddNote`、`UpdateNote`、`DeleteNotes`、`SetTags`、`MoveCategory`、`LockCategory` 等）；
   - 所有 UI 面板纯化为 Dumb View，产生用户意图后只组装 `AppCommand` 并投递给 `CoreEngine::instance().executeCommand(cmd)`；
2. **引入统一事件总线 `CentralEventHub`**：
   - `CoreEngine` 执行完原子事务与数据持久化后，统一通过 `CentralEventHub` 向外广播领域事件（如 `NoteCreated`、`NoteUpdated`、`NotesDeleted`、`CategoryChanged`）；
   - 各 UI 视图统一监听该总线信号，执行局部增量更新，彻底斩断跨窗口私相授受与网状耦合。

---

## 九、 工业级架构四道红线硬锁 (Four Hard Architectural Locks)

> 本章节整理自用户的最高指令要求与核心设计红线，作为 `RapidNotes` 后续任何重构、接口设计与代码审查的**一票否决红线**。

### 9.1 算力与资源利用红线：严禁“笨拙且极度浪费计算资源”的手法
- **一票否决**：严禁采用全表全量重新查询、整个控件树清空重建、高频全量文件重新扫描、或主线程死循环/粗暴 sleep 等极度浪费计算资源的手法来勉强实现功能；
- **标准**：必须建立基于增量（Diff / Delta）的高性能更新体系，局部变动仅局部渲染与局部持久化。

### 9.2 接口归一化与批量优先红线：不支持批量操作则绝对不予通过
- **一票否决**：全软件任何功能（读取、写入、查询、更新、删除、加锁、移动分类等），**必须且只能归一化为支持批量（Batch）的标准接口**；
- **标准**：
  1. 底层存储与服务层必须提供统一的批量接口（如 `insertBatch`、`updateBatch`、`deleteBatch`、`queryByIds` 等），内部严格包裹在单一原子事务中执行；
  2. 控制器与调度层禁止用 `for` 循环单条串行调用伪装成批量处理，禁止逐条发射广播事件；
  3. **凡是不支持批量操作的功能与接口，一律判定为不合格，绝不通过！**

### 9.3 补丁零容忍红线：凡采用打补丁方式直接判定失败
- **一票否决**：任何试图通过 `singleShot(0)` 延迟调用压制时序竞态、通过临时全局布尔标志位强行闭合逻辑漏洞、通过 Win32 `SetForegroundWindow` / `AttachThreadInput` 强抢焦点等治标不治本的代码实现，直接判定为彻底失败；
- **标准**：必须深挖并解决根本原因，重构状态机与所有权模型，杜绝一切“碰运气”补丁。

### 9.4 对象生命周期与指针安全红线：严禁“偷指针”事件
- **一票否决**：严禁“偷指针”行为（包括但不限于：绕过父子层级通过私有成员强行获取裸指针、跨模块窥探或私自持有其他 Widget 的内部子控件裸指针、通过多级 `parent()->parentWidget()->findChild<T>()` 偷取依赖、或者直接跨线程传递捕获裸指针）；
- **标准**：
  1. 跨模块协作必须基于公开纯粹的 DTO 契约或信号槽解耦，严禁跨层非法渗透；
  2. 异步回调与跨线程通信必须使用 `QPointer` 弱引用守卫或值语义拷贝，严格防止对象销毁后的野指针、悬空引用与非法内存访问。

---

## 十、 快捷键架构规范化契约 (Keyboard Shortcut Architecture Standard)

> 本章节整理自用户的最高设计理念与要求，明确 `RapidNotes` 的快捷键系统**必须严格遵循 `QuarkMeta` 的架构方式全面重构**。

### 10.1 核心原则与架构分层
彻底推翻并废除 `RapidNotes` 目前三套割裂的快捷键实现（`HotkeyManager`、`KeyboardHook`、`ShortcutManager`），全面对齐 `QuarkMeta` 的两级防护与中枢调度架构：

1. **第一层：顶层窗口防抢占控制器 (`AppShortcutController` + `LocalShortcutInterceptor`)**：
   - **Windows 原生防抢占钩子 (`LocalShortcutInterceptor`)**：采用局部低级键盘钩子（`WH_KEYBOARD_LL`），在 Windows 消息队列最前端进行局内判定；
   - **输入焦点安全断言 (`isEditingFocus()`)**：任何快捷键分发前，必须强制检查当前是否处于文本输入态（如 `QLineEdit`、`QTextEdit`、搜索框、行内重命名等），若处于输入焦点中，**严格放行字符输入，绝对禁止触发全局快捷键业务动作**；
   - **非 Windows 降级保护**：非 Windows 平台降级为带 `Qt::WindowShortcut` 上下文的 `QShortcut`，严格限制作用域仅在应用窗口内部，严禁侵入操作系统全局。

2. **第二层：中枢调度器与业务解耦 (`PanelMediator` / `CoreEngine`)**：
   - 快捷键触发后**绝不直接执行业务逻辑**，而是发射抽象的语义信号（例如 `copyNamesEnumeratedRequested()`、`moveToTrashRequested()`、`toggleRecursiveRequested()`）；
   - 由中枢中介者（`PanelMediator`）或中央大脑（`CoreEngine`）捕获信号，统一从当前活跃视图（`activeContentPanel`）获取状态并驱动不可变命令（`AppCommand`）；
   - 消除视图之间私自持有快捷键指针与硬编码调用的混乱局面。

3. **第三层：局部键盘处理器与视图闭环 (`ContentKeyHandler`)**：
   - 针对视图组件内部的导航按键（方向键、PageUp/PageDown、空格预览、F2 重命名、Delete 等），由专职的 `KeyHandler` 统一处理并返回 `bool`（消费事件）；
   - 严格区分单按键与组合按键（如单按 `Space` 呼出预览，而 `Ctrl+Space` 执行移入回收站），杜绝按键冲突与竞争。

---

## 十一、 图标与视觉呈现规范 (Icon & Visual Asset Standard)

> 本章节整理自用户的最高设计规范指令，明确 `RapidNotes` 的所有界面图标呈现**必须严格遵循矢量化契约**。

### 11.1 核心铁律：禁止使用文本符号，一律从 SvgIcons.h 引用 SVG 矢量图标
1. **一票否决**：
   - 严禁在按钮（`QPushButton`）、标签（`QLabel`）、菜单项（`QAction`）、树节点或绘制委托中使用任何硬编码的 Unicode 文本符号（包括但不限于：`"×"`、`"✕"`、`"★"`、`"☆"`、`"▼"`、`"▲"`、`"▶"`、`"◀"`、`"√"`、`"+"` 等）来充当图标；
   - 使用文本符号会导致跨操作系统平台字体渲染缺失、DPI 缩放锯齿失真、以及不同 Windows 版本字形显示不一致的低级廉价感。
2. **唯一合法引用真理源 (SSOT)**：
   - 全软件所有图形图标**必须且只能通过 `src/ui/SvgIcons.h`（或 `IconHelper::getIcon`）引入 SVG 矢量图标**；
   - 保持图标线条风格、缩放比例、中性配色与现代桌面端暗色主题严谨统一。

---

## 十二、 数据存储架构革新：彻底剥离文件 Blob，全面改用托管库模式 (Managed Library Storage Architecture)

> 本章节整理自用户的重大架构决策，明确 `RapidNotes` 的存储底层**彻底废除 SQLite 储存物理二进制文件/大图片（data_blob）的反模式，全面改用“托管库（Managed Library）”目录存储模式**。

### 12.1 核心病灶：SQLite 直接存二进制文件的危害
1. **数据库极速膨胀与劣化**：
   - 过去 `RapidNotes` 在 `notes` 表中使用 `data_blob BLOB` 字段直接把截图、剪贴板图片、二进制文件塞进 SQLite 数据库；
   - 这会导致数据库文件迅速膨胀到数百 MB 甚至数 GB，引发严重的 SQLite 全局写锁争用、主线程查询延迟飙升、甚至数据库碎裂与只读损坏。
2. **读写性能低下与内存雪崩**：
   - 每次分页查询或遍历笔记时，哪怕只显示标题列表，SQLite 引擎也必须扫描携带巨型 BLOB 的数据页，造成严重的 I/O 阻塞与内存浪费。

### 12.2 托管库模式的核心设计理念 (Managed Library Pattern)
1. **文件与元数据彻底解耦**：
   - **数据库仅存轻量元数据**：SQLite 的 `notes` 表**彻底物理移除 `data_blob` 字段**，只保留文件路径/相对路径、文件大小、SHA-256、创建时间、修改时间等轻量元数据；
   - **物理文件落入托管目录**：所有通过截图、粘贴或导入生成的文件与图片，统一以独立文件的形式落入应用的“托管库（Managed Library）物理目录”中（如 `storage/files/{year}/{month}/{hash}.ext`）；
2. **唯一文件哈希去重与引用管理**：
   - 托管库内的文件采用 SHA-256 内容寻址或 UUID 命名，相同内容的文件在托管目录中仅物理保留一份，数据库中通过逻辑路径关联；
   - 删除笔记时，由后台清理服务安全回收孤儿文件，杜绝垃圾残留；
3. **极速 I/O 与异步缩略图渲染**：
   - 界面列表仅读取数据库文本元数据（0ms 级秒开）；
   - 图片与文件的加载走后台线程异步提取管道（Pipeline），按需缓存，彻底解决界面卡顿与滚动掉帧。

---

## 十三、 UI 样式架构红线：彻底禁止内联样式，全面改用外联 QSS (External Style Sheet Standard)

> 本章节整理自用户的最高 UI 样式架构指示，明确 `RapidNotes` **彻底禁止在 C++ 中编写内联 setStyleSheet，必须严格遵循与 `QuarkMeta` 完全一致的外联样式表架构**。

### 13.1 核心病灶：内联样式的严重危害
1. **460+ 处散落的 `setStyleSheet` 性能与维护灾难**：
   - 经全局排查，`RapidNotes` 的 C++ 源码中竟然散落了 **461 处内联 `setStyleSheet(...)` 硬编码**；
   - 导致 Qt 样式解析器在运行期高频重复解析 CSS 字符串，引发严重的内存开销与控件构造卡顿；
   - 颜色、边框、间距等设计参数分散在数十个不同的 `.cpp` 源码深处，改动一个主色调需要翻查全工程数十个文件，根本无法统一管理主题（暗色/亮色切换）。

### 13.2 外联 QSS 架构规范（对齐 QuarkMeta）
1. **绝对禁止 C++ 内联样式（一票否决）**：
   - **严禁**在 C++ 控件构造或事件响应中调用 `setStyleSheet("...")` 硬编码样式；
   - **严禁**通过字符串拼接的方式在运行时动态注入样式字符串。
2. **唯一合法外联样式集中归口**：
   - 建立集中式外联样式表文件（如 `resources/style.qss`）；
   - 在应用启动时（`main.cpp`）统一读取并一次性加载注入至 `qApp->setStyleSheet(...)`；
   - 样式更新与换肤由统一的 `ThemeManager` 进行框架级驱动。
3. **基于 ObjectName 与 Property 的标准选择器设计**：
   - 所有独立控件必须赋予专职的 `setObjectName("...")`，并在外联 QSS 中通过 `#ObjectName` 进行样式隔离；
   - 控件的状态切换（激活、高亮、选中、错误等）**必须通过 `setProperty("state", "...")` 配合 `style()->unpolish(w); style()->polish(w);` 触发样式重算**，严禁在 C++ 槽函数中就地重写 `setStyleSheet` 覆盖属性。

---

## 十四、 引入智能文件夹与数据库索引架构规范 (Smart Folder & Database Indexing Architecture)

> 本章节整理自用户的重大产品与数据架构决策，明确 `RapidNotes` **必须突破目前仅支持单一普通物理分类的局限，全面引入“智能文件夹（Smart Folder）”动态规则筛选体系，并建立高性能复合索引**。

### 14.1 核心病灶：仅有普通分类的局限性
1. **静态分类无法应对复杂多维检索**：
   - 目前 `RapidNotes` 仅支持树状普通物理分类（`categories` 表），一条笔记只能归属于单一分类 ID（`category_id`）；
   - 用户无法实现跨分类的动态维度筛选（例如：“星级 $\ge 4$ 且包含 #工作 标签且为最近 7 天修改的所有待办笔记”）。
2. **缺乏复合索引引发全表扫描卡顿**：
   - 现数据库索引极其单薄，缺失星级、创建时间、修改时间、标签关联、颜色标记的有效复合索引；
   - 随着数据量达到上万条，一旦进行多条件动态组合查询，将直接触发 SQLite `SCAN TABLE` 全表扫描，造成严重的 UI 掉帧与查询超时。

### 14.2 智能文件夹设计架构（对齐 QuarkMeta 工业级标准）
1. **持久化表设计：`smart_folders`**：
   ```sql
   CREATE TABLE IF NOT EXISTS smart_folders (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       name TEXT NOT NULL,
       match_mode TEXT NOT NULL DEFAULT 'all', -- 'all' (AND) / 'any' (OR)
       icon_key TEXT NOT NULL DEFAULT 'folder_smart',
       color_hex TEXT NOT NULL DEFAULT '#378ADD',
       sort_order INTEGER NOT NULL DEFAULT 0,
       rules_json TEXT NOT NULL,                -- 动态规则定义 JSON
       created_at INTEGER NOT NULL
   );
   ```
2. **多维度规则引擎 (`SmartRuleEngine`)**：
   - 支持多类型动态规则匹配：
     - **关键字/标题/内容**（LIKE 模糊匹配，严格转义 Wildcard 字符）；
     - **星级**（等于、大于、小于或“无星级”）；
     - **颜色标记**（等于、不等于、无颜色）；
     - **标签**（包含任意、包含全部、不包含）；
     - **创建/修改日期**（单日期、日期范围、最近 N 天）；
     - **分类归属**（属于指定分类或排除某分类）。

### 14.3 高性能数据库索引体系（索引建设备战）
为了支撑智能文件夹毫秒级极速查询与统计计数，数据库必须强制建立以下专用索引：
```sql
-- 1. 智能文件夹自身排序索引
CREATE INDEX IF NOT EXISTS idx_smart_folders_sort ON smart_folders(sort_order ASC, id ASC);

-- 2. 笔记生命周期与多维过滤复合索引（覆盖未删除笔记的快速检索）
CREATE INDEX IF NOT EXISTS idx_notes_active_lookup ON notes(is_deleted, updated_at DESC, created_at DESC);

-- 3. 分类与星级复合索引
CREATE INDEX IF NOT EXISTS idx_notes_cat_rating ON notes(category_id, rating) WHERE is_deleted = 0;

-- 4. 颜色与星级多维索引
CREATE INDEX IF NOT EXISTS idx_notes_color_rating ON notes(color, rating) WHERE is_deleted = 0;

-- 5. 标签关联表双向覆盖索引（保障标签多对多高效 JOIN 与 IN 查询）
CREATE INDEX IF NOT EXISTS idx_note_tags_tag_note ON note_tags(tag_id, note_id);
CREATE INDEX IF NOT EXISTS idx_note_tags_note_tag ON note_tags(note_id, tag_id);

-- 6. 待办/提醒时间索引（保障时间维度智能筛选与日历联动）
CREATE INDEX IF NOT EXISTS idx_notes_remind_time ON notes(remind_time) WHERE remind_time > 0 AND is_deleted = 0;
```

### 14.4 异步计数与主线程零阻塞契约
- 对齐 `QuarkMeta` 的 `SmartFolder Sidebar Asynchronous Counting Rule`；
- 智能文件夹列表中的统计数字计算**绝对禁止在主 UI 线程中执行**，必须走后台工作线程（如 `QtConcurrent::run`），使用带有 `SQLITE_OPEN_READONLY` 的只读数据库连接，彻底杜绝主线程假死与全局写锁争用！

---

## 十五、 对标 QuarkMeta：RapidNotes 核心短板与架构缺陷全面对比

> 本章节从工业级软件架构、健壮性设计与用户交互体验出发，深度对标 `QuarkMeta`，全面剖析 `RapidNotes` 目前仍存在的**六大系统级严重缺陷**。

### 15.1 缺少统一的可撤销快照引擎 (Missing Operation Snapshot & Undo Engine)
- **`QuarkMeta` 标杆实现**：
  - 拥有完善的 `OperationSnapshotEngine` 与 `UndoManager`，任何用户操作（移动、删除、设置星级/颜色/标签、重命名、移入回收站等）都会被记录为可回退的原子操作快照；
  - 配套非阻塞轻量 Toast（`UndoToastOverlay`），支持用户 0 风险快速撤销（Ctrl+Z / 点击撤销条），给予用户极高的安全感。
- **`RapidNotes` 致命短板**：
  - **完全没有全局撤销体系**：笔记状态更新、移动分类、删除项目、属性覆写全部是“一锤子买卖”；
  - 只要用户误操作，数据直接在数据库中被就地覆盖或删除，除了单独的关键字替换有局部的临时 Undo，整个主框架完全无法撤销，数据安全性极低。

### 15.2 缺少任务进度与耗时 I/O 托管中枢 (Missing TaskProgressService)
- **`QuarkMeta` 标杆实现**：
  - 拥有全局统一的 `TaskProgressService` 与配套的 `TaskProgressStrip` / `TaskProgressToolBar`；
  - 任何超过阈值或后台异步任务（大文件扫描、永久抹除、标签批量写入、缩略图提取、文件分发等）全量自动托管给进度服务；
  - 用户能实时感知任务进度百分比、已处理项数/总项数，并提供一键安全中断通道。
- **`RapidNotes` 致命短板**：
  - **完全没有进度托管服务**：在执行大量文件扫描、数据库大批量操作、OCR 批量识别或导入时，没有任何系统级的统一进度反馈；
  - 界面通常陷入无感知的“假死/忙等待”状态，或者在状态栏随意打印字符串，用户完全无法获知任务是否卡死，更无法中途安全中止任务。

### 15.3 无边框拉伸与窗口行为重复造轮子 (Fragmented Frameless Window Logic)
- **`QuarkMeta` 标杆实现**：
  - 统一抽象了工业级的 `FramelessWindowHelper`（基于原生 `WM_NCHITTEST` / `WM_NCCALCSIZE`）；
  - 主窗口、 QuickLook、各类模态与非模态对话框统一通过 `FramelessWindowHelper::apply(this, titleBar)` 赋能，窗口四周九宫格拉伸、双击最大化/还原、多屏 DPI 边缘裁切 100% 统一，零重复代码。
- **`RapidNotes` 致命短板**：
  - `FramelessDialog.cpp` 自己写了一套 Win32 消息过滤；
  - `QuickWindow.cpp` 又单独复制了一套多达数百行的 `WM_NCHITTEST` / `WM_NCCALCSIZE` / `WM_SETCURSOR` 消息处理（第 3410~3530 行）；
  - `StringUtils.h` 中又散落了一套窗口边缘缩放补丁；
  - 出现典型的“改了一处、另一处忘改，各个窗口缩放手感和白条 Bug 表现各不相同”的严重设计割裂。

### 15.4 缺少统一的矢量图标库与图标着色引擎 (Missing Central SvgIcons & Palette Engine)
- **`QuarkMeta` 标杆实现**：
  - 统一建立 `SvgIcons.h`，收录数百个标准矢量 SVG 路径，并配合 `UiHelper::getIcon` 支持运行时动态统一着色（如严格 neutral 灰 `#EEEEEE` 或主题色）；
  - 结合 `ColorPaletteEngine` 对文件扩展名徽章、色标进行科学的色彩防碰撞计算。
- **`RapidNotes` 致命短板**：
  - 图标资源四分五裂：部分在 `SvgIcons.h`，部分在 `IconHelper.h`，部分直接在 C++ 中硬编码 QPainter 绘制，更有大量直接使用粗糙的文本符号（`"×"`、`"★"`）；
  - 颜色配置杂乱无章，没有全局统一的图标中性色（`#EEEEEE`）约束，视觉呈现缺乏专业现代质感。

### 15.5 缺少统一防抖刷新与视图协同中介 (Missing Stats Debounce & PanelMediator)
- **`QuarkMeta` 标杆实现**：
  - 建立了基于定时器聚合的 `m_statsDebounceTimer`（50ms 防抖）与增量模型通知 `updateRecordsMetadata`；
  - 高频元数据变更或连续按键时，防抖合并为单次刷新，极度节省 UI 渲染开销；
  - 通过 `PanelMediator` 规范管理各面板间的激活焦点与跨面板协同。
- **`RapidNotes` 致命短板**：
  - 缺乏防抖机制：笔记变动或搜索过滤时，高频直接触发全量列表刷新与 SQL 重查；
  - 跨面板交互全靠强耦合的互相硬调，导致界面频繁闪烁，高频输入时顿挫感明显。

### 15.6 数据落地缺乏文件侧车与分布式持久化机制 (Lack of Sidecar Persistence)
- **`QuarkMeta` 标杆实现**：
  - 采用现代化的“侧车文件（`.QuarkMeta.json`）+ 中心数据库索引”双轨制；
  - 文件夹元数据就地跟随物理目录漫游，不依赖单一中心数据库的绝对路径，即使目录改名或移动硬盘拔插也能无损恢复。
- **`RapidNotes` 致命短板**：
  - 所有数据死死绑在单一的 `rapid_notes.db` SQLite 文件中；
  - 一旦数据库损坏或路径迁移，用户所有分类关系、标签元数据、历史记录将瞬间永久丢失，缺乏高可用冗余容灾能力。
