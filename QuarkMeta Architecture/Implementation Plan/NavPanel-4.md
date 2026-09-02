# NavPanel-4.md Implementation Plan: 精确统一 TreeItemDelegate 与 QTreeView::branch 高亮透明度色值

## 1. 架构原则评估 (3 Questions Check)

### 1.1 单一真理源 (Single Source of Truth)
- **选中/悬停色值 SSOT**：全应用主高亮色统一为 `#378ADD`（选中态透明度 `0.15` / `rgba(55, 138, 221, 0.15)`，悬停态底色 `#2A2D2E`）。
- **QSS 与 Delegate 视觉统一**：`TreeItemDelegate`（C++ 绘制层）与 `resources/style.qss`（QSS 样式层）针对 `item` 与 `branch` 必须维持 100% 物理色值一致。

### 1.2 封装完整性 (Encapsulation Integrity)
- 修改仅限于 `resources/style.qss` 中 `#NavTreeView` / `QTreeView` 的 `branch` 与 `item` 色值声明，以及 `TreeItemDelegate.h` 色值映射，不破坏任何 C++ 类或 View 架构协议。

### 1.3 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **现象**：选中“回收站”时，左侧 20px 缩进 `branch` 区域依然呈现出一个比右侧更深、更突兀的蓝色高亮方块。
- **根因**：
  1. `TreeItemDelegate::paint()` 内部针对 `selected` 状态自绘的色值是 `QColor("#378ADD")` 加上 `setAlphaF(0.15f)`（即 **15% 不透明度** `rgba(55, 138, 221, 0.15)`）；
  2. 而 `resources/style.qss` 中的 `QTreeView::branch:selected` 与 `QTreeView::item:selected` 之前写的是 `rgba(55, 138, 221, 0.3)`（即 **30% 不透明度**）；
  3. 左侧 `branch` 区域由 QSS 绘制（30% 蓝），右侧 `item` 区域由 Delegate 绘制（15% 蓝）。30% 与 15% 的浓度差异导致左侧缩进区整整深了一倍，割裂出明显的深蓝色方框！

---

## 2. 详细实现方案

### 2.1 统一 QSS 中的 `branch` / `item` 选中与悬停透明度（`resources/style.qss`）
将 `resources/style.qss` 中 `QTreeView::branch` 及 `QTreeView::item` 的 `selected` 和 `hover` 状态色值调整为与 `TreeItemDelegate` 100% 物理匹配：

```qss
QTreeView::branch:hover {
    background-color: #2A2D2E;
}
QTreeView::branch:selected {
    background-color: rgba(55, 138, 221, 0.15);
}
QTreeView::item:hover {
    background-color: #2A2D2E;
}
QTreeView::item:selected {
    background-color: rgba(55, 138, 221, 0.15);
    color: #FFFFFF;
}
```

---

## 3. 验证与测试计划

1. **色值浓度对比校验**：
   - 选中“回收站”或任意驱动器节点，对比左侧 20px `branch` 区域与右侧 `item` 区域的 RGB / Alpha 颜色，确认左右色彩无缝连接、无任何浓度差或方块撕裂感。
2. **悬停态连贯性校验**：
   - 鼠标悬停在“回收站”上，确认左侧 `branch` 区域与右侧 `item` 区域统一呈现 `#2A2D2E` 灰色底色。

---

## 4. 检查与合规声明
- 严禁 C++ 内联 `setStyleSheet`；
- 色值统一遵循 `Guide & Preference.md` 与全局 QSS 规范。
