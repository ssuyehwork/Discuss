# Implementation Plan: ColumnViewWidget Role Mapping Fix

## 1. Overview
This implementation plan fixes a critical bug where clicking a folder in `ColumnViewPane` failed to expand sub-columns. `ColumnViewPane` previously queried `Qt::UserRole + 1` (IdRole) and `Qt::UserRole + 2` (NameRole) which return empty/invalid variants for `DiskItemModel`. Updating the queries to `PathRole` (`Qt::UserRole + 3`) and `TypeRole` (`Qt::UserRole + 0`) allows `ColumnViewPane` to correctly read the directory path and type ("folder"), emitting `folderSelected` and appending the next column.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(Qt::UserRole + 1).toString();
        bool isDir = index.data(Qt::UserRole + 2).toBool();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        } else {
            emit fileSelected(itemPath, paneIdx);
        }
    });
=======
    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder");
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        } else {
            emit fileSelected(itemPath, paneIdx);
        }
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    if (!m_proxyModel) return;
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        if (idx.data(Qt::UserRole + 1).toString() == targetPath) {
            m_listView->setCurrentIndex(idx);
            m_listView->scrollTo(idx);
            break;
        }
    }
}
=======
void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    if (!m_proxyModel) return;
    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        QModelIndex idx = m_proxyModel->index(r, 0);
        if (idx.data(PathRole).toString() == targetPath) {
            m_listView->setCurrentIndex(idx);
            m_listView->scrollTo(idx);
            break;
        }
    }
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Recompile target with CMake.
2. In Column View, single click any folder in the column.
3. Confirm that a new column opens to the right showing the contents of the clicked folder.
