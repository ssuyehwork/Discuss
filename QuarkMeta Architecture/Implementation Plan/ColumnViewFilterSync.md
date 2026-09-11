# ColumnViewFilterSync Implementation Plan (ColumnViewFilterSync.md)

## 1. Overview
This implementation plan establishes full integration between Column View (`ColumnViewWidget` / `ColumnViewPane`) and the Filter Panel (`FilterPanel`).
Currently, when users open new column panes or switch active columns in Column View, the Filter Panel retains obsolete statistical data from previous directories.
This plan specifies emitting a signal containing the current active column's `ItemRecord` records whenever a column finishes loading or changes selection, driving `ContentStatsWorker` in `ContentPanel` to update `FilterPanel` via `directoryStatsReady`.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h` (Declare `activeColumnRecordsChanged` signal in `ColumnViewPane` and `ColumnViewWidget`)
- `src/ui/ColumnViewWidget.cpp` (Emit records when directory scanning completes in `ColumnViewPane::loadDirectory` and when active pane switches)
- `src/ui/ContentPanel.h` (Declare slot or connection handler for `activeColumnRecordsChanged`)
- `src/ui/ContentPanel.cpp` (Connect `ColumnViewWidget::activeColumnRecordsChanged` to `m_statsWorker->processAsync(...)`)

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/ColumnViewWidget.h`

```cpp
<<<<<<< SEARCH
signals:
    void folderSelected(const QString& folderPath, int paneIdx);
    void fileSelected(const QString& filePath, int paneIdx);
    void selectionChanged();
=======
signals:
    void folderSelected(const QString& folderPath, int paneIdx);
    void fileSelected(const QString& filePath, int paneIdx);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
signals:
    void selectionChanged();
    void pathNavigated(const QString& path);
=======
signals:
    void selectionChanged();
    void pathNavigated(const QString& path);
    void activeColumnRecordsChanged(const std::vector<ItemRecord>& records);
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/ColumnViewWidget.cpp`

```cpp
<<<<<<< SEARCH
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                // 触发图标与缩略图提取管线
                int count = weakSelf->m_model->rowCount();
                if (count > 0) {
                    QList<int> visibleRows;
                    visibleRows.reserve(count);
                    for (int r = 0; r < count; ++r) visibleRows.append(r);
                    weakSelf->m_model->loadThumbnailsForRows(visibleRows);
                }
=======
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                // 触发图标与缩略图提取管线
                int count = weakSelf->m_model->rowCount();
                if (count > 0) {
                    QList<int> visibleRows;
                    visibleRows.reserve(count);
                    for (int r = 0; r < count; ++r) visibleRows.append(r);
                    weakSelf->m_model->loadThumbnailsForRows(visibleRows);
                }
                emit weakSelf->recordsLoaded(items);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::selectionChanged, this, [this, pane]() {
        m_activePaneIndex = pane->property("paneIndex").toInt();
        emit selectionChanged();
    });
=======
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
>>>>>>> REPLACE
```

### 3.3 Update `src/ui/ContentPanel.cpp`

```cpp
<<<<<<< SEARCH
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        if (!QFileInfo(path).isDir()) {
            emit fileActivated(path);
        }
    });
=======
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        if (!QFileInfo(path).isDir()) {
            emit fileActivated(path);
        }
    });
    connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<ItemRecord>& records) {
        if (m_statsWorker && m_currentViewMode == ViewModeColumn) {
            m_statsWorker->processAsync(records, m_currentFilter.showHidden);
        }
    });
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project with CMake:
   ```bash
   cmake --build build --config Release
   ```
2. Launch QuarkMeta and switch to Column View (`ViewModeColumn`).
3. Click through multiple folders in succession to cascade open new sub-columns.
4. Verify that whenever a new active pane is focused or opened, the right-hand `FilterPanel` instantly updates its statistics, tag badges, and file type counters for the active column.
