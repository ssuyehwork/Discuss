# QuarkMeta 实施方案：统一视图整页流式滚动架构 (ContentPanel)

## 1. Overview（概述与解决的问题）

### 解决的核心问题
在列视图（ColumnView）中，用户滚动鼠标滚轮时，整列像网页长页面一样整体流动，顶部的【文件夹折叠条】会自然随内容一同向上移动并滚出视口。
但在当前代码的**列表视图（ListView）**与**网格/流式视图（GridView/JustifiedView）**中，折叠组件（`FolderSectionHeaderBar` 与 `FileSectionHeaderBar`）被生硬钉死在外层 `QVBoxLayout` 静态布局上，且子视图被强制限高（200px / 局部卡片高）并在内部独立滚动，导致滚轮滚动时折叠栏死死定在屏幕上纹丝不动，严重割裂了用户的浏览体验。

### 架构治理目标
以列视图的优秀体验为唯一真理源，对列表视图与网格/流式视图进行统一流式改造：
1. **引入统一滚动视口（Unified QScrollArea）**：将静态容器升级为可平滑垂直滚动的视口容器；
2. **剔除强制限高，子视图交出私有滚动权**：关闭子列表/网格内部的垂直滚动条，根据项目数量 100% 物理自适应撑开高度；
3. **折叠组件随流位移**：用户向下滚动滚轮时，【文件夹折叠条】与文件夹项一并平滑向上移出视口，中间的分界栏平滑向上推移，完全达到与列视图完全一致的整页自然滚动效果。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准代码替换块）

### 修改文件 1：`src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    DropTreeView* m_folderTreeView = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    QWidget* m_gridContainerWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    DropJustifiedView* m_folderGridView = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;
=======
    QScrollArea* m_listScrollArea = nullptr;
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    DropTreeView* m_folderTreeView = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    QScrollArea* m_gridScrollArea = nullptr;
    QWidget* m_gridContainerWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    DropJustifiedView* m_folderGridView = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;
>>>>>>> REPLACE
```

---

### 修改文件 2：`src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QHeaderView>
#include <QScrollBar>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QSignalBlocker>
=======
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QHeaderView>
#include <QScrollBar>
#include <QScrollArea>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QSignalBlocker>
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_viewStack->addWidget(m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView));
    m_viewStack->addWidget(m_listContainerWidget ? m_listContainerWidget : static_cast<QWidget*>(m_treeView));
    m_viewStack->addWidget(m_columnView);
    m_viewStack->setCurrentWidget(m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView));
