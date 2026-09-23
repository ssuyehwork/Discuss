# Implementation Plan - ColumnViewWidget Single-Click Decoupling & Selection Preservation

## Overview
This implementation plan resolves two major architectural defects in Column View (`ColumnViewWidget`):
1. **Selection Eradication Defect**: Single-clicking or clicking blank canvas in a column previously invoked `clearOtherSelections(paneIdx)`, forcibly erasing active item selections in all rightward child columns.
2. **Spurious Navigation & Filter Reset Defect**: Single-clicking an item or clicking blank space in a column previously emitted `pathNavigated(pane->currentPath())`. `PanelMediator` handled `pathNavigated` by resetting unpinned filters on `FilterPanel` (`clearAllFilters(false)`) and triggering global `NavigationService::navigateTo`, mistaking local item selection for directory navigation.

### Solution
- Remove `clearOtherSelections(paneIdx)` calls from single-click handlers (`folderClicked`, `fileClicked`) and blank space click handler (`activatePaneFromBlankClick`), preserving selections across child columns.
- Remove `emit pathNavigated(...)` from single-click handlers (`folderClicked`, `fileClicked`, `fileSelected`) and blank space click handler (`activatePaneFromBlankClick`).
- Retain `pathNavigated` emission exclusively during explicit folder expansion (`handleFolderExpand`) triggered by double-clicking or pressing Enter/Right keys.

---

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
void ColumnViewWidget::activatePaneFromBlankClick(int paneIndex) {
    if (paneIndex >= 0 && paneIndex < m_panes.size()) {
        setActivePaneIndex(paneIndex);
        ColumnViewPane* pane = m_panes[paneIndex];
        if (pane) {
            pane->clearSelection();
            clearOtherSelections(paneIndex);
            focusPane(paneIndex);
            emit selectionChanged();
            emit pathNavigated(pane->currentPath());
        }
    }
}
=======
void ColumnViewWidget::activatePaneFromBlankClick(int paneIndex) {
    if (paneIndex >= 0 && paneIndex < m_panes.size()) {
        setActivePaneIndex(paneIndex);
        ColumnViewPane* pane = m_panes[paneIndex];
        if (pane) {
            pane->clearSelection();
            focusPane(paneIndex);
            emit selectionChanged();
        }
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::folderClicked, this, [this, pane](const QString&, int paneIdx) {
        setActivePaneIndex(paneIdx);
        clearOtherSelections(paneIdx);
        emit selectionChanged();
        if (pane) {
            emit pathNavigated(pane->currentPath());
        }
    });

    connect(pane, &ColumnViewPane::fileClicked, this, [this, pane](const QString&, int paneIdx) {
        setActivePaneIndex(paneIdx);
        clearOtherSelections(paneIdx);
        emit selectionChanged();
        if (pane) {
            emit pathNavigated(pane->currentPath());
        }
    });
=======
    connect(pane, &ColumnViewPane::folderClicked, this, [this](const QString&, int paneIdx) {
        setActivePaneIndex(paneIdx);
        emit selectionChanged();
    });

    connect(pane, &ColumnViewPane::fileClicked, this, [this](const QString&, int paneIdx) {
        setActivePaneIndex(paneIdx);
        emit selectionChanged();
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString& filePath, int paneIdx) {
        setActivePaneIndex(paneIdx);
        emit selectionChanged();
        emit pathNavigated(filePath);
    });
=======
    connect(pane, &ColumnViewPane::fileSelected, this, [this](const QString&, int paneIdx) {
        setActivePaneIndex(paneIdx);
        emit selectionChanged();
    });
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Clean and rebuild the project using CMake.
2. Open Column View mode.
3. Single-click folders to expand child columns.
4. Select a file in the rightmost column, then single-click an item in the parent column. Verify that the selection in the rightmost column is NOT erased.
5. Set unpinned filter rules in `FilterPanel`, then single-click items in Column View. Verify that unpinned filters are NOT reset.
6. Double-click a folder in Column View. Verify that a new column is expanded and `pathNavigated` is emitted to update the address bar and reset unpinned filters.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- `setActivePaneIndex(paneIdx)` is reused as SSOT for active column focus and visual header highlight.
- `selectionChanged()` is reused as SSOT for broadcasting selection state to `MetaPanel` and status bars.
- `pathNavigated` remains the sole SSOT for physical folder expansion navigation, decoupled from local single clicks.

---

## Header API Signature Verification Table

| Header File | Member Function / Type | Exact Physical Signature in `.h` | Verified Existing |
| :--- | :--- | :--- | :--- |
| `src/ui/ColumnViewWidget.h` | `ColumnViewWidget::setActivePaneIndex` | `void setActivePaneIndex(int newIndex);` | Yes |
| `src/ui/ColumnViewWidget.h` | `ColumnViewWidget::activatePaneFromBlankClick` | `void activatePaneFromBlankClick(int paneIndex);` | Yes |
| `src/ui/ColumnViewWidget.h` | `ColumnViewWidget::selectionChanged` | `void selectionChanged();` (signal) | Yes |
| `src/ui/ColumnViewWidget.h` | `ColumnViewWidget::pathNavigated` | `void pathNavigated(const QString& path);` (signal) | Yes |
| `src/ui/ColumnViewWidget.h` | `ColumnViewPane::clearSelection` | `void clearSelection();` | Yes |
