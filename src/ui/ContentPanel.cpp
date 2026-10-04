#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ContentPanel.h"
#include "models/SectionProxyModel.h"
#include "ContentHeaderWidget.h"
#include "controllers/ContentContextMenu.h"
#include "controllers/ContentKeyHandler.h"
#include "controllers/ContentSortController.h"
#include "controllers/ContentDataLoader.h"
#include "controllers/ContentFileOpsHandler.h"
#include "controllers/ContentViewCoordinator.h"
#include "controllers/ContentPaneSplitManager.h"
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
#include "../core/NavigationHistoryService.h"

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


    // 统计重算防抖定时器 (50ms)：兼顾实时响应与批量修改时的去噪
    m_statsDebounceTimer = new QTimer(this);
    m_statsDebounceTimer->setSingleShot(true);
    m_statsDebounceTimer->setInterval(50);
    connect(m_statsDebounceTimer, &QTimer::timeout, this, &ContentPanel::recalculateAndEmitStats);

    // 核心架构闭环：监听底层模型元数据变更（卡片点击、列表点击、快捷键赋予、F4重复等），自动防抖驱动统计重算与筛选器同步
    connect(m_diskModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
        if (roles.isEmpty() || roles.contains(RatingRole) || roles.contains(ColorRole) || roles.contains(TagsRole) || roles.contains(HasThumbnailRole)) {
            if (m_statsDebounceTimer) {
                m_statsDebounceTimer->start();
            }
        }
    });

    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(60);
    connect(m_visibleTimer, &QTimer::timeout, this, &ContentPanel::refreshVisibleThumbnails);

    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType type, Qt::SortOrder order) {
        applySort();
        if (m_treeView && m_treeView->header()) {
            int col = -1;
            switch (type) {
                case SortType::SortByName: col = static_cast<int>(FileListColumn::Name); break;
                case SortType::SortByRating: col = static_cast<int>(FileListColumn::Rating); break;
                case SortType::SortByDimension: col = static_cast<int>(FileListColumn::Dimension); break;
                case SortType::SortByExtension: col = static_cast<int>(FileListColumn::Type); break;
                case SortType::SortBySize: col = static_cast<int>(FileListColumn::Size); break;
                case SortType::SortByModifyDate: col = static_cast<int>(FileListColumn::ModifiedDate); break;
                default: break;
            }
            if (col >= 0) {
                m_treeView->header()->setSortIndicator(col, order);
            }
        }
        if (m_columnView) {
            m_columnView->applySort(static_cast<int>(type), order);
        }
    });

    m_dataLoader = new ContentDataLoader(this);
    m_fileOpsHandler = new ContentFileOpsHandler(this);
    m_statsWorker = new ContentStatsWorker(this);
    m_splitManager = new ContentPaneSplitManager(this);
    m_viewCoordinator = new ContentViewCoordinator(this);
    setAcceptDrops(true);

    connect(m_statsWorker, &ContentStatsWorker::statsReady, this, [this](const ScanStats& stats) {
        if (m_proxyModel) {
            m_proxyModel->setCachedDuplicatePaths(stats.duplicatePaths);
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

    connect(this, &ContentPanel::closePaneRequested, this, [this]() {
        closePane(this);
    });

    initUi();
    updateGridSize();

    int savedMode = AppConfig::instance().getValue("ContentPanel/ViewMode", static_cast<int>(GridView)).toInt();
    setViewMode(static_cast<ViewMode>(savedMode));
}

void ContentPanel::initUi() {
    // ── 顶部 Header 区域 ──
    m_headerWidget = new ContentHeaderWidget(this);
    m_headerWidget->setFilterState(m_currentFilter);

    connect(m_headerWidget, &ContentHeaderWidget::splitViewRequested, this, [this]() {
        if (isSecondaryPane()) {
            emit closePaneRequested();
            return;
        }
        if (isSplitMode()) {
            closeSecondaryPane();
        } else {
            QStringList history = NavigationHistoryService::instance().getHistory();
            QString lastPath;
            for (const QString& hPath : history) {
                if (!hPath.isEmpty() && QDir::cleanPath(hPath) != QDir::cleanPath(m_currentPath)) {
                    lastPath = hPath;
                    break;
                }
            }
            if (lastPath.isEmpty()) {
                lastPath = m_currentPath;
            }
            splitPane(Qt::Horizontal, lastPath);
        }
    });

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

    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<QuarkMeta::ItemRecord>& records) {
        if (m_statsWorker && !records.empty()) {
            m_statsWorker->processAsync(records, m_currentFilter.showHidden);
        }
        restoreSelections();
    });

    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->currentFilter = m_currentFilter;

    m_sectionModel = new SectionProxyModel(this);
    m_sectionModel->setSourceModel(m_proxyModel);

    initGridView();
    initListView();

    if (m_gridView) {
        m_viewStack->addWidget(m_gridView);
    }
    if (m_treeView) {
        m_viewStack->addWidget(m_treeView);
    }
    m_viewStack->addWidget(m_columnView);
    if (m_gridView) {
        m_viewStack->setCurrentWidget(m_gridView);
    }

    m_mainLayout->addWidget(m_viewStack, 1);
}

