# Implementation Plan - ColumnView Single Column Incremental Expansion Fix

## Overview
When double-clicking a folder in Column View mode to expand a subfolder into a new column (`handleFolderExpand`), `ColumnViewWidget::appendColumn` appends the new column and invokes `pane->loadDirectory()` locally for that new column. However, `appendColumn` subsequently emitted `pathNavigated(folderPath)`, which propagated through `NavigationService::navigateTo` and `NavigationService::currentUrlChanged` back to `ContentPanel::loadDirectory(url)`.

In `ContentPanel::loadDirectory`, because `m_columnView->containsPath(path)` evaluated to `true`, it invoked `m_columnView->refreshAllColumns()`. This triggered a redundant full-column reload feedback loop where EVERY existing parent column in the column stack was re-scanned from disk and re-decorated, causing significant disk I/O overhead and visual flickering.

### Solution
Remove `m_columnView->refreshAllColumns()` from the `containsPath(path)` branch in `ContentPanel::loadDirectory`. When navigating to a path that is already present in Column View (such as a newly appended subfolder column), the newly appended column handles its own local directory load, preventing the redundant re-scanning of all parent columns in the stack.

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
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        m_currentPath = path;
        setIsRecursive(recursive);
        if (m_columnView) {
            if (!m_columnView->containsPath(path)) {
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
3. Double-click a subfolder in the leftmost column.
4. Verify that only the newly created right column performs a directory scan, while all existing parent columns remain intact without re-scanning or reloading.
5. Verify that the address bar, navigation sidebar, and status bar update correctly to the new folder path.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- `ColumnViewPane::loadDirectory()` remains the sole SSOT for loading individual column directory items asynchronously.
- `ContentPanel::loadDirectory()` acts as the SSOT for top-level view navigation, correctly skipping redundant full-stack reloads when columns are incrementally appended.

---

## Header API Signature Verification Table

| Header File | Member Function / Type | Exact Physical Signature in `.h` | Verified Existing |
| :--- | :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `ContentPanel::loadDirectory` | `void loadDirectory(const QString& path, bool recursive = false);` | Yes |
| `src/ui/ColumnViewWidget.h` | `ColumnViewWidget::containsPath` | `bool containsPath(const QString& path) const;` | Yes |
