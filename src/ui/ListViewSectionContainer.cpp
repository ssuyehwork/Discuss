#include "ListViewSectionContainer.h"
#include "ContentPanel.h"
#include "TreeItemDelegate.h"
#include <QSignalBlocker>

namespace QuarkMeta {

ListViewSectionContainer::ListViewSectionContainer(ContentPanel* panel, QWidget* parent)
    : QWidget(parent), m_panel(panel) {
    setupUi();
}

void ListViewSectionContainer::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(4);

    // 1. 文件夹折叠标题栏
    m_folderHeader = new FolderSectionHeaderBar(this);
    m_mainLayout->addWidget(m_folderHeader);

    // 2. 文件夹专用列表控件
    m_folderListView = new DropTreeView(this);
    m_folderListView->setFrameShape(QFrame::NoFrame);
    m_folderListView->setAlternatingRowColors(true);
    m_folderListView->setSortingEnabled(true);
    m_folderListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_folderListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderListView->setRootIsDecorated(false);
    m_folderListView->setItemDelegate(new TreeItemDelegate(this, true, true));
    if (m_folderListView->header()) {
        m_folderListView->header()->setFixedHeight(32);
        m_folderListView->header()->setMinimumSectionSize(0);
    }
    m_folderListView->applyColumnPolicies();
    m_mainLayout->addWidget(m_folderListView);

    // 3. 文件折叠标题栏
    m_fileHeader = new FileSectionHeaderBar(this);
    m_mainLayout->addWidget(m_fileHeader);

    // 4. 文件专用列表控件
    m_fileListView = new DropTreeView(this);
    m_fileListView->setFrameShape(QFrame::NoFrame);
    m_fileListView->setAlternatingRowColors(true);
    m_fileListView->setSortingEnabled(true);
    m_fileListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_fileListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileListView->setRootIsDecorated(false);
    m_fileListView->setItemDelegate(new TreeItemDelegate(this, true, true));
    if (m_fileListView->header()) {
        m_fileListView->header()->setFixedHeight(32);
        m_fileListView->header()->setMinimumSectionSize(0);
    }
    m_fileListView->applyColumnPolicies();
    m_mainLayout->addWidget(m_fileListView);

    // 5. 安装 ContentPanel 事件过滤器
    if (m_panel) {
        m_folderListView->installEventFilter(m_panel);
        m_folderListView->viewport()->installEventFilter(m_panel);
        m_fileListView->installEventFilter(m_panel);
        m_fileListView->viewport()->installEventFilter(m_panel);
    }

    // 6. 折叠信号双向连接
    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        m_folderListView->setVisible(!collapsed);
    });

    connect(m_fileHeader, &FileSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        m_fileListView->setVisible(!collapsed);
    });

    // 7. 信号转发代理至 ListViewSectionContainer
    connect(m_folderListView, &QTreeView::doubleClicked, this, &ListViewSectionContainer::doubleClicked);
    connect(m_folderListView, &QTreeView::customContextMenuRequested, this, &ListViewSectionContainer::customContextMenuRequested);
    connect(m_folderListView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        emit pathsDropped(paths, targetIndex, m_folderProxyModel);
    });

    connect(m_fileListView, &QTreeView::doubleClicked, this, &ListViewSectionContainer::doubleClicked);
    connect(m_fileListView, &QTreeView::customContextMenuRequested, this, &ListViewSectionContainer::customContextMenuRequested);
    connect(m_fileListView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        emit pathsDropped(paths, targetIndex, m_fileProxyModel);
    });

    // 8. 跨列表表头列宽像素级对齐
    syncHeaderColumnWidths();
}

void ListViewSectionContainer::syncHeaderColumnWidths() {
    auto* folderHeaderView = m_folderListView->header();
    auto* fileHeaderView = m_fileListView->header();

    connect(folderHeaderView, &QHeaderView::sectionResized, this, [fileHeaderView](int logicalIndex, int oldSize, int newSize) {
        QSignalBlocker blocker(fileHeaderView);
        fileHeaderView->resizeSection(logicalIndex, newSize);
    });

    connect(fileHeaderView, &QHeaderView::sectionResized, this, [folderHeaderView](int logicalIndex, int oldSize, int newSize) {
        QSignalBlocker blocker(folderHeaderView);
        folderHeaderView->resizeSection(logicalIndex, newSize);
    });
}

