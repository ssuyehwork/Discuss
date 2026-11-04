# Memories_RapidNotes.md - RapidNotes 顶层架构设计理念与历史记忆真理源 (SSOT)

> 本文档由 Jules（资深 C++/Qt 软件架构师与协同工程师）根据用户的最高设计意图、严格共识与《RapidNotes-1.md》排查报告权威整理编写，作为 RapidNotes 全局架构重构、代码规范、五大红线与所有实施方案的唯一真理源 (SSOT)。

---

## 1. 顶层架构治理五大一票否决红线 (Five Hard Architectural Locks)

全软件任何代码修改、功能新增、重构或 Bug 修复，必须无条件遵守以下五大红线，违者直接一票否决判定失败：

1. **算力与资源利用红线（严禁“笨拙且极度浪费计算资源”的手法）**：
   - 严禁采用全表全量重新查询、整个控件树粗暴清空重建、高频全量文件重新扫描、或主线程死循环/Sleep 等笨拙手法勉强实现功能；
   - 必须建立基于增量（Diff / Delta）的高性能更新体系，局部变动仅局部渲染与局部持久化。

2. **接口归一化与批量优先红线（不支持批量操作则绝对不予通过）**：
   - 全软件任何操作功能必须归一化，创建批量写入、读取、查询接口（`QList<int>` / `QStringList` / 批量 DTO）；
   - 严禁在调用方使用 `for` 循环单条串行调用伪装成批量处理，严禁逐条发射广播事件；
   - 凡是不支持批量操作的功能与接口，一律判定为不合格，绝不通过。

3. **打补丁零容忍红线（凡采用打补丁方式则是失败的，绝不可以通过）**：
   - 严禁采用 `singleShot(0)` 延迟调用压制时序竞态、通过临时全局标志位强行闭合逻辑漏洞、通过 Win32 `SetForegroundWindow` / `AttachThreadInput` 强抢焦点等治标不治本手段；
   - 必须深挖并解决根本原因，重构状态机与所有权模型，杜绝一切“碰运气”补丁。

4. **对象生命周期与指针安全红线（不可发生“偷指针”事件）**：
   - 严禁“偷指针”行为：严禁绕过父子层级通过私有成员强行获取裸指针、严禁跨模块窥探或私自持有其他 Widget 的内部子控件裸指针、严禁通过多级 `parent()->parentWidget()->findChild<T>()` 偷取依赖、严禁直接跨线程传递捕获裸指针；
   - 跨模块协作必须基于公开纯粹的 DTO 契约或信号槽解耦；异步回调与跨线程通信必须使用 `QPointer` 弱引用守卫或值语义拷贝。

5. **快捷键架构规范化红线（快捷键必须遵循 QuarkMeta 方式去架构）**：
   - 废除割裂散落的热键实现，采用两级防护与中枢调度架构：顶层 `AppShortcutController` + `LocalShortcutInterceptor`；
   - 强制执行输入态焦点断言 `isEditingFocus()`：在用户打字时不越权截断，原生放行，绝不抢占。

---

## 2. 中央大脑与单向依赖分层架构 (Central Brain & Layering)

1. **中央调度中心 (`CoreEngine`)**：
   - 所有业务逻辑的唯一决策中心，作为单一事实来源 (SSOT) 通道；
   - 引入标准命令结构体 `AppCommand` 与事件广播总线 `CentralEventHub`。
2. **UI 纯化为 Dumb View 容器**：
   - UI 面板（如 `QuickWindow`、`NoteEditWindow` 等）只负责渲染与收集用户交互意图，产生意图后组装 `AppCommand` 投递给 `CoreEngine`，严禁直接直连 SQLite 或修改 Windows 注册表。
3. **分层依赖流向**：
   $$\text{View (UI)} \longrightarrow \text{Mediator / Controller} \longrightarrow \text{Domain / CoreEngine} \longrightarrow \text{Dao / Repository} \longrightarrow \text{SQLite / Disk Storage}$$
   严禁逆向包含与跨层下钻。

---

## 3. 存储底层革新：托管库存储模式 (Managed Library Pattern)

1. **彻底物理移除 SQLite `data_blob` 字段**：
   - 数据库仅存轻量元数据（标题、标签、时间、文件相对路径、SHA-256、大小等）；
   - 物理文件（截图、大图、附件）落入托管库物理目录（`storage/files/{year}/{month}/{hash}.ext`）。
