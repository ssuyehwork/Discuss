# Implementation Plan - ColumnViewWidget Single Click & No Edit Triggers Fix

## Overview
Disable default inline edit triggers (`setEditTriggers(QAbstractItemView::NoEditTriggers)`) on `QListView` instances in `ColumnViewPane` to prevent double-clicking or clicking folders from opening in-line rename edit boxes. Change item selection in `ColumnViewPane` so that clicking a folder immediately triggers sub-column expansion on the right side.

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

## Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`

```diff
<<<<<<< SEARCH
    m_listView = new QListView(this);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new TreeItemDelegate(this, false, false));
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: none; border-right: 1px solid #2D2D2D; color: #CCCCCC; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; }");
    layout->addWidget(m_listView);

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
=======
    m_listView = new QListView(this);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new TreeItemDelegate(this, false, false));
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: none; border-right: 1px solid #2D2D2D; color: #CCCCCC; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; }");
    layout->addWidget(m_listView);

    connect(m_listView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder");
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderSelected(itemPath, paneIdx);
        }
    });

    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder");
        int paneIdx = property("paneIndex").toInt();
        if (!isDir) {
            emit fileSelected(itemPath, paneIdx);
        }
    });
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile project:
   `cmake -B build -S . && cmake --build build --config Release`
2. Run application and switch to Column View mode.
3. Click on folders to verify right-hand sub-columns expand immediately without bringing up inline edit text boxes.
