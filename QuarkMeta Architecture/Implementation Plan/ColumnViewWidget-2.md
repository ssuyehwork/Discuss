# Implementation Plan: ColumnViewWidget Double-Click Interaction Fix

## 1. Overview
This implementation plan changes the folder expansion and file activation trigger in `ColumnViewPane` from `QListView::clicked` (single click) to `QListView::doubleClicked` (double click). Single clicking an item will now perform normal selection highlighting, while double clicking a folder expands the sub-column on the right.

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
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
=======
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
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

## 4. Build & Verification Steps
1. Recompile target with CMake.
2. In Column View, single click a folder or file item. Confirm it only highlights the row without opening sub-columns.
3. Double click a folder item. Confirm it expands the next sub-column to the right.
