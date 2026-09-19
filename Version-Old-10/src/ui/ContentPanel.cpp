#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ContentPanel.h"
#include "ContentHeaderWidget.h"
#include "FolderSectionWidget.h"
#include "controllers/ContentContextMenu.h"
#include "controllers/ContentKeyHandler.h"
#include "controllers/ContentSortController.h"
#include "controllers/ContentDataLoader.h"
#include "controllers/ContentFileOpsHandler.h"
#include "workers/ContentStatsWorker.h"
#include "DropJustifiedView.h"
#include "DropTreeView.h"
#include "DropListView.h"
#include "ColumnViewWidget.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
#include "CardLayoutEngine.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"

#include "../core/AppConfig.h"
#include "../core/CoreEngine.h"
#include "../core/CoreController.h"
#include "../core/TrashService.h"
#include "../core/PermanentDeleteService.h"
#include "../core/ClipboardService.h"
#include "../meta/MediaExtractorPipeline.h"
#include "../util/ThumbnailPipelineService.h"
#include "../core/NavigationService.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QHeaderView>
#include <QScrollBar>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QSignalBlocker>

namespace QuarkMeta {

QTreeView* ContentPanel::treeView() const {
    return static_cast<QTreeView*>(m_treeView);
}

ContentPanel::ContentPanel(QWidget* parent) : QFrame(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    setObjectName("EditorContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_diskModel = new DiskItemModel(this);
    m_model = m_diskModel;
    m_model->setCurrentPath(m_currentPath);

    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(60);
    connect(m_visibleTimer, &QTimer::timeout, this, &ContentPanel::refreshVisibleThumbnails);

    // 统计重算防抖定时器 (50ms)：兼顾实时响应与批量修改时的去噪
    m_statsDebounceTimer = new QTimer(this);
    m_statsDebounceTimer->setSingleShot(true);
    m_statsDebounceTimer->setInterval(50);
    connect(m_statsDebounceTimer, &QTimer::timeout, this, &ContentPanel::recalculateAndEmitStats);

    // 核心架构闭环：监听底层模型元数据变更（卡片点击、列表点击、快捷键赋予、F4重复等），自动防抖驱动统计重算与筛选器同步
    connect(m_diskModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
        if (roles.isEmpty() || roles.contains(RatingRole) || roles.contains(ColorRole) || roles.contains(TagsRole)) {
            if (m_statsDebounceTimer) {
                m_statsDebounceTimer->start();
            }
        }
    });

    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType type, Qt::SortOrder order) {
        applySort();
        if (m_columnView) {
            m_columnView->applySort(static_cast<int>(type), order);
        }
    });

    m_dataLoader = new ContentDataLoader(this);
    m_fileOpsHandler = new ContentFileOpsHandler(this);
    m_statsWorker = new ContentStatsWorker(this);

    connect(m_statsWorker, &ContentStatsWorker::statsReady, this, [this](const ScanStats& stats) {
        if (m_fileProxyModel) {
            m_fileProxyModel->setCachedDuplicatePaths(stats.duplicatePaths);
        }
        emit directoryStatsReady(stats);
    });

    m_zoomLevel = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
    m_currentFilter.showFolders = AppConfig::instance().getValue("ContentPanel/ShowFolders", true).toBool();
    m_currentFilter.showFiles = AppConfig::instance().getValue("ContentPanel/ShowFiles", true).toBool();
    m_currentFilter.showHidden = AppConfig::instance().getValue("ContentPanel/ShowHidden", false).toBool();

    connect(&TrashService::instance(), &TrashService::trashOperationCompleted, this, &ContentPanel::refreshAll);
    connect(&PermanentDeleteService::instance(), &PermanentDeleteService::permanentDeleteCompleted, this, &ContentPanel::refreshAll);
    connect(&ClipboardService::instance(), &ClipboardService::pasteCompleted, this, [this](const QString& dir) {
        if (m_currentPath == dir) refreshAll();
    });

    m_keyHandler = new ContentKeyHandler(this);

    initUi();
    updateGridSize();

    int savedMode = AppConfig::instance().getValue("ContentPanel/ViewMode", static_cast<int>(GridView)).toInt();
    setViewMode(static_cast<ViewMode>(savedMode));
}

