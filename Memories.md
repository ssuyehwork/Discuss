# 核心记忆与顶层规范指导文档 (Memories.md)

本文档为 QuarkMeta 项目**唯一权威的历史记忆、全局设计理念、UI 交互哲学与工程架构规范指南**。
在进行任何项目结构调整、UI 改版、功能新增、重构或 Bug 修复时，Agent 与开发者**必须优先读取并无条件遵守本文档所约定的各项规范与红线**。

> **注意**：原 `QuarkMeta-Architecture-Planning.md` 与 `Guide & Preference.md` 已全面废除并无损迁移合并至本文档。从现在起，系统全面以 `Memories.md` 为唯一真理源 (SSOT)。

---

## 0. 全局视觉、品牌规范与核心红线

- **品牌橙色 (BrandOrange)**: 物理色值为 **`#cb7208`**。该颜色仅用于 FERREX 品牌 Logo、标题栏名称文字等品牌识别元素。
- **置顶激活色 (ActiveOrange)**: 物理色值为 **`#ff551c`**。该颜色用于全应用置顶激活按钮、内容面板置顶角标及侧边栏置顶状态。
- **强制解耦**: 品牌色与置顶激活色必须独立，严禁共同引用同一个常量或色值。
- **滑杆交互反馈**: 筛选面板中的百分比滑杆（如“占比”）必须支持实时数值回显，通过 `ToolTipOverlay` 在悬停或滑动时动态展示百分比，并在释放或离开时自动隐藏。
- **彻底杜绝 UI 焦点虚线框与虚线边框铁律**: 
  1. 除了内容面板中“显示空文件夹”可采用虚线边框作为特定视觉标识外，全应用其他所有 UI 控件、按钮、图标与元素（包括所有标题栏控制按钮、工具栏按钮、标签按钮、“+ 添加标签”按钮等）**绝对禁止出现任何形式的虚线框或虚线边框**！
  2. **代码级双重强锁**: 所有新建或修改的 `QPushButton`、`QToolButton` 等控制按钮，在 C++ 代码中**必须强制调用 `setFocusPolicy(Qt::NoFocus)`** 剥夺焦点抢占能力，同时在 QSS 样式中**必须显式声明 `outline: none;`**，双保险彻底从根源上消除 Qt 样式引擎绘制默认 Focus Rect 虚线框的所有可能！
- **图标格式限制**: 全应用所有 UI 元素与控件只允许使用 SVG 格式矢量图标，严禁使用任何文本或特殊符号（如 Unicode 符号、键盘字符等）替代/充当图标！
- **星级表达与真理源契约 (Rating Display SSOT Contract)**: 全软件所有涉及星级评级的 UI 展示、下拉选择框、筛选菜单与规则编辑器（包括但不限于 FilterPanel、SmartFolderDialog 智能文件夹规则配置、属性面板 MetaPanel 等），绝对禁止使用“1星”、“2星”等阿拉伯数字与文字组合，同时也严格遵守“严禁使用 Unicode 符号（如 ★）替代图标”的红线！必须统一使用 SVG 矢量图标（`star_filled`）动态绘制 1~5 颗星的 Pixmap/QIcon 进行渲染展示，保持全软件矢量化与图形 SSOT 通道 100% 物理一致。

---

## 1. UI 样式与主题管理规范

### 1.1 核心职责分工 (QSS + ThemeManager)
在本项目中，全局 UI 样式与主题控制遵循“**QSS 负责静态样式声明 + ThemeManager.cpp 负责代码层控制与统一加载**”的分工架构：

- **`resources/style.qss` ( 静态样式真理源 )**：
  负责集中定义应用中所有**全局统一的静态 UI 样式**，包括但不限于：
  * 顶层窗口与基础容器背景色、文字颜色；
  * 五大实体栏区卡片底板、边框与 5px 实体物理切缝 (`margin: 0px 2px`)；
  * 全局统一 `QMenu` 菜单样式 (背景、项 hover/selected、分隔线)；
  * 全局统一滚动条 (`QScrollBar`) 样式；
  * 全局统一输入框 (`QLineEdit`, `QTextEdit`, `QPlainTextEdit`) 样式；
  * 全局 `QTreeView` 列表视图暗色斑马纹背景与选中项高亮；
  * 全局 `QCheckBox` 复选框样式。

