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

```git
<<<<<<< SEARCH
    void applyFilterState(const FilterState& state);
    void setRootPath(const QString& path);
    void refreshActiveColumn();
=======
    void applyFilterState(const FilterState& state);
    void setRootPath(const QString& path);
    void refreshActiveColumn();
    void updateMetadataForPath(const QString& path);
>>>>>>> REPLACE
```

---

### File: `src/ui/ColumnViewWidget.cpp`

```git
<<<<<<< SEARCH
void ColumnViewWidget::refreshActiveColumn() {
    ColumnViewPane* pane = activePane();
    if (pane) {
        pane->loadDirectory();
    }
}
=======
void ColumnViewWidget::refreshActiveColumn() {
    ColumnViewPane* pane = activePane();
    if (pane) {
        pane->loadDirectory();
    }
}

void ColumnViewWidget::updateMetadataForPath(const QString& path) {
    for (auto* pane : m_panes) {
        if (pane && pane->model()) {
            pane->model()->updateRecordMetadata(path);
            if (pane->listView() && pane->listView()->viewport()) {
                pane->listView()->viewport()->update();
            }
        }
    }
}
>>>>>>> REPLACE
```

---

### File: `src/ui/ContentPanel.cpp`

```git
<<<<<<< SEARCH
void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) m_model->updateRecordMetadata(path);
    if (m_gridView && m_gridView->viewport()) m_gridView->viewport()->update();
    if (m_treeView && m_treeView->viewport()) m_treeView->viewport()->update();
    recalculateAndEmitStats();
}
=======
void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) m_model->updateRecordMetadata(path);
    if (m_columnView) m_columnView->updateMetadataForPath(path);
    if (m_gridView && m_gridView->viewport()) m_gridView->viewport()->update();
    if (m_treeView && m_treeView->viewport()) m_treeView->viewport()->update();
    recalculateAndEmitStats();
}
>>>>>>> REPLACE
```

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
