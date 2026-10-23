# Implementation Plan: ColumnView Rightmost Column Recursive Scanning Support

## Overview
This implementation plan addresses the requirement for enabling recursive subfolder scanning ("显示子文件夹中的项目") in ColumnView (Miller Columns) mode. To preserve the hierarchical cascade navigation of intermediate columns while allowing deep file browsing, recursive scanning is applied **exclusively to the rightmost column (`rightmostPane()`)**. When toggling the recursive button, only the last column is reloaded in-place with flattened subfolder items.

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.cpp`
In `ColumnViewPane::loadDirectory()`, dynamically check if `m_contentPanel->isRecursive()` is enabled and whether `this` pane is the `rightmostPane()` of the ColumnViewWidget. If so, pass `recursive = true` to `DiskScanService::scanDirectory`.

```
<<<<<<< SEARCH
        if (path.isEmpty() || path == "computer://") {
            for (const QFileInfo& drive : QDir::drives()) {
                items.push_back(ItemRecord::create(drive.absolutePath()));
            }
        } else {
            items = DiskScanService::scanDirectory(path, false, std::function<bool()>());
        }
=======
        if (path.isEmpty() || path == "computer://") {
            for (const QFileInfo& drive : QDir::drives()) {
                items.push_back(ItemRecord::create(drive.absolutePath()));
            }
        } else {
            bool recursive = false;
            if (weakSelf && weakSelf->m_contentPanel && weakSelf->m_contentPanel->isRecursive()) {
                if (weakSelf->m_contentPanel->columnView() &&
                    weakSelf->m_contentPanel->columnView()->rightmostPane() == weakSelf.data()) {
                    recursive = true;
                }
            }
            items = DiskScanService::scanDirectory(path, recursive, std::function<bool()>());
        }
>>>>>>> REPLACE
```

### 2. `src/ui/ContentPanel.cpp`
In the `recursiveToggled` signal handler in `ContentPanel::initHeaderWidget` / `initUI`, when in `ColumnView` mode, trigger `loadDirectory()` on `m_columnView->rightmostPane()` to refresh only the last column in-place.

```
<<<<<<< SEARCH
    connect(m_headerWidget, &ContentHeaderWidget::recursiveToggled, this, [this](bool recursive) {
        if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
            if (m_headerWidget) m_headerWidget->setRecursive(false);
            return;
        }
        m_isRecursive = recursive;
        loadDirectory(m_currentPath, recursive);
    });
=======
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
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Compilation check command:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel
   ```
2. Functional verification steps:
   - Switch to ColumnView mode and expand folders up to column 5.
   - Click "显示子文件夹中的项目" toggle button on the header toolbar.
   - Confirm that columns 1 to 4 remain standard single-folder cascade lists, while column 5 loads and flattens all nested items recursively.
   - Toggle off the button and confirm column 5 reverts to single-folder list.

## SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reused `m_contentPanel->isRecursive()` (SSOT state) and `DiskScanService::scanDirectory(path, recursive, ...)` instead of creating custom directory recursive scanners.
- **Anti-Redundancy**: Reused `m_columnView->rightmostPane()->loadDirectory()` to refresh only the target column without destroying the column stack.

## Header API Signature Verification
- `ContentPanel::isRecursive()`: Defined in `src/ui/ContentPanel.h` line 71: `bool isRecursive() const`
- `ContentPanel::columnView()`: Defined in `src/ui/ContentPanel.h` line 116: `ColumnViewWidget* columnView() const`
- `ColumnViewWidget::rightmostPane()`: Defined in `src/ui/ColumnViewWidget.h` line 63: `ColumnViewPane* rightmostPane() const`
- `DiskScanService::scanDirectory(path, recursive, cancelCheck)`: Defined in `src/core/DiskScanService.h` line 19: `static std::vector<ItemRecord> scanDirectory(const QString& path, bool recursive, const std::function<bool()>& shouldContinue)`