void ContentPanel::initUi() {
    // ── 顶部 Header 区域 ──
    m_headerWidget = new ContentHeaderWidget(this);
    m_headerWidget->setFilterState(m_currentFilter);

    connect(m_headerWidget, &ContentHeaderWidget::filterStateChanged, this, [this](const FilterState& state) {
        m_currentFilter = state;
        AppConfig::instance().setValue("ContentPanel/ShowHidden", state.showHidden);
        AppConfig::instance().setValue("ContentPanel/ShowFolders", state.showFolders);
        AppConfig::instance().setValue("ContentPanel/ShowFiles", state.showFiles);
        applyFilters();
    });

    connect(m_headerWidget, &ContentHeaderWidget::recursiveToggled, this, [this](bool recursive) {
        if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
            if (m_headerWidget) m_headerWidget->setRecursive(false);
            return;
        }
        m_isRecursive = recursive;
        if (m_currentViewMode == ColumnView) {
            if (m_columnView && m_columnView->rightmostPane()) {
                m_columnView->rightmostPane()->loadDirectory();
            }
        } else {
            loadDirectory(m_currentPath, recursive);
        }
    });

    m_mainLayout->addWidget(m_headerWidget);

    m_viewStack = new QStackedWidget(this);
    m_viewStack->setFrameShape(QFrame::NoFrame);
    initListView();
    initGridView();
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<QuarkMeta::ItemRecord>& records) {
        if (m_statsWorker && !records.empty()) {
            m_statsWorker->processAsync(records, m_currentFilter.showHidden);
        }
        restoreSelections();
    });
    m_gridScrollArea = new QScrollArea(this);
    m_gridScrollArea->setFrameShape(QFrame::NoFrame);
    m_gridScrollArea->setWidgetResizable(true);
    m_gridScrollArea->setWidget(m_gridContainerWidget);

    m_listScrollArea = new QScrollArea(this);
    m_listScrollArea->setFrameShape(QFrame::NoFrame);
    m_listScrollArea->setWidgetResizable(true);
    m_listScrollArea->setWidget(m_listContainerWidget);

    m_viewStack->addWidget(m_gridScrollArea);
    m_viewStack->addWidget(m_listScrollArea);
    m_viewStack->addWidget(m_columnView);
    m_viewStack->setCurrentWidget(m_gridScrollArea);

    m_mainLayout->addWidget(m_viewStack, 1);
}

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
    m_folderGridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
    m_gridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
    layout->addWidget(m_gridView, 0);
    layout->addStretch(1);

    if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
        connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int h) {
            if (m_folderGridView && m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                m_folderGridView->setFixedHeight(h);
            }
        });
    }
    if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
        connect(jv, &JustifiedView::totalHeightChanged, this, [this](int h) {
            if (m_gridView && m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                m_gridView->setFixedHeight(h);
            }
        });
    }

    connect(m_folderGridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_folderGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_folderGridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_folderGridView, &DropJustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_folderProxyModel);
    });

    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    if (auto* dropJv = qobject_cast<DropJustifiedView*>(m_gridView)) {
        connect(dropJv, &DropJustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            onPathsDropped(paths, targetIndex, currentPath(), m_fileProxyModel);
        });
    }

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
                if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
                    m_folderGridView->setFixedHeight(fjv->totalHeight());
                }
            }
        }
        if (m_gridFileHeader) {
            m_gridFileHeader->setCount(fileCount);
            m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
        if (m_gridView) {
            if (fileCount == 0) {
                m_gridView->hide();
            } else {
                m_gridView->show();
                if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
                    m_gridView->setFixedHeight(jv->totalHeight());
                }
            }
        }
    };

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
}

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
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
    connect(m_folderTreeView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_folderProxyModel);
    });

    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_treeView, &QTreeView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_treeView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_fileProxyModel);
    });

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
            if (fileCount == 0) {
                m_treeView->hide();
            } else {
                m_treeView->show();
                int rowH = m_treeView->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (m_treeView->header() && m_treeView->header()->isVisible()) ? m_treeView->header()->height() : 0;
                int fileH = fileCount * rowH + hdrH + 2;
                m_treeView->setFixedHeight(fileH);
            }
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

bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    if (event && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent && mouseEvent->button() == Qt::LeftButton) {
            QAbstractItemView* view = nullptr;
            if (m_gridView && (obj == m_gridView || obj == m_gridView->viewport())) {
                view = m_gridView;
            } else if (m_treeView && (obj == m_treeView || obj == m_treeView->viewport())) {
                view = m_treeView;
            }
            if (view) {
                QModelIndex idx = view->indexAt(mouseEvent->pos());
                if (!idx.isValid()) {
                    NavigationService::instance().goUp();
                    return true;
                }
            }
        }
    }

    if (m_keyHandler && m_keyHandler->handleEvent(obj, event)) return true;
    return QFrame::eventFilter(obj, event);
}

void ContentPanel::ensureSourceModelIsDiskModel() {
    if (m_model != m_diskModel) {
        m_model = m_diskModel;
        m_model->setCurrentPath(m_currentPath);
        if (m_folderProxyModel) m_folderProxyModel->setSourceModel(m_model);
        if (m_fileProxyModel) m_fileProxyModel->setSourceModel(m_model);
    }
}

void ContentPanel::applySort() {
    if (m_sortController) {
        if (m_folderProxyModel) m_sortController->applySortToModel(m_folderProxyModel);
        if (m_fileProxyModel) m_sortController->applySortToModel(m_fileProxyModel);
        if (m_columnView) {
            m_columnView->applySort(static_cast<int>(m_sortController->sortType()), m_sortController->sortOrder());
        }
    }
}

void ContentPanel::setIsRecursive(bool recursive) {
    m_isRecursive = recursive;
    if (m_headerWidget) {
        m_headerWidget->setRecursive(recursive);
    }
}

void ContentPanel::incrementModelGeneration() {
    if (m_diskModel) m_diskModel->incrementGeneration();
}

void ContentPanel::reloadThumbnailForPath(const QString& path) {
    if (m_diskModel) m_diskModel->reloadThumbnailForPath(path);
}

bool ContentPanel::isTreeView(QObject* view) const {
    return (view == m_treeView);
}

void ContentPanel::startVisibleTimer() {
    if (m_visibleTimer) {
        m_visibleTimer->start();
    }
}

void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) view = activeItemView();
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}

void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        m_currentPath = path;
        m_isRecursive = recursive;
        if (m_columnView) {
            if (m_columnView->containsPath(path)) {
                m_columnView->refreshAllColumns();
            } else {
                m_columnView->setRootPath(path);
                restoreSelections();
            }
        }
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}

void ContentPanel::loadCategory(const QString& categoryType) {
    if (m_dataLoader) m_dataLoader->loadCategory(categoryType);
}

void ContentPanel::loadPaths(const QStringList& paths, int reqId) {
    if (m_dataLoader) m_dataLoader->loadPaths(paths, reqId);
}

void ContentPanel::appendPaths(const QStringList& paths, int reqId) {
    if (m_dataLoader) m_dataLoader->appendPaths(paths, reqId);
}

bool ContentPanel::canPaste(const QString& targetOverride) const {
    return ClipboardService::instance().canPaste(targetOverride.isEmpty() ? m_currentPath : targetOverride);
}

void ContentPanel::performCopy(bool cutMode) {
    if (cutMode) ClipboardService::instance().cutItems(getSelectedPaths());
    else ClipboardService::instance().copyItems(getSelectedPaths());
}

void ContentPanel::performPaste() {
    if (canPaste()) ClipboardService::instance().executePaste(m_currentPath, this);
}

bool ContentPanel::resolvePasteDestination() {
    return m_fileOpsHandler ? m_fileOpsHandler->resolvePasteDestination() : false;
}

void ContentPanel::createNewItem(const QString& type) {
    if (m_fileOpsHandler) m_fileOpsHandler->createNewItem(type);
}

