# Implementation Plan: ContentPanel & ColumnViewWidget Fixes

## 1. Overview
This implementation plan addresses two critical issues in QuarkMeta's view mode management and column view layout:
1. **View Mode Desynchronization ("False Highlight")**: When entering Column View and triggering folder navigation or data refresh, `ContentPanel::restoreActiveView()` previously fell back to `m_gridView` because it only handled `ListView` vs `GridView`. Consequently, `m_currentViewMode` remained `ViewModeColumn` (keeping the status bar Column View button checked), while the content stack displayed Grid View.
2. **Column Width Excessive Stretching**: `ColumnViewWidget::updatePaneWidths()` forced the last column to stretch across all remaining viewport width when total column width was less than window width. For a single column, this caused extreme horizontal stretching across the window.

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
void ContentPanel::restoreActiveView() {
    m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? static_cast<QWidget*>(m_treeView) : static_cast<QWidget*>(m_gridView));
}
=======
void ContentPanel::restoreActiveView() {
    if (m_currentViewMode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (m_currentViewMode == ViewModeColumn) {
        if (m_columnView) {
            m_columnView->setRootPath(m_currentPath);
        }
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        m_viewStack->setCurrentWidget(m_gridView);
    }
}
>>>>>>> REPLACE
```

---

### `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int availableWidth = width();
    if (availableWidth <= 0) availableWidth = 800;

    int colCount = m_panes.size();
    int defaultWidth = 230;

    if (colCount * defaultWidth < availableWidth) {
        // 列数少时，最后一列铺满剩余宽度，消灭右侧巨幅黑色空白死区
        for (int i = 0; i < colCount - 1; ++i) {
            m_panes[i]->setFixedWidth(defaultWidth);
        }
        m_panes.last()->setMinimumWidth(availableWidth - (colCount - 1) * defaultWidth - 4);
        m_panes.last()->setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        for (auto* pane : m_panes) {
            pane->setFixedWidth(defaultWidth);
        }
    }
}
=======
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 240;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build the target with CMake:
   ```bash
   cmake --build build --config Debug
   ```
2. Run QuarkMeta application and toggle to Column View via status bar.
3. Verify that navigating folders inside Column View keeps Column View active in `ContentPanel` and status bar highlight matches the active view mode.
4. Verify that single or few columns render at a fixed standard width (240px) without stretching across the whole window width.
