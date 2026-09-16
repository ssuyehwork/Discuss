# ListViewCollapsibleContainer.md: ListView Collapsible Section Headers Implementation

## 1. Overview
This implementation plan adds collapsible section headers ("文件夹 (N)" and "文件 (M)") to List View mode (`ListView`), aligning its container layout with Column View (`ColumnView`).
Specifically:
1. `ContentPanel` initializes `m_listContainerWidget` containing `FolderSectionHeaderBar`, `m_folderTreeView` (`m_folderProxyModel`), `FileSectionHeaderBar`, and `m_treeView` (`m_fileProxyModel`).
2. `m_folderProxyModel` filters folders (`showFolders = true, showFiles = false`), while `m_fileProxyModel` filters files (`showFolders = false, showFiles = true`).
3. Toggling `FolderSectionHeaderBar` expands/collapses `m_folderTreeView`.
4. Selection and sorting synchronization is extended to cover both `m_folderProxyModel` and `m_fileProxyModel`.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentPanel.h`

```cpp
<<<<<<< SEARCH
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;

    QStackedWidget* m_viewStack = nullptr;
    QAbstractItemView* m_gridView = nullptr;
    DropTreeView* m_treeView = nullptr;
=======
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    DropTreeView* m_folderTreeView = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    QStackedWidget* m_viewStack = nullptr;
    QAbstractItemView* m_gridView = nullptr;
    DropTreeView* m_treeView = nullptr;
>>>>>>> REPLACE
```

---

### File 2: `src/ui/ContentPanel.cpp`

```cpp
<<<<<<< SEARCH
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);
    m_viewStack->addWidget(m_columnView);
=======
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_listContainerWidget ? m_listContainerWidget : static_cast<QWidget*>(m_treeView));
    m_viewStack->addWidget(m_columnView);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void ContentPanel::initListView() {
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
    header->setFixedHeight(32);
    header->setMinimumSectionSize(0);
    m_treeView->applyColumnPolicies();

    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_treeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_treeView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));

    if (m_treeView->verticalScrollBar()) {
        connect(m_treeView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_visibleTimer) m_visibleTimer->start();
        });
    }
}
=======
void ContentPanel::initListView() {
    m_listContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_listContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 1. 文件夹专用代理模型
    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);
    m_folderProxyModel->setFilterKeyColumn(0);
    m_folderProxyModel->setDynamicSortFilter(true);
    FilterState folderOnlyFilter = m_currentFilter;
    folderOnlyFilter.showFolders = true;
    folderOnlyFilter.showFiles = false;
    m_folderProxyModel->currentFilter = folderOnlyFilter;

    // 2. 文件专用代理模型
    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);
    m_fileProxyModel->setFilterKeyColumn(0);
    m_fileProxyModel->setDynamicSortFilter(true);
    FilterState fileOnlyFilter = m_currentFilter;
    fileOnlyFilter.showFolders = false;
    fileOnlyFilter.showFiles = true;
    m_fileProxyModel->currentFilter = fileOnlyFilter;

    // 3. 文件夹折叠标题栏
    m_listFolderHeader = new FolderSectionHeaderBar(m_listContainerWidget);
    m_listFolderHeader->hide();
    layout->addWidget(m_listFolderHeader);

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

    auto* fHeader = m_folderTreeView->header();
    fHeader->setFixedHeight(32);
    fHeader->setMinimumSectionSize(0);
    m_folderTreeView->applyColumnPolicies();

    connect(m_listFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderTreeView && m_listFolderHeader->count() > 0) {
            m_folderTreeView->setVisible(!collapsed);
        }
    });

    // 5. 文件分界标题栏
    m_listFileHeader = new FileSectionHeaderBar(m_listContainerWidget);
    m_listFileHeader->hide();
    layout->addWidget(m_listFileHeader);

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

    auto* header = m_treeView->header();
    header->setFixedHeight(32);
    header->setMinimumSectionSize(0);
    m_treeView->applyColumnPolicies();

    connect(m_folderTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_folderTreeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_folderTreeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_folderTreeView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));

    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_treeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_treeView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));

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
                int folderH = qMin(200, qMax(32, folderCount * 30 + 32));
                m_folderTreeView->setMaximumHeight(folderH);
            }
        }
        if (m_listFileHeader) {
            m_listFileHeader->setCount(fileCount);
            m_listFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
    };

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateListSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateListSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateListSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateListSectionCounts);

    if (m_treeView->verticalScrollBar()) {
        connect(m_treeView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_visibleTimer) m_visibleTimer->start();
        });
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Commands
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Verification Checklist
1. Switch to List View (`ListView`).
2. Verify that `FolderSectionHeaderBar` ("文件夹 (N)") and `FileSectionHeaderBar` ("文件 (M)") render above respective folder and file lists.
3. Click "文件夹 (N)" header bar to confirm folder list collapse/expand behavior.
4. Verify selection and context menu operations remain functional.
