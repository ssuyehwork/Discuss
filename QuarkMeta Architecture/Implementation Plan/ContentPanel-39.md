# Implementation Plan - ContentPanel Single DropTreeView Normalization (`ContentPanel-39.md`)

## Overview
This implementation plan normalizes the List View in `ContentPanel` by eliminating the dual tree view setup (`m_folderTreeView` and `m_fileTreeView`) along with external header widgets (`m_listFolderHeader` and `m_listFileHeader`).
`ContentPanel` now uses a **single `DropTreeView* m_treeView`** instance, matching the unified single-view design pattern of `JustifiedView`.

---

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h` (Physical removal of dual tree view members)

```
<<<<<<< SEARCH
    DropTreeView* m_treeView = nullptr;
    DropTreeView* m_folderTreeView = nullptr;
    DropTreeView* m_fileTreeView = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;
=======
    DropTreeView* m_treeView = nullptr;
>>>>>>> REPLACE
```

---

### 2. `src/ui/ContentPanel.cpp` (`setupListView` normalization)

```
<<<<<<< SEARCH
void ContentPanel::setupListView() {
    if (m_listContainerWidget) return;

    m_listContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_listContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listFolderHeader = new FolderSectionHeaderBar(m_listContainerWidget);
    layout->addWidget(m_listFolderHeader);

    m_folderTreeView = new DropTreeView(m_listContainerWidget);
    m_folderTreeView->setFrameShape(QFrame::NoFrame);
    m_folderTreeView->setAlternatingRowColors(true);
    m_folderTreeView->setSortingEnabled(true);
    m_folderTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_folderTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_folderTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderTreeView->setRootIsDecorated(false);
    m_folderTreeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_folderTreeView->setModel(m_folderProxyModel);
    m_folderTreeView->installEventFilter(this);
    m_folderTreeView->viewport()->installEventFilter(this);
    layout->addWidget(m_folderTreeView, 1);

    m_listFileHeader = new FileSectionHeaderBar(m_listContainerWidget);
    layout->addWidget(m_listFileHeader);

    m_fileTreeView = new DropTreeView(m_listContainerWidget);
    m_fileTreeView->setFrameShape(QFrame::NoFrame);
    m_fileTreeView->setAlternatingRowColors(true);
    m_fileTreeView->setSortingEnabled(true);
    m_fileTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_fileTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTreeView->setRootIsDecorated(false);
    m_fileTreeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_fileTreeView->setModel(m_fileProxyModel);
    m_fileTreeView->installEventFilter(this);
    m_fileTreeView->viewport()->installEventFilter(this);
    layout->addWidget(m_fileTreeView, 1);

    m_viewStack->addWidget(m_listContainerWidget);
}
=======
void ContentPanel::setupListView() {
    if (m_treeView) return;

    m_treeView = new DropTreeView(this);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_treeView->setModel(m_proxyModel);
    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);

    auto* header = m_treeView->header();
    if (header) {
        header->setSortIndicatorShown(true);
        header->setSectionsClickable(true);
    }
    m_treeView->applyColumnPolicies();

    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_treeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_treeView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_proxyModel);
    });

    if (m_treeView->verticalScrollBar()) {
        connect(m_treeView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_visibleTimer) m_visibleTimer->start();
        });
    }

    m_viewStack->addWidget(m_treeView);
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Compile the project:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. Verify that `ContentPanel` instantiates a single `DropTreeView` (`m_treeView`) in `ListView` mode with zero duplicate views or split scrollbars.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **Single View SSOT**: Aligns `ListView` with `JustifiedView` to use a single view instance per mode.
- **Physical Clean-up**: Eliminates redundant dual `DropTreeView` pointers and external header widgets.

---

## Header API Signature Verification Table

| File | Class / Member | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `ContentPanel` | `DropTreeView* dropTreeView() const { return m_treeView; }` | Verified 100% Match |