void ContentPanel::performBatchRename() {
    if (m_fileOpsHandler) m_fileOpsHandler->performBatchRename();
}

void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, QAbstractItemModel* sourceModelOverride) {
    if (m_fileOpsHandler) m_fileOpsHandler->onPathsDropped(paths, targetIndex, targetDirOverride, sourceModelOverride);
}

void ContentPanel::onDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QString path = index.data(PathRole).toString();
    if (path.isEmpty()) return;
    if (QFileInfo(path).isDir()) {
        emit directorySelected(path);
    } else {
        emit fileActivated(path);
    }
}

void ContentPanel::toggleFolderSectionCollapse() {
    FolderSectionHeaderBar* header = nullptr;
    if (m_currentViewMode == ListView) {
        header = m_listFolderHeader;
    } else if (m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) {
        header = m_gridFolderHeader;
    } else if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
        return;
    }
    if (header && header->isVisible() && header->count() > 0) {
        header->setCollapsed(!header->isCollapsed());
    }
}

void ContentPanel::setViewMode(ViewMode mode) {
    if (m_currentViewMode == mode) {
        return;
    }
    // 1. 在原视图中上报并更新 SelectionState (SSOT)，修正临时对象迭代器野指针闪退
    m_selectionState.currentFolder = m_currentPath;
    QStringList selList = getSelectedPaths();
    m_selectionState.selectedPaths = QSet<QString>(selList.begin(), selList.end());
    if (!m_selectionState.selectedPaths.isEmpty()) {
        m_selectionState.focusedPath = *m_selectionState.selectedPaths.begin();
    }

    ViewMode oldMode = m_currentViewMode;
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_listScrollArea ? static_cast<QWidget*>(m_listScrollArea) : static_cast<QWidget*>(m_treeView));
    } else if (mode == ColumnView) {
        if (m_columnView) {
            QString targetPath = !m_selectionState.focusedPath.isEmpty() ? m_selectionState.focusedPath : m_currentPath;
            m_columnView->setRootPath(targetPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView);
        if (fjv) fjv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridScrollArea ? static_cast<QWidget*>(m_gridScrollArea) : static_cast<QWidget*>(m_gridView));
    }

    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    // 2. 消费 SelectionState 真理源同步恢复选区
    restoreSelections();

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
    emit zoomLevelChanged(m_zoomLevel);

    if (m_visibleTimer) m_visibleTimer->start();
}

void ContentPanel::setZoomLevel(int level) {
    int minZoom = (m_currentViewMode == ListView) ? 30 : 93;
    int bounded = qBound(minZoom, level, 230);
    if (m_zoomLevel == bounded) return;
    m_zoomLevel = bounded;
    updateGridSize();
    emit zoomLevelChanged(m_zoomLevel);
}

void ContentPanel::updateGridSize() {
    if (m_viewStack->currentWidget() == m_gridScrollArea) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
            jv->setTargetRowHeight(m_zoomLevel);
        }
        if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
            fjv->setTargetRowHeight(m_zoomLevel);
        }
    } else if (m_viewStack->currentWidget() == m_listScrollArea) {
        if (auto* dropTree = qobject_cast<DropTreeView*>(m_treeView)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(dropTree->header())) {
                hdr->setZoomLevel(m_zoomLevel);
            }
        }
        m_treeView->setIconSize(QSize(qMax(16, m_zoomLevel - 8), qMax(16, m_zoomLevel - 8)));
        m_treeView->doItemsLayout();
    }
    AppConfig::instance().setValue("UI/GridZoomLevel", m_zoomLevel);
}

void ContentPanel::applyFilters(const FilterState& state) {
    QString currentKw = m_currentFilter.keyword; // 1. 暂存当前搜索框中的活跃关键词
    bool sf = m_currentFilter.showFolders;
    bool sfi = m_currentFilter.showFiles;
    bool sh = m_currentFilter.showHidden;
    m_currentFilter = state;
    m_currentFilter.keyword = currentKw;          // 2. 锁定并恢复关键词，严禁被空状态冲刷
    m_currentFilter.showFolders = sf;
    m_currentFilter.showFiles = sfi;
    m_currentFilter.showHidden = sh;
    applyFilters();
}