void ContentPanel::initGridView() {
    auto* jv = new JustifiedView(this);
    m_gridView = jv;
    jv->setSectionModel(m_sectionModel);
    jv->setFrameShape(QFrame::NoFrame);
    jv->setSelectionMode(QAbstractItemView::ExtendedSelection);
    jv->setContextMenuPolicy(Qt::CustomContextMenu);
    jv->setEditTriggers(QAbstractItemView::NoEditTriggers);
    jv->setModel(m_sectionModel);

    jv->setAspectRatioRole(AspectRatioRole);
    auto* delegate = new ThumbnailDelegate(this);
    delegate->setHasThumbnailRole(HasThumbnailRole);
    delegate->setRatingRole(RatingRole);
    delegate->setPathRole(PathRole);
    delegate->setPinnedRole(PinnedRole);
    delegate->setTypeRole(TypeRole);
    delegate->setIsEmptyRole(IsEmptyRole);
    delegate->setColorRole(ColorRole);
    m_gridView->setItemDelegate(delegate);

    jv->installEventFilter(this);
    jv->viewport()->installEventFilter(this);

    connect(jv, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(jv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(jv, &QAbstractItemView::customContextMenuRequested, this, [this](const QPoint& pos) {
        onCustomContextMenuRequested(m_gridView, pos);
    });
    connect(jv, &JustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath());
    });
}

void ContentPanel::initListView() {
    auto* tree = new DropTreeView(this);
    m_treeView = tree;
    tree->setFrameShape(QFrame::NoFrame);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree->setModel(m_sectionModel);
    tree->setAlternatingRowColors(true);
    tree->setRootIsDecorated(false);
    tree->setSortingEnabled(false);
    tree->header()->setSectionsClickable(true);
    tree->header()->setFixedHeight(32);
    tree->header()->setMinimumSectionSize(0);
    tree->applyColumnPolicies();
    tree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto* delegate = new TreeItemDelegate(this, true, true);
    tree->setItemDelegate(delegate);

    tree->installEventFilter(this);
    tree->viewport()->installEventFilter(this);

    connect(tree, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(tree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(tree, &QAbstractItemView::customContextMenuRequested, this, [this](const QPoint& pos) {
        onCustomContextMenuRequested(m_treeView, pos);
    });
    connect(tree, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath());
    });
    connect(tree->header(), &QHeaderView::sectionClicked, this, [this](int logicalIndex) {
        if (logicalIndex == static_cast<int>(FileListColumn::Status)) return;
        SortType newType = SortType::SortByName;
        switch (static_cast<FileListColumn>(logicalIndex)) {
            case FileListColumn::Name: newType = SortType::SortByName; break;
            case FileListColumn::Rating: newType = SortType::SortByRating; break;
            case FileListColumn::Dimension: newType = SortType::SortByDimension; break;
            case FileListColumn::Type: newType = SortType::SortByExtension; break;
            case FileListColumn::Size: newType = SortType::SortBySize; break;
            case FileListColumn::ModifiedDate: newType = SortType::SortByModifyDate; break;
            default: return;
        }
        if (currentSortType() == newType) {
            setSortOrder(currentSortOrder() == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder);
        } else {
            setSortCriteria(newType, Qt::AscendingOrder);
        }
    });
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
        if (m_proxyModel) m_proxyModel->setSourceModel(m_model);
    }
}

