# ContentViewCoordinator Implementation Plan

## 1. Overview
This implementation plan strengthens `ContentViewCoordinator` to serve as the unified Mediator/Coordinator for all four view modes (GridView, ListView, JustifiedViewMode, ColumnView) in `ContentPanel`.

It centralizes:
- View selection & model routing (`getActiveProxyModel`, `getSelectedIndexes`, `getSelectedPaths`).
- Unified FilterState application and FilterProxyModel routing across all active views.
- Selection Model synchronization & restoration without scattered `if-else` branches in `ContentPanel`.
- Section counts & total height calculation for split canvases.

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

    // 统一 FilterState 消息广播应用
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
    return m_panel->m_fileProxyModel ? m_panel->m_fileProxyModel : nullptr;
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

    if (m_panel->m_folderProxyModel) m_panel->m_folderProxyModel->setFilterState(state);
    if (m_panel->m_fileProxyModel) m_panel->m_fileProxyModel->setFilterState(state);

    if (m_panel->listCanvas()) m_panel->listCanvas()->applyFilters(state);
    if (m_panel->gridCanvas()) m_panel->gridCanvas()->applyFilters(state);
    if (m_panel->columnView()) m_panel->columnView()->applyFilterState(state);
}
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
2. Verify all 4 view modes (Grid, List, Justified, Column) switch seamlessly.
3. Verify FilterState filter application propagates through `ContentViewCoordinator`.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **`refreshAll()` Reuse**: Maintained as single SSOT entry point.
- **`loadDirectory()` Reuse**: Maintained as single navigation SSOT entry point.
- **No Parallel Routing**: Selection model and proxy model routing strictly delegated to `ContentViewCoordinator`.

## 6. Header API Signature Verification
| Class | Function / Member | Header File | Signature Verification |
|---|---|---|---|
| `ContentViewCoordinator` | `getActiveProxyModel()` | `ContentViewCoordinator.h` | `QSortFilterProxyModel* getActiveProxyModel() const;` |
| `ContentViewCoordinator` | `getSelectedPaths()` | `ContentViewCoordinator.h` | `QStringList getSelectedPaths() const;` |
| `ContentViewCoordinator` | `applyFilterStateToAllViews()` | `ContentViewCoordinator.h` | `void applyFilterStateToAllViews(const FilterState& state);` |
| `ContentPanel` | `getActiveProxyModel()` | `ContentPanel.h` | `QSortFilterProxyModel* getActiveProxyModel() const;` |