void ContentPanel::applyFilters() {
    if (m_folderProxyModel) {
        FilterState s = m_currentFilter;
        s.showFolders = true;
        s.showFiles = false;
        m_folderProxyModel->currentFilter = s;
        m_folderProxyModel->updateFilter();
    }
    if (m_fileProxyModel) {
        FilterState s = m_currentFilter;
        s.showFolders = false;
        s.showFiles = true;
        m_fileProxyModel->currentFilter = s;
        m_fileProxyModel->updateFilter();
    }
    if (m_columnView) {
        m_columnView->applyFilterState(m_currentFilter);
    }
    updateStatusBarStats();
}

void ContentPanel::search(const QString& query) {
    m_currentFilter.keyword = query;
    applyFilters();
}

void ContentPanel::refreshAll() {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView) m_columnView->refreshAllColumns();
        return;
    }
    if (m_currentCategoryType == "trash") {
        loadCategory("trash");
        return;
    }
    if (!m_currentPath.isEmpty() && m_currentPath != "computer://") loadDirectory(m_currentPath, m_isRecursive);
    else loadDirectory("computer://");
}

void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) m_model->updateRecordMetadata(path);
    if (m_gridView && m_gridView->viewport()) m_gridView->viewport()->update();
    if (m_treeView && m_treeView->viewport()) m_treeView->viewport()->update();
    if (m_columnView) m_columnView->updateMetadataForPath(path);
    recalculateAndEmitStats();
}
void ContentPanel::migrateModelCache(const QString& oldPath, const QString& newPath) { if (m_model) m_model->migrateCache(oldPath, newPath); }
void ContentPanel::clearFolderCache(const QString& folderPath) { if (m_model) m_model->clearCacheForFolder(folderPath); }

void ContentPanel::onSelectionChanged() {
    emitSelectionChangedSignal();
}

void ContentPanel::emitSelectionChangedSignal() {
    QStringList paths = getSelectedPaths();
    emit selectionChanged(paths);
    updateStatusBarStats();
}

void ContentPanel::updateStatusBarStats() {
    int folderCount = m_folderProxyModel ? m_folderProxyModel->rowCount() : 0;
    int fileCount = m_fileProxyModel ? m_fileProxyModel->rowCount() : 0;
    int visibleCount = folderCount + fileCount;
    int fullCount = m_model ? m_model->rowCount() : visibleCount;
    int hiddenCount = fullCount - visibleCount;
    int selectedCount = getSelectedIndexes().size();

    QString statusText = (hiddenCount > 0)
        ? QString("%1个项目，%2个已隐藏，选中了%3个").arg(visibleCount).arg(hiddenCount).arg(selectedCount)
        : QString("%1个项目，选中了%2个").arg(visibleCount).arg(selectedCount);

    emit statusBarMessageReady(statusText);
}

void ContentPanel::recalculateAndEmitStats() {
    std::vector<ItemRecord> records;
    if (m_currentViewMode == ColumnView && m_columnView) {
        ColumnViewPane* pane = m_columnView->rightmostPane();
        if (!pane) pane = m_columnView->activePane();
        if (pane && pane->model()) {
            records = pane->model()->allRecords();
        }
    } else if (m_model) {
        records = m_model->allRecords();
    }

    if (records.empty()) return;

    if (m_statsWorker) {
        m_statsWorker->processAsync(records, m_currentFilter.showHidden);
    }
}

void ContentPanel::refreshVisibleThumbnails() {
    if (!m_model || CoreController::isShuttingDown()) return;

    QList<QAbstractItemView*> views;
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            if (m_columnView->activePane()->folderListView()) views << m_columnView->activePane()->folderListView();
            if (m_columnView->activePane()->listView()) views << m_columnView->activePane()->listView();
        }
    } else if (m_currentViewMode == ListView) {
        if (m_folderTreeView) views << m_folderTreeView;
        if (m_treeView) views << m_treeView;
    } else {
        if (m_folderGridView) views << m_folderGridView;
        if (m_gridView) views << m_gridView;
    }

    QSet<int> visibleRows;
    for (auto* view : views) {
        if (!view || !view->viewport()) continue;
        auto* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy || proxy->rowCount() == 0) continue;

        QRect vpRect = view->viewport()->rect();
        QModelIndex topIdx = view->indexAt(vpRect.topLeft());
        QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    }

    if (!visibleRows.isEmpty()) {
        m_model->loadThumbnailsForRows(visibleRows.values());
    }
}

