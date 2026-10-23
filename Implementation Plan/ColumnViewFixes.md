# Implementation Plan - ColumnView AddressBar Sync, Focus Outline Removal, and Thumbnail Pipeline Fix

## Overview
Fix three specific Column View issues:
1. Synchronize AddressBar (and NavPanel) when navigating in Column View by emitting `directorySelected(path)` while protecting Column View from full-stack resets.
2. Remove ugly dash-line focus rectangle (`QStyle::State_HasFocus`) and add `outline: none;` to `QListView` stylesheet.
3. Trigger thumbnail/icon extraction pipeline in `ColumnViewPane::loadDirectory()` after `DiskItemModel` records are populated.

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/TreeItemDelegate.h`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.cpp`
- In `ColumnViewPane::loadDirectory()` callback, invoke `m_model->loadThumbnailsForRows(...)` for all rows after setting records.
- In `ColumnViewPane` stylesheet, ensure `QListView { outline: none; ... }` is present.

### 2. `src/ui/TreeItemDelegate.h`
- Mask out `QStyle::State_HasFocus` from `option.state` inside `paint()`:
  ```cpp
  QStyleOptionViewItem opt = option;
  opt.state &= ~QStyle::State_HasFocus;
  ```

### 3. `src/ui/ContentPanel.cpp`
- In `initUi()`'s `pathNavigated` connection, emit `directorySelected(path)` so `AddressBar` and `NavPanel` update.
- In `loadDirectory()`, check if `m_currentViewMode == ViewModeColumn`. If true, update `m_currentPath` without invoking `m_columnView->setRootPath(path)` if `m_columnView` is already showing the path stack.

## Build & Verification Steps
1. Recompile project:
   `cmake -B build -S . && cmake --build build --config Release`
2. Run application and switch to Column View.
3. Expand directories and verify AddressBar (`G:\...`) updates continuously.
4. Verify no dotted/dashed focus rectangle appears on item text selection.
5. Verify file icons and thumbnails load for files in Column View.