- **`src/ui/ThemeManager.cpp` ( 样式加载与底层控制入口 )**：
  用于处理 QSS 格式无法独立完成或表达的代码层控制逻辑，包括但不限于：
  * **全局样式加载**：提供 `initialize(app)` 与 `getGlobalStyleSheet()` 入口，将 `:/style.qss` 集中读取并注入到全局 `QApplication`；
  * **原生窗口属性控制**：针对右键/托盘菜单等控件设置 `Qt::WA_TranslucentBackground`（背景半透明）、`Qt::FramelessWindowHint`（无边框）及 `Qt::NoDropShadowWindowHint`（消除原生阴影）等代码层窗口属性；
  * **动态覆盖与集中入口**：提供统一的 C++ 方法入口（如 `applyMenuStyle`）以便对特定场景实施局部覆盖与集中管理。

### 1.2 UI 样式开发规范与红线
1. **禁止内联硬编码**：在后续开发与重构中，**严格禁止在控件 C++ 代码中随意调用 `setStyleSheet(...)` 采用内联方式硬编码样式**。
2. **全局统一修改**：凡涉及通用控件外观、颜色、边距、圆角等静态样式调整，一律在 `resources/style.qss` 中进行集中修改或扩充选择器。
3. **全局样式表注入对象规范（作用域红线）**：全局 QSS 样式表**必须且只能注入至 `QApplication` 实例**（通过 `ThemeManager::initialize(app)`），**绝对禁止仅挂载在 `MainWindow` 窗口实例上**。因为 Qt 样式表仅沿着父子部件树（Widget Tree）向下级联继承，若挂在 `MainWindow` 上，会导致所有 `parent = nullptr` 的独立顶层窗口（如 `QuickLookWindow`、各类无边框对话框）无法继承 QSS，退化为原生控件外观。
4. **极少数例外**：仅在纯运行时根据用户输入/数据动态计算生成的数值色彩（如 `MetaRatingColorWidget` 依据动态 hex 色彩绘制）时，才允许在 C++ 中进行必要的高效动态处理。

### 1.3 顶层 UI 设计哲学与菜单/输入框规范
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
   - **无彩色单色绝对统一（绝对禁止例外）**：全局所有右键菜单与上下文菜单项图标**统一且仅可采用同一个中性单色设计 (`#EEEEEE`)**，绝对禁止在右键菜单任何图标上出现或使用任何特殊色彩/着色（包括但不限于 `#ff551c`、红、黄、蓝、绿等），绝对不存在任何例外场景（例如“标记为目标文件夹”项也必须使用 `#EEEEEE` 中性单色），确保界面视觉沉静高档。
4. **输入框清除按钮响应式呈现与原生契约**：
   - 每个可编辑的单行输入框必须且只能配置 Qt 原生的 `setClearButtonEnabled(true)`，杜绝脑补另创自定义清除按钮；
   - 全系统中所有具备清除能力的单行文本输入框（如全局搜索栏、标签/日期/类型筛选输入框、路径编辑框），其清除按钮 (×) 必须严格遵循“有文本时动态显示、无文本时绝对隐匿”的响应式契约。严禁在输入框为空（包括呈现 Placeholder 占位文本）时显示清除按钮。
5. **文本输入框应用专属上下文菜单契约 (App-Exclusive LineEdit Context Menu Contract)**：
   全软件所有文本输入框（包括单行 `QLineEdit` 与多行 `QTextEdit`）严格禁止弹出操作系统默认的原生英文右键菜单。必须统一拦截并替换为 QuarkMeta 应用专属暗色右键菜单（完整提供 `撤销 Ctrl+Z`、`重做 Ctrl+Y`、`剪切 Ctrl+X`、`复制 Ctrl+C`、`粘贴 Ctrl+V`、`删除`、`全选 Ctrl+A`），且每一个菜单项必须配备 100% 语义匹配的中性单色 SVG 矢量图标、物理 `10px` 图文间距与 QuarkMeta 统一暗色视觉样式（`UiHelper::applyMenuStyle`）。
