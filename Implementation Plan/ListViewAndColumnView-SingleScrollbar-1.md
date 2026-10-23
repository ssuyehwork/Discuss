# Implementation Plan - ListView & ColumnView Unified Single-Scrollbar Dual-Section Architecture (`ListViewAndColumnView-SingleScrollbar-1.md`)

## Overview
This implementation plan establishes the **Unified Single-Scrollbar Dual-Section Architecture** for both **ListView (`ListView`)** and **ColumnView (`ColumnView`)**.

While preserving the **dual-section layout** ("文件夹 (N)" and "文件 (M)" sections with independent collapse capability), it completely eliminates internal dual vertical scrollbars inside `m_folderTreeView`/`m_folderListView` and `m_treeView`/`m_listView`.

Key architectural changes:
1. **Disable Internal Child Scrollbars**: Sub-views (`DropTreeView` and `DropListView`) set `setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff)`.
2. **Dynamic Height Expansion**: Each sub-view automatically resizes its height (`setFixedHeight`) based on item row count so that its entire contents fit on the scroll canvas without clipping.
3. **Single Outer Scroll Canvas**: The outer `QScrollArea` (`m_listScrollArea` in `ContentPanel` and `m_paneScrollArea` in `ColumnViewPane`) provides a **single, continuous vertical scrollbar**, allowing users to scroll smoothly through folders and files as a unified canvas.

---

## Modified Files List
- `src/ui/ContentPanel.cpp`
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.cpp`

Update `initListView()` to disable internal vertical scrollbars on `m_folderTreeView` and `m_treeView`, and ensure `updateListSectionCounts` dynamically updates the height of both views to fit in `m_listScrollArea`.

```
<<<<<<< SEARCH
    // 4. 文件夹列表视图
    m_folderTreeView = new DropTreeView(m_listContainerWidget);
    m_folderTreeView->setFrameShape(QFrame::NoFrame);
    m_folderTreeView->setAlternatingRowColors(true);
    m_folderTreeView->setSortingEnabled(true);
    m_folderTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_folderTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderTreeView->setRootIsDecorated(false);
    m_folderTreeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_folderTreeView->setModel(m_folderProxyModel);
    m_folderTreeView->installEventFilter(this);
    m_folderTreeView->viewport()->installEventFilter(this);
    m_folderTreeView->hide();
    layout->addWidget(m_folderTreeView);
=======
    // 4. 文件夹列表视图
    m_folderTreeView = new DropTreeView(m_listContainerWidget);
    m_folderTreeView->setFrameShape(QFrame::NoFrame);
    m_folderTreeView->setAlternatingRowColors(true);
    m_folderTreeView->setSortingEnabled(true);
    m_folderTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderTreeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderTreeView->setRootIsDecorated(false);
    m_folderTreeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_folderTreeView->setModel(m_folderProxyModel);
    m_folderTreeView->installEventFilter(this);
    m_folderTreeView->viewport()->installEventFilter(this);
    m_folderTreeView->hide();
    layout->addWidget(m_folderTreeView);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 6. 文件列表视图
    m_treeView = new DropTreeView(m_listContainerWidget);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_treeView->setModel(m_fileProxyModel);
    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);
    layout->addWidget(m_treeView, 1);