=======
    m_viewStack->addWidget(m_gridScrollArea ? static_cast<QWidget*>(m_gridScrollArea) : static_cast<QWidget*>(m_gridView));
    m_viewStack->addWidget(m_listScrollArea ? static_cast<QWidget*>(m_listScrollArea) : static_cast<QWidget*>(m_treeView));
    m_viewStack->addWidget(m_columnView);
    m_viewStack->setCurrentWidget(m_gridScrollArea ? static_cast<QWidget*>(m_gridScrollArea) : static_cast<QWidget*>(m_gridView));
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::initGridView() {
    m_gridContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_gridContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 1. 文件夹折叠标题栏
    m_gridFolderHeader = new FolderSectionHeaderBar(m_gridContainerWidget);
    m_gridFolderHeader->hide();
    layout->addWidget(m_gridFolderHeader);

    // 2. 文件夹专用网格视图
    m_folderGridView = new DropJustifiedView(m_gridContainerWidget);
    m_folderGridView->setFrameShape(QFrame::NoFrame);
    m_folderGridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderGridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderGridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderGridView->setModel(m_folderProxyModel);
    auto* fJustifiedView = qobject_cast<JustifiedView*>(m_folderGridView);
    if (fJustifiedView) {
        fJustifiedView->setAspectRatioRole(AspectRatioRole);
        auto* fDelegate = new ThumbnailDelegate(this);
        fDelegate->setHasThumbnailRole(HasThumbnailRole);
        fDelegate->setRatingRole(RatingRole);
        fDelegate->setPathRole(PathRole);
        fDelegate->setPinnedRole(PinnedRole);
        fDelegate->setTypeRole(TypeRole);
        fDelegate->setIsEmptyRole(IsEmptyRole);
        fDelegate->setColorRole(ColorRole);
        m_folderGridView->setItemDelegate(fDelegate);
    }
    m_folderGridView->installEventFilter(this);
    m_folderGridView->viewport()->installEventFilter(this);
    m_folderGridView->hide();
    layout->addWidget(m_folderGridView);

    connect(m_gridFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderGridView && m_gridFolderHeader->count() > 0) {
            m_folderGridView->setVisible(!collapsed);
        }
    });

    // 3. 文件分界标题栏
    m_gridFileHeader = new FileSectionHeaderBar(m_gridContainerWidget);
    m_gridFileHeader->hide();
    layout->addWidget(m_gridFileHeader);

    // 4. 普通文件网格视图
    m_gridView = new DropJustifiedView(m_gridContainerWidget);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setModel(m_fileProxyModel);

    auto* justifiedView = qobject_cast<JustifiedView*>(m_gridView);
    if (justifiedView) {
        justifiedView->setAspectRatioRole(AspectRatioRole);
        auto* delegate = new ThumbnailDelegate(this);
        delegate->setHasThumbnailRole(HasThumbnailRole);
        delegate->setRatingRole(RatingRole);
        delegate->setPathRole(PathRole);
        delegate->setPinnedRole(PinnedRole);
        delegate->setTypeRole(TypeRole);
        delegate->setIsEmptyRole(IsEmptyRole);
        delegate->setColorRole(ColorRole);
        m_gridView->setItemDelegate(delegate);
    }

    m_gridView->installEventFilter(this);
    m_gridView->viewport()->installEventFilter(this);
    layout->addWidget(m_gridView, 1);

    connect(m_folderGridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_folderGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_folderGridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_folderGridView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));

    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_gridView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));

    auto updateGridSectionCounts = [this]() {
        if (!m_folderProxyModel || !m_fileProxyModel) return;
        int folderCount = m_folderProxyModel->rowCount();
        int fileCount = m_fileProxyModel->rowCount();

        if (m_gridFolderHeader) {
            m_gridFolderHeader->setCount(folderCount);
            m_gridFolderHeader->setVisible(folderCount > 0);
        }
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                int cardH = m_zoomLevel + CardLayoutEngine::extraHeight() + 20;
                m_folderGridView->setMaximumHeight(cardH);
            }
        }
        if (m_gridFileHeader) {
            m_gridFileHeader->setCount(fileCount);
            m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
    };

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
}
=======
void ContentPanel::initGridView() {
    m_gridScrollArea = new QScrollArea(this);
    m_gridScrollArea->setObjectName("ContentGridScrollArea");
    m_gridScrollArea->setFrameShape(QFrame::NoFrame);
    m_gridScrollArea->setWidgetResizable(true);
    m_gridScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_gridContainerWidget = new QWidget(m_gridScrollArea);
    m_gridContainerWidget->setObjectName("ContentGridCanvasWidget");
    auto* layout = new QVBoxLayout(m_gridContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 1. 文件夹折叠标题栏
    m_gridFolderHeader = new FolderSectionHeaderBar(m_gridContainerWidget);
    m_gridFolderHeader->hide();
    layout->addWidget(m_gridFolderHeader);

    // 2. 文件夹专用网格视图（关闭私有垂直滚动，自适应撑开高度）
    m_folderGridView = new DropJustifiedView(m_gridContainerWidget);
    m_folderGridView->setFrameShape(QFrame::NoFrame);
    m_folderGridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderGridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderGridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderGridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderGridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderGridView->setModel(m_folderProxyModel);
    auto* fJustifiedView = qobject_cast<JustifiedView*>(m_folderGridView);
    if (fJustifiedView) {
        fJustifiedView->setAspectRatioRole(AspectRatioRole);
        auto* fDelegate = new ThumbnailDelegate(this);
        fDelegate->setHasThumbnailRole(HasThumbnailRole);
        fDelegate->setRatingRole(RatingRole);
        fDelegate->setPathRole(PathRole);
        fDelegate->setPinnedRole(PinnedRole);
        fDelegate->setTypeRole(TypeRole);
        fDelegate->setIsEmptyRole(IsEmptyRole);
        fDelegate->setColorRole(ColorRole);
        m_folderGridView->setItemDelegate(fDelegate);
    }
    m_folderGridView->installEventFilter(this);
    m_folderGridView->viewport()->installEventFilter(this);
    m_folderGridView->hide();
    layout->addWidget(m_folderGridView);

    connect(m_gridFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderGridView && m_gridFolderHeader->count() > 0) {
            m_folderGridView->setVisible(!collapsed);
        }
    });

    // 3. 文件分界标题栏
    m_gridFileHeader = new FileSectionHeaderBar(m_gridContainerWidget);
    m_gridFileHeader->hide();
    layout->addWidget(m_gridFileHeader);

    // 4. 普通文件网格视图（关闭私有垂直滚动，自适应撑开高度）
    m_gridView = new DropJustifiedView(m_gridContainerWidget);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridView->setModel(m_fileProxyModel);

    auto* justifiedView = qobject_cast<JustifiedView*>(m_gridView);
    if (justifiedView) {
        justifiedView->setAspectRatioRole(AspectRatioRole);
        auto* delegate = new ThumbnailDelegate(this);
        delegate->setHasThumbnailRole(HasThumbnailRole);
        delegate->setRatingRole(RatingRole);
        delegate->setPathRole(PathRole);
        delegate->setPinnedRole(PinnedRole);
        delegate->setTypeRole(TypeRole);
        delegate->setIsEmptyRole(IsEmptyRole);
        delegate->setColorRole(ColorRole);
        m_gridView->setItemDelegate(delegate);
    }

    m_gridView->installEventFilter(this);
    m_gridView->viewport()->installEventFilter(this);
    layout->addWidget(m_gridView);

    m_gridScrollArea->setWidget(m_gridContainerWidget);

    connect(m_folderGridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_folderGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_folderGridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_folderGridView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));

    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_gridView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));

    auto updateGridSectionCounts = [this]() {
        if (!m_folderProxyModel || !m_fileProxyModel) return;
        int folderCount = m_folderProxyModel->rowCount();
        int fileCount = m_fileProxyModel->rowCount();

        if (m_gridFolderHeader) {
            m_gridFolderHeader->setCount(folderCount);
            m_gridFolderHeader->setVisible(folderCount > 0);
        }
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                // 自适应撑开网格高度，彻底消灭局部滚动条
                auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView);
                int desiredH = fjv ? fjv->sizeHint().height() : (folderCount * (m_zoomLevel + CardLayoutEngine::extraHeight() + 10));
                m_folderGridView->setFixedHeight(qMax(60, desiredH));
            }
        }
        if (m_gridFileHeader) {
            m_gridFileHeader->setCount(fileCount);
            m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
        if (m_gridView) {
            auto* jv = qobject_cast<JustifiedView*>(m_gridView);
            int fileH = jv ? jv->sizeHint().height() : (fileCount * (m_zoomLevel + CardLayoutEngine::extraHeight() + 10));
            m_gridView->setFixedHeight(qMax(100, fileH));
        }
        if (m_gridContainerWidget) {
            m_gridContainerWidget->adjustSize();
        }
    };

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);

    if (m_gridScrollArea->verticalScrollBar()) {
        connect(m_gridScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_visibleTimer) m_visibleTimer->start();
        });
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
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
=======
void ContentPanel::initListView() {
    m_listScrollArea = new QScrollArea(this);
    m_listScrollArea->setObjectName("ContentListScrollArea");
    m_listScrollArea->setFrameShape(QFrame::NoFrame);
    m_listScrollArea->setWidgetResizable(true);
    m_listScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_listContainerWidget = new QWidget(m_listScrollArea);
    m_listContainerWidget->setObjectName("ContentListCanvasWidget");
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

    // 4. 文件夹列表视图（关闭私有垂直滚动，自适应撑开高度）
    m_folderTreeView = new DropTreeView(m_listContainerWidget);
    m_folderTreeView->setFrameShape(QFrame::NoFrame);
    m_folderTreeView->setAlternatingRowColors(true);
    m_folderTreeView->setSortingEnabled(true);
    m_folderTreeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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

    // 6. 文件列表视图（关闭私有垂直滚动，自适应撑开高度）
    m_treeView = new DropTreeView(m_listContainerWidget);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_treeView->setModel(m_fileProxyModel);
    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);
    layout->addWidget(m_treeView);

    auto* header = m_treeView->header();
    header->setFixedHeight(32);
    header->setMinimumSectionSize(0);
    m_treeView->applyColumnPolicies();

    m_listScrollArea->setWidget(m_listContainerWidget);

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
                // 自适应撑开物理高度（每行 30px + 表头 32px），彻底消灭 200px 盒内局域滚动
                int folderH = folderCount * 30 + 32;
                m_folderTreeView->setFixedHeight(folderH);
            }
        }
        if (m_listFileHeader) {
            m_listFileHeader->setCount(fileCount);
            m_listFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
        if (m_treeView) {
            int fileH = qMax(64, fileCount * 30 + 32);
            m_treeView->setFixedHeight(fileH);
        }
        if (m_listContainerWidget) {
            m_listContainerWidget->adjustSize();
        }
    };

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateListSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateListSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateListSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateListSectionCounts);

    if (m_listScrollArea->verticalScrollBar()) {
        connect(m_listScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_visibleTimer) m_visibleTimer->start();
        });
    }
}
```

---

## 4. Build & Verification Steps（编译与验证方法）

### 编译命令
在项目根目录下，使用 CMake 与 MSVC 进行 Release 构建：
```bash
cmake --build . --config Release --target QuarkMeta
```

### 交互验证标准（100% 对标列视图）
1. **列表视图（ListView）验证**：
   - 导航至同时包含大量文件夹和文件的目录（如 `src` 或根目录）；
   - 确认顶部出现【文件夹 (xx) ⌄】，中间出现【文件 (xx)】；
   - **在屏幕任何区域滚动滚轮**：
     - 【文件夹 (xx) ⌄】折叠条随内容一同平滑向上平移，滚出视口顶部；
     - 随着滚动向下推进，中间的【文件 (xx)】分界条平滑移入视野并继续向上滚走；
     - 整个列表只有一个右侧主垂直滚动条，绝无 200px 局部小滚动框。
2. **网格与流式视图（GridView / JustifiedView）验证**：
   - 切换到网格视图，向下滚动滚轮；
   - 确认【文件夹 (xx) ⌄】与文件夹卡片一同向上滑出视口，行为与列视图和列表视图完全一致。
3. **折叠行为联动验证**：
   - 点击【文件夹 (xx) ⌄】的折叠按钮收起文件夹；
   - 确认文件夹区域瞬间收拢，下方的【文件】区域平滑向上吸顶，主滚动条范围自适应更新。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（真理源复用与防另起炉灶自查）
- **复用既有代理模型体系**：本次方案完整沿用 `m_folderProxyModel` 与 `m_fileProxyModel`，未另起炉灶建立任何新模型；
- **复用既有折叠组件与代理**：完整复用 `FolderSectionHeaderBar`、`FileSectionHeaderBar` 与 `ThumbnailDelegate` / `TreeItemDelegate`；
- **彻底清除分散的局部限高死代码**：彻底物理移除了 `qMin(200, ...)` 这一破坏全局滚动流的硬编码代码。

---

## 6. Header API Signature Verification（物理签名核查表）

| 调用类与成员函数 / 宏 | 所在物理头文件 | 签名核查状态 |
| :--- | :--- | :--- |
| `QScrollArea::setWidgetResizable(bool)` | `<QScrollArea>` | 标准 Qt6 API，匹配通过 |
| `QScrollArea::setWidget(QWidget*)` | `<QScrollArea>` | 标准 Qt6 API，匹配通过 |
| `QScrollArea::verticalScrollBar()` | `<QScrollArea>` | 标准 Qt6 API，匹配通过 |
| `QWidget::adjustSize()` | `<QWidget>` | 标准 Qt6 API，匹配通过 |
| `QAbstractItemView::setVerticalScrollBarPolicy(Qt::ScrollBarPolicy)` | `<QAbstractItemView>` | 标准 Qt6 API，匹配通过 |
| `FolderSectionHeaderBar::setCount(int)` | `FolderSectionWidget.h` | 既有头文件物理签名核实一致 |
| `FileSectionHeaderBar::setCount(int)` | `FolderSectionWidget.h` | 既有头文件物理签名核实一致 |