6. **规则构造器紧凑按钮与控件样式契约 (Rule Builder Controls Style Contract)**：
   批量创建与批量重命名界面中的规则构造器控件（`RuleRow` / `CreateRuleRow`），其加减号控制按钮（`QPushButton#RuleDeleteBtn`）、规则下拉框（`QComboBox#RuleCombo`）与文本/数字输入框必须保持严格统一的紧凑视觉规范：
   - 加减号按钮（`-` / `+`）必须具备 1px 明确的灰边框 (`1px solid #434343`)、14px 加粗文字与像素级居中对齐，严禁出现文本溢出裁剪或无边框坍塌；
   - 规则下拉框与输入框高度锁定为 25px，具备 1px 实线边框 (`#444`) 与 4px 圆角，下拉弹出列表统一使用 QuarkMeta 深色背景 (`#2D2D2D`) 与 `#3E3E42` 项选中高亮。
7. **Tooltip 提示控件限制**:
   全应用**严禁使用 QWidget 原生 `setToolTip(...)` 硬编码悬浮提示**，悬浮提示只可使用统一的 `ToolTipOverlay` 控件（结合 `m_hoverFilter` 事件过滤器）实现感知与展示，以确保主题视觉一致性与平滑交互体验。
8. **“在新标签页中打开”图标矢量键规范契约 (“Open in New Tab” Vector Icon Specification Contract)**：
   全软件所有右键上下文菜单（包括“标记”面板 `MarkedPanel`、库面板 `LibraryPanel`、收藏夹 `FavoritePanel` 等），针对“在新标签页中打开”菜单项，其矢量图标必须统一绑定且仅可使用 **`Folder-add`**（严禁使用裸 `add` 或 `plus` 等泛化图标），确保图标语义精准对齐新建 Tab/文件夹卡片，且图标颜色无条件严格遵循 `#EEEEEE` 中性单色红线。

---

## 2. 界面组件与布局物理规范

### 2.1 标题栏容器 (TitleBar)
- **高度 (Height)**: 物理高度固定为 `34px`。
- **布局边距 (ContentsMargins)**: `(5, 0, 5, 0)`（左侧边距 `5px`，右侧物理对齐 `5px`）。
- **全局间距 (Spacing)**: `5px` (应用名与按钮组之间的间距)。

### 2.2 按钮组容器 (TitleBarButtons Container)
- **布局间距 (Spacing)**: 按钮与按钮之间的物理间距固定为 `5px`。
- **布局边距 (ContentsMargins)**: `(0, 0, 0, 0)`。

### 2.3 按钮物理参数 (Button Parameters)
- **外框尺寸 (FixedSize)**: `24x24px`。
- **图标尺寸 (IconSize)**: `18x18px`。
- **圆角 (BorderRadius)**: `4px`。
- **背景样式**:
  - 默认状态: `transparent` (透明)；
  - 悬停状态 (Hover): `#3E3E42` (关闭按钮除外)；
  - 按下状态 (Pressed): `#4E4E52`。

### 2.4 特殊按钮与交互规范
- **关闭按钮 (Close Button)**:
  - **全应用标准**: 所有界面（主窗口、面板、对话框、标签块）的关闭按钮必须保持视觉一致性；
  - **背景颜色**: 默认状态固定为 `ErrorRed` (`#e81123`)，持续显示红色高亮；
  - **悬停状态 (Hover)**: `ErrorRed` (`#e81123`)；
  - **按下状态 (Pressed)**: `#A50000`；
  - **圆角**: `4px`。
- **置顶按钮与窗口置顶逻辑 (Pin Button & Window Pinning)**:
  - **激活颜色**: 选中状态下图标颜色必须切换为唯一合法色值 **`#ff551c`** (`ActiveOrange`)；
  - **底层绝对法则**: 窗口置顶逻辑**只许可使用 Win32 原生 API `SetWindowPos(HWND_TOPMOST / HWND_NOTOPMOST)`**！**严禁使用其他任何方式**（严禁使用 `setWindowFlags`，严禁使用 `Qt::WindowStaysOnTopHint` 等），确保置顶时零闪烁、不改变窗口句柄与尺寸，绝对对齐 Windows 平台最高堆叠层级标准！
- **同步按钮 (Sync Button)**:
  - **状态联动**: 存在待同步元数据时，图标强制显示为 `ErrorRed`；同步完成后恢复为 `TextMain`。
- **标题栏按钮交互**: 所有标题栏按钮必须开启 `Qt::WA_Hover` 属性以触发悬停事件。必须安装 `m_hoverFilter` 事件过滤器以支持全局 ToolTip 悬浮提醒。新建按钮 (+) 采用手动 `popup` 菜单模式，严禁使用 `setMenu` 以免破坏图标的绝对居中对齐。

