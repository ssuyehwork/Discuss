# Implementation Plan - ColumnView Filter Stats Sync (ColumnViewFilterSync-1.md)

## Overview
This implementation plan addresses the issue where changing item ratings, color tags, or other metadata in Column View mode (`ViewModeColumn`) fails to update the stats in the Filter Panel (`FilterPanel`).

### Root Causes
1. **Column View Model Isolation**: In Column View mode, `ContentPanel`'s primary model (`m_model` / `m_diskModel`) is not populated with directory items because each `ColumnViewPane` maintains its own `DiskItemModel`. When `ContentPanel::recalculateAndEmitStats()` was invoked, `if (!m_model || m_model->allRecords().empty()) return;` immediately returned, preventing `ContentStatsWorker` from running and suppressing `directoryStatsReady` events.
2. **Filter Panel Zero-Count Rating Row Stripping**: In `FilterPanel::rebuildGroups()`, rating filter rows were dropped if their count dropped to zero (`if (!m_ratingCounts.contains(r) || m_ratingCounts[r] <= 0) continue;`), ignoring whether `currentSt.ratings` still contained that rating (`isChecked`).

### Key Fixes
1. **View-Aware Stats Source in ContentPanel**: Modify `ContentPanel::recalculateAndEmitStats()` to fetch `ItemRecord`s from `m_columnView->activePane()->model()->allRecords()` when in `ViewModeColumn`, falling back to `m_model->allRecords()` for other views.
2. **Column View Signal Relay**: Wire `ColumnViewPane::recordsLoaded` and column navigation events in `ColumnViewWidget` to trigger `recalculateAndEmitStats()` so that expanding new columns or switching active columns automatically updates `FilterPanel` stats.
3. **Filter Panel Zero-Count Preservation**: Update `FilterPanel::rebuildGroups()` rating row generation logic to preserve rows that are currently checked (`isChecked`), matching the color row filter logic.

---

## Modified Files List
- `src/ui/ContentPanel.cpp`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/FilterPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
void ContentPanel::recalculateAndEmitStats() {
    if (!m_model || m_model->allRecords().empty()) return;
    if (m_statsWorker) {
        m_statsWorker->processAsync(m_model->allRecords(), m_currentFilter.showHidden);
    }
}
=======
void ContentPanel::recalculateAndEmitStats() {
    std::vector<ItemRecord> records;
    if (m_viewMode == ViewModeColumn && m_columnView && m_columnView->activePane() && m_columnView->activePane()->model()) {
        records = m_columnView->activePane()->model()->allRecords();
    } else if (m_model) {
        records = m_model->allRecords();
    }

    if (records.empty()) return;

    if (m_statsWorker) {
        m_statsWorker->processAsync(records, m_currentFilter.showHidden);
    }
}
>>>>>>> REPLACE
```

---

### 2. `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        clearOtherSelections(m_panes.indexOf(pane));
        emit selectionChanged();
    });

    m_layout->addWidget(pane);
=======
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        clearOtherSelections(m_panes.indexOf(pane));
        emit selectionChanged();
    });

    connect(pane, &ColumnViewPane::recordsLoaded, this, [this, pane](const std::vector<ItemRecord>&) {
        if (pane == activePane() && m_contentPanel) {
            m_contentPanel->recalculateAndEmitStats();
        }
    });

    m_layout->addWidget(pane);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_activePaneIndex = paneIndex;
    clearOtherSelections(paneIndex);
    emit pathNavigated(folderPath);
=======
    m_activePaneIndex = paneIndex;
    clearOtherSelections(paneIndex);
    emit pathNavigated(folderPath);
    if (m_contentPanel) {
        m_contentPanel->recalculateAndEmitStats();
    }
>>>>>>> REPLACE
```

---

### 3. `src/ui/FilterPanel.cpp`

```
<<<<<<< SEARCH
        for (int r : {0, 1, 2, 3, 4, 5}) {
            if (!m_ratingCounts.contains(r) || m_ratingCounts[r] <= 0) continue;
            QCheckBox* cb = addFilterRow(gl, ratingDisplayName(r), m_ratingCounts[r]);
            cb->blockSignals(true);
            cb->setChecked(currentSt.ratings.contains(r));
            cb->blockSignals(false);
=======
        for (int r : {0, 1, 2, 3, 4, 5}) {
            int cnt = m_ratingCounts.value(r, 0);
            bool isChecked = currentSt.ratings.contains(r);
            if (cnt <= 0 && !isChecked) continue;

            QCheckBox* cb = addFilterRow(gl, ratingDisplayName(r), cnt);
            cb->blockSignals(true);
            cb->setChecked(isChecked);
            cb->blockSignals(false);
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. Build Instructions
Run the build script or CMake build in the terminal:
```bash
cmake --build build --config Release
```

### 2. Verification Steps
1. Launch QuarkMeta and switch to Column View mode (`ViewModeColumn`).
2. Navigate through folders so multiple cascade panes appear.
3. Select an item in the rightmost/active column pane.
4. Modify the star rating or color tag of the item in the right-hand Meta Panel (`MetaPanel`).
5. Verify that the Filter Panel (`FilterPanel`) on the right-hand side immediately updates its rating and color counts to reflect the new state in real time.
6. Verify that switching active columns in Column View updates the Filter Panel statistics to match the newly active column's contents.
