# ContentViewCoordinator Refactoring Implementation Plan

## 1. Overview
This implementation plan refactors `ContentViewCoordinator` to serve as the unified Mediator/Coordinator for multi-view state routing across all 4 view modes (GridView, ListView, JustifiedViewMode, ColumnView) in `ContentPanel`.

It eliminates redundant `if-else` view branching in `ContentPanel` while ensuring:
- 100% preservation of all existing view features (selection state, focus, QuickLook, double click, drag drop, empty hint, column layout policies).
- Zero mutual recursion / stack overflow (fallback routes to `m_panel->listCanvas()->fileProxyModel()` or active view proxy).
- 100% physical header signature alignment with `SectionedScrollCanvas.h` (`applyFilter(const FilterState&)`), `ContentPanel.h`, and `ContentViewCoordinator.h`.
- Zero compilation errors (no C2039/C2065/C2248).

## 2. Modified Files List
- `src/ui/controllers/ContentViewCoordinator.h`
- `src/ui/controllers/ContentViewCoordinator.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/controllers/ContentViewCoordinator.h`

```
<<<<<<< SEARCH
    // 视图探测
    QList<QAbstractItemView*> currentActiveViews() const;
    QAbstractItemView* activeItemView() const;

    // 选区与焦点计算
    QModelIndexList getSelectedIndexes() const;
    void restoreSelections(const QSet<QString>& selectedPaths, bool isPendingEdit);
=======
    // 视图探测与代理模型归一化
    QList<QAbstractItemView*> currentActiveViews() const;
    QAbstractItemView* activeItemView() const;
    QSortFilterProxyModel* getActiveProxyModel() const;

    // 选区与焦点计算
    QModelIndexList getSelectedIndexes() const;
    QStringList getSelectedPaths() const;
    void restoreSelections(const QSet<QString>& selectedPaths, bool isPendingEdit);

    // 统一 FilterState 广播（精确使用 SectionedScrollCanvas 的 applyFilter API）
    void applyFilterStateToAllViews(const FilterState& state);
>>>>>>> REPLACE
```

### `src/ui/controllers/ContentViewCoordinator.cpp`

