# Implementation Plan: 侧边栏“标记”与“历史”面板选中高亮归一化消除特殊化实施方案

本实施方案旨在严格遵循顶层架构规范与 `Memories.md`（第 24 章），在 `QuarkMeta_Source` 桌面应用中消除侧边栏“标记”选项卡（`MarkedPanel`）及历史记录面板的独立特殊样式，实现全局选中高亮 100% 统一。

---

## 一、方案设计与架构分析 (Architecture & Design)

### 1.1 问题现状与根因分析
- 在现有代码中，主内容视图列表（`TreeItemDelegate`）、收藏夹（`FavoritePanel`）、库面板（`LibraryPanel`）、目录导航（`NavPanel`）以及分栏视图中，选中项的高亮背景均严格遵循设计规范：
  ```cpp
  QColor selBg("#378ADD");
  selBg.setAlphaF(0.15f); // 即 rgba(55, 138, 221, 0.15)
  ```
- 然而在 `resources/style.qss` 中，针对 `MarkedTreeWidget`（标记面板）与 `HistoryTreeWidget`（历史记录面板）单独编写了特殊的高亮规则：
  ```qss
  QTreeWidget#HistoryTreeWidget::item:selected, QTreeWidget#MarkedTreeWidget::item:selected {
      background-color: #094771;
      color: #FFFFFF;
  }
  ```
- 这种高饱和度、不透明的 `#094771` 深蓝色块造成了严重的视觉割裂，违背了“禁止搞特殊、禁止另起炉灶”的全局设计原则。

### 1.2 归一化重构策略
- 将 `resources/style.qss` 中 `MarkedTreeWidget` 与 `HistoryTreeWidget` 的 `::item:selected` 背景色直接调整为统一的 `rgba(55, 138, 221, 0.15)`；
- 彻底消除不协调的硬深蓝底色，使侧边栏所有面板的选中视觉表现与全系统完全一致。

---

## 二、受影响文件清单 (Affected Files)

1. `QuarkMeta_Source/resources/style.qss`

---

## 三、精准代码修改方案 (Precise Code Changes)

### 3.1 `QuarkMeta_Source/resources/style.qss`

<<<<<<< SEARCH
QTreeWidget#HistoryTreeWidget::item:selected, QTreeWidget#MarkedTreeWidget::item:selected {
    background-color: #094771;
    color: #FFFFFF;
}
=======
QTreeWidget#HistoryTreeWidget::item:selected, QTreeWidget#MarkedTreeWidget::item:selected {
    background-color: rgba(55, 138, 221, 0.15);
    color: #FFFFFF;
}
>>>>>>> REPLACE

---

## 四、自检核验与无偏执行承诺 (Self-Audit)
1. 方案完全独立物理隔离，针对“标记选项卡选中高亮特殊化”做单点归一化；
2. SEARCH 块在 `resources/style.qss` 中 100% 存在且唯一匹配；
3. 不破坏其他任何 QSS 选择器和组件。
