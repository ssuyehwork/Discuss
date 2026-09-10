# QuarkMeta 全局设计理念与顶层架构规划 (QuarkMeta-Architecture-Planning.md)

---

## 🏛️ 第一章：顶层设计理念与 UI 交互哲学

`QuarkMeta` 致力于打造工业级、高性能、纯磁盘架构的桌面资产管理系统。为使用户获得极致流畅、一贯且高辨识度的视觉交互体验，全系统遵循以下**顶层 UI 设计哲学**：

1. **全局统一视觉与零差异体验**：
   应用中所有弹出的右键菜单（上下文菜单）、托盘菜单及系统控制菜单，必须在外观样式、几何边距、高亮颜色与交互动画上保持 100% 绝对一致，杜绝任何窗口或面板回退至原生或不一致样式。

2. **100% 语义化矢量图标契约 (Zero Unadorned Text)**：
   全软件所有右键菜单与上下文菜单项，**必须为每一个菜单项配置与功能语义精准匹配的矢量图标**，严禁出现任何纯文本裸露项。
   - **语义精准契约**：图标必须严格对齐操作功能（如打开用文件夹/应用开启类图标、重命名用编辑类图标、复制用双页类图标、删除用垃圾桶类图标），严禁乱用或滥用无关图标；
   - **格式唯一限制**：全软件图标统一且只能使用 SVG 矢量格式，严格禁止使用任何字符、Unicode 符号或文本标点充当/替代图标；
   - **图标缺失补全**：若系统中缺失对应语义的图标，必须在矢量图标库中创建并注册相符的 SVG 图标定义。

3. **像素级控制与统一间距布局**：
   - 全局菜单中，图标与文本之间的物理间距统一并精确锁定为 **`10px`**；
   - 悬停/选中状态颜色统一对标 **`#3E3E42`**；
   - 具有状态属性的操作（如置顶/取消置顶、收藏/取消收藏）必须进行状态感知与动态图标形态联动；
   - **无彩色单色统一**：全局所有右键菜单与上下文菜单项图标统一采用中性单色设计 (`#EEEEEE`)，严格禁止在右键菜单图标上使用任何彩色着色（如红、黄、蓝、绿等），确保界面视觉沉静高档。

4. **输入框清除按钮响应式呈现契约 (Reactive Clear Button Contract)**：
   全系统中所有具备清除能力的单行文本输入框（如全局搜索栏、标签/日期/类型筛选输入框、路径编辑框），其清除按钮 (×) 必须严格遵循“有文本时动态显示、无文本时绝对隐匿”的响应式契约。严禁在输入框为空（包括呈现 Placeholder 占位文本）时显示清除按钮，确保界面视觉干净与交互逻辑严谨。

5. **文本输入框应用专属上下文菜单契约 (App-Exclusive LineEdit Context Menu Contract)**：
   全软件所有文本输入框（包括单行 `QLineEdit` 与多行 `QTextEdit`）严格禁止弹出操作系统默认的原生英文右键菜单。必须统一拦截并替换为 QuarkMeta 应用专属暗色右键菜单（完整提供 `撤销 Ctrl+Z`、`重做 Ctrl+Y`、`剪切 Ctrl+X`、`复制 Ctrl+C`、`粘贴 Ctrl+V`、`删除`、`全选 Ctrl+A`），且每一个菜单项必须配备 100% 语义匹配的中性单色 SVG 矢量图标、物理 `10px` 图文间距与 QuarkMeta 统一暗色视觉样式（`UiHelper::applyMenuStyle`）。

6. **规则构造器紧凑按钮与控件样式契约 (Rule Builder Controls Style Contract)**：
   批量创建与批量重命名界面中的规则构造器控件（`RuleRow` / `CreateRuleRow`），其加减号控制按钮（`QPushButton#RuleDeleteBtn`）、规则下拉框（`QComboBox#RuleCombo`）与文本/数字输入框必须保持严格统一的紧凑视觉规范：
   - 加减号按钮（`-` / `+`）必须具备 1px 明确的灰边框 (`1px solid #434343`)、14px 加粗文字与像素级居中对齐，严禁出现文本溢出裁剪或无边框坍塌；
   - 规则下拉框与输入框高度锁定为 25px，具备 1px 实线边框 (`#444`) 与 4px 圆角，下拉弹出列表统一使用 QuarkMeta 深色背景 (`#2D2D2D`) 与 `#3E3E42` 项选中高亮。