void ContentPanel::selectAndScrollToPath(const QString& path) { selectAndScrollToItem(path); }
void ContentPanel::selectAndScrollToItem(const QString& path) {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView) {
            QFileInfo info(path);
            if (info.exists() && !info.isDir()) {
                QString dirPath = info.absolutePath();
                if (!m_columnView->containsPath(dirPath)) {
                    m_columnView->setRootPath(path);
                } else if (m_columnView->rightmostPane()) {
                    m_columnView->rightmostPane()->selectItemByPath(path);
                }
            } else {
                if (!m_columnView->containsPath(path)) {
                    m_columnView->setRootPath(path);
                }
            }
        }
        return;
    }
    QSortFilterProxyModel* proxy = getActiveProxyModel();
    if (!proxy || path.isEmpty()) return;
    for (int i = 0; i < proxy->rowCount(); ++i) {
        QModelIndex proxyIdx = proxy->index(i, 0);
        if (proxyIdx.data(PathRole).toString() == path) {
            QAbstractItemView* view = activeItemView();
            if (view && view->selectionModel()) {
                view->scrollTo(proxyIdx);
                view->setCurrentIndex(proxyIdx);
                view->selectionModel()->select(proxyIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            break;
        }
    }
}

QString ContentPanel::getAdjacentFilePath(const QString& currentPath, int delta) {
    QSortFilterProxyModel* proxy = getActiveProxyModel();
    if (!proxy || proxy->rowCount() == 0) return QString();
    int curIdx = -1;
    for (int i = 0; i < proxy->rowCount(); ++i) {
        if (proxy->index(i, 0).data(PathRole).toString() == currentPath) { curIdx = i; break; }
    }
    if (curIdx == -1) return QString();
    int target = curIdx + delta;
    if (target < 0 || target >= proxy->rowCount()) return QString();
    return proxy->index(target, 0).data(PathRole).toString();
}

QSortFilterProxyModel* ContentPanel::getActiveProxyModel() const {
    if (m_currentViewMode == ColumnView && m_columnView && m_columnView->activePane()) {
        if (m_columnView->activePane()->proxyModel()) {
            return m_columnView->activePane()->proxyModel();
        }
    }
    QAbstractItemView* view = activeItemView();
    if (view && view->model()) {
        return qobject_cast<QSortFilterProxyModel*>(view->model());
    }
    return m_fileProxyModel ? m_fileProxyModel : nullptr;
}

QStringList ContentPanel::getSelectedPaths() const {
    QStringList paths;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    return paths;
}

QList<int> ContentPanel::getSelectedTrashIds() const {
    QList<int> ids;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0 && idx.data(IsDiskTrashRole).toBool()) {
            int id = idx.data(DiskTrashIdRole).toInt();
            if (id > 0) ids << id;
        }
    }
    return ids;
}

QAbstractItemView* ContentPanel::activeItemView() const {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            DropListView* folderV = m_columnView->activePane()->folderListView();
            if (folderV && (folderV->hasFocus() || (folderV->selectionModel() && folderV->selectionModel()->hasSelection()))) {
                return folderV;
            }
            return m_columnView->activePane()->listView();
        }
        return nullptr;
    }

    if (m_currentViewMode == ListView) {
        if (m_folderTreeView && (m_folderTreeView->hasFocus() || 
            (m_folderTreeView->selectionModel() && m_folderTreeView->selectionModel()->hasSelection()))) {
            return m_folderTreeView;
        }
        return m_treeView;
    }

    // GridView / JustifiedViewMode
    if (m_folderGridView && (m_folderGridView->hasFocus() || 
        (m_folderGridView->selectionModel() && m_folderGridView->selectionModel()->hasSelection()))) {
        return m_folderGridView;
    }
    return m_gridView;
}

