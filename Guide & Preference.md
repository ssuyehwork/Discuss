# Guide & Preference ( UI 样式与主题管理规范 )

## 1. 核心职责分工

在本项目中，全局 UI 样式与主题控制遵循“**QSS 负责静态样式声明 + ThemeManager.cpp 负责代码层控制与统一加载**”的分工架构：

### 1.1 `resources/style.qss` ( 静态样式真理源 )
`resources/style.qss` 负责集中定义应用中所有**全局统一的静态 UI 样式**，包括但不限于：
* 顶层窗口与基础容器背景色、文字颜色；
* 五大实体栏区卡片底板、边框与 5px 实体物理切缝 (`margin: 0px 2px`)；
* 全局统一 `QMenu` 菜单样式 (背景、项 hover/selected、分隔线)；
* 全局统一滚动条 (`QScrollBar`) 样式；
* 全局统一输入框 (`QLineEdit`, `QTextEdit`, `QPlainTextEdit`) 样式；
* 全局 `QTreeView` 列表视图暗色斑马纹背景与选中项高亮；
* 全局 `QCheckBox` 复选框样式。

### 1.2 `src/ui/ThemeManager.cpp` ( 样式加载与底层控制入口 )
`ThemeManager.cpp` 用于处理 QSS 格式无法独立完成或表达的代码层控制逻辑，包括但不限于：
* **全局样式加载**：提供 `initialize(app)` 与 `getGlobalStyleSheet()` 入口，将 `:/style.qss` 集中读取并注入到全局 `QApplication`；
* **原生窗口属性控制**：针对右键/托盘菜单等控件设置 `Qt::WA_TranslucentBackground`（背景半透明）、`Qt::FramelessWindowHint`（无边框）及 `Qt::NoDropShadowWindowHint`（消除原生阴影）等代码层窗口属性；
* **动态覆盖与集中入口**：提供统一的 C++ 方法入口（如 `applyMenuStyle`）以便对特定场景实施局部覆盖与集中管理。

---

## 2. UI 样式开发规范与红线

1. **禁止内联硬编码**：在后续开发与重构中，**严格禁止在控件 C++ 代码中随意调用 `setStyleSheet(...)` 采用内联方式硬编码样式**。
2. **全局统一修改**：凡涉及通用控件外观、颜色、边距、圆角等静态样式调整，一律在 `resources/style.qss` 中进行集中修改或扩充选择器。
3. **极少数例外**：仅在纯运行时根据用户输入/数据动态计算生成的数值色彩（如 `MetaRatingColorWidget` 依据动态 hex 色彩绘制）时，才允许在 C++ 中进行必要的高效动态处理。