2. **内容寻址去重与异步加载**：
   - 基于 SHA-256 去重，数据库记录指向同一物理存储；
   - 界面列表 0ms 秒开，图片/文件由后台工作线程流水线异步加载，彻底杜绝数据库体积膨胀与 UI 掉帧。

---

## 4. UI 样式架构：外联 QSS 与矢量化契约 (External QSS & Vector Standard)

1. **彻底拔除 C++ 内联样式**：
   - 严禁在 C++ 中调用 `setStyleSheet`；
   - 全面收敛至 `resources/style.qss` 与 `ThemeManager`；通过 `setObjectName` 与 `setProperty` 驱动状态。
2. **矢量图标契约**：
   - 严禁使用 Unicode 文本符号（如 `"★"`、`"×"`）；
   - 统一由 `SvgIcons.h` 引用标准 SVG 矢量路径，保持动态着色与现代桌面端暗色一致性。

---

## 5. 智能文件夹与数据库索引架构规范 (Smart Folder & Database Indexing)

1. **智能文件夹 (`smart_folders`)**：
   - 支持多维动态规则（关键字、星级、颜色、标签、日期区间）；
   - 规则匹配与统计计算必须走后台工作线程（`QtConcurrent::run`）与只读数据库句柄 (`SQLITE_OPEN_READONLY`)，主线程零卡顿。
2. **复合索引构建**：
   - 强制建立覆盖 `is_deleted`、`updated_at`、`category_id`、`rating`、`color` 的高性能复合索引，杜绝全表扫描。

---

## 6. 底层逻辑重构步骤 1：词表与纯函数真理源规范 (Step 1: Vocabularies & Pure Functions SSOT)

根据《RapidNotes_Prompts.md》第一部分与第二部分要求，底层重构第一步必须把散落在各处的判定与数据加工规则归一化为唯一纯函数模块，表结构和 SQL 在此步骤保持不变：

1. **条目类型词表唯一真理源 (`ItemType`)**：
   - `ItemType` 为全工程唯一允许出现 `item_type` 字符串字面量的地方；
   - 包含枚举 `Type`（`Text`, `Image`, `Code`, `Link`, `Color`, `Palette`, `OcrText`, `CapturedMessage`, `PixelRuler`, `DeletedCategory`, 以及旧文件类型中间态 `File`, `Files`, `Folder`, `Folders`, `LocalFile`, `LocalFolder`, `LocalBatch`，后续在步骤 3 合并为 `Path`）；
   - 提供双向无损转换：`QString toString(Type)` 与 `Type fromString(const QString&)`，以及类型分类断言（`isFileOrFolder(Type)`、`isPureText(Type)` 等）。

2. **类型与特征判定唯一真理源 (`ItemClassifier`)**：
   - 消除 `main.cpp`、`NoteModel::data`、`StringUtils::detectItemType` 和 `DatabaseManager` 中分散手写的判定规则；
   - 统一提供高内聚纯谓词：`isUrl(text)`、`isHexColor(text)`、`isRgbTriplet(text)`、`isCodeLike(text)`、`isAbsolutePathText(text)`；
   - 提供两个明确的入口：
     - 入库类型判定：`detectIngestType(text)`；
     - 视图显示图标类别（不得访问磁盘）：`detectDisplayCategory(type, content)`。

3. **路径归并种类唯一真理源 (`PathSummary`)**：
   - 输入路径列表，根据路径类型与数量得出归一化种类：`SingleFile`（单文件）、`SingleFolder`（单文件夹）、`MultipleFilesOrMixed`（多文件或混合）、`MultipleFolders`（多文件夹）；
   - `NoteModel` 界面图标判定全部委托给 `PathSummary`，彻底杜绝分散的路径分割与判断。

4. **辅助纯函数归一化 (`WordBucket` / `TagCodec` / `ThumbnailMaker`)**：
   - `WordBucket::calculate(content, itemType)`：字数阶梯计算唯一来源（0, 10..100, 200..1000, 1001），原样承接清洗与阶梯逻辑；
   - `TagCodec`：全工程唯一的标签编码/解码器；拆分统一支持半角逗号 `,` 与全角逗号 `，`（`split(tagsStr)`），拼接严格只用半角逗号加空格 `, ` 或 `,`（`join(tagList)`）；
   - `ThumbnailMaker::makeThumbnail(img, size = QSize(64, 64))`：原样封装 `NoteModel` 现有的 64×64 平滑保持纵横比缩略图生成算法。