QModelIndexList ContentPanel::getSelectedIndexes() const {
    if (!m_viewStack) return {};
    QModelIndexList res;

    // 🚀【真理源多视图巡检】：直接巡检真实子视图，不再依赖外层容器类型
    QList<QAbstractItemView*> views;
    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            if (m_columnView->activePane()->folderListView()) views << m_columnView->activePane()->folderListView();
            if (m_columnView->activePane()->listView()) views << m_columnView->activePane()->listView();
        }
    } else if (m_currentViewMode == ListView) {
        if (m_folderTreeView) views << m_folderTreeView;
        if (m_treeView) views << m_treeView;
    } else { // GridView / JustifiedViewMode
        if (m_folderGridView) views << m_folderGridView;
        if (m_gridView) views << m_gridView;
    }

    for (auto* view : views) {
        if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) {
                    res.append(idx);
                }
            }
        }
    }
    return res;
}

void ContentPanel::restoreActiveView() {
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? (m_listScrollArea ? static_cast<QWidget*>(m_listScrollArea) : static_cast<QWidget*>(m_treeView)) : (m_gridScrollArea ? static_cast<QWidget*>(m_gridScrollArea) : static_cast<QWidget*>(m_gridView)));
    }
}

void ContentPanel::restoreSelections() {
    if (m_selectionState.selectedPaths.isEmpty() || m_isRestoringSelections) return;

    m_isRestoringSelections = true;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->rightmostPane()) {
            m_columnView->rightmostPane()->setPendingSelectPaths(m_selectionState.selectedPaths);
        }
        m_isRestoringSelections = false;
        return;
    }

    QList<QAbstractItemView*> views;
    if (m_currentViewMode == ListView) {
        if (m_folderTreeView) views << m_folderTreeView;
        if (m_treeView) views << m_treeView;
    } else if (m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) {
        if (m_folderGridView) views << m_folderGridView;
        if (m_gridView) views << m_gridView;
    }

    for (auto* view : views) {
        if (!view || !view->selectionModel()) continue;
        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy) proxy = getActiveProxyModel();
        DiskItemModel* diskModel = m_diskModel;

        if (diskModel && proxy) {
            QSignalBlocker blocker(view->selectionModel());
            QItemSelection sel;
            QModelIndex lastIdx;
            const auto& recs = diskModel->allRecords();
            for (size_t i = 0; i < recs.size(); ++i) {
                if (m_selectionState.selectedPaths.contains(recs[i].path)) {
                    QModelIndex pIdx = proxy->mapFromSource(diskModel->index(static_cast<int>(i), 0));
                    if (pIdx.isValid()) { sel.select(pIdx, pIdx); lastIdx = pIdx; }
                }
            }
            view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (m_isPendingEdit) view->edit(lastIdx); }
        }
    }

    m_isRestoringSelections = false;
}

void ContentPanel::setPendingSelectName(const QString& name, bool edit) {
    m_selectionState.selectedPaths.clear();
    if (!name.isEmpty()) {
        QString fullPath = m_currentPath + "/" + name;
        m_selectionState.selectedPaths.insert(fullPath);
        m_selectionState.focusedPath = fullPath;
    }
    m_isPendingEdit = edit;
}

void ContentPanel::updateLayersButtonState() {
    if (!m_headerWidget) return;
    bool isComp = m_currentPath.isEmpty() || m_currentPath == "computer://";
    m_headerWidget->setLayersEnabled(!isComp, isComp ? "“此电脑”不支持递归显示" : "显示子文件夹中的项目");
}

ContentPanel::DataSourceType ContentPanel::dataSourceType() const {
    return (m_currentCategoryType == "path_list" || m_currentCategoryType == "search") ? DataSourceType::PathList : DataSourceType::DiskNav;
}

void ContentPanel::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        setZoomLevel(m_zoomLevel + (event->angleDelta().y() > 0 ? 8 : -8));
        event->accept();
        return;
    }
    QFrame::wheelEvent(event);
}

} // namespace QuarkMeta