void ContentPanel::applySort() {
    if (m_sortController) {
        if (m_proxyModel) m_sortController->applySortToModel(m_proxyModel);
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
    } else {
        refreshVisibleThumbnails();
    }
}

void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) view = activeItemView();
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}

void ContentPanel::onCustomContextMenuRequested(QAbstractItemView* view, const QPoint& pos) {
    if (!view) view = activeItemView();
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}

bool ContentPanel::isSplitMode() const {
    return m_splitManager ? m_splitManager->isSplitMode() : false;
}

bool ContentPanel::isSecondaryPane() const {
    return m_splitManager ? m_splitManager->isSecondaryPane() : false;
}

void ContentPanel::setIsSecondaryPane(bool secondary) {
    if (m_splitManager) m_splitManager->setIsSecondaryPane(secondary);
}

ContentPanel* ContentPanel::secondaryContentPanel() const {
    return m_splitManager ? m_splitManager->secondaryContentPanel() : nullptr;
}

QList<ContentPanel*> ContentPanel::panes() const {
    return m_splitManager ? m_splitManager->panes() : QList<ContentPanel*>{const_cast<ContentPanel*>(this)};
}

int ContentPanel::paneCount() const {
    return m_splitManager ? m_splitManager->paneCount() : 1;
}

ContentPanel* ContentPanel::rootPane() const {
    return m_splitManager ? m_splitManager->rootPane() : const_cast<ContentPanel*>(this);
}

void ContentPanel::splitPane(Qt::Orientation orientation, const QString& secondaryPath) {
    if (m_splitManager) m_splitManager->splitPane(orientation, secondaryPath);
}

void ContentPanel::closePane(ContentPanel* pane) {
    if (m_splitManager) m_splitManager->closePane(pane);
}

void ContentPanel::closeSecondaryPane() {
    if (m_splitManager) m_splitManager->closeSecondaryPane();
}

void ContentPanel::requestClosePane() {
    emit closePaneRequested();
}

void ContentPanel::setActivePane(bool active) {
    if (m_splitManager) m_splitManager->setActivePane(active);
    if (active) emit panelActivated(this);
}

QString ContentPanel::activePath() const {
    return m_currentPath;
}

void ContentPanel::redistributePaneSizes() {
    if (m_splitManager) m_splitManager->redistributePaneSizes();
}

void ContentPanel::updateDragOverlay(const QPoint& pos) {
    if (m_splitManager) m_splitManager->updateDragOverlay(pos);
}

void ContentPanel::hideDragOverlay() {
    if (m_splitManager) m_splitManager->hideDragOverlay();
}

void ContentPanel::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        if (paneCount() < kMaxPanes) {
            event->acceptProposedAction();
            return;
        }
    }
    QFrame::dragEnterEvent(event);
}

void ContentPanel::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        if (paneCount() < kMaxPanes) {
            event->acceptProposedAction();
            updateDragOverlay(event->position().toPoint());
            return;
        }
    }
    QFrame::dragMoveEvent(event);
}

void ContentPanel::dragLeaveEvent(QDragLeaveEvent* event) {
    hideDragOverlay();
    QFrame::dragLeaveEvent(event);
}