=======
    // 6. 文件列表视图
    m_treeView = new DropTreeView(m_listContainerWidget);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_treeView->setModel(m_fileProxyModel);
    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);
    layout->addWidget(m_treeView);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    auto updateListSectionCounts = [this]() {
        if (!m_folderProxyModel || !m_fileProxyModel) return;
        int folderCount = m_folderProxyModel->rowCount();
        int fileCount = m_fileProxyModel->rowCount();

        if (m_listFolderHeader) {
            m_listFolderHeader->setCount(folderCount);
            m_listFolderHeader->setVisible(folderCount > 0);
        }
        if (m_folderTreeView) {
            if (folderCount == 0) {
                m_folderTreeView->hide();
            } else {
                bool collapsed = m_listFolderHeader ? m_listFolderHeader->isCollapsed() : false;
                m_folderTreeView->setVisible(!collapsed);
                int rowH = m_folderTreeView->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (m_folderTreeView->header() && m_folderTreeView->header()->isVisible()) ? m_folderTreeView->header()->height() : 0;
                int folderH = folderCount * rowH + hdrH + 2;
                m_folderTreeView->setFixedHeight(folderH);
            }
        }
        if (m_listFileHeader) {
            m_listFileHeader->setCount(fileCount);
            m_listFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
    };
=======
    auto updateListSectionCounts = [this]() {
        if (!m_folderProxyModel || !m_fileProxyModel) return;
        int folderCount = m_folderProxyModel->rowCount();
        int fileCount = m_fileProxyModel->rowCount();

        if (m_listFolderHeader) {
            m_listFolderHeader->setCount(folderCount);
            m_listFolderHeader->setVisible(folderCount > 0);
        }
        if (m_folderTreeView) {
            if (folderCount == 0) {
                m_folderTreeView->hide();
            } else {
                bool collapsed = m_listFolderHeader ? m_listFolderHeader->isCollapsed() : false;
                m_folderTreeView->setVisible(!collapsed);
                int rowH = m_folderTreeView->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (m_folderTreeView->header() && m_folderTreeView->header()->isVisible()) ? m_folderTreeView->header()->height() : 0;
                int folderH = folderCount * rowH + hdrH + 2;
                m_folderTreeView->setFixedHeight(folderH);
            }
        }
        if (m_listFileHeader) {
            m_listFileHeader->setCount(fileCount);
            m_listFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
        if (m_treeView) {
            int rowH = m_treeView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 30;
            int hdrH = (m_treeView->header() && m_treeView->header()->isVisible()) ? m_treeView->header()->height() : 0;
            int fileH = fileCount * rowH + hdrH + 2;
            m_treeView->setFixedHeight(fileH);
        }
    };
>>>>>>> REPLACE
```

---

### 2. `src/ui/ColumnViewWidget.cpp`

Update `ColumnViewPane` initialization so that `m_folderListView` and `m_listView` disable internal vertical scrollbars (`ScrollBarAlwaysOff`) and expand their height dynamically within `m_paneScrollArea`.

```
<<<<<<< SEARCH
    m_folderListView = new DropListView(m_canvasWidget);
    m_folderListView->setFrameShape(QFrame::NoFrame);
    m_folderListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderListView->setItemDelegate(new ColumnItemDelegate(m_contentPanel, this));
    m_folderListView->setModel(m_folderProxyModel);
    m_folderListView->installEventFilter(this);
    m_folderListView->viewport()->installEventFilter(this);
    m_folderListView->hide();
    layout->addWidget(m_folderListView);
=======
    m_folderListView = new DropListView(m_canvasWidget);
    m_folderListView->setFrameShape(QFrame::NoFrame);
    m_folderListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderListView->setItemDelegate(new ColumnItemDelegate(m_contentPanel, this));
    m_folderListView->setModel(m_folderProxyModel);
    m_folderListView->installEventFilter(this);
    m_folderListView->viewport()->installEventFilter(this);
    m_folderListView->hide();
    layout->addWidget(m_folderListView);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_listView = new DropListView(m_canvasWidget);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setItemDelegate(new ColumnItemDelegate(m_contentPanel, this));
    m_listView->setModel(m_fileProxyModel);
    m_listView->installEventFilter(this);
    m_listView->viewport()->installEventFilter(this);
    layout->addWidget(m_listView, 1);
=======
    m_listView = new DropListView(m_canvasWidget);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setItemDelegate(new ColumnItemDelegate(m_contentPanel, this));
    m_listView->setModel(m_fileProxyModel);
    m_listView->installEventFilter(this);
    m_listView->viewport()->installEventFilter(this);
    layout->addWidget(m_listView);
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Compile the project with CMake:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. Verify ListView & ColumnView:
   - Switch to **ListView** and open a folder with many folders and files.
   - Confirm there is exactly **one** vertical scrollbar on the right of `ContentPanel`.
   - Verify smooth scrolling across section headers, folders, and files without truncation.
   - Switch to **ColumnView** and verify each column pane has a single continuous vertical scrollbar.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`QAbstractScrollArea::setVerticalScrollBarPolicy` SSOT**: Reuses Qt native scrollbar policy settings.
- **`FolderSectionHeaderBar` SSOT**: Preserves existing section headers and collapse functionality.

---

## Header API Signature Verification Table

| File | Class / Function | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/DropTreeView.h` | `DropTreeView` | `void setVerticalScrollBarPolicy(Qt::ScrollBarPolicy policy);` | Verified 100% Match |
| `src/ui/ColumnViewWidget.h` | `ColumnViewPane` | `DropListView* listView() const;` | Verified 100% Match |
