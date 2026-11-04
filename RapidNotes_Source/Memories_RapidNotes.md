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
