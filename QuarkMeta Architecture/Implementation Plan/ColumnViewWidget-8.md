# Implementation Plan - ColumnView Focus-Driven Recursive Scanning Refactoring

## Overview
Previously, when toggling the "Show items in subfolders" button (`m_btnLayers`) in Column View mode, the recursive scan was hardcoded to target `rightmostPane()` (the rightmost column in the stack). This violated the core user experience principle where operation focus and scope must follow the user's active/focused pane (`activePane()`). If the user clicked/focused Column 3, toggling the recursive button ignored Column 3 and performed a recursive scan on Column 4 instead.

### Solution
Refactor recursive subfolder scanning scope in Column View mode from `rightmostPane()` to `activePane()`:
1. In `ContentPanel.cpp`'s `recursiveToggled` lambda, invoke `m_columnView->activePane()->loadDirectory()` instead of `rightmostPane()`.
2. In `ColumnViewPane::loadDirectory()`, check `m_contentPanel->columnView()->activePane() == this` instead of `rightmostPane() == this`.

---

## Modified Files List
- `src/ui/ContentPanel.cpp`
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    connect(m_headerWidget, &ContentHeaderWidget::recursiveToggled, this, [this](bool recursive) {
        if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
            if (m_headerWidget) m_headerWidget->setRecursive(false);
            return;
        }
        m_isRecursive = recursive;
        if (m_currentViewMode == ColumnView) {
            if (m_columnView && m_columnView->rightmostPane()) {
                m_columnView->rightmostPane()->loadDirectory();
            }
        } else {
            loadDirectory(m_currentPath, recursive);
        }
    });
=======
    connect(m_headerWidget, &ContentHeaderWidget::recursiveToggled, this, [this](bool recursive) {
        if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
            if (m_headerWidget) m_headerWidget->setRecursive(false);
            return;
        }
        m_isRecursive = recursive;
        if (m_currentViewMode == ColumnView) {
            if (m_columnView && m_columnView->activePane()) {
                m_columnView->activePane()->loadDirectory();
            }
        } else {
            loadDirectory(m_currentPath, recursive);
        }
    });
>>>>>>> REPLACE
```

### 2. `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
void ColumnViewPane::loadDirectory() {
    QString path = m_path;
    bool recursive = false;
    if (m_contentPanel && m_contentPanel->isRecursive()) {
        if (m_contentPanel->columnView() &&
            m_contentPanel->columnView()->rightmostPane() == this) {
            recursive = true;
        }
    }
=======
void ColumnViewPane::loadDirectory() {
    QString path = m_path;
    bool recursive = false;
    if (m_contentPanel && m_contentPanel->isRecursive()) {
        if (m_contentPanel->columnView() &&
            m_contentPanel->columnView()->activePane() == this) {
            recursive = true;
        }
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Clean and rebuild the project using CMake.
2. Open Column View mode with multiple expanded columns (e.g. 4 columns).
3. Click/focus Column 3 (or any non-rightmost column).
4. Click the "Show items in subfolders" toggle button.
5. Verify that Column 3 (the active column) performs recursive directory scanning on its items, matching the user's active focus.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- `ColumnViewWidget::activePane()` is reused as the single SSOT entry point for obtaining the focused column.

---

## Header API Signature Verification Table

| Header File | Member Function / Type | Exact Physical Signature in `.h` | Verified Existing |
| :--- | :--- | :--- | :--- |
| `src/ui/ColumnViewWidget.h` | `ColumnViewWidget::activePane` | `ColumnViewPane* activePane() const;` | Yes |
| `src/ui/ColumnViewWidget.h` | `ColumnViewPane::loadDirectory` | `void loadDirectory();` | Yes |
