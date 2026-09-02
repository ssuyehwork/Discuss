# NavPanel-6.md Implementation Plan: 彻底消灭高亮“一长一短”切断感，实现整行（x=0 至最右侧）100% 贯通高亮

## 1. 架构原则评估 (3 Questions Check)

### 1.1 单一真理源 (Single Source of Truth)
- **整行高亮背景色 SSOT**：
  - 选中态底色：`rgba(55, 138, 221, 0.15)`
  - 悬停态底色：`#2A2D2E`
  - 常态/斑马纹底色：`#1E1E1E` / `#252526`

### 1.2 封装完整性 (Encapsulation Integrity)
- 修改收敛在 `resources/style.qss` 与 `TreeItemDelegate.h` 的自绘底色协调，不影响任何 C++ 控件接口。

### 1.3 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **现象**：在最新的截图（例如“最近访问”展开节点或“回收站”节点）中，选中高亮条从左侧 20px 缩进箭头（`v` 或 `>`）之后才开始，导致高亮条左侧缺了一块，呈现出“一长一短”的被切断感。
- **根因**：
  1. `QTreeView` 的物理矩形被拆分为 **左侧 `branch` 区域**（展开箭头/缩进）与 **右侧 `item` 区域**（图标和文字）；
  2. `TreeItemDelegate::paint()` 中的 `option.rect` 仅覆盖 **右侧 `item` 区域**，`TreeItemDelegate` 在 `item` 上自绘了 `rgba(55, 138, 221, 0.15)` 的蓝色底色；
  3. 当此前把 `QTreeView::branch:selected` 设为 `background: transparent;` 时，左侧 `branch` 区域没有绘制任何蓝色底色（保持暗色 `#1E1E1E`），而右侧 `item` 区域绘制了蓝色底色，**这就直接导致了左侧缩进区被切断、高亮条“一长一短”的缺陷**；
  4. 此外，如果 QSS 中 `QTreeView::item:selected` 也自带背景色，就会与 `TreeItemDelegate` 在 `item` 区域进行叠加双重绘制（导致 `item` 与 `branch` 颜色浓度不一致）。
- **正解**：
  - 让 `QTreeView::branch:selected` 在 QSS 中精准填充 `rgba(55, 138, 221, 0.15)`；
  - 让 `QTreeView::branch:hover` 在 QSS 中精准填充 `#2A2D2E`；
  - 将 `QTreeView::item:selected` 的 QSS 背景设为 `transparent`，避免与 `TreeItemDelegate` 在 `item` 上的自绘发生叠加；
  - 这样，`branch` 区域（由 QSS 绘制 `rgba(55, 138, 221, 0.15)`）与 `item` 区域（由 `TreeItemDelegate` 绘制 `rgba(55, 138, 221, 0.15)`）在相同的 `#1E1E1E` 底板上叠加，**渲染出 100% 物理色值完全一致、从最左侧边缘到最右侧边缘无缝贯通的高亮条**！

---

## 2. 详细实现方案

### 2.1 修改 `resources/style.qss` 完美对齐色值
```qss
QTreeView::branch {
    background: transparent;
}
QTreeView::branch:hover {
    background-color: #2A2D2E;
}
QTreeView::branch:selected {
    background-color: rgba(55, 138, 221, 0.15);
}
QTreeView::branch:has-children:closed {
    image: url(:/Icon/arrow_right.svg);
}
QTreeView::branch:has-children:open {
    image: url(:/Icon/arrow_down.svg);
}
QTreeView::item {
    height: 28px;
    border: none;
}
QTreeView::item:alternate {
    background-color: #252526;
}
QTreeView::item:hover {
    background-color: #2A2D2E;
}
QTreeView::item:selected {
    background-color: transparent;
    color: #FFFFFF;
}
```

---

## 3. 验证与测试计划

1. **整行贯通校验**：
   - 选中包含子节点的“最近访问”节点，观察展开箭头 `v` 左侧矩形与右侧“最近访问”文本矩形，确认蓝色高亮底色从最左边缘无缝贯通至最右侧，没有任何“一长一短”切断缺口。
2. **普通节点贯通校验**：
   - 选中“回收站”节点，确认左侧 20px 缩进区域与右侧图标文本区域的高亮底色完全一致、无缝连贯。

---

## 4. 检查与合规声明
- 严禁 C++ 内联 `setStyleSheet`；
- 遵循 `Guide & Preference.md` 与 QSS 架构规范。
