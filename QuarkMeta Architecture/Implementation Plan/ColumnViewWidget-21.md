# ColumnViewWidget-21.md Implementation Plan

## Overview
本实施方案旨在彻底纠正列视图 (`ColumnViewWidget` / `ColumnViewPane`) 中单列宽度被随手脑补为 `240px` 与 `220px` 的架构割裂问题。

### 根因与修复策略
1. **SSOT 基准归一化**：
   全系统所有栏区（`NavPanel`、`FavoritePanel`、`FilterPanel`、`MetaPanel`、`ContentPanel`）的唯一物理基准宽度均锁定在 `PanelLayoutManager::kBasePanelWidth = 230px`（以及 `resources/style.qss:47` 中 *“确保面板精确呈现 230px 物理满额宽度”*）。
2. **彻底消灭 240px 与 220px 脑补硬编码**：
   - 引入 `PanelLayoutManager.h` 权威常量；
   - 在 `ColumnViewPane` 构造函数中，将初始化下限从 `220px` 修正为 `PanelLayoutManager::kBasePanelWidth` (`230px`)；
   - 在 `ColumnViewWidget::updatePaneWidths()` 中，将生效列宽 `defaultWidth` 从 `240px` 修正为 `PanelLayoutManager::kBasePanelWidth` (`230px`)；
   - 保持列视图与应用全系统五大实体栏区的尺寸规范 100% 绝对一致。

---

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`
```diff
<<<<<<< SEARCH
#include "ColumnItemDelegate.h"
#include "UiHelper.h"
=======
#include "ColumnItemDelegate.h"
#include "PanelLayoutManager.h"
#include "UiHelper.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    setObjectName("ColumnViewPane");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(220);
=======
    setObjectName("ColumnViewPane");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(PanelLayoutManager::kBasePanelWidth);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 240;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}
=======
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = PanelLayoutManager::kBasePanelWidth;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. 运行构建：
   ```powershell
   cmake --build build --config Release
   ```
2. 启动应用并切换至列视图（分栏视图模式）。
3. 检查并测量列视图中每一列的物理宽度，确认其与左侧目录导航 (`230px`) 完全一致，无任何 10px 突兀错位。
