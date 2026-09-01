# Guide & Preference ( 工程系统指南与设计规范总则 )

## 0. 本文档的定位与原则

本文档为 QuarkMeta 工程系统的核心规范指南，旨在**统一系统开发标准、防止另起炉灶、杜绝重复造轮子与规范割裂**。所有开发者与 AI Agent 在进行项目结构调整、UI 改版、功能新增及重构时，必须严格遵守本文档定义的各项规范。

**如果本文档里的某条通用原则与写明了具体类名/文件名的项目专属规则文档发生冲突，以具体的专属规则文档为准。**

---

## 1. UI 样式与主题管理规范

### 1.1 核心职责分工
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
3. **极少数例外**：仅在纯运行时根据用户输入/数据动态计算生成的数值色彩（如 `MetaRatingColorWidget` 依据动态 hex 色彩绘制）时，才允许在 C++ 中进行必要的高效动态处理。

### 1.3 视觉色彩与品牌规范
- **品牌橙色 (BrandOrange)**: 物理色值为 **`#cb7208`**。该颜色仅用于 FERREX 品牌 Logo、标题栏名称文字等品牌识别元素。
- **置顶激活色 (ActiveOrange)**: 物理色值为 **`#ff551c`**。该颜色用于全应用置顶激活按钮、内容面板置顶角标及侧边栏置顶状态。
- **强制解耦**: 品牌色与置顶激活色必须独立，严禁共同引用同一个常量或色值。
- **高亮与选中色**: 托盘菜单与右键上下文菜单选中/高亮背景色统一为 **`#3E3E42`**。

### 1.4 UI 控件通用约束
- **输入框清除按钮**: 每个可编辑的输入框必须且只能配置 Qt 原生的 `setClearButtonEnabled(true)`，杜绝脑补另创自定义清除按钮。
- **图标格式限制**: 全应用所有 UI 元素与控件只允许使用 SVG 格式矢量图标，严禁使用任何文本或特殊符号（如 Unicode 符号、键盘字符等）替代/充当图标。
- **UI 虚线边框限制**: 除了内容面板中“显示空文件夹”可采用虚线边框作为特定视觉标识外，全应用其他所有 UI 控件、按钮与元素（如“+ 添加标签”按钮等）一律禁止使用虚线边框。

---

## 2. 界面组件与布局物理规范

### 2.1 标题栏容器与按钮组物理规范
- **标题栏容器 (TitleBar)**:
  - 物理高度固定为 `34px`；
  - 布局边距 `(5, 0, 5, 0)`（左侧边距 5px，右侧物理对齐 5px）；
  - 全局间距 `5px`。
- **按钮组容器 (TitleBarButtons Container)**:
  - 布局间距 `5px`；
  - 布局边距 `(0, 0, 0, 0)`。
- **按钮物理参数**:
  - 外框尺寸 `24x24px`；
  - 图标尺寸 `18x18px`；
  - 圆角 `4px`；
  - 背景样式：默认 `transparent`，悬停 `#3E3E42`（关闭按钮除外），按下 `#4E4E52`。

### 2.2 特殊按钮与交互规范
- **关闭按钮 (Close Button)**:
  - 默认与悬停背景颜色固定为 `ErrorRed` (`#e81123`)，持续显示红色高亮；
  - 按下状态 `#A50000`；圆角 `4px`。
- **置顶按钮 (Pin Button)**:
  - 选中激活状态下图标颜色必须切换为唯一合法色值 **`#ff551c`** (`ActiveOrange`)。
- **同步按钮 (Sync Button)**:
  - 存在待同步元数据时图标强制为 `ErrorRed`，同步完成后恢复为 `TextMain`。
- **按钮事件**: 所有标题栏按钮必须开启 `Qt::WA_Hover` 属性；新建按钮 (+) 采用手动 `popup` 菜单模式，严禁使用 `setMenu`。

### 2.3 全局 QMenu 菜单规范 (ThemeManager)
- 菜单背景 `#252526`，边框 `#333333`，圆角 `6px`。
- 项悬停/选中背景色统一为 **`#3E3E42`**。

---

## 3. UI 交互体验与渲染防抖规范