void ContentPanel::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        if (paneCount() < kMaxPanes) {
            hideDragOverlay();
            QString tabUrl = QString::fromUtf8(event->mimeData()->data("application/x-quarkmeta-taburl"));
            if (tabUrl.isEmpty()) {
                tabUrl = event->mimeData()->text();
            }

            if (!tabUrl.isEmpty()) {
                QPoint pos = event->position().toPoint();
                int w = width();
                int h = height();
                Qt::Orientation orientation = Qt::Horizontal;

                if (pos.y() < h * 0.25 || pos.y() > h * 0.75) {
                    orientation = Qt::Vertical;
                } else if (pos.x() < w * 0.25 || pos.x() > w * 0.75) {
                    orientation = Qt::Horizontal;
                } else {
                    orientation = Qt::Horizontal;
                }

                splitPane(orientation, tabUrl);
                event->acceptProposedAction();
                return;
            }
        }
    }
    hideDragOverlay();
    QFrame::dropEvent(event);
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

void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride) {
    if (m_fileOpsHandler) m_fileOpsHandler->onPathsDropped(paths, targetIndex, targetDirOverride);
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
    if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
    } else if (m_sectionModel) {
        m_sectionModel->setFolderCollapsed(!m_sectionModel->isFolderCollapsed());
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
        m_viewStack->setCurrentWidget(m_treeView);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            QString targetPath = !m_selectionState.focusedPath.isEmpty() ? m_selectionState.focusedPath : m_currentPath;
            m_columnView->setRootPath(targetPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty() && m_currentPath != "computer://") {
            if (!m_diskModel || m_diskModel->rowCount() == 0) {
                loadDirectory(m_currentPath, m_isRecursive);
            }
        }
    }

    // 更新窗格及容器的动态最小宽度约束（列视图锁定 460px 保证 1列数据230px + 1列留白230px）
    int minW = (mode == ColumnView) ? 460 : 230;
    setMinimumWidth(minW);
    if (parentWidget() && parentWidget()->objectName() == "EditorContainer") {
        parentWidget()->setMinimumWidth(minW);
    }

    // 2. 消费 SelectionState 真理源同步恢复选区
    restoreSelections();

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
    emit zoomLevelChanged(m_zoomLevel);

    refreshVisibleThumbnails();
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
    if (m_viewCoordinator) {
        m_viewCoordinator->updateGridSize(m_zoomLevel);
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
    if (m_proxyModel) {
        m_proxyModel->currentFilter = m_currentFilter;
        m_proxyModel->updateFilter();
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
    if (m_isLoading) return;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView) m_columnView->refreshAllColumns();
        return;
    }

    if (m_currentCategoryType == "trash") {
        loadCategory("trash");
        return;
    }

    if (m_currentCategoryType == "library") {
        loadPaths(m_lastLoadedLibraryPaths);
        return;
    }

    if (!m_currentPath.isEmpty() && m_currentPath != "computer://" && !m_currentPath.startsWith("library://")) {
        loadDirectory(m_currentPath, m_isRecursive);
    } else if (m_currentPath == "computer://") {
        loadDirectory("computer://");
    }
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

void ContentPanel::updateStatusBarStats(int cachedSelectedCount) {
    int folderCount = 0;
    int fileCount = 0;
    if (m_currentViewMode != ColumnView && m_sectionModel) {
        folderCount = m_sectionModel->folderCount();
        fileCount = m_sectionModel->fileCount();
    } else if (m_currentViewMode == ColumnView && m_columnView && m_columnView->rightmostPane()) {
        folderCount = m_columnView->rightmostPane()->sectionProxyModel() ? m_columnView->rightmostPane()->sectionProxyModel()->folderCount() : 0;
        fileCount = m_columnView->rightmostPane()->sectionProxyModel() ? m_columnView->rightmostPane()->sectionProxyModel()->fileCount() : 0;
    }
    int visibleCount = folderCount + fileCount;
    int fullCount = m_model ? m_model->rowCount() : visibleCount;
    int hiddenCount = fullCount - visibleCount;
    int selectedCount = (cachedSelectedCount >= 0) ? cachedSelectedCount : getSelectedIndexes().size();

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

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            m_columnView->activePane()->refreshVisibleThumbnails();
        }
    } else {
        if (m_viewCoordinator) m_viewCoordinator->refreshVisibleThumbnails();
    }
}

