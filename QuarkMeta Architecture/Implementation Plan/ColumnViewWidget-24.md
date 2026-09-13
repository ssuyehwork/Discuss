# Implementation Plan - ColumnViewWidget-24.md

## Overview
This implementation plan guarantees two core architectural behaviors for `ColumnViewWidget` in QuarkMeta:
1. **Filter Panel Data Source Lock (SSOT)**: The Filter Panel and `ContentStatsWorker` data source MUST strictly and unconditionally originate from the rightmost (deepest) column (`m_panes.last()`). Regardless of which column currently has active user selection or clicks, `activeColumnRecordsChanged` will always broadcast records from `rightmostPane()`.
2. **Ancestor Path Persistent Highlighting**: Opening sub-columns by clicking folders retains selection in parent columns. `dismissSubColumns` and `folderSelected` will only clear selections in sub-columns strictly to the right of the target column, keeping parent folder selection visible as an unbroken navigation trail.

## Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.h`
Add `rightmostPane()` declaration.

```git
<<<<<<< SEARCH
    ColumnViewPane* activePane() const;
    bool containsPath(const QString& path) const;
=======
    ColumnViewPane* activePane() const;
    ColumnViewPane* rightmostPane() const;
    bool containsPath(const QString& path) const;
>>>>>>> REPLACE
```

### 2. `src/ui/ColumnViewWidget.cpp`
Implement `rightmostPane()` and update signal broadcasting in `appendColumn` and `dismissSubColumns`.

```git
<<<<<<< SEARCH
ColumnViewPane* ColumnViewWidget::activePane() const {
    if (m_activePaneIndex >= 0 && m_activePaneIndex < m_panes.size()) {
        return m_panes[m_activePaneIndex];
    }
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}
=======
ColumnViewPane* ColumnViewWidget::activePane() const {
    if (m_activePaneIndex >= 0 && m_activePaneIndex < m_panes.size()) {
        return m_panes[m_activePaneIndex];
    }
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}

ColumnViewPane* ColumnViewWidget::rightmostPane() const {
    return m_panes.isEmpty() ? nullptr : m_panes.last();
}
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_layout->removeWidget(pane);
        pane->deleteLater();
    }
    updatePaneWidths();
}
=======
void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_layout->removeWidget(pane);
        pane->deleteLater();
    }
    updatePaneWidths();
    if (rightmostPane() && rightmostPane()->model()) {
        emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
    }
}
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
ColumnViewPane* ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setProperty("paneIndex", newIdx);
    pane->setFilterState(m_currentFilter);
    pane->loadDirectory();

    connect(pane, &ColumnViewPane::recordsLoaded, this, [this, pane](const std::vector<ItemRecord>& records) {
        if (pane == activePane()) {
            emit activeColumnRecordsChanged(records);
        }
    });

    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        if (pane->model()) {
            emit activeColumnRecordsChanged(pane->model()->allRecords());
        }
    });
=======
ColumnViewPane* ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setProperty("paneIndex", newIdx);
    pane->setFilterState(m_currentFilter);
    pane->loadDirectory();

    connect(pane, &ColumnViewPane::recordsLoaded, this, [this, pane](const std::vector<ItemRecord>& records) {
        if (pane == rightmostPane()) {
            emit activeColumnRecordsChanged(records);
        }
    });

    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
        if (rightmostPane() && rightmostPane()->model()) {
            emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
        }
    });
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Execute `mkdir -p build && cd build && cmake .. && ninja` to compile.
2. Confirm zero syntax or link errors.
3. Verify `ColumnViewWidget.h` and `ColumnViewWidget.cpp` using `read_file`.
