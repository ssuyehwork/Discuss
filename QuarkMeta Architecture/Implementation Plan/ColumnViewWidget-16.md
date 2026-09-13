# Implementation Plan - ColumnViewWidget-16: Ancestor Path Cascade & Icon Pipeline Integration

## 1. Overview
This implementation plan restores the `Version-6` Miller Columns ancestor path unpacking, cascade selection highlighting, and global system icon/thumbnail pipeline integration in `ColumnViewWidget`, `ColumnViewPane`, and `ColumnItemDelegate`.

Key objectives:
- **Ancestor Path Cascade**: Unpack target path into its full parent directory stack upon `setRootPath`, dynamically building cascade columns and selecting/highlighting parent folders.
- **Selection Highlight Preservation**: Modify column selection handling to only clear sub-columns (right side), maintaining blue selection highlights for all ancestor parent columns on the left.
- **Icon Pipeline & Async Selection**: Trigger `loadThumbnailsForRows` on `DiskItemModel` after directory scan completes, and support deferred pending selection (`tryPendingSelection`) when column data finishes loading asynchronously.
- **System Icon & Thumbnail Rendering**: Support `Qt::DecorationRole` (`QIcon` / `QPixmap`) rendering inside `ColumnItemDelegate` to display system file association icons and image thumbnails uniformly.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ColumnItemDelegate.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ColumnViewWidget.h`
Add `m_pendingSelectPath`, `tryPendingSelection()`, `recordsLoaded` signal to `ColumnViewPane`, and update `ColumnViewWidget` methods for path stack unpacking.

### 3.2 `src/ui/ColumnViewWidget.cpp`
Implement `setRootPath` path stack decomposition, `dismissSubColumns` index-based pruning, `clearOtherSelections` scope limit, and `loadThumbnailsForRows` invocation.

### 3.3 `src/ui/ColumnItemDelegate.cpp`
Implement `Qt::DecorationRole` rendering for `QIcon` and `QPixmap` inside `ColumnItemDelegate::paint`.

## 4. Build & Verification Steps
1. Build the project using CMake.
2. Launch `QuarkMeta` and navigate to a deeply nested folder via FavoritePanel.
3. Verify that all parent columns are expanded and highlighted in blue.
4. Verify that icons and thumbnails load asynchronously and match the list/grid views.