void ContentPanel::selectAndEditPath(const QString& path) {
    if (path.isEmpty()) return;

    QAbstractItemView* view = activeItemView();
    QAbstractItemModel* model = view ? view->model() : nullptr;

    if (m_currentViewMode == ColumnView) {
        if (m_columnView && m_columnView->activePane()) {
            view = m_columnView->activePane()->listView();
            if (view) model = view->model();
        }
    }

    if (!model || !view) return;

    for (int i = 0; i < model->rowCount(); ++i) {
        QModelIndex idx = model->index(i, 0);
        if (idx.data(SectionHeaderRole).toBool()) continue;
        QString idxPath = idx.data(PathRole).toString();
        if (QString::compare(QDir::cleanPath(idxPath), QDir::cleanPath(path), Qt::CaseInsensitive) == 0) {
            view->setFocus();
            view->scrollTo(idx);
            view->setCurrentIndex(idx);
            if (view->selectionModel()) {
                view->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            view->edit(idx);
            break;
        }
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
    QAbstractItemView* view = activeItemView();
    if (!view || !view->model() || path.isEmpty()) return;
    QAbstractItemModel* model = view->model();
    int total = model->rowCount();
    for (int i = 0; i < total; ++i) {
        QModelIndex idx = model->index(i, 0);
        if (idx.data(SectionHeaderRole).toBool()) continue;
        if (idx.data(PathRole).toString() == path) {
            if (view->selectionModel()) {
                view->scrollTo(idx);
                view->setCurrentIndex(idx);
                view->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
            break;
        }
    }
}

QString ContentPanel::getAdjacentFilePath(const QString& currentPath, int delta) {
    QAbstractItemView* view = activeItemView();
    if (!view || !view->model()) return QString();
    QAbstractItemModel* model = view->model();
    int rowCount = model->rowCount();
    if (rowCount == 0) return QString();

    int curIdx = -1;
    for (int i = 0; i < rowCount; ++i) {
        QModelIndex idx = model->index(i, 0);
        if (idx.data(SectionHeaderRole).toBool()) continue;
        if (idx.data(PathRole).toString() == currentPath) {
            curIdx = i;
            break;
        }
    }

    if (curIdx == -1) return QString();

    int step = (delta > 0) ? 1 : -1;
    int remaining = std::abs(delta);
    int targetRow = curIdx;

    while (remaining > 0) {
        targetRow += step;
        if (targetRow < 0 || targetRow >= rowCount) return QString();
        QModelIndex idx = model->index(targetRow, 0);
        if (idx.data(SectionHeaderRole).toBool()) continue;
        remaining--;
    }

    return model->index(targetRow, 0).data(PathRole).toString();
}

QSortFilterProxyModel* ContentPanel::getActiveProxyModel() const {
    if (m_viewCoordinator) {
        return m_viewCoordinator->getActiveProxyModel();
    }
    return m_proxyModel;
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
            return m_columnView->activePane()->listView();
        }
        return nullptr;
    }

    if (m_currentViewMode == ListView) {
        return m_treeView;
    }

    return m_gridView;
}

QModelIndexList ContentPanel::getSelectedIndexes() const {
    if (m_viewCoordinator) {
        return m_viewCoordinator->getSelectedIndexes();
    }
    QAbstractItemView* view = activeItemView();
    if (view && view->selectionModel()) {
        return view->selectionModel()->selectedIndexes();
    }
    return {};
}

void ContentPanel::restoreActiveView() {
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? static_cast<QWidget*>(m_treeView) : static_cast<QWidget*>(m_gridView));
    }
}

void ContentPanel::restoreSelections() {
    if (m_selectionState.selectedPaths.isEmpty() || m_isRestoringSelections) return;

    m_isRestoringSelections = true;

    if (m_viewCoordinator) {
        m_viewCoordinator->restoreSelections(m_selectionState.selectedPaths, m_isPendingEdit);
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