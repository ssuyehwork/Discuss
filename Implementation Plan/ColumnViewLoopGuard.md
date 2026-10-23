# Implementation Plan - ColumnView Guard Loop Protection

## Overview
Add idempotent path check (`if (m_currentPath != path)`) in `ContentPanel::loadDirectory()` when running in `ViewModeColumn`. This breaks the signal feedback loop (`pathNavigated` -> `directorySelected` -> `loadDirectory` -> `setRootPath`) that was causing continuous column destruction, reloading, and screen flickering.

## Modified Files List
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`
```diff
<<<<<<< SEARCH
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ViewModeColumn && m_columnView) {
        m_currentPath = path;
        m_columnView->setRootPath(path);
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ViewModeColumn && m_columnView) {
        if (m_currentPath != path) {
            m_currentPath = path;
            m_columnView->setRootPath(path);
        }
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile project:
   `cmake -B build -S . && cmake --build build --config Release`
2. Run application and switch to Column View.
3. Click through multiple folders and verify no screen flickering, reloading loop, or continuous column resets occur.
