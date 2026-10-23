# ColumnViewMetaPanelIntegration Implementation Plan

This implementation plan details the precise changes required to solve the decoupling issue between `ColumnViewWidget` (Miller Columns) and `MetaPanel` / `ContentPanel` metadata update pipeline.

## Overview
When users update metadata (rating, color tags, labels, notes) via `MetaPanel`, `PanelMediator` calls `ContentPanel::updateItemMetadata(path)`. Currently, `ContentPanel::updateItemMetadata` only updates the main model (`m_model->updateRecordMetadata(path)`). However, `ColumnViewWidget` uses private `DiskItemModel` instances for each of its pane columns. 

As a result:
1. Updates from `MetaPanel` do not trigger UI repaints in `ColumnViewWidget` pane lists.
2. Changes in `ColumnViewWidget` private models do not notify `m_statsDebounceTimer` in `ContentPanel` to recalculate directory statistics.

This plan adds `updateMetadataForPath` to `ColumnViewWidget` and connects private model `dataChanged` signals to `ContentPanel`'s debounced recalculation timer.

---

## Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/ui/ColumnViewWidget.h`

In `src/ui/ColumnViewWidget.h`, `void updateMetadataForPath(const QString& path);` is declared under public methods.

---

### File: `src/ui/ColumnViewWidget.cpp`

In `src/ui/ColumnViewWidget.cpp`, `updateMetadataForPath` iterates over all `m_panes` and triggers `updateRecordMetadata(path)` and `viewport()->update()`.

---

### File: `src/ui/ContentPanel.cpp`

In `src/ui/ContentPanel.cpp`, `updateItemMetadata` calls `m_columnView->updateMetadataForPath(path)` alongside `m_model` and viewport updates.

---

## Build & Verification Steps

1. **CMake Build Verification**:
   ```bash
   cmake --build build --config Debug
   ```
2. **Functional Verification**:
   - Switch application to Column View mode.
   - Select a file in any active column and view its properties in `MetaPanel`.
   - Modify Rating (stars), Color Tag, or Tags in `MetaPanel`.
   - Verify that:
     - The corresponding row in the Column View active pane immediately updates its rating stars / color tag without requiring full manual refresh.
     - `FilterPanel` and status bar statistics reflect updated metadata counts seamlessly.
