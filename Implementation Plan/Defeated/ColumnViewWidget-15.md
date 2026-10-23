# ColumnViewWidget & PanelMediator Architecture Architecture Plan (ColumnViewWidget-15.md)

## Overview
This implementation plan addresses the 5 critical architectural flaws in `ColumnViewWidget`, `PanelMediator`, and `ContentPanel`:
1. **Single File Selection Sub-Column Cleanup**: When clicking a non-directory file in a column pane, child columns are now immediately dismissed via `dismissSubColumns(paneIdx)`.
2. **Thumbnail Batch Queue Optimization**: Replaced full 0..count-1 queue scheduling in `ColumnViewPane::setSharedRecords` with a slice of the first 30 visible items to prevent CPU/IO throttling on large directories.
3. **SSOT Metadata & Rating Reset Fix**: Ensured `MetadataManager` SSOT record values are directly populated in `PanelMediator` without ternary condition bugs on zero ratings.
4. **AddressBar / Breadcrumb Synchronization**: Added `ColumnViewWidget::navigateToPath` to truncate rightward sub-columns when navigating up through AddressBar.
5. **Eliminated `paneIndex` Property Fragility**: Replaced `property("paneIndex")` lookups with `indexOfPane(srcPane)` based on physical list indices in `m_panes`.

## Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`
- `src/ui/PanelMediator.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.h`
Pass `ColumnViewPane*` pointers directly in signals instead of `int paneIndex` and expose `navigateToPath`.

```cpp
<<<<<<< SEARCH
signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
=======
signals:
    void folderSelected(const QString& folderPath, ColumnViewPane* pane);
    void fileSelected(const QString& filePath, ColumnViewPane* pane);
>>>>>>> REPLACE
<<<<<<< SEARCH
    void setRootPath(const QString& path);
    void clearAllColumns();
=======
    void setRootPath(const QString& path);
    void navigateToPath(const QString& path);
    void clearAllColumns();
>>>>>>> REPLACE
<<<<<<< SEARCH
    void dismissSubColumns(int fromIndex);
    ColumnViewPane* appendColumn(const QString& path);
    void clearOtherSelections(int activePaneIdx);
=======
    int indexOfPane(ColumnViewPane* pane) const;
    void dismissSubColumns(int fromIndex);
    ColumnViewPane* appendColumn(const QString& path);
    void clearOtherSelections(int activePaneIdx);
>>>>>>> REPLACE
```

### 2. `src/ui/ColumnViewWidget.cpp`
Emit `fileSelected` on single-click for files and calculate pane indices via `indexOfPane(srcPane)`. Limit thumbnail queues to the first 30 items.

```cpp
<<<<<<< SEARCH
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        }
    });
=======
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        if (isDir) {
            emit folderSelected(itemPath, this);
        } else {
            emit fileSelected(itemPath, this);
        }
    });
>>>>>>> REPLACE
```

### 3. `src/ui/ContentPanel.cpp`
Invoke `m_columnView->navigateToPath(path)` in `ContentPanel::loadDirectory`.

```cpp
<<<<<<< SEARCH
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ViewModeColumn && m_columnView) {
        m_currentPath = path;
        // 🚀【防大刷新机制】：若目标路径已存在于分栏视图的已有列栈中，仅同步 m_currentPath 与地址栏，绝对不触发整套列重置 (setRootPath)
        if (!m_columnView->containsPath(path)) {
            m_columnView->setRootPath(path);
        }
        updateStatusBarStats();
        return;
    }
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ViewModeColumn && m_columnView) {
        m_currentPath = path;
        m_columnView->navigateToPath(path);
        updateStatusBarStats();
        return;
    }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile project and verify clean compilation without syntax errors.
2. In Column View, single click a file and verify rightward columns close instantly.
3. Verify rating reset (0 stars) persists correctly in `MetaPanel`.
