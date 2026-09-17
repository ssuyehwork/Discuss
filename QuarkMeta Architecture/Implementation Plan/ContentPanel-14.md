# Implementation Plan - Active View Routing and Selection Hub Refactoring

## 1. Overview
This implementation plan restores active view routing and selection index retrieval after wrapping view containers with `QScrollArea`. Because `m_viewStack->currentWidget()` returns the outer `QScrollArea` instead of `QAbstractItemView`, `qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget())` failed, causing selection retrieval (`getSelectedIndexes()`) to return empty lists. Consequently, star rating, inline rename (F2), delete, QuickLook preview, and context menu actions were unresponsive.

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    // 4. 视图与控制器引用
    QStackedWidget* viewStack() const { return m_viewStack; }
    QAbstractItemView* gridView() const { return m_gridView; }
    QTreeView* treeView() const;
=======
    // 4. 视图与控制器引用
    QAbstractItemView* activeItemView() const;
    QStackedWidget* viewStack() const { return m_viewStack; }
    QAbstractItemView* gridView() const { return m_gridView; }
    QTreeView* treeView() const;
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
QModelIndexList ContentPanel::getSelectedIndexes() const {
    if (!m_viewStack) return {};
    QAbstractItemView* curView = nullptr;
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            curView = m_columnView->activePane()->listView();
        }
    } else {
        curView = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    }
    if (!curView || !curView->selectionModel()) return {};

    QModelIndexList res;
    const auto& selected = curView->selectionModel()->selectedIndexes();
    res.reserve(selected.size());
    for (const auto& idx : selected) {
        if (idx.column() == 0) {
            res.append(idx);
        }
    }
    return res;
}
=======
QAbstractItemView* ContentPanel::activeItemView() const {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            DropListView* folderV = m_columnView->activePane()->folderListView();
            if (folderV && (folderV->hasFocus() || (folderV->selectionModel() && folderV->selectionModel()->hasSelection()))) {
                return folderV;
            }
            return m_columnView->activePane()->listView();
        }
        return nullptr;
    }

    if (m_currentViewMode == ListView) {
        if (m_folderTreeView && (m_folderTreeView->hasFocus() ||
            (m_folderTreeView->selectionModel() && m_folderTreeView->selectionModel()->hasSelection()))) {
            return m_folderTreeView;
        }
        return m_treeView;
    }

    // GridView / JustifiedViewMode
    if (m_folderGridView && (m_folderGridView->hasFocus() ||
        (m_folderGridView->selectionModel() && m_folderGridView->selectionModel()->hasSelection()))) {
        return m_folderGridView;
    }
    return m_gridView;
}

QModelIndexList ContentPanel::getSelectedIndexes() const {
    if (!m_viewStack) return {};
    QModelIndexList res;

    QList<QAbstractItemView*> views;
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            if (m_columnView->activePane()->folderListView()) views << m_columnView->activePane()->folderListView();
            if (m_columnView->activePane()->listView()) views << m_columnView->activePane()->listView();
        }
    } else if (m_currentViewMode == ListView) {
        if (m_folderTreeView) views << m_folderTreeView;
        if (m_treeView) views << m_treeView;
    } else { // GridView / JustifiedViewMode
        if (m_folderGridView) views << m_folderGridView;
        if (m_gridView) views << m_gridView;
    }

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
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) view = (m_viewStack && m_viewStack->currentWidget() == m_gridView) ? m_gridView : m_treeView;
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}
=======
void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) view = activeItemView();
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
            QModelIndex proxyIdx = m_proxyModel->index(i, 0);
            if (proxyIdx.data(PathRole).toString() == path) {
                QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
                if (view && view->selectionModel()) {
                    view->scrollTo(proxyIdx);
                    view->setCurrentIndex(proxyIdx);
                    view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                }
                break;
            }
        }
=======
        for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
            QModelIndex proxyIdx = m_proxyModel->index(i, 0);
            if (proxyIdx.data(PathRole).toString() == path) {
                QAbstractItemView* view = activeItemView();
                if (view && view->selectionModel()) {
                    view->scrollTo(proxyIdx);
                    view->setCurrentIndex(proxyIdx);
                    view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                }
                break;
            }
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
```bash
cmake --build build --target QuarkMeta
```
1. Star Rating Verification: Select cards in Grid/Justified/List views and press 1-5 or click star icons in metadata panel.
2. Inline Rename (F2): Select any file or folder card, press F2 to trigger inline rename.
3. Delete Action: Select item and press Delete to send to trash.
4. QuickLook Preview: Select item and press Spacebar to trigger preview.
5. Context Menu: Right-click on card and verify menu items are active and usable.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `activeItemView()` helper across selection, context menu, and single item scroll targeting.
- Avoided duplicating view type checks.

## 6. Header API Signature Verification
- `ContentPanel::activeItemView()`: Declared in `src/ui/ContentPanel.h`.
- `QAbstractItemView::hasFocus()`: Verified Qt6 API.
- `QItemSelectionModel::hasSelection()`: Verified Qt6 API.
- `QItemSelectionModel::selectedIndexes()`: Verified Qt6 API.