7. **三栏视界物理绝对下限契约 (Three-Column View Physical Absolute Minimum Width Contract)**：
   主窗口（`MainWindow`）的全局绝对物理最小宽度限制锁定为 **`710px`**（标准三栏物理宽度计算：$3 \times 230\text{px} + 10\text{px} + 10\text{px} = 710\text{px}$）。无论主界面处于单栏、双栏还是侧边栏隐藏的沉浸模式，窗口物理宽度均不得低于 710px，为列视图（Column View / 米勒列下钻）提供至少流畅平铺 3 列视图的舒展显示空间，并保障顶栏地址栏、搜索栏及操作按钮绝不发生重叠挤压。

---

## 🛑 第二章：上下文菜单控制协调层 (ContextMenu Architecture)

根据 QuarkMeta 标准五层 Clean Architecture 规范，右键上下文菜单属于 **【3. 控制协调层 (Controller / Mediator Layer)】** 的核心职责：

1. **控制层职责与组装解耦**：
   上下文菜单的构建与分发统一由对应的 Controller（如 `ContentContextMenu`）承载。视图呈现层（View）仅负责捕获右键事件并透传坐标，禁止在视图类内部混杂菜单项硬编码与业务分发。

2. **状态感知与响应式动作 (State-Aware Actions)**：
   Controller 在构建菜单项时，必须通过领域层（Domain Service / Core Engine）或模型接口获取当前选中实体的权威状态（SSOT），根据置顶状态、收藏状态、锁状态动态生成精准的语义图标与动作文本。

3. **交互与阻断机制 (Non-Blocking Preview & Modal Actions)**：
   涉及连点、就地预览或连续选色的复杂子菜单（如色彩选择条、图标选择网格），必须采用即时数据更新与流畅退出机制，避免不必要的菜单强行关闭，保障流畅的交互节奏。

---

## ⌨️ 第四章：全局快捷键契约与窗口关闭规范 (Keyboard Shortcuts & Window Dismissal Architecture)

为确保系统交互习惯符合桌面应用最佳实践，全系统必须遵循以下**全局快捷键与窗口关闭设计规范**：

1. **`Ctrl + W` 窗口/对话框通用关闭契约**：
   - **统一关闭响应**：全系统中所有独立窗口（如 `QuickLookWindow`）、模态/非模态对话框（如 `TagManagerDialog`）、设置弹窗与自定义 Overlay 浮窗，必须统一将 `Ctrl + W` 绑定为窗口关闭/隐藏/取消（Close / Reject / Dismiss）的标准动作；
   - **焦点感知优先**：`Ctrl + W` 快捷键捕获必须遵循当前获得焦点的最顶层激活窗口（Active Top-Level Window / Focus Widget）优先原则，按下时仅触发当前处于激活状态的窗口关闭，严禁跨层或误关背景主窗口；
   - **零副作用退出**：按下 `Ctrl + W` 关闭窗口时，必须优雅释放该窗口持有的临时资源或事件监听器，确保状态同步更新，不留残余浮窗与僵尸对象。

---

## 🪟 第五章：对话框与弹窗顶层 HWND 激活与事件循环治理规范 (Dialog & HWND Architecture)

为彻底杜绝弹窗销毁后主窗口失去响应、非客户区 `WM_NCHITTEST` 命中检测失效、标题栏无法拖拽及光标滞留在手型状态等底层 HWND 激活失步问题，全系统必须无条件遵守以下**对话框与弹窗顶层架构治理规范**：

1. **【父级溯源铁律】全软件对话框 Parent 绑定契约 (Root HWND Binding Contract)**：
   全软件所有 `QDialog`、`FramelessDialog` 或 Overlay 浮窗在创建/弹出时，其 `parent` 必须且只能绑定为**顶层 `MainWindow`（或 `window()`）**，绝对禁止将 `DriveBarWidget`、`ContentPanel` 等局部 ToolBar 或 Child Widget 作为 Parent 传入。确保 Win32 消息循环在对话框销毁退栈时，能够 100% 精准恢复主窗口 HWND 的激活 (`SetActiveWindow`) 与输入使能 (`EnableWindow`) 状态。

2. **【模态嵌套禁令】去 `exec()` 阻塞与平原化事件循环 (Flattened Event Loop Contract)**：
   严禁在 `exec()` 运行期间再次嵌套调用二级 `exec()`（如在对话框内部再次弹出 `exec()` 阻断弹窗）。复杂管理面板（如 `TagManagerDialog`）应当向非模态 / Inline 内联编辑交互演进；对话框内部的二次输入/确认交互必须使用嵌入式 Inline 控件或异步响应，确保 C++ 调用栈与 Qt 事件循环始终保持平原化，消除堆栈交织死锁风险。

