# Implementation Plan - ColumnView Refactoring Sync (ColumnViewRefactoring-Sync.md)

## 1. Overview
Restores three critical Column View integration capabilities that were absent in `src/` compared to `Version-6`:
1. **Drag-and-Drop Capability (`DropListView`)**: Replaces standard `QListView` in `ColumnViewPane` with `DropListView` to support dragging items to `FavoritePanel` or external targets.
2. **FilterPanel Stats Synchronization (`activeColumnRecordsChanged`)**: Emits `activeColumnRecordsChanged(records)` whenever the active column/pane changes or reloads in `ColumnViewWidget`, and connects it in `ContentPanel` to drive real-time statistics update in `FilterPanel`.
3. **ViewMode Switch Self-Healing (`setViewMode`)**: In `ContentPanel::setViewMode`, when switching away from `ColumnView` back to Grid/List views, checks if `m_diskModel` is unpopulated/out-of-sync and automatically re-triggers `loadDirectory(m_currentPath)` to heal false empty states.

---

## 2. Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/ColumnViewWidget.h`
Add `DropListView` header and `activeColumnRecordsChanged` signal declaration.

```git
<<<<<<< SEARCH
signals:
    void activePaneChanged(ColumnViewPane* pane);
    void directorySelected(const QString& path);
=======
signals:
    void activePaneChanged(ColumnViewPane* pane);
    void activeColumnRecordsChanged(const std::vector<QuarkMeta::ItemRecord>& records);
    void directorySelected(const QString& path);
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/ColumnViewWidget.cpp`
Replace `QListView` with `DropListView`, and emit `activeColumnRecordsChanged` on column updates.

```git
<<<<<<< SEARCH
#include "ColumnViewWidget.h"
#include "ColumnItemDelegate.h"
#include "ContentPanel.h"
=======
#include "ColumnViewWidget.h"
#include "ColumnItemDelegate.h"
#include "DropListView.h"
#include "ContentPanel.h"
>>>>>>> REPLACE
<<<<<<< SEARCH
    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new QListView(this);
=======
    m_model = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new DropListView(this);
>>>>>>> REPLACE
<<<<<<< SEARCH
    connect(this, &ColumnViewWidget::activePaneChanged, this, [this](ColumnViewPane* pane) {
        if (pane) {
            scrollToPane(pane);
        }
    });
=======
    connect(this, &ColumnViewWidget::activePaneChanged, this, [this](ColumnViewPane* pane) {
        if (pane) {
            scrollToPane(pane);
            if (pane->model()) {
                emit activeColumnRecordsChanged(pane->model()->allRecords());
            }
        }
    });
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/ContentPanel.cpp`
Connect `activeColumnRecordsChanged` signal to stats worker and add view mode switch self-healing logic.

```git
<<<<<<< SEARCH
        m_columnView = new ColumnViewWidget(this, this);
        m_viewStack->addWidget(m_columnView);

        connect(m_columnView, &ColumnViewWidget::directorySelected, this, &ContentPanel::directorySelected);
=======
        m_columnView = new ColumnViewWidget(this, this);
        m_viewStack->addWidget(m_columnView);

        connect(m_columnView, &ColumnViewWidget::directorySelected, this, &ContentPanel::directorySelected);
        connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<QuarkMeta::ItemRecord>& records) {
            if (m_statsWorker && !records.empty()) {
                m_statsWorker->processAsync(records, m_currentFilter.showHidden);
            }
        });
>>>>>>> REPLACE
<<<<<<< SEARCH
void ContentPanel::setViewMode(ViewMode mode) {
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            m_columnView->setRootPath(m_currentPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
}
=======
void ContentPanel::setViewMode(ViewMode mode) {
    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            m_columnView->setRootPath(m_currentPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    // 🚀【自愈数据同步机制】：若从分栏视图切回网格/列表/瀑布流视图，且主模型处于空装载状态，自动自愈驱动 loadDirectory
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_model || m_model->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake -B build -G Ninja
cmake --build build
```

### Verification Method
1. **Drag-and-Drop Test**: Switch to Column View, select an item, and drag it to `FavoritePanel`. Verify the drag operation initiates seamlessly.
2. **FilterPanel Sync Test**: Expand subdirectories in Column View or click different active columns. Check the right-hand `FilterPanel` to confirm tag/type/date counts update in real time.
3. **ViewMode Healing Test**: Navigate into subfolders in Column View, then switch to Grid View. Verify items render instantly without showing empty states.
