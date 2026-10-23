# Implementation Plan - ColumnItemDelegate-StyleSync.md (Column View Row Height and Style Sync with NavPanel)

## 1. Overview
Investigation revealed that `QTreeView` (used in `NavPanel` for directory navigation) has its item height restricted to `28px` by the QSS rule `QTreeView::item { height: 28px; }` in `resources/style.qss`. In contrast, `QListView#ColumnViewPaneListView` (used in `ColumnViewWidget`) had no QSS item height specification and hardcoded `32px` in `ColumnItemDelegate::sizeHint`.

To eliminate this visual discrepancy and unify the row height, icon padding, and item styling between Column View and Directory Navigation:
1. Add `QListView#ColumnViewPaneListView::item { height: 28px; border: none; }` to `resources/style.qss`.
2. Update `ColumnItemDelegate::sizeHint` in `src/ui/ColumnItemDelegate.cpp` to return a height of `28px`.

## 2. Modified Files List
- `resources/style.qss`
- `src/ui/ColumnItemDelegate.cpp`

## 3. Detailed Line-by-Line Changes

### `resources/style.qss`
```diff
<<<<<<< SEARCH
QListView#ColumnViewPaneListView {
    background: transparent;
    border: none;
    color: #CCCCCC;
    outline: none;
}
=======
QListView#ColumnViewPaneListView {
    background: transparent;
    border: none;
    color: #CCCCCC;
    outline: none;
}

QListView#ColumnViewPaneListView::item {
    height: 28px;
    border: none;
}
>>>>>>> REPLACE
```

### `src/ui/ColumnItemDelegate.cpp`
```diff
<<<<<<< SEARCH
QSize ColumnItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = RenameCapableDelegate::sizeHint(option, index);
    sz.setHeight(32);
    return sz;
}
=======
QSize ColumnItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = RenameCapableDelegate::sizeHint(option, index);
    sz.setHeight(28);
    return sz;
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild application using CMake:
   `cmake --build build --config Debug`
2. Launch application and visually verify that item row heights in Column View match the Directory Navigation panel (28px height).
