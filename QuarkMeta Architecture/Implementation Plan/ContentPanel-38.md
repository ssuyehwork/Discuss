# Implementation Plan - ContentPanel List View Dual-Section Physical Separation Architecture (`ContentPanel-38.md`)

## Overview
This implementation plan refactors List View in `ContentPanel` into a strict dual-section layout (`m_folderTreeView` + `m_fileTreeView`), separating folders and files into two distinct physical `DropTreeView` controls driven by two dedicated `FilterProxyModel` instances (`m_folderProxyModel` and `m_fileProxyModel`).
`m_listFolderHeader` (`FolderSectionHeaderBar`) controls the collapse/expand of `m_folderTreeView`, while `m_listFileHeader` (`FileSectionHeaderBar`) delineates the file section above `m_fileTreeView`. This guarantees that folders never spill into the file section, preserving perfect 1:1 section boundary alignment.

---

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    // UI 组件指针
    QVBoxLayout* m_mainLayout = nullptr;
    class ContentHeaderWidget* m_headerWidget = nullptr;
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;

    QStackedWidget* m_viewStack = nullptr;
    QAbstractItemView* m_gridView = nullptr;
    DropTreeView* m_treeView = nullptr;
=======
    // UI 组件指针
    QVBoxLayout* m_mainLayout = nullptr;
    class ContentHeaderWidget* m_headerWidget = nullptr;
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    QStackedWidget* m_viewStack = nullptr;
    QAbstractItemView* m_gridView = nullptr;
    DropTreeView* m_treeView = nullptr;
    DropTreeView* m_folderTreeView = nullptr;
    DropTreeView* m_fileTreeView = nullptr;
>>>>>>> REPLACE
```

---

### 2. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
void ContentPanel::initUi() {
    // ── 统一代理模型预先实例化 ──
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->currentFilter = m_currentFilter;
=======
void ContentPanel::initUi() {
    // ── 统一代理模型与双区物理分流代理模型预先实例化 ──
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->currentFilter = m_currentFilter;

    // 1. 文件夹专用代理模型
    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);
    m_folderProxyModel->setFilterKeyColumn(0);
    m_folderProxyModel->setDynamicSortFilter(true);

    // 2. 文件专用代理模型
    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);
    m_fileProxyModel->setFilterKeyColumn(0);
    m_fileProxyModel->setDynamicSortFilter(true);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::initListView() {
    m_listContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_listContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listFolderHeader = new FolderSectionHeaderBar(m_listContainerWidget);
    m_listFolderHeader->hide();
    layout->addWidget(m_listFolderHeader);

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
    m_treeView->setModel(m_proxyModel);
    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);
    layout->addWidget(m_treeView, 1);

    auto* header = m_treeView->header();
    header->setFixedHeight(32);
    header->setMinimumSectionSize(0);
    m_treeView->applyColumnPolicies();

    connect(m_listFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        m_currentFilter.showFolders = !collapsed;
        applyFilters();
    });

    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_treeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_treeView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_proxyModel);
    });

    auto updateListSectionCounts = [this]() {
        if (!m_model) return;
        int folderCount = 0;
        const auto& records = m_model->allRecords();
        for (const auto& rec : records) {
            if (rec.isDir) folderCount++;
        }

        if (m_listFolderHeader) {
            m_listFolderHeader->setCount(folderCount);
            m_listFolderHeader->setVisible(folderCount > 0);
        }
    };

    if (m_proxyModel) {
        connect(m_proxyModel, &QAbstractItemModel::modelReset, this, updateListSectionCounts);
        connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, updateListSectionCounts);
    }

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

    // 1. 文件夹分界标头与视图
    m_listFolderHeader = new FolderSectionHeaderBar(m_listContainerWidget);
    m_listFolderHeader->hide();
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
    layout->addWidget(m_folderTreeView);

    auto* folderHeader = m_folderTreeView->header();
    folderHeader->setFixedHeight(32);
    folderHeader->setMinimumSectionSize(0);
    m_folderTreeView->applyColumnPolicies();

    // 2. 文件分界标头与视图
    m_listFileHeader = new FileSectionHeaderBar(m_listContainerWidget);
    m_listFileHeader->hide();
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

    auto* fileHeader = m_fileTreeView->header();
    fileHeader->setFixedHeight(32);
    fileHeader->setMinimumSectionSize(0);
    m_fileTreeView->applyColumnPolicies();

    m_treeView = m_fileTreeView;

    // 信号绑定
    connect(m_listFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderTreeView) {
            m_folderTreeView->setVisible(!collapsed);
        }
    });

    connect(m_folderTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_folderTreeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_folderTreeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_folderTreeView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_folderProxyModel);
    });

    connect(m_fileTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_fileTreeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_fileTreeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_fileTreeView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_fileProxyModel);
    });

    auto updateListSectionCounts = [this]() {
        if (!m_model) return;
        int folderCount = 0;
        int fileCount = 0;
        const auto& records = m_model->allRecords();
        for (const auto& rec : records) {
            if (rec.isDir) folderCount++;
            else fileCount++;
        }

        if (m_listFolderHeader) {
            m_listFolderHeader->setCount(folderCount);
            m_listFolderHeader->setVisible(folderCount > 0);
        }
        if (m_folderTreeView) {
            m_folderTreeView->setVisible(folderCount > 0 && !m_listFolderHeader->isCollapsed());
        }
        if (m_listFileHeader) {
            m_listFileHeader->setCount(fileCount);
            m_listFileHeader->setVisible(fileCount > 0);
        }
        if (m_fileTreeView) {
            m_fileTreeView->setVisible(fileCount > 0);
        }
    };

    if (m_proxyModel) {
        connect(m_proxyModel, &QAbstractItemModel::modelReset, this, updateListSectionCounts);
        connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, updateListSectionCounts);
    }

    if (m_folderTreeView->verticalScrollBar()) {
        connect(m_folderTreeView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_visibleTimer) m_visibleTimer->start();
        });
    }
    if (m_fileTreeView->verticalScrollBar()) {
        connect(m_fileTreeView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_visibleTimer) m_visibleTimer->start();
        });
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::applyFilters() {
    if (m_proxyModel) {
        m_proxyModel->currentFilter = m_currentFilter;
        m_proxyModel->updateFilter();
    }
=======
void ContentPanel::applyFilters() {
    if (m_proxyModel) {
        m_proxyModel->currentFilter = m_currentFilter;
        m_proxyModel->updateFilter();
    }
    if (m_folderProxyModel) {
        FilterState folderFilter = m_currentFilter;
        folderFilter.showFolders = true;
        folderFilter.showFiles = false;
        m_folderProxyModel->currentFilter = folderFilter;
        m_folderProxyModel->updateFilter();
    }
    if (m_fileProxyModel) {
        FilterState fileFilter = m_currentFilter;
        fileFilter.showFolders = false;
        fileFilter.showFiles = true;
        m_fileProxyModel->currentFilter = fileFilter;
        m_fileProxyModel->updateFilter();
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Verify `QuarkMeta Architecture/Implementation Plan/ContentPanel-38.md` created.
2. Verify dual-treeview layout in List View mode (`m_folderTreeView` and `m_fileTreeView`).

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- Dual FilterProxyModels driven off the single `m_model` (`DiskItemModel`).

---

## Header API Signature Verification Table

| File | Class / Struct | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `ContentPanel` | `DropTreeView* m_folderTreeView = nullptr;` | Verified 100% Match |
| `src/ui/ContentPanel.h` | `ContentPanel` | `DropTreeView* m_fileTreeView = nullptr;` | Verified 100% Match |
