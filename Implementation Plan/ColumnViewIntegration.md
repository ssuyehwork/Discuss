# Implementation Plan - ColumnView Integration into ContentPanel Controller Architecture

## Overview
Integrate `ColumnViewWidget` and `ColumnViewPane` fully into `ContentPanel`'s unified controller, selection model, context menu, filter state, and shortcut handling ecosystem. This removes the "isolated widget" defect and makes Column View behave consistently with Grid View and List View.

## Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.h` & `src/ui/ColumnViewWidget.cpp`
- Expose `ColumnViewPane::listView()` and `ColumnViewWidget::activePane()` / `ColumnViewWidget::selectedPaths()`.
- Pass `ContentPanel*` into `ColumnViewPane` constructor to install event filters on `m_listView` and `m_listView->viewport()`.
- Connect `customContextMenuRequested` on `QListView` to `ContentPanel::onCustomContextMenuRequested`.
- Apply `FilterState` from `ContentPanel` to `ColumnViewPane`'s `FilterProxyModel`.
- Configure `TreeItemDelegate` roles (`HasThumbnailRole`, `RatingRole`, `ColorRole`, `PathRole`, `TypeRole`).

### 2. `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`
- Update `ContentPanel::getSelectedIndexes()` and `getSelectedPaths()` to handle `ViewModeColumn`.
- Ensure `applyFilters()` propagates `m_currentFilter` down to `m_columnView`.
- Connect selection change signals from `ColumnViewWidget` to `ContentPanel::onSelectionChanged`.

## Build & Verification Steps
1. Recompile project:
   `cmake -B build -S . && cmake --build build --config Release`
2. Run application and switch to Column View mode.
3. Test right-click context menu on items in Column View.
4. Test shortcut keys (`F2` rename, `Delete`, `Ctrl+C`, `Space` QuickLook preview).
5. Test keyword search and filter toggles (show hidden files) in Column View.
