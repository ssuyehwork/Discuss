# FilterProxyModel-CrashFix Implementation Plan

## 1. Overview
This implementation plan addresses the application crash upon startup after loading the main window.

### Root Cause Analysis
In `FilterProxyModel.cpp`, the manual overrides for proxy mapping (`mapToSource`, `mapFromSource`, `index`, `rowCount`) created arbitrary `QModelIndex` instances via `createIndex(proxyIndex.row() - 1, proxyIndex.column())` and passed them directly to `QSortFilterProxyModel::mapToSource()`. Because `QSortFilterProxyModel` relies on internal pointer structures (`internalPointer()`) created during index mapping, passing index instances without internal pointers caused null-pointer dereferences in Qt's core proxy model mapping logic, leading to immediate application crashes upon population.

### Resolution Strategy
1. Remove dangerous custom proxy index overrides (`mapToSource`, `mapFromSource`, `index`, `rowCount`) from `FilterProxyModel`.
2. Revert `FilterProxyModel` to standard `QSortFilterProxyModel` subclassing, restoring full application stability without crashes.

## 2. Modified Files List
- `src/ui/models/FilterProxyModel.h`
- `src/ui/models/FilterProxyModel.cpp`
- `src/ui/ContentPanel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/models/FilterProxyModel.h`
Remove custom proxy mapping virtual method overrides.

```diff
<<<<<<< SEARCH
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
=======
>>>>>>> REPLACE
```

### 3.2 `src/ui/models/FilterProxyModel.cpp`
Remove unsafe custom `mapToSource`/`mapFromSource` index manipulation logic.

```diff
<<<<<<< SEARCH
int FilterProxyModel::rowCount(const QModelIndex& parent) const {
    int baseCount = QSortFilterProxyModel::rowCount(parent);
    if (!m_groupHeadersEnabled || parent.isValid()) {
        return baseCount;
    }
    return baseCount + (baseCount > 0 ? 1 : 0);
}

QModelIndex FilterProxyModel::index(int row, int column, const QModelIndex& parent) const {
    if (!m_groupHeadersEnabled || parent.isValid()) {
        return QSortFilterProxyModel::index(row, column, parent);
    }
    return createIndex(row, column);
}

QModelIndex FilterProxyModel::mapToSource(const QModelIndex& proxyIndex) const {
    if (!m_groupHeadersEnabled || !proxyIndex.isValid()) {
        return QSortFilterProxyModel::mapToSource(proxyIndex);
    }
    if (proxyIndex.row() == 0) return QModelIndex();
    return QSortFilterProxyModel::mapToSource(createIndex(proxyIndex.row() - 1, proxyIndex.column()));
}

QModelIndex FilterProxyModel::mapFromSource(const QModelIndex& sourceIndex) const {
    if (!m_groupHeadersEnabled || !sourceIndex.isValid()) {
        return QSortFilterProxyModel::mapFromSource(sourceIndex);
    }
    QModelIndex baseProxy = QSortFilterProxyModel::mapFromSource(sourceIndex);
    if (!baseProxy.isValid()) return QModelIndex();
    return createIndex(baseProxy.row() + 1, baseProxy.column());
}

QVariant FilterProxyModel::data(const QModelIndex& index, int role) const {
    if (m_groupHeadersEnabled && index.isValid() && index.row() == 0) {
        if (role == IsGroupHeaderRole) return true;
        if (role == Qt::DisplayRole) return QString("内容项目");
        if (role == TypeRole) return QString("folder_group_header");
        if (role == IsGroupCollapsedRole) return m_foldersCollapsed;
        return QVariant();
    }
    return QSortFilterProxyModel::data(index, role);
}
=======
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.cpp`
Remove group header call on `m_proxyModel`.

```diff
<<<<<<< SEARCH
void ContentPanel::initListView() {
    if (m_proxyModel) {
        m_proxyModel->setGroupHeadersEnabled(true);
    }
    m_treeView = new DropTreeView(this);
=======
void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build with CMake:
   ```bash
   cmake -B build -S .
   cmake --build build --config Debug
   ```
2. Launch `QuarkMeta` application.
3. Verify that the main window launches smoothly without crashes.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Restores standard Qt `QSortFilterProxyModel` architecture guarantees.
