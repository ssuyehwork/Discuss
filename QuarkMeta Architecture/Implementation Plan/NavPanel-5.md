# NavPanel-5.md Implementation Plan: 将 QTreeView::branch 彻底设为透明，消除 20px 分离式蓝色方框

## 1. 架构原则评估 (3 Questions Check)

### 1.1 单一真理源 (Single Source of Truth)
- **QSS 样式 SSOT**：`resources/style.qss` 掌控全局 QSS 样式。`QTreeView::branch` 作为 QSS 的独立子控件（Sub-control），其背景设置影响树节点的左侧缩进区。

### 1.2 封装完整性 (Encapsulation Integrity)
- 修改仅限于 `resources/style.qss`，无需改动任何 C++ 代码结构或公共 API。

### 1.3 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **现象**：在“回收站”等节点左侧始终存在一个突兀的 20px 蓝色方块。
- **根因**：
  在 QSS 中给 `QTreeView::branch:selected` 和 `QTreeView::branch:hover` 设置 `background-color` 时，Qt 的 QSS 渲染引擎会专门为 20px 宽的 `branch` 子控件独立绘制一个 20px×28px 的填充矩形框。正是这个独立的 QSS 子控件背景填充，造成了左侧突兀的 20px 蓝色方框！
  **正解**：`QTreeView::branch` 的背景必须全局保持 `background: transparent;`（包括 `:selected` 和 `:hover` 伪状态），禁止给 `branch` 子控件设置背景色。

---

## 2. 详细实现方案

### 2.1 修改 QSS 样式（`resources/style.qss`）
清理 `QTreeView::branch:selected` 和 `QTreeView::branch:hover` 的背景色设置，确保 `QTreeView::branch` 在所有伪状态下背景完全透明：

```qss
QTreeView::branch {
    background: transparent;
}
QTreeView::branch:selected {
    background: transparent;
}
QTreeView::branch:hover {
    background: transparent;
}
```

---

## 3. 验证与测试计划

1. **视觉验证**：
   - 选中“回收站”或任意树节点，确认左侧 20px `branch` 缩进区域不再绘制任何独立的蓝色方块，整体视觉干净统一。

---

## 4. 检查与合规声明
- 严禁 C++ 内联 `setStyleSheet`；
- 遵循 `Guide & Preference.md` 规范。
