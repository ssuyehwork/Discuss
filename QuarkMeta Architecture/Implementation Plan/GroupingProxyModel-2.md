# Implementation Plan - GroupingProxyModel-2.md

## 1. Overview
This implementation plan addresses essential proxy model method forwarding and cross-model index mapping fixes for `GroupingProxyModel` and `ContentPanel`.

Because `GroupingProxyModel` inherits directly from `QAbstractProxyModel`, Qt does not automatically forward `headerData()`, `sort()`, and `setData()` to `sourceModel()`. Without explicitly forwarding these API calls, column header titles disappear, sorting by header clicks is ignored, and inline renaming via `RenameCapableDelegate` fails.

Additionally, `restoreSelections()` and `refreshVisibleThumbnails()` in `ContentPanel.cpp` previously bypassed proxy index mapping, operating directly on `FilterProxyModel` indices while views were bound to `GroupingProxyModel`.

### Core Fixes & Architectural Enhancements:
1. **`GroupingProxyModel` API Passthroughs**:
   - `headerData(section, orientation, role)`: Forwards header data requests to `sourceModel()->headerData()`.
   - `sort(column, order)`: Forwards sorting requests to `sourceModel()->sort(column, order)`.
   - `setData(index, value, role)`: Maps proxy index via `mapToSource(index)` and forwards data changes to `sourceModel()->setData()`, restoring inline renaming functionality.
2. **`ContentPanel` Cross-Model Mapping Corrections**:
   - `restoreSelections()`: Uses `mapToActiveViewModelIndex(...)` to ensure indices passed to `selectionModel()->select()` align with the model bound to the active view.
   - `refreshVisibleThumbnails()`: Obtains source item records using `mapToSource(view->indexAt(...))` instead of mismatched raw row indices.

---

## 2. Modified Files List
- `src/ui/models/GroupingProxyModel.h`
- `src/ui/models/GroupingProxyModel.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/models/GroupingProxyModel.h`
Add function declarations for `headerData`, `sort`, and `setData` in `GroupingProxyModel`.

```
<<<<<<< SEARCH
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
=======
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
>>>>>>> REPLACE
```

---

### 2. `src/ui/models/GroupingProxyModel.cpp`
Implement `headerData`, `sort`, and `setData` in `GroupingProxyModel.cpp`.

```
<<<<<<< SEARCH
Qt::ItemFlags GroupingProxyModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    if (isGroupHeader(index)) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    QModelIndex srcIdx = mapToSource(index);
    return srcIdx.isValid() ? srcIdx.flags() : Qt::NoItemFlags;
}
=======
Qt::ItemFlags GroupingProxyModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    if (isGroupHeader(index)) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    QModelIndex srcIdx = mapToSource(index);
    return srcIdx.isValid() ? srcIdx.flags() : Qt::NoItemFlags;
}

QVariant GroupingProxyModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (sourceModel()) {
        return sourceModel()->headerData(section, orientation, role);
    }
    return QAbstractProxyModel::headerData(section, orientation, role);
}

void GroupingProxyModel::sort(int column, Qt::SortOrder order) {
    if (sourceModel()) {
        sourceModel()->sort(column, order);
    }
}

bool GroupingProxyModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || isGroupHeader(index)) {
        return false;
    }
    QModelIndex srcIdx = mapToSource(index);
    if (srcIdx.isValid() && sourceModel()) {
        return sourceModel()->setData(srcIdx, value, role);
    }
    return false;
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/ContentPanel.cpp`
Update `restoreSelections()` and `refreshVisibleThumbnails()` to map indices via `mapToActiveViewModelIndex`.

#### `restoreSelections()` Fix
```
<<<<<<< SEARCH
        QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
        if (proxyIdx.isValid()) {
            targetSelection.select(proxyIdx, proxyIdx);
        }
=======
        QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
        QModelIndex viewIdx = mapToActiveViewModelIndex(proxyIdx);
        if (viewIdx.isValid()) {
            targetSelection.select(viewIdx, viewIdx);
        }
>>>>>>> REPLACE
```

#### `refreshVisibleThumbnails()` Fix
```
<<<<<<< SEARCH
        QModelIndex idx = m_proxyModel->index(r, 0);
        ItemRecord rec = idx.data(Qt::UserRole).value<ItemRecord>();
=======
        QModelIndex viewIdx = activeView->model() ? activeView->model()->index(r, 0) : QModelIndex();
        QModelIndex srcIdx = (activeView->model() == m_groupingProxyModel)
                             ? m_groupingProxyModel->mapToSource(viewIdx)
                             : m_proxyModel->mapToSource(viewIdx);
        if (!srcIdx.isValid()) continue;
        ItemRecord rec = srcIdx.data(Qt::UserRole).value<ItemRecord>();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. **Compilation Check**:
   Run CMake build to verify clean compilation with `GroupingProxyModel` proxy method forwardings.
2. **Behavioral Verification**:
   - Column Header Titles: Confirm column headers display titles (`名称`, `大小`, `修改时间`, etc.).
   - Header Sorting: Click column headers and verify item sorting updates.
   - Inline Renaming: Rename a file or folder and press Enter; verify `model->setData()` commits changes cleanly.
   - Selection Restoration: Create a new folder or file and confirm it is automatically selected and brought into view.
   - Thumbnail Loading: Scroll through items in list view mode and verify thumbnails correspond to correct rows.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Qt Proxy Contract**: Restores standard Qt proxy model method forwarding (`headerData`, `sort`, `setData`).
- **Unified Index Mapping**: Reuses `ContentPanel::mapToActiveViewModelIndex` to guarantee consistent model index mapping across all view modes.

---

## 6. Header API Signature Verification Table

| Calling File | Target Class / Header | Function / Method Signature | Verification Status |
| :--- | :--- | :--- | :--- |
| `RenameCapableDelegate.cpp` | `QAbstractItemModel` | `bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole)` | Verified |
| `ContentHeaderView.cpp` | `QAbstractItemModel` | `QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const` | Verified |
| `ContentPanel.cpp` | `ContentPanel.h` | `QModelIndex mapToActiveViewModelIndex(const QModelIndex& index) const` | Verified |
