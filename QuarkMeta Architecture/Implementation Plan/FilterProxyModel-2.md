# FilterProxyModel-2 Implementation Plan

## 1. Overview
In accordance with the Single View Architecture (abandoning dual sub-views for folders and files), each view mode (List, Grid, Column) will use a **single unified FilterProxyModel** instead of maintaining two separate proxy instances (`folderProxyModel` and `fileProxyModel`).

To support collapsing the folder section while preserving all files in the unified view:
1. `FilterProxyModel` adds a `setFoldersCollapsed(bool)` interface.
2. In `filterAcceptsRow()`, if `m_foldersCollapsed == true` and `record.isDir == true`, the row is filtered out (hidden).
3. Files remain visible and naturally scroll up under a single scroll bar.
4. When `computer://` is the current path, drive entries are recognized as folders and respond to `showFolders` and `foldersCollapsed`.

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.h`
- `src/ui/models/FilterProxyModel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/models/FilterProxyModel.h`
Add `setFoldersCollapsed(bool)` and `isFoldersCollapsed() const` to `FilterProxyModel`.

```diff
<<<<<<< SEARCH
    void setSortType(int type) { m_sortType = type; invalidate(); }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; invalidate(); }


protected:
=======
    void setSortType(int type) { m_sortType = type; invalidate(); }
    void setSortOrder(Qt::SortOrder order) { m_sortOrder = order; invalidate(); }

    void setFoldersCollapsed(bool collapsed);
    bool isFoldersCollapsed() const { return m_foldersCollapsed; }

protected:
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    QSet<QString> m_cachedDuplicatePaths;
    int m_sortType = 0;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};
=======
    QSet<QString> m_cachedDuplicatePaths;
    int m_sortType = 0;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    bool m_foldersCollapsed = false;
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/models/FilterProxyModel.cpp`
Implement `setFoldersCollapsed()` and integrate with `filterAcceptsRow()`.

```diff
<<<<<<< SEARCH
void FilterProxyModel::setCachedDuplicatePaths(const QSet<QString>& paths) {
    if (m_cachedDuplicatePaths == paths) return;
    m_cachedDuplicatePaths = paths;
    updateFilter();
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
=======
void FilterProxyModel::setCachedDuplicatePaths(const QSet<QString>& paths) {
    if (m_cachedDuplicatePaths == paths) return;
    m_cachedDuplicatePaths = paths;
    updateFilter();
}

void FilterProxyModel::setFoldersCollapsed(bool collapsed) {
    if (m_foldersCollapsed == collapsed) return;
    m_foldersCollapsed = collapsed;
    invalidateFilter();
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // 🚀【此电脑根路径准则】：当加载“此电脑”(computer://)盘符列表时，盘符仅归属于驱动器/文件夹区，在“文件”区代理中严禁重复显示！
    if (sourceModelPtr->currentPath() == "computer://") {
        return currentFilter.showFolders;
    }

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    bool isTrashView = contentPanel && (contentPanel->getCurrentCategoryType() == "trash");

    // 0. 隐藏属性过滤
    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    // 1. 文件夹与文件显隐控制 (showFolders/showFiles 为顶栏切换按钮的绝对关断最高优先级)
    if (!isTrashView) {
        if (record.isDir) {
            if (!currentFilter.showFolders) return false;
        } else {
            if (!currentFilter.showFiles) return false;
        }
    }
=======
    // 🚀【此电脑根路径准则】：当加载“此电脑”(computer://)盘符列表时，盘符归属于驱动器/文件夹
    if (sourceModelPtr->currentPath() == "computer://") {
        if (m_foldersCollapsed) return false;
        return currentFilter.showFolders;
    }

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    bool isTrashView = contentPanel && (contentPanel->getCurrentCategoryType() == "trash");

    // 0. 隐藏属性过滤
    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    // 1. 文件夹与文件显隐控制 (showFolders/showFiles 为顶栏切换按钮的绝对关断最高优先级)
    if (!isTrashView) {
        if (record.isDir) {
            if (!currentFilter.showFolders || m_foldersCollapsed) return false;
        } else {
            if (!currentFilter.showFiles) return false;
        }
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure and compile using CMake:
   ```powershell
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. Verify that `FilterProxyModel` compiles without warnings or missing symbol errors.
3. Test that calling `setFoldersCollapsed(true)` causes `filterAcceptsRow()` to reject all `record.isDir == true` rows while leaving file rows intact.
4. Verify that sorting via `lessThan()` continues to maintain directories strictly above files via the existing priority rule.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses the existing `filterAcceptsRow()` and `invalidateFilter()` mechanism.
- Avoids splitting into dual models for filtering.
- Preserves the existing `lessThan()` directory-first priority wall.

## 6. Header API Signature Verification
- `QSortFilterProxyModel::invalidateFilter()`: Verified standard Qt 6 API.
- `FilterProxyModel::setFoldersCollapsed(bool)`: New void setter.
- `FilterProxyModel::isFoldersCollapsed() const`: New boolean getter.

## 7. Header Inclusion Chain & Type Completeness Check
- No `#include` directives are added or removed in `FilterProxyModel.h`.
- `ItemRecord` and `FilterPanel.h` definitions remain intact.