### 2.5 全局 QMenu 菜单规范 (ThemeManager)
- **菜单背景**: `#252526` (边界 `#333333`，圆角 6px)；
- **选中/高亮背景色 (Item Hover/Selected)**: 全局项目与菜单 hover 背景色统一为 **`#3E3E42`**（或 `#2A2D2E`），选中高亮主题蓝色统一为 **`#378ADD`**（15% 透明度，即 `QColor("#378ADD"); bg.setAlphaF(0.15f);`）；侧边栏 (`FavoritePanel`、`LibraryPanel`、`NavPanel`) 与右侧内容区域 (`TreeItemDelegate`、`ThumbnailDelegate`、`ColumnItemDelegate`) 项目选中时背景色必须严格统一在底层绘制 **`#378ADD` 15% 半透明蓝色**，确保图标与文本清晰位于高亮层之上，绝对禁止出现强行覆盖图标的不透明硬色块或硬编码暗灰色 (`#37373D`)；
- **图标与文本间距 (Icon-Text Spacing)**: 图标与文本之间的物理间距统一并精确锁定为 **`10px`**（通过 `QMenu::icon { margin-right: 10px; }` 控制）。

### 2.6 分栏布局初始化与尺寸计算规范 (PanelLayoutManager)
- **`initLayout()` 单一调用收敛**: `PanelLayoutManager::initLayout()` 中，`updateDynamicMinimumSize()` **必须且只能**在 `QTimer::singleShot(0, ...)` 延迟回调函数内部调用唯一一次。
- **严禁同步二次调用**: 严禁在 `initLayout()` 函数末尾同步调用 `updateDynamicMinimumSize()`。由于 `setMinimumWidth()` 会在被调用且当前窗口宽度小于新设最小值时立即撑大窗口，两次调用时机不同计算值不一致会导致应用每次启动时主窗口被莫名撑大。
- **独立场景不连带**: `resetSplitterLayout()`、`setPanelVisible()`、`toggleImmersiveMode()` 等各自独立场景中调用一次 `updateDynamicMinimumSize()` 为正常行为，保持独立性。

---

## 3. UI 交互体验与渲染防抖规范

### 3.1 UI 异步加载与防闪烁规范
- **原则**: 在内容面板（`ContentPanel`）进行异步数据扫描（如物理目录扫描、数据库分类查询）前，**禁止**先行调用 `m_model->clear()`。
- **目的**: 避免在数据就绪前的空窗期内出现“白屏/黑屏”视觉抖动，保留旧数据直至新数据通过 `setRecords` 实现毫秒级原子替换。
- **例外**: 当目标路径列表确定为空（如路径不存在或搜索重置）时，必须执行同步 `clear()` 以反馈真实状态。
- **竞态保护**: 加载流程必须绑定 `m_loadRequestId`。在异步回调中，必须校验回调携带的 ID 是否与当前面板 ID 一致，否则丢弃结果以防止快速切换导致的数据串扰。

### 3.2 缩略图平滑加载规范 (Plan-108)
- **原则**: 针对图形文件（图像、SVG等），在异步加载缩略图期间，`data()` 接口必须返回空图标 (`QIcon()`)。
- **目的**: 拦截 Delegate 的默认图标绘制逻辑，防止出现“系统图标 -> 缩略图”的二段式闪烁抖动。
- **视觉反馈**: `ThumbnailDelegate` 必须通过检测空图标状态，在单元格区域绘制轻量的灰色圆角矩形 (`#3A3A3A`) 作为占位背景，确保从占位态到内容态的过渡平滑且不突兀。