---

## 7. 底层逻辑重构步骤 2：模块拆分与单一职责架构规范 (Step 2: Module Decomposition & SSOT Architecture)

根据《RapidNotes_Prompts.md》步骤 2 要求，彻底拆解并物理删除职责过载的上帝类 `DatabaseManager`，严格在保证“不改动表结构、不改动 SQL 语义”的前提下，将所有能力解耦归入独立专职的模块中：

### 7.1 核心模块与职责矩阵（唯一归属）
1. **`ConnectionManager`**：
   - 唯一职责：打开与关闭数据库连接、配置连接 PRAGMA、管理数据库物理文件；
   - 事务作用域 `TransactionScope`：支持 RAII 事务管理与任意层级安全嵌套，只在最外层执行真实的 `commit` 或 `rollback`。对外写接口遵循“一个写操作恰好一个事务”原则，彻底消灭循环刷盘与无事务裸写。
2. **`NoteRepository`**：
   - 笔记及其关联实体的底层原子读写：增、删、改、状态更新（收藏/置顶/评分/颜色）、排序写入、`recordAccess`、`getNoteById`、`getLastCreatedNoteId`、回收站还原/物理删除；
   - **纯粹数据仓储**：`NoteRepository` 绝对不包含任何业务规则判定，仅执行纯粹 SQL 操作。
3. **`NoteIngestService`**：
   - 所有创建与采集笔记的业务指挥中心：按内容哈希（`content_hash`）去重合并、已有分类优先保留、标签集合并集计算、原标题为通用标题时才替换、`link` 类型自动归入 "Link" 分类、分类预设标签自动追加；
   - 由它调用 `NoteRepository` 的原子写接口完成落库。
4. **`TagRepository`**：
   - 标签增删改查、note_tags 关联维护、全局重命名与批量删除、标签使用频次统计。
5. **`CategoryRepository`**：
   - 分类树的增删改、树形节点迁移、排序更新、预设标签管理。
6. **`CategoryLockService`**：
   - 分类密码设置、密码验证、解锁状态维护，以及锁定分类集合（`lockedCategoryIds`）的内存缓存，分类加锁状态变化时自动失效与重新计算，查询时不再高频读表。
7. **`TodoRepository`**：
   - 待办事项（`todos` 表）的 CRUD 与状态流转。
8. **`NoteQueryService`**：
   - 笔记检索与统计服务：`searchNotes`、`getNotesCount`、`getFilterStats`、`getCounts`、`applyCommonFilters`、`applySecurityFilter`；
   - 封装分页常量（`DEFAULT_PAGE_SIZE = 500`），本步骤保持既有 SQL 语义 100% 相同。
9. **`AppSettings`**：
   - 业务配置唯一真理源：自动归档开关、自动归档目标分类、隐藏加锁分类等，沿用现有 QSettings 的组织名、应用名与键名。
10. **`TagClipboard`**：
    - 标签剪贴板（原 DatabaseManager 中 `s_tagClipboard`）独立管理，互斥锁安全保护。
11. **`DataEvents`**：
    - 全局领域数据变更总线，沿用现有信号名与参数（如 `noteCreated`、`noteUpdated`、`notesDeleted`、`categoryChanged` 等）；
    - 铁律：必须在事务提交（Commit）成功后发射，一个业务操作最多发射一次。
12. **`LicenseService` 与 `LegacyDatabaseRescue`**：
    - 原样搬迁旧版授权判定与旧数据库抢救逻辑，不改动任何核心逻辑与数值。

### 7.2 依赖单向流动与反向依赖解耦铁律
- **拔除核心层对 UI 的逆向依赖**：`src/core` 下任何文件绝对禁止包含 `src/ui` 的头文件（如 `StringUtils.h`、`FramelessDialog.h` 等）；
- **删除操作与剪贴板解耦**：`NoteRepository` 执行删除后发射 `DataEvents::notesDeleted` 信号，`ClipboardMonitor` 通过应用装配层订阅该信号清理内部最后哈希，严禁仓储层反向调用 UI 或剪贴板单例；
- **哈希互斥保护**：`ClipboardMonitor::m_lastHash` 的所有读写与重置操作必须受 `m_hashMutex` 互斥锁保护。
