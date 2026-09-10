# Implementation Plan: Prevent Column View Reload Loop on Internal Folder Double-Click

## 1. Overview
When double-clicking a folder inside `ColumnViewWidget`, `ColumnViewWidget` emits `pathNavigated`, which `ContentPanel` forwarded via `directorySelected`. Global navigation (`NavigationService` / `PanelMediator`) responded by calling `navigateTo()`, which triggered `ContentPanel::loadDirectory()`. `loadDirectory()` in turn called `setRootPath()`, destroying all existing column panes and rebuilding them from scratch (causing a full view reset/refresh).

To fix this:
In `ContentPanel.cpp`, when `pathNavigated` is emitted by `m_columnView`:
1. If the target is a directory, update `m_currentPath` internally and update address bar/status bar, but do **NOT** invoke `loadDirectory()` / `setRootPath()` to clear the active column panes.

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    m_columnView = new ColumnViewWidget(this);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        if (QFileInfo(path).isDir()) {
            emit directorySelected(path);
        } else {
            emit fileActivated(path);
        }
    });
=======
    m_columnView = new ColumnViewWidget(this);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        if (QFileInfo(path).isDir()) {
            m_currentPath = path;
            emit selectionChanged({path});
            updateStatusBarStats();
        } else {
            emit fileActivated(path);
        }
    });
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Recompile target with CMake.
2. In Column View, double click a folder inside any column.
3. Verify that a new column pane opens to the right showing the folder contents, WITHOUT clearing existing columns or triggering a full view reload reset.
