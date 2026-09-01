# 批次三：对话框、浮层与小组件内联 `setStyleSheet` 归一化重构方案

## 1. 核心架构问题回答 (3 Questions)

### Q1: 唯一事实源 (Single Source of Truth, SSOT) 是什么？在哪里？
- **静态与对话框组件外观 SSOT**：`resources/style.qss`。所有对话框（标签管理、调色盘、冲突检测、批量重命名等）、浮层（快捷预览 QuickLook、标签选择浮层、撤销 Toast 提示、搜索/地址历史浮层）以及小组件（任务进度条、规则行组件等）的外观与交互状态，统一在 `style.qss` 中定义。
- **主题控制与动态场景 SSOT**：`src/ui/ThemeManager.h/cpp` 负责 QSS 加载与窗口级透明/无边框属性设置；纯运行时动态色块（如调色盘实时预览块）允许保留高效动态更新。

### Q2: 封装完整性 (Encapsulation Integrity) 如何保证？
- 冻结所有对话框与浮层组件的 C++ 公开 API 签名。
- C++ 控件统一通过 `setObjectName(...)` 标注样式 ID（例如 `#TagManagerSearchEdit`、`#QuickLookContainer`、`#UndoToastMsg`、`#ColorPickerHexEdit` 等），将内联样式字符串彻底剥离并统一抽离至 `resources/style.qss`。

### Q3: 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **表面现象**：对话框与浮层小组件内部散落着 130+ 处内联 `setStyleSheet(...)` 字符串，且含有许多重复的颜色与边框样式。
- **根因**：历史开发中各个独立的 Dialog 与 Overlay 属于单独文件创建，开发者习惯于在各自 C++ 类的构造函数里就地编写 QSS 字符串，未将公共规则统一抽离至样式表。

---

## 2. 批次三排查与分类清单 (共计 130+ 处)

### 2.1 对话框类 (Dialogs)
1. `src/ui/TagManagerDialog.cpp` (13 处): 顶栏、搜索框、侧边栏、滚动区、标签按钮 `#TagManagerSearchEdit` 等
2. `src/ui/ColorPicker.cpp` (10 处): 调色盘主框、底栏、Hex 编辑框 `#ColorPickerHexEdit`（保留预览块动态 setStyleSheet 1 处）
3. `src/ui/DuplicateConflictDialog.cpp` (10 处): 冲突列表、比较视图、操作按钮
4. `src/ui/BatchCreateDialog.cpp` (8 处), `BatchRenameDialog.cpp` (5 处), `FramelessDialog.cpp` (8 处), `FramelessFileDialog.cpp` (9 处)
5. `src/ui/dialogs/FramelessInputDialog.cpp` (4 处), `FramelessConfirmDialog.cpp` (3 处), `FramelessColorPicker.cpp` (2 处)

### 2.2 浮层类 (Overlays & Panels)
1. `src/ui/QuickLookWindow.cpp` (8 处) & `QuickLookGraphicsView.cpp` (3 处): 预览容器 `#QuickLookContainer`、滚动条与提示标签
2. `src/ui/TagSelectorOverlay.cpp` (8 处): 搜索 Edit、侧边组列表、网格 ScrollArea
3. `src/ui/AddressHistoryPanel.cpp` (9 处) & `SearchHistoryPanel.cpp` (9 处): 历史下拉框、标题行、清除按钮
4. `src/ui/UndoToastOverlay.cpp` (4 处) & `ToolTipOverlay.cpp` (1 处): Toast 提示框与按钮

### 2.3 小组件类 (Components & Toolbars)
1. `src/ui/TaskProgressToolBar.cpp` (5 处): 进度条底栏、标题、进度数字
2. `src/ui/RuleRow.cpp` (6 处), `CreateRuleRow.cpp` (6 处): 下拉框、Edit、SpinBox
3. `src/ui/components/ClickableRow.cpp` (2 处), `TagPill.cpp` (2 处), `MetaInfoSection.cpp` (5 处), `MetaTagSection.cpp` (2 处)

---

## 3. 归一化实施步骤

1. **设置规范 ID 选择器**：在 C++ 对话框与浮层初始化时，通过 `setObjectName(...)` 给各部件分配规范选择器名。
2. **提取并扩充 `resources/style.qss`**：将各 Dialog、Overlay 及 Component 的 130+ 处静态样式统一整理并存入 `resources/style.qss`。
3. **C++ 代码剥离**：彻底清除 C++ 中的硬编码内联 `setStyleSheet(...)` 字符串。

---

## 4. 验证计划
1. 编译运行与交互测试：打开标签管理器、调色盘、冲突对话框、Space 快捷预览及撤销 Toast，确保外观、圆角、暗色风格完全符合规范。
2. 执行 Pre-commit 检查与内存记录。