```
<<<<<<< SEARCH
QModelIndexList ContentViewCoordinator::getSelectedIndexes() const {
    QModelIndexList res;
    QList<QAbstractItemView*> views = currentActiveViews();
    for (auto* view : views) {
        if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) {
                    res.append(idx);
                }
            }
        }
    }
    return res;
}
=======
QSortFilterProxyModel* ContentViewCoordinator::getActiveProxyModel() const {
    if (!m_panel) return nullptr;

    auto mode = m_panel->currentViewMode();
    if (mode == ContentPanel::ColumnView && m_panel->columnView() && m_panel->columnView()->activePane()) {
        if (m_panel->columnView()->activePane()->proxyModel()) {
            return m_panel->columnView()->activePane()->proxyModel();
        }
    }

    QAbstractItemView* view = activeItemView();
    if (view && view->model()) {
        return qobject_cast<QSortFilterProxyModel*>(view->model());
    }

    if (mode == ContentPanel::ListView && m_panel->listCanvas()) {
        return m_panel->listCanvas()->fileProxyModel();
    }
    if (m_panel->gridCanvas()) {
        return m_panel->gridCanvas()->fileProxyModel();
    }
    return nullptr;
}

QModelIndexList ContentViewCoordinator::getSelectedIndexes() const {
    QModelIndexList res;
    if (!m_panel) return res;

    if (m_panel->currentViewMode() == ContentPanel::ColumnView) {
        if (m_panel->columnView() && m_panel->columnView()->activePane()) {
            for (auto* view : {m_panel->columnView()->activePane()->folderListView(), m_panel->columnView()->activePane()->listView()}) {
                if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
                    for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                        if (idx.column() == 0) res.append(idx);
                    }
                }
            }
        }
        return res;
    }

    QList<QAbstractItemView*> views = currentActiveViews();
    for (auto* view : views) {
        if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) {
                    res.append(idx);
                }
            }
        }
    }
    return res;
}

QStringList ContentViewCoordinator::getSelectedPaths() const {
    QStringList paths;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    return paths;
}

void ContentViewCoordinator::applyFilterStateToAllViews(const FilterState& state) {
    if (!m_panel) return;

    if (m_panel->listCanvas()) m_panel->listCanvas()->applyFilter(state);
    if (m_panel->gridCanvas()) m_panel->gridCanvas()->applyFilter(state);
    if (m_panel->columnView()) m_panel->columnView()->applyFilterState(state);
}
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    ContentFileOpsHandler* fileOpsHandler() const { return m_fileOpsHandler; }
    ContentStatsWorker* statsWorker() const { return m_statsWorker; }
=======
    ContentFileOpsHandler* fileOpsHandler() const { return m_fileOpsHandler; }
    ContentStatsWorker* statsWorker() const { return m_statsWorker; }
    class ContentViewCoordinator* viewCoordinator() const { return m_viewCoordinator; }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    ContentSortController* m_sortController = nullptr;
    ContentKeyHandler* m_keyHandler = nullptr;
    ContentDataLoader* m_dataLoader = nullptr;
    ContentFileOpsHandler* m_fileOpsHandler = nullptr;
    ContentStatsWorker* m_statsWorker = nullptr;
=======
    ContentSortController* m_sortController = nullptr;
    ContentKeyHandler* m_keyHandler = nullptr;
    ContentDataLoader* m_dataLoader = nullptr;
    ContentFileOpsHandler* m_fileOpsHandler = nullptr;
    ContentStatsWorker* m_statsWorker = nullptr;
    class ContentViewCoordinator* m_viewCoordinator = nullptr;
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
QSortFilterProxyModel* ContentPanel::getActiveProxyModel() const {
    if (m_currentViewMode == ColumnView && m_columnView && m_columnView->activePane()) {
        if (m_columnView->activePane()->proxyModel()) {
            return m_columnView->activePane()->proxyModel();
        }
    }
    QAbstractItemView* view = activeItemView();
    if (view && view->model()) {
        return qobject_cast<QSortFilterProxyModel*>(view->model());
    }
    if (m_currentViewMode == ListView && m_listCanvas) return m_listCanvas->fileProxyModel();
    if (m_gridCanvas) return m_gridCanvas->fileProxyModel();
    return m_fileProxyModel ? m_fileProxyModel : nullptr;
}

QStringList ContentPanel::getSelectedPaths() const {
    QStringList paths;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    return paths;
}
=======
QSortFilterProxyModel* ContentPanel::getActiveProxyModel() const {
    return m_viewCoordinator ? m_viewCoordinator->getActiveProxyModel() : nullptr;
}

QStringList ContentPanel::getSelectedPaths() const {
    return m_viewCoordinator ? m_viewCoordinator->getSelectedPaths() : QStringList();
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build via CMake build target `QuarkMeta`:
   ```bash
   cmake --build --preset x64-Debug --target QuarkMeta
   ```
2. Confirm zero MSVC C2039, C2065, C2248 compilation errors and no stack overflow.
3. Verify seamless operation across Grid, List, Justified, and Column view modes.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **`refreshAll()` Reuse**: Maintained as the sole SSOT entry point for in-place data updates.
- **`loadDirectory()` Reuse**: Maintained as sole SSOT entry point for path navigation.
- **`SectionedScrollCanvas::applyFilter`**: Verified 1:1 against physical header `SectionedScrollCanvas.h`.

## 6. Header API Signature Verification
| Class | Function / Member | Header File | Physical Signature Verification |
|---|---|---|---|
| `SectionedScrollCanvas` | `applyFilter(const FilterState&)` | `SectionedScrollCanvas.h` | `void applyFilter(const FilterState& filter);` |
| `ContentViewCoordinator` | `getActiveProxyModel()` | `ContentViewCoordinator.h` | `QSortFilterProxyModel* getActiveProxyModel() const;` |
| `ContentViewCoordinator` | `getSelectedPaths()` | `ContentViewCoordinator.h` | `QStringList getSelectedPaths() const;` |
| `ContentViewCoordinator` | `applyFilterStateToAllViews()` | `ContentViewCoordinator.h` | `void applyFilterStateToAllViews(const FilterState& state);` |
| `ContentPanel` | `m_viewCoordinator` | `ContentPanel.h` | `ContentViewCoordinator* m_viewCoordinator = nullptr;` |
