# NavPanel-3.md Implementation Plan: 修复“回收站”高亮方块撕裂与点击联动劫持缺陷

## 1. 架构原则评估 (3 Questions Check)

### 1.1 单一真理源 (Single Source of Truth)
- **导航视图布局与选中状态 SSOT**：`NavPanel` 中的 `m_treeView`（`NavTreeView`）与 `TreeItemDelegate` 掌控第一栏导航面板的绘制与选中行为。
- **导航历史记录 SSOT**：`NavigationHistoryService` 掌控全局历史纪录。`trash://` 作为虚拟协议路径，不应作为常规历史记录影响导航树节点的焦点展开状态。

### 1.2 封装完整性 (Encapsulation Integrity)
- 修改限制在 `NavPanel` 与 `resources/style.qss` 范围内，保持 `NavPanel` 的公共 API（信号 `requestOpenTrash()`、`directorySelected(...)` 等）完全不变。

### 1.3 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **现象 1：点击“回收站”触发/被抢占回“最近访问”**
  - **根因**：`NavPanel::updateRecentVisitedList()` 末端存在 `m_treeView->expand(m_recentRootItem->index());`。当点击“回收站”导航至 `trash://` 时，`NavigationHistoryService::historyChanged` 信号被触发，从而同步调用 `updateRecentVisitedList()`。每次调用时强制调用 `expand(...)` 夺取了视图的焦点与重绘焦点状态。
- **现象 2：左侧多出突兀高亮方块格子**
  - **根因**：`QTreeView` 把行划分为 `branch`（展开折叠箭头/缩进区）与 `item`（内容区）。`QSS` 的 `QTreeView::item:selected` 仅填充 `item` 矩形，导致 `branch` 区域留白；此外 `TreeItemDelegate::paint()` 中的全矩形绘制与 `branch` 不贯通。而在 `NavPanel::initUi()` 中，没有为 `m_treeView` 显式开启整行选择模式 `setSelectionBehavior(QAbstractItemView::SelectRows)` 导致渲染撕裂。

---

## 2. 详细实现方案

### 2.1 修复焦点劫持与无效历史（`src/ui/NavPanel.cpp`）
1. 在 `updateRecentVisitedList()` 中：
   - 过滤 `trash` 开头的路径（如 `trash://`）；
   - 彻底移除末尾的强行夺焦代码 `m_treeView->expand(m_recentRootItem->index());`。
2. 仅在 `deferredInit()` 初始化“最近访问”节点时，展开一次 `m_recentRootItem`。

### 2.2 修复渲染区域错位与高亮方块撕裂（`src/ui/NavPanel.cpp` & `resources/style.qss`）
1. 在 `NavPanel::initUi()` 中：
   - 显式为 `m_treeView` 设置 `m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);`，确保整行在逻辑与绘制层面统一选中。
2. 在 `resources/style.qss` 中：
   - 确认并优化 `#NavTreeView` 的 `QTreeView::branch` 选 zone 背景处理，使 `branch` 区域在选中/悬停时与 `item` 区域一体化填充。

---

## 3. 验证与测试计划

1. **焦点控制校验**：
   - 点击“回收站”节点，确认视图焦点与选中状态稳定保持在“回收站”上，不再被“最近访问”抢占。
2. **视觉连贯性校验**：
   - 观察“回收站”及其他没有子节点的行被选中时的背景，确认左侧 `branch` 区域与右侧 `item` 区域高亮完全连贯贯通，无方块切割缝隙。

---

## 4. 检查与合规声明
- 冻结公开 API：`NavPanel` 信号与公共接口无变动。
- 单一真理源：遵循 `Guide & Preference.md` 规范。
