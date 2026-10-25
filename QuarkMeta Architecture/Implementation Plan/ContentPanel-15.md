# ContentPanel-15.md: ColumnView Multi-Selection Preservation Fix

## 1. Overview
When selecting multiple items (or all items) in Grid View, List View, or Justified View, and then switching to Column View (or switching back and forth), the selection was lost (unselected).

Deep diagnosis identified the precise root causes:

1. **Pending Selection Conflict in `ColumnViewPane::loadDirectory()`**:
   When `m_columnView->setRootPath()` is called during view switching, it programmatically sets `m_pendingSelectPath` (single target file/folder path) on the rightmost pane. Shortly after, `ContentPanel::restoreSelections()` sets `m_pendingSelectPaths` (the set of all selected files).
   Because `ColumnViewPane::loadDirectory()` runs asynchronously in a background thread, when it finishes loading records, it checked `if (!m_pendingSelectPath.isEmpty()) selectItemByPath(...)` BEFORE checking `m_pendingSelectPaths`. Calling `selectItemByPath()` first executed a `ClearAndSelect` on only 1 single item, immediately wiping out the multi-selection set!

2. **Active Pane Index Hijacking by Parent Column Selection**:
   In `ColumnViewWidget`, when parent columns programmatically selected subfolder items to display directory hierarchy highlights, parent column `selectionChanged` signals fired and updated `m_activePaneIndex` to the parent column's index. When `getSelectedPaths()` was called during view switching, it queried the parent pane (which contained 0 selected files), returning an empty selection list and overwriting `m_selectionState.selectedPaths`.

3. **`m_pendingSelectPath` Overwriting `m_pendingSelectPaths`**:
   `setPendingSelectPaths(paths)` did not clear `m_pendingSelectPath`. As a result, when async loading completed, the single-item selection logic still executed and overrode the batch selection.

This implementation plan fixes all three root causes:
- In `ColumnViewPane`, when `setPendingSelectPaths(paths)` is called, `m_pendingSelectPath` is cleared.
- In `ColumnViewPane::loadDirectory()`, `m_pendingSelectPaths` (batch selection) is given higher precedence over `m_pendingSelectPath` (single item selection).
- In `ColumnViewWidget`, parent column selection signals when programmatically highlighting ancestor directories do not hijack `m_activePaneIndex`. `m_activePaneIndex` remains locked to `rightmostPane()` unless user explicitly interacts with a parent pane.

---

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ColumnViewWidget.cpp`

<<<<<<< SEARCH
void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths) {
    m_pendingSelectPaths = paths;
    tryPendingSelection();
}
=======
void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths) {
    m_pendingSelectPaths = paths;
    if (!paths.isEmpty()) {
        m_pendingSelectPath.clear();
    }
    tryPendingSelection();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                if (!weakSelf->m_pendingSelectPaths.isEmpty()) {
                    weakSelf->tryPendingSelection();
                }
=======
                if (!weakSelf->m_pendingSelectPaths.isEmpty()) {
                    weakSelf->m_pendingSelectPath.clear();
                    weakSelf->tryPendingSelection();
                } else if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });
=======
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        if (pane == rightmostPane() || (pane->listView() && pane->listView()->hasFocus())) {
            m_activePaneIndex = pane->property("paneIndex").toInt();
        }
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });
>>>>>>> REPLACE

---

## 4. Build & Verification Steps

### Verification Commands
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Verification Checklist
1. Select all files (or multiple files) in Grid View, List View, or Justified View.
2. Switch view mode to Column View (`ColumnView`).
3. Verify that all selected files remain selected and highlighted in Column View.
4. Switch view mode back to Grid View / List View / Justified View.
5. Verify that all selected files remain selected and highlighted without selection loss.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Preserves `SelectionState` as the Single Source of Truth across all four view modes.
- Ensures async loading completion prioritizes batch selections (`m_pendingSelectPaths`) over single item focus (`m_pendingSelectPath`).

---

## 6. Header API Signature Verification
- `ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths)` -> `src/ui/ColumnViewWidget.h`
- `ColumnViewPane::loadDirectory()` -> `src/ui/ColumnViewWidget.h`