3. **【HWND 激活与 Win32 拖拽安全隔离契约】**：
   统一无边框对话框的生命周期与拖拽机制，严格禁止在模态阻塞事件循环中通过 `SendMessage(WM_NCLBUTTONDOWN)` 强抢线程控制权。对话框在 `reject()` / `accept()` 退出时，必须显式做好 HWND 状态清理与光标形态复位（复位为 `Qt::ArrowCursor`），保障主窗口非客户区拉伸与拖拽机制的完美平滑。

4. **【框架级顶层入口自愈契约 (QuarkApplication Self-Healing Framework Contract)】**：
   全系统统一使用自定义 `QuarkApplication` 继承并替代 `QApplication`。通过重写虚函数 `notify(QObject* receiver, QEvent* event)`，在框架事件派发的最源头实时监听全局所有 `QWindow` 及 `QDialog` 窗口的 `QEvent::Close` 与 `QEvent::Hide` 事件。每当任何弹窗或窗口关闭/隐藏的第 0 毫秒，由框架层自动且强制执行全局清场动作：彻底弹栈全局 `QGuiApplication::overrideCursor()`、释放孤儿鼠标抓取 (`releaseMouse()`)、解开 Win32 原生捕获锁 (`::ReleaseCapture()`) 并通过 `QCursor::setPos(QCursor::pos())` 驱动 Windows DWM 立即重新进行 WM_SETCURSOR Hit-Test 判定。全软件所有界面 100% 无感实现框架级自愈。

5. **【架构绝对归一化与零补丁契约 (Unified Clean Architecture & Zero-Patch Contract)】**：
   全软件在面对跨模块横切关注点（如输入焦点收回、鼠标抓取释放、光标形态复位、主题与样式派发）时，**必须且只能通过最顶层的框架门禁（如 `QuarkApplication`）或中介协调层（Mediator）做统一高内聚治理**。绝对禁止在下层具体的子对话框、局部面板或叶子 Widget 控件内部手写“擦屁股”式的碎片化补丁代码，彻底抹除打地鼠式的补丁遗留，从架构源头上降低全生命周期的维护与重构成本。

---

## 📁 第六章：文件冲突处理交互与对话框规范 (File Collision Resolution Architecture)

在批量文件粘贴或移动场景下，系统必须遵循标准且高辨识度的文件冲突处理规范：

1. **Adobe Bridge 风格直观冲突处理选项契约**：
   冲突选项包含“自动解析”、“替换”、“跳过”、“取消”四个标准椭圆胶囊按钮：
   - **自动解析**：同时保留目标文件，新写入的文件按标准递增规则编号重命名自动保留（如 `文件 (1).ext`）；
   - **替换**：直接覆盖目标文件夹中的同名文件；
   - **跳过**：忽略当前同名文件，不进行复制/移动；
   - **取消**：终止当前粘贴/移动操作。
   - **全部文件复选框**：左侧提供 `是否应用于全部文件？`，勾选后将当前选项策略应用至同批次所有冲突文件。

2. **全局统一冲突感知与 Apply to All 规则**：
   - 冲突对话框必须顶部清晰呈现当前操作路径与冲突数量 Stack（如 `正在将 X 个项目从 [源目录] 复制到 [目标目录]` 以及 `目标包含 X 个同名文件`）；
   - 提供 `为所有冲突执行此操作` (Apply to All) 动态复选框，勾选后对同批次后续所有同名冲突自动应用相同策略，杜绝连续弹窗打扰用户；
   - 样式与窗口必须继承 `FramelessDialog` 无边框优雅视觉体系，禁止原生窗口风格倒退。

---

## 📋 第七章：设计理念与物理实施隔离铁律

为了保障本规划文档作为顶层设计理念的权威性与纯洁性：

1. **本文档仅且只能记录高级设计理念、顶层架构规划与全局规范**；
2. **严禁将任何具体的代码修改点、Search/Replace Git Merge Diff 替换块、具体代码行号或编译调试命令写入本文档**；
3. **所有具体的代码重构与修改实施方案，必须物理隔离在 `QuarkMeta Architecture/Implementation Plan/` 目录下**（采用英文小写或类名映射命名，如 `FileCollisionDialog.md`）。