### 3.3 快速预览 (QuickLook) 规范 (Plan-109)
- **拦截机制**: 必须采用“黑名单拦截+白名单准入”的双重防御机制。严禁预览文件夹、安装包 (.msi, .exe), 系统库 (.dll, .sys) 及各类压缩包 (.zip, .7z 等)。
- **画质**: 预览标准图像 (jpg, png, webp 等) 时必须加载全分辨率原图，并全程开启 `SmoothPixmapTransform` 以消除锯齿。
- **性能红线**: `renderImage` 在加载原图前必须检查文件物理大小。若超过 50MB，必须自动降级调用高清缩略图引擎以确保 UI 响应性能。
- **样式**: 预览窗口内的滚动条样式必须严格遵循全局规范：宽度 10px、圆角 3px、背景透明、Handle 颜色对齐 `BorderColor` (#333333)。

### 3.4 列视图（ColumnView）双击空白处交互与逻辑规范
- **以“最后一列/当前视角”为锚点**: 在列视图（`ColumnView`）中，无论展开了多少列分栏，在任何列分栏的空白处或视图最右侧的背景留白区域双击左键，始终以最后一列（最右侧列）当前展示的文件夹数据为锚点执行回退上一级。
- **父列高亮持续保留**: 退回上一级（即移除最右侧分栏列）后，上一级列中原先选中的文件夹节点必须**持续保持高亮选中状态**，方便用户无须重新查找即可再次点击进入。
- **逐级精准回退与单列降级**: 每次双击空白处/背景留白处均以当前最后一列数据为基准向上一级逐级退回；当列视图仅剩一列（根列）时，继续双击空白处则自动降级触发全局 `NavigationService::instance().goUp()`。

---

## 20. 智能文件夹规则拆分与颜色属性隔离规范 (Smart Folder Rule & Color Attribute Isolation Architecture)

> 本章节整理自用户明确指示与架构研讨，作为智能文件夹规则类型扩展、颜色属性隔离及相关界面渲染的唯一真理源 (SSOT)。

### 20.1 规则类型名称与顺序规范
智能文件夹（`SmartFolderDialog` / `SmartFolderRuleRow`）类型下拉框的显示名称与排列顺序必须严格保持如下 16 项统一规范：
1. **评分** (`SmartRuleType::Rating` = 0)
2. **名称** (`SmartRuleType::Name` = 4，保持原 Keyword 枚举值 4 不变)
3. **扩展名** (`SmartRuleType::Extension` = 3)
4. **标签** (`SmartRuleType::Tag` = 2)
5. **文件夹** (`SmartRuleType::Folder` = 5)
6. **文件夹名** (`SmartRuleType::FolderName` = 6)
7. **颜色标记** (`SmartRuleType::Color` = 1，手动选择标记色，枚举值 1 保持不变)
8. **图像颜色** (`SmartRuleType::AutoColor` = 15，系统算法自动提取主色，新增枚举值)
9. **添加日期** (`SmartRuleType::AddedDate` = 7)
10. **修改日期** (`SmartRuleType::ModifiedDate` = 8)
11. **创建日期** (`SmartRuleType::CreatedDate` = 9)
12. **链接** (`SmartRuleType::Link` = 10)
13. **注释** (`SmartRuleType::Note` = 11)
14. **宽度** (`SmartRuleType::Width` = 12)
15. **高度** (`SmartRuleType::Height` = 13)
16. **文件大小** (`SmartRuleType::FileSize` = 14)

### 20.2 颜色标记与图像颜色绝对隔离铁律 (Color vs AutoColor Isolation)
- **设计理念绝对隔离**: “颜色标记”（`Color`）与“图像颜色”（`AutoColor`）代表截然不同的设计理念：
  - **颜色标记**：指用户手动贴上的 9 种标记色标（存储与匹配 `color_hex` 数据列，使用 `Style::getColorPalette()` 真理源）；
  - **图像颜色**：指算法从图像自动提取的主色（存储与匹配 `auto_color` 数据列，包含色值与 `tolerance` 容差 slider）；
- **全链路严禁混淆/兜底**: 在界面显示、JSON 存储、DAO SQL 查询构建 (`buildColorCondition` vs `buildAutoColorCondition`)、`FilterPanel` 筛选过滤映射 (`applySmartFolderRecord`) 的任何环节，**绝对禁止**相互混用、互相兜底或合并为同一个过滤项！

---

## 21. 面板拖拽迁移遮罩透明度视觉规范 (Drag Overlay Opacity Specification)

- **应用对象**: 元数据面板（`MetaPanel`）与筛选器面板（`FilterPanel`）的拖拽迁移遮罩组件（`DragOverlayWidget`）。
- **QSS 样式表达**: 必须统一声明为 `background-color: rgba(0, 122, 255, 0.50); border: 2px solid #007AFF;`（即 50% 不透明度 / 50% 半透明），在保持极佳文字辨识度的同时透出底层面板视觉，严禁与 `OrientationPreviewWidget`（25% 不透明度）混用。