### 3.1 UI 异步加载与防闪烁规范
- **原则**: 在内容面板（`ContentPanel`）进行异步数据扫描前，**禁止**先行调用 `m_model->clear()`，保留旧数据直至新数据通过 `setRecords` 实现毫秒级原子替换，避免白屏/黑屏视觉抖动。
- **例外**: 当目标路径列表确定为空（如路径不存在或搜索重置）时，执行同步 `clear()`。
- **竞态保护**: 加载流程必须绑定 `m_loadRequestId`，在异步回调中校验 ID 是否一致，丢弃失效回调。

### 3.2 缩略图平滑加载规范
- **原则**: 针对图形文件在异步加载缩略图期间，`data()` 接口必须返回空图标 (`QIcon()`)，拦截 Delegate 默认图标绘制。
- **占位态**: `ThumbnailDelegate` 必须在单元格区域绘制轻量灰色圆角矩形 (`#3A3A3A`) 作为占位背景，确保过度平滑。

### 3.3 快速预览 (QuickLook) 规范
- **拦截机制**: 采取“黑名单拦截+白名单准入”机制，严禁预览文件夹、可执行文件 (.exe, .dll, .msi)、系统库 (.sys) 及各类压缩包 (.zip, .7z 等)。
- **性能防线**: `renderImage` 加载原图前检查文件大小，若超过 50MB 自动降级调用高清缩略图引擎。
- **画质与样式**: 全程开启 `SmoothPixmapTransform`；预览窗口内滚动条样式对齐全局规范（宽 10px、圆角 3px、背景透明、Handle `#333333`）。

### 3.4 滑杆交互反馈
- 筛选面板中的百分比滑杆必须支持实时数值回显，通过 `ToolTipOverlay` 在悬停或滑动时动态展示百分比，离开时自动隐藏。

---

## 4. 业务领域与数据契约规范

### 4.1 内容面板数据源判定与强类型契约
- **核心定义**: 统一使用强类型枚举 `DataSourceType` 规范 `ContentPanel` 的数据源来源（`DiskNav` / `UserCategory` / `SystemCategory` / `PathList`）。
- **铁律**: 严禁在代码中手写散落的弱类型字符串判定（如 `if (m_currentCategoryType == "all")`），判定统一通过 `ContentPanel::dataSourceType()` 接口。
- **辅助接口**: `ContentPanel` 必须公开 `isMirrorSource()` 与 `isManagedContext()` 辅助判定方法。

### 4.2 双轨元数据落盘路由
- 在托管库上下文 (`isManagedContext() == true`) 内，元数据 100% 写入统一 SQLite 本地数据库；
- 在库外普通磁盘模式 (`isManagedContext() == false`) 下，元数据自动调用 `AmMetaJson` 精准写入同级 `.QuarkMeta.json` 离散缓存中，不污染用户原始物理盘。

### 4.3 隔离式多维关联索引与范围感知搜索
- **关联索引**: `MetadataManager` 通过 `m_fileNameToFids`（仅文件）、`m_folderNameToFids`（仅文件夹）及 `m_extensionToFids`（仅小写后缀）三个隔离的倒排索引管理关联，注册时须校验去重。
- ** Scope-Aware 搜索**: 搜索请求通过 `CoreController::performSearch` 转发，实时绑定顶部蓝色提示线 (Focus Line) 位置，分类模式下限定于当前分类及子类，导航模式下限定于当前路径及子目录。

---

## 5. 线程安全与底层 API 调用边界

### 5.1 媒体提取管道线程安全边界
- `MediaExtractorPipeline` 与 `CapsuleMediaExtractor` 后台提取管道中，任何触碰 `QSvgRenderer` / `QPainter` / `QPixmap` / `QIcon` 等 Qt Gui 模块 API 的代码段，必须用 `CapsuleMediaExtractor::s_qtGuiMutex` 显式串行化保护。
- Qt Gui 模块 API 不保证线程安全，并发访问会导致内部缓存越界写入引发进程崩溃 (`0xC0000005`)。
- 文件 I/O、哈希计算、数据库读写等与 Qt Gui 无关的部分维持并行。
