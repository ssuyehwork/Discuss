# 批次二：顶层窗口与导航栏内联 `setStyleSheet` 归一化重构方案

## 1. 核心架构问题回答 (3 Questions)

### Q1: 唯一事实源 (Single Source of Truth, SSOT) 是什么？在哪里？
- **静态与组件外观 SSOT**：`resources/style.qss`。顶层窗口标题栏、导航栏、地址栏、面包屑导航、全局搜索框、驱动器盘符栏及状态栏按钮的外观与交互状态，统一在 `style.qss` 中声明。
- **主题控制与属性加载 SSOT**：`src/ui/ThemeManager.h/cpp`。

### Q2: 封装完整性 (Encapsulation Integrity) 如何保证？
- 不修改 `MainWindow`, `AddressBar`, `BreadcrumbBar`, `SearchController` 的公开 API 接口签名。
- C++ 控件统一通过 `setObjectName(...)` 指定唯一选择器名称（如 `#TitleLogoLabel`、`#AppNameLabel`、`#AddressPathEdit`、`#SearchEdit` 等），将内联样式剥离并外联至 `resources/style.qss`。

### Q3: 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **表面现象**：`MainWindow.cpp` 等顶层导航控件中，存在 20+ 处硬编码调用 `setStyleSheet(QString("...").arg(color.name()))`，导致全局配色修改无法通过单一 `style.qss` 统一管辖。
- **根因**：历史代码直接在 C++ 中将主题颜色变量（如 `BrandOrange`、`BorderColor`）格式化注入控件内联样式，未建立使用类名/ObjectName 统一收拢至 QSS 的结构。

---

## 2. 批次二内联 `setStyleSheet` 排查清单 (共 26 处)

1. **`src/ui/MainWindow.cpp` (17 处)**:
   - 窗口控制按钮 `#WinMinBtn`, `#WinMaxBtn`, `#WinCloseBtn`
   - 主窗口中央区域 `#CentralWidget`, `#TitleBar`, `#TitleLogoLabel`, `#AppNameLabel`, `#NavBar`
   - 状态栏标签与视图缩放滑动条 `#StatusBarLeft`, `#SizeSlider`
   - 驱动器盘符工具栏 `#DriveBarWidget` 及标签管理按钮 `#BtnTagManager`
2. **`src/ui/AddressBar.cpp` (4 处)**:
   - 地址栏容器 `#AddressContainer`、堆叠窗口 `#PathStack`、地址输入框 `#AddressPathEdit`、刷新按钮 `#BtnRefreshAddress`
3. **`src/ui/BreadcrumbBar.cpp` (3 处)**:
   - 面包屑节点容器、分隔符 `#BreadcrumbSep`、节点按钮 `#BreadcrumbNodeBtn`
4. **`src/ui/SearchController.cpp` (2 处)**:
   - 搜索框容器 `#SearchContainer`、搜索输入框 `#SearchEdit`

---

## 3. 归一化实施步骤

1. **配置 `setObjectName`**：在批次二文件的构造与 `initUi` 流程中，为上述控件分配标准的 ID。
2. **提取并扩充 `resources/style.qss`**：将 26 处内联样式规则完整收拢并规范写入 `resources/style.qss`。
3. **剥离 C++ 内联硬编码**：清理批次二文件中的 `setStyleSheet(...)` 格式化字符串。

---

## 4. 验证计划
1. 编译运行与 UI 视觉核对：确保标题栏、驱动器栏、缩放滑动条、地址栏及搜索框的高亮 hover/pressed 效果与 `UI_DESIGN_SPEC.md` 完全一致。
2. 执行 Pre-commit 检查与内存记录。
