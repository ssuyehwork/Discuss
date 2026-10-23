# Implementation Plan - ContentPanel-2.md

## 1. Overview
This implementation plan addresses the remaining runtime state-conflict issues ("死穴 1", "死穴 2", and "死穴 4") when loading Library categories:

1. **ColumnView Auto-Fallback (死穴 1)**: When loading custom path lists (like Library categories) via `ContentDataLoader::loadPaths`, if the current view mode is `ColumnView` (Miller Columns), `ContentPanel` automatically falls back to `GridView` (or `ListView` if configured) so that virtual item lists render properly instead of being ignored by Miller Columns.
2. **Virtual Protocol & Refresh Shield (死穴 2)**: When loading a Library category, `ContentPanel` sets `m_currentCategoryType = "library"`, stores the virtual protocol URL (e.g. `library://category?paths=...`) in `m_currentPath`, and keeps track of `m_lastLoadedLibraryPaths`. In `ContentPanel::refreshAll()`, if `m_currentCategoryType == "library"`, it re-calls `loadPaths(m_lastLoadedLibraryPaths)` instead of wiping out the loaded items with stale disk paths (`m_currentPath`).
3. **Sidebar Tab Lifecycle Sync (死穴 4)**: In `SidebarContainerWidget`, when switching tabs (e.g., to "库"), `m_libraryPanel->loadLibrary()` is triggered to ensure database synchronization, and `sidebarTabChanged(int index)` is emitted to inform `PanelMediator` of the active sidebar view.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/ContentPanel-2.md`.

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentDataLoader.cpp`
- `src/ui/SidebarContainerWidget.h`
- `src/ui/SidebarContainerWidget.cpp`
- `src/ui/PanelMediator.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.h`

Add members for tracking last loaded library paths:
```diff
<<<<<<< SEARCH
    QStringList getSelectedPaths() const;
=======
    QStringList lastLoadedLibraryPaths() const { return m_lastLoadedLibraryPaths; }
    void setLastLoadedLibraryPaths(const QStringList& paths) { m_lastLoadedLibraryPaths = paths; }
    QStringList getSelectedPaths() const;
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    FilterState m_currentFilter;
=======
    QStringList m_lastLoadedLibraryPaths;
    FilterState m_currentFilter;
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`

Update `refreshAll()` to handle the `"library"` category type without wiping out library paths:
```diff
<<<<<<< SEARCH
void ContentPanel::refreshAll() {
    if (m_isLoading) return;

    if (m_currentCategoryType == "trash") {
        loadCategory("trash");
        return;
    }

    if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
        loadDirectory(m_currentPath, m_isRecursive);
    }
}
=======
void ContentPanel::refreshAll() {
    if (m_isLoading) return;

    if (m_currentCategoryType == "trash") {
        loadCategory("trash");
        return;
    }

    if (m_currentCategoryType == "library") {
        loadPaths(m_lastLoadedLibraryPaths);
        return;
    }

    if (!m_currentPath.isEmpty() && m_currentPath != "computer://" && !m_currentPath.startsWith("library://")) {
        loadDirectory(m_currentPath, m_isRecursive);
    }
}
>>>>>>> REPLACE
```

### `src/ui/controllers/ContentDataLoader.cpp`

In `loadPaths`, handle ColumnView fallback and virtual protocol state:
```diff
<<<<<<< SEARCH
void ContentDataLoader::loadPaths(const QStringList& paths, int reqId) {
    if (!m_panel) return;
    m_panel->restoreActiveView();
    m_panel->ensureSourceModelIsDiskModel();
=======
void ContentDataLoader::loadPaths(const QStringList& paths, int reqId) {
    if (!m_panel) return;

    // 死穴 1 解法：如果当前处于分栏视图，自动自愈切换为网格视图以保证路径列表正常呈现
    if (m_panel->currentViewMode() == ContentPanel::ColumnView) {
        m_panel->setViewMode(ContentPanel::GridView);
    }

    m_panel->restoreActiveView();
    m_panel->ensureSourceModelIsDiskModel();
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    if (m_panel->getCurrentCategoryType().isEmpty()) {
        m_panel->setCurrentCategoryType("path_list");
    }
=======
    if (m_panel->getCurrentCategoryType().isEmpty()) {
        m_panel->setCurrentCategoryType("path_list");
    } else if (m_panel->getCurrentCategoryType() == "library") {
        m_panel->setLastLoadedLibraryPaths(paths);
    }
>>>>>>> REPLACE
```

### `src/ui/SidebarContainerWidget.h`

Emit signal when active sidebar tab changes:
```diff
<<<<<<< SEARCH
signals:
=======
signals:
    void sidebarTabChanged(int index);
>>>>>>> REPLACE
```

### `src/ui/SidebarContainerWidget.cpp`

Connect tab changes to reload library and emit signal:
```diff
<<<<<<< SEARCH
    connect(m_tabBar, &QTabBar::currentChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);
=======
    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        if (index == 1 && m_libraryPanel) {
            m_libraryPanel->loadLibrary();
        }
        emit sidebarTabChanged(index);
    });
>>>>>>> REPLACE
```

### `src/ui/PanelMediator.cpp`

Update library category selection handler to set category type `"library"` and update virtual path:
```diff
<<<<<<< SEARCH
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel](const QStringList& paths) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->loadPaths(paths);
                }
            });
        }
=======
        if (libraryPanel) {
            connect(libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel](const QStringList& paths) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->setCurrentCategoryType("library");
                    target->setLastLoadedLibraryPaths(paths);
                    target->loadPaths(paths);
                }
            });
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Test Scenario 1 (ColumnView Fallback):
   - Switch `ContentPanel` to ColumnView mode.
   - Click a category in the "库" sidebar tab.
   - Confirm that `ContentPanel` smoothly switches to GridView and renders the category items properly.
3. Test Scenario 2 (Refresh Shield):
   - Click a category in the "库" sidebar tab.
   - Press F5 (Refresh) or trigger metadata update.
   - Confirm that the loaded category items remain on screen and are NOT wiped out by stale disk paths.
4. Test Scenario 3 (Sidebar Tab Lifecycle):
   - Add/modify a category in the database.
   - Click the "库" tab in the sidebar.
   - Confirm that `loadLibrary()` runs and loads the latest categories from database.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reuses `m_panel->setViewMode(GridView)`, `m_panel->loadPaths(paths)`, and `m_libraryPanel->loadLibrary()`.
- **Zero Redundancy**: Resolves state conflicts using existing architecture signals and state variables.

## 6. Header API Signature Verification
- `ContentPanel` class signature in `src/ui/ContentPanel.h` extends getter/setter for `lastLoadedLibraryPaths()` in a backward-compatible manner.
- `SidebarContainerWidget` signature in `src/ui/SidebarContainerWidget.h` adds signal `sidebarTabChanged(int)`.

## 7. Header Inclusion Chain & Type Completeness Check
- All modified `.cpp` files include required headers. No broken inclusion chains or incomplete types.