void ListViewSectionContainer::setModel(QSortFilterProxyModel* mainProxyModel) {
    m_mainProxyModel = mainProxyModel;
    if (!m_mainProxyModel) return;

    // 上方文件夹模型：仅放行 isDir == true
    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(m_mainProxyModel->sourceModel());
    FilterState folderState;
    folderState.showFolders = true;
    folderState.showFiles = false;
    m_folderProxyModel->currentFilter = folderState;

    // 下方文件模型：仅放行 isDir == false
    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(m_mainProxyModel->sourceModel());
    FilterState fileState;
    fileState.showFolders = false;
    fileState.showFiles = true;
    m_fileProxyModel->currentFilter = fileState;

    m_folderListView->setModel(m_folderProxyModel);
    m_fileListView->setModel(m_fileProxyModel);

    // 绑定选区变更
    if (m_folderListView->selectionModel()) {
        connect(m_folderListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            if (m_folderListView->selectionModel()->hasSelection() && m_fileListView->selectionModel()) {
                QSignalBlocker blocker(m_fileListView->selectionModel());
                m_fileListView->selectionModel()->clearSelection();
            }
            emit selectionChanged();
        });
    }

    if (m_fileListView->selectionModel()) {
        connect(m_fileListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            if (m_fileListView->selectionModel()->hasSelection() && m_folderListView->selectionModel()) {
                QSignalBlocker blocker(m_folderListView->selectionModel());
                m_folderListView->selectionModel()->clearSelection();
            }
            emit selectionChanged();
        });
    }

    // 初始统计显示数量
    int folderCount = m_folderProxyModel->rowCount();
    int fileCount = m_fileProxyModel->rowCount();

    m_folderHeader->setCount(folderCount);
    m_fileHeader->setCount(fileCount);

    m_folderHeader->setVisible(folderCount > 0);
    m_folderListView->setVisible(folderCount > 0);
    m_fileHeader->setVisible(fileCount > 0);
    m_fileListView->setVisible(fileCount > 0);
}

void ListViewSectionContainer::applyFilters(const FilterState& state) {
    if (m_folderProxyModel) {
        FilterState st = state;
        st.showFolders = true;
        st.showFiles = false;
        m_folderProxyModel->currentFilter = st;
        m_folderProxyModel->invalidateFilter();
    }
    if (m_fileProxyModel) {
        FilterState st = state;
        st.showFolders = false;
        st.showFiles = true;
        m_fileProxyModel->currentFilter = st;
        m_fileProxyModel->invalidateFilter();
    }

    int folderCount = m_folderProxyModel ? m_folderProxyModel->rowCount() : 0;
    int fileCount = m_fileProxyModel ? m_fileProxyModel->rowCount() : 0;

    if (m_folderHeader) {
        m_folderHeader->setCount(folderCount);
        m_folderHeader->setVisible(folderCount > 0);
    }
    if (m_folderListView) {
        m_folderListView->setVisible(folderCount > 0 && (!m_folderHeader || !m_folderHeader->isCollapsed()));
    }
    if (m_fileHeader) {
        m_fileHeader->setCount(fileCount);
        m_fileHeader->setVisible(fileCount > 0);
    }
    if (m_fileListView) {
        m_fileListView->setVisible(fileCount > 0 && (!m_fileHeader || !m_fileHeader->isCollapsed()));
    }
}

void ListViewSectionContainer::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_folderProxyModel) {
        m_folderProxyModel->setSortType(sortType);
        m_folderProxyModel->setSortOrder(sortOrder);
    }
    if (m_fileProxyModel) {
        m_fileProxyModel->setSortType(sortType);
        m_fileProxyModel->setSortOrder(sortOrder);
    }
}

void ListViewSectionContainer::toggleFolderSectionCollapse() {
    if (m_folderHeader) {
        m_folderHeader->setCollapsed(!m_folderHeader->isCollapsed());
    }
}

} // namespace QuarkMeta
