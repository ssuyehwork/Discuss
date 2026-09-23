# Implementation Plan - ContentPanel Column View Recursive Mode UI State Sync Fix

## Overview
When navigating or switching directories in Column View mode, the "Show items in subfolders" toggle button (`m_btnLayers` in `ContentHeaderWidget`) stayed checked (green highlight `#2ecc71`) instead of resetting to unchecked (gray `#888888`), unlike the other three view modes (List, Grid, Justified).

### Cause
In `ContentPanel::loadDirectory(const QString& path, bool recursive)`, the `ColumnView` branch directly assigned `m_isRecursive = recursive;` bypassing `setIsRecursive(recursive)`. Consequently, `m_headerWidget->setRecursive(recursive)` was never called to uncheck the header button and update its icon color during directory navigation in Column View.

### Solution
Replace `m_isRecursive = recursive;` in `ContentPanel::loadDirectory` with `setIsRecursive(recursive);`. When navigating to a new directory (where `recursive` defaults to `false`), `setIsRecursive(false)` updates `m_isRecursive` and invokes `m_headerWidget->setRecursive(false)`, resetting the toggle button state and icon color across all view modes.

---

## Modified Files List
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        m_currentPath = path;
        m_isRecursive = recursive;
        if (m_columnView) {
            if (m_columnView->containsPath(path)) {
                m_columnView->refreshAllColumns();
            } else {
                m_columnView->setRootPath(path);
                restoreSelections();
            }
        }
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        m_currentPath = path;
        setIsRecursive(recursive);
        if (m_columnView) {
            if (m_columnView->containsPath(path)) {
                m_columnView->refreshAllColumns();
            } else {
                m_columnView->setRootPath(path);
                restoreSelections();
            }
        }
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Clean and rebuild the project using CMake.
2. Open Column View mode.
3. Click the "Show items in subfolders" (`m_btnLayers`) button in `ContentHeaderWidget`. Verify it turns green (`#2ecc71`).
4. Navigate to another folder or double-click a subfolder to expand/open it.
5. Verify that `m_btnLayers` automatically unchecks and returns to gray (`#888888`), matching the behavior of List, Grid, and Justified view modes.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- `ContentPanel::setIsRecursive(bool recursive)` is reused as the single SSOT entry point for setting recursive state and synchronizing `ContentHeaderWidget::setRecursive`.

---

## Header API Signature Verification Table

| Header File | Member Function / Type | Exact Physical Signature in `.h` | Verified Existing |
| :--- | :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `ContentPanel::setIsRecursive` | `void setIsRecursive(bool recursive);` | Yes |
| `src/ui/ContentHeaderWidget.h` | `ContentHeaderWidget::setRecursive` | `void setRecursive(bool recursive);` | Yes |
