# 全工程内联 `setStyleSheet` 排查与外联 QSS 归一化重构方案

## 1. 核心架构问题回答 (3 Questions)

### Q1: 唯一事实源 (Single Source of Truth, SSOT) 是什么？在哪里？
- **静态样式唯一事实源**：`resources/style.qss`。应用中所有 UI 控件的外观、颜色、边框、字号、Hover/Selected 交互状态及布局 Margin，均必须收拢在此文件中定义。
- **主题控制唯一事实源**：`src/ui/ThemeManager.h/cpp`。仅负责从 `:/style.qss` 加载样式并应用至全局 `QApplication`，以及管理平台级原生窗口属性（如 `QMenu` 半透明/无边框属性）。

### Q2: 封装完整性 (Encapsulation Integrity) 如何保证？
- 不变动任何类的公开 API 接口签名。
- 通过为控件分配规范的 `objectName`（如 `setObjectName("BtnCopyPath")`）或自定义 CSS 属性选择器，使 `resources/style.qss` 能通过 CSS 选择器精确匹配并控制控件样式。
- 彻底移除 C++ 控件构造及初始化流程中硬编码的内联样式字符串，只保留极少数纯动态计算值（如 `MetaRatingColorWidget` 中根据运行时动态 Hex 颜色绘制背景的按钮）。

### Q3: 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **表面现象**：虽然引入了 `Guide & Preference.md` 规范，但历史代码中全工程 41 个 C++ 源文件中仍散落着 **241 处** 内联 `setStyleSheet(...)` 调用，导致直接修改 `style.qss` 无法完全掌控全软件 UI 样式。
- **根因**：过去在开发具体 UI 面板时，开发者随手通过 C++ 内联样式快速设置外观，未建立“分配 `objectName` / class + 统一在 `style.qss` 声明规则”的规范习惯。

---

## 2. 全工程内联 `setStyleSheet` 排查与分类

经过对 `src/` 目录的全面扫描，共发现 241 处 `setStyleSheet` 调用。按其性质分为两类：

### 2.1 第一类：必须彻底迁移至 `resources/style.qss` 的静态/状态样式 (占比 > 98%)
包含各面板与弹窗中的 Label、Button、Edit、ScrollArea、Frame、Splitter、CheckBox 等静态或伪状态样式：
- `src/ui/MetaPanel.cpp` (26 处): `#PathEdit`, `#BtnCopyPath`, `#BtnOpenLocation`, `#BtnAddTagBig` 等
- `src/ui/FilterPanel.cpp` (24 处): 日期排序按钮、筛选组 Header、ScrollArea 等
- `src/ui/MainWindow.cpp` (17 处): `#SizeSlider` 等
- `src/ui/TagManagerDialog.cpp` (13 处), `ColorPicker.cpp` (10 处), `DuplicateConflictDialog.cpp` (10 处)
- `src/ui/AddressHistoryPanel.cpp` (9 处), `FramelessFileDialog.cpp` (9 处), `SearchHistoryPanel.cpp` (9 处)
- `src/ui/QuickLookWindow.cpp` (8 处), `TagSelectorOverlay.cpp` (8 处), `BatchCreateDialog.cpp` (8 处)
- 其余面板如 `NavPanel`, `FavoritePanel`, `ContentPanel`, `AddressBar`, `BreadcrumbBar`, `UndoToastOverlay`, `TaskProgressToolBar` 等 20+ 个文件。

### 2.2 第二类：允许保留的运行时动态计算样式 (仅 2 处)
1. `src/ui/MetaRatingColorWidget.cpp`: `btn->setStyleSheet(QString("background: %1; ...").arg(colHex));` —— 依赖用户运行时动态添加的自定义 Hex 调色盘色彩。
2. `src/ui/ColorPicker.cpp`: `m_previewBlock->setStyleSheet(...)` —— 随用户调色盘滑动实时变化的预览背景色。

---

## 3. 归一化重构实施策略 (分步实施)

为确保代码稳健与安全，采取**选择器规范化 + 样式收拢 + C++ 剥离**的标准化步骤：

1. **选择器设计**：
   - 命名空间及 ObjectName 规范：控件统一设置简明 `objectName`（例如 `#BtnCopyPath`、`#AddressPathEdit`、`#NavHeaderTitle`）。
   - 扩展 `resources/style.qss`：将 230+ 处散落的样式规则分类写入 `resources/style.qss`。
2. **C++ 代码剥离**：
   - 清理 C++ 代码中的内联 `setStyleSheet(...)` 字符串，替换为 `setObjectName(...)`。
3. **分批治理目录**：
   - 批次一：核心五大面板（`MetaPanel`, `FilterPanel`, `NavPanel`, `FavoritePanel`, `ContentPanel`）
   - 批次二：顶层窗口与导航（`MainWindow`, `AddressBar`, `BreadcrumbBar`, `SearchController`）
   - 批次三：弹窗与浮层（`TagManagerDialog`, `ColorPicker`, `QuickLookWindow`, `TagSelectorOverlay` 等）

---

## 4. 验证计划
1. 编译与运行验证：确保全软件无 QSS 语法错误，界面布局、暗色主题、hover 高亮与圆角完全符合 `UI_DESIGN_SPEC.md`。
2. Pre-commit 检查与内存记录。
