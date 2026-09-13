# ColumnViewWidget Implementation Plan - Restrict Filter State to Rightmost Pane

## 1. Overview
Currently, `ColumnViewWidget::applyFilterState` and `appendColumn` apply the `FilterState` to all `ColumnViewPane` instances across all columns. This causes ancestor parent columns to filter out items, breaking parent folder availability and interrupting path navigation.

This implementation plan restricts `FilterState` execution strictly to `rightmostPane()` (`m_panes.last()`), while ensuring all ancestor parent columns (`m_panes[0 ... size - 2]`) remain in an unfiltered state (`FilterState()`).

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`

```git
<<<<<<< SEARCH
void ColumnViewWidget::applyFilterState(const FilterState& state) {
    m_currentFilter = state;
    for (auto* pane : m_panes) {
        pane->setFilterState(state);
    }
}
=======
void ColumnViewWidget::applyFilterState(const FilterState& state) {
    m_currentFilter = state;
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            m_panes[i]->setFilterState(FilterState());
        }
    }
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
    if (rightmostPane() && rightmostPane()->model()) {
        emit activeColumnRecordsChanged(rightmostPane()->model()->allRecords());
    }
}
=======
void ColumnViewWidget::dismissSubColumns(int fromIndex) {
    while (m_panes.size() > fromIndex + 1) {
        ColumnViewPane* pane = m_panes.takeLast();
        m_layout->removeWidget(pane);
        pane->deleteLater();
    }
    updatePaneWidths();
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            m_panes[i]->setFilterState(FilterState());
        }
    }
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
=======
ColumnViewPane* ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setProperty("paneIndex", newIdx);
    pane->loadDirectory();
>>>>>>> REPLACE
```

```git
<<<<<<< SEARCH
    m_panes.append(pane);
    m_layout->addWidget(pane);
    updatePaneWidths();
    scrollToRightmostPane();
    return pane;
}
=======
    m_panes.append(pane);
    m_layout->addWidget(pane);
    updatePaneWidths();
    for (int i = 0; i < m_panes.size(); ++i) {
        if (i == m_panes.size() - 1) {
            m_panes[i]->setFilterState(m_currentFilter);
        } else {
            m_panes[i]->setFilterState(FilterState());
        }
    }
    scrollToRightmostPane();
    return pane;
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`
2. Launch QuarkMeta in Column View.
3. Select filter options in `FilterPanel` (e.g. 4-star rating or specific file type).
4. Verify that only the rightmost column filters its items, while all parent ancestor columns display all files and folders without missing parent items.
