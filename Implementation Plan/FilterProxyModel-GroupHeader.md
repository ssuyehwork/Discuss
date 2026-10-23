# FilterProxyModel-GroupHeader Implementation Plan

## 1. Overview
This implementation plan addresses the root cause of why section headers ("文件夹 (N)" and "文件 (M)") were missing in List View (`DropTreeView`):
While `DropTreeView` and `TreeItemDelegate` had rendering and column-spanning logic for `IsGroupHeaderRole`, the underlying model layer (`FilterProxyModel` / `DiskItemModel`) was a flat proxy model that contained no virtual sentinel header rows (`IsGroupHeaderRole == true`).

To solve this natively in single-view architecture:
1. `FilterProxyModel` introduces optional Grouping Mode (`setGroupHeadersEnabled(bool)`).
2. When group headers are enabled, `FilterProxyModel` maps virtual header rows for "文件夹 (N)" and "文件 (M)" at row indices preceding folders and files respectively.
3. Collapsing a group header in `DropTreeView` via `setData(IsGroupCollapsedRole)` toggles visibility of that group's child items within `FilterProxyModel::filterAcceptsRow`.

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.h`
- `src/ui/models/FilterProxyModel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/models/FilterProxyModel.h`
Add group header state management, virtual row mapping, and role query handlers.

```diff
<<<<<<< SEARCH
    void setSortType(int type) { m_sortType = type; invalidate(); }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; invalidate(); }


protected:
=======
    void setSortType(int type) { m_sortType = type; invalidate(); }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; invalidate(); }

    void setGroupHeadersEnabled(bool enabled);
    bool groupHeadersEnabled() const { return m_groupHeadersEnabled; }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

protected:
>>>>>>> REPLACE
```

### 3.2 `src/ui/models/FilterProxyModel.cpp`
Implement group collapse state toggling and header row data mapping.

```diff
<<<<<<< SEARCH
FilterProxyModel::FilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}
=======
FilterProxyModel::FilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

void FilterProxyModel::setGroupHeadersEnabled(bool enabled) {
    if (m_groupHeadersEnabled != enabled) {
        m_groupHeadersEnabled = enabled;
        invalidateFilter();
    }
}

bool FilterProxyModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role == IsGroupCollapsedRole) {
        if (index.data(TypeRole).toString() == "folder_group_header") {
            m_foldersCollapsed = value.toBool();
            invalidateFilter();
            return true;
        } else if (index.data(TypeRole).toString() == "file_group_header") {
            m_filesCollapsed = value.toBool();
            invalidateFilter();
            return true;
        }
    }
    return QSortFilterProxyModel::setData(index, value, role);
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build with CMake:
   ```bash
   cmake -B build -S .
   cmake --build build --config Debug
   ```
2. Enable group headers on `m_proxyModel` in `ContentPanel` for List View.
3. Launch `QuarkMeta` in List View mode and confirm "文件夹 (N)" and "文件 (M)" group headers appear above folder and file sections.
4. Click group headers to verify that items collapse and expand smoothly.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `IsGroupHeaderRole` and `IsGroupCollapsedRole` defined in `src/core/ModelContract.h`.
- Operates directly within `FilterProxyModel` without creating separate models or split views.

## 6. Header API Signature Verification
- `QSortFilterProxyModel::setData(const QModelIndex&, const QVariant&, int)`: Standard Qt signature verified.
- `FilterProxyModel::setGroupHeadersEnabled(bool)`: New API added cleanly without altering public headers of other components.
