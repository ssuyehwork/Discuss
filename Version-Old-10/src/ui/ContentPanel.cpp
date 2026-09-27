#include "ContentPanel.h"
#include "ContentHeaderWidget.h"
#include "FolderSectionWidget.h"
#include <QSplitter>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include "SectionedScrollCanvas.h"
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
#include "Logger.h"
#include "../core/NavigationHistoryService.h"
#include <QElapsedTimer>

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
    m_splitManager = new ContentPaneSplitManager(this);
    setAcceptDrops(true);
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
    m_viewCoordinator = new ContentViewCoordinator(this);

    connect(m_statsWorker, &ContentStatsWorker::statsReady, this, [this](const ScanStats& stats) {
        if (m_gridCanvas && m_gridCanvas->fileProxyModel()) {
            m_gridCanvas->fileProxyModel()->setCachedDuplicatePaths(stats.duplicatePaths);
        }
        if (m_listCanvas && m_listCanvas->fileProxyModel()) {
            m_listCanvas->fileProxyModel()->setCachedDuplicatePaths(stats.duplicatePaths);
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
        if (m_currentPath == dir || (m_currentViewMode == ColumnView && m_columnView && m_columnView->containsPath(dir))) {
            refreshAll();
        }
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
            if (m_columnView && m_columnView->activePane()) {
                m_columnView->activePane()->loadDirectory();
            }
        } else {
            loadDirectory(m_currentPath, recursive);
        }
    });

    m_mainLayout->addWidget(m_headerWidget);

    m_viewStack = new QStackedWidget(this);
    m_viewStack->setFrameShape(QFrame::NoFrame);

    // 统一在 ContentPanel 里构建代理模型，SectionedScrollCanvas 只负责排版/滚动
    auto makeProxyPair = [this](bool wantFolders) {
        auto* p = new FilterProxyModel(this);
        p->setSourceModel(m_model);
        p->setFilterKeyColumn(0);
        p->setDynamicSortFilter(true);
        FilterState s = m_currentFilter;
        s.showFolders = wantFolders;
        s.showFiles = !wantFolders;
        p->currentFilter = s;
        return p;
    };
    m_folderProxyModel = makeProxyPair(true);
    m_fileProxyModel = makeProxyPair(false);
    m_gridFolderProxyModel = makeProxyPair(true);
    m_gridFileProxyModel = makeProxyPair(false);

    m_listCanvas = new SectionedScrollCanvas(SectionedScrollCanvas::CanvasType::List, m_folderProxyModel, m_fileProxyModel, this, this);
    m_treeView = static_cast<DropTreeView*>(m_listCanvas->fileView());
    m_folderTreeView = static_cast<DropTreeView*>(m_listCanvas->folderView());

    m_gridCanvas = new SectionedScrollCanvas(SectionedScrollCanvas::CanvasType::Grid, m_gridFolderProxyModel, m_gridFileProxyModel, this, this);
    m_gridView = m_gridCanvas->fileView();
    m_folderGridView = static_cast<DropJustifiedView*>(m_gridCanvas->folderView());

    // 统一信号透传
    for (auto* canvas : {m_gridCanvas, m_listCanvas}) {
        connect(canvas, &SectionedScrollCanvas::selectionChanged, this, &ContentPanel::onSelectionChanged);
        connect(canvas, &SectionedScrollCanvas::doubleClicked, this, &ContentPanel::onDoubleClicked);
        connect(canvas, &SectionedScrollCanvas::customContextMenuRequested, this, [this](const QPoint& pos) {
            onCustomContextMenuRequested(pos);
        });
        connect(canvas, &SectionedScrollCanvas::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx, QAbstractItemModel* proxy) {
            onPathsDropped(p, idx, currentPath(), proxy);
        });
    }

    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::activeColumnRecordsChanged, this, [this](const std::vector<QuarkMeta::ItemRecord>& records) {
        if (m_statsWorker && !records.empty()) {
            m_statsWorker->processAsync(records, m_currentFilter.showHidden);
        }
        restoreSelections();
    });

    m_viewStack->addWidget(m_gridCanvas);
    m_viewStack->addWidget(m_listCanvas);
    m_viewStack->addWidget(m_columnView);
    m_viewStack->setCurrentWidget(m_gridCanvas);

    m_mainLayout->addWidget(m_viewStack, 1);

    if (m_viewCoordinator) {
        m_viewCoordinator->installActivationFilters();
    }
}

void ContentPanel::initGridView() {
    // 已完整归一化迁移至 SectionedScrollCanvas
}

void ContentPanel::initListView() {
    // 已完整归一化迁移至 SectionedScrollCanvas
}

bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    if (event && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn)) {
        emit panelActivated(this);
    }

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
        for (auto* p : {m_folderProxyModel, m_fileProxyModel, m_gridFolderProxyModel, m_gridFileProxyModel}) {
            if (p) p->setSourceModel(m_model);
        }
    }
}

void ContentPanel::applySort() {
    if (m_sortController) {
        for (auto* p : {m_folderProxyModel, m_fileProxyModel, m_gridFolderProxyModel, m_gridFileProxyModel}) {
            m_sortController->applySortToModel(p);
        }
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
    return m_splitManager ? m_splitManager->panes() : QList<ContentPanel*>();
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

void ContentPanel::redistributePaneSizes() {
    if (m_splitManager) m_splitManager->redistributePaneSizes();
}

void ContentPanel::closePane(ContentPanel* pane) {
    if (m_splitManager) m_splitManager->closePane(pane);
}

void ContentPanel::closeSecondaryPane() {
    if (m_splitManager) m_splitManager->closeSecondaryPane();
}

void ContentPanel::requestClosePane() {
    if (isSecondaryPane()) {
        emit closePaneRequested();
    } else if (isSplitMode()) {
        closeSecondaryPane();
    }
}

void ContentPanel::setActivePane(bool active) {
    if (m_splitManager) m_splitManager->setActivePane(active);
}

void ContentPanel::updateDragOverlay(const QPoint& pos) {
    if (m_splitManager) m_splitManager->updateDragOverlay(pos);
}

void ContentPanel::hideDragOverlay() {
    if (m_splitManager) m_splitManager->hideDragOverlay();
}

void ContentPanel::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText() ||
        event->mimeData()->hasFormat("application/x-quarkmeta-tabindex") ||
        event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        event->acceptProposedAction();
    }
}

void ContentPanel::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("application/x-quarkmeta-tabindex") ||
        event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        updateDragOverlay(event->position().toPoint());
    } else {
        hideDragOverlay();
    }
    event->acceptProposedAction();
}

void ContentPanel::dragLeaveEvent(QDragLeaveEvent* event) {
    Q_UNUSED(event);
    hideDragOverlay();
}

void ContentPanel::dropEvent(QDropEvent* event) {
    QPoint pos = event->position().toPoint();
    hideDragOverlay();

    if (event->mimeData()->hasFormat("application/x-quarkmeta-tabindex") ||
        event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
        int w = width();
        int h = height();

        QString targetUrl;
        if (event->mimeData()->hasFormat("application/x-quarkmeta-taburl")) {
            targetUrl = QString::fromUtf8(event->mimeData()->data("application/x-quarkmeta-taburl"));
        } else if (event->mimeData()->hasText()) {
            targetUrl = event->mimeData()->text();
        }

        if (pos.x() > w * 0.75 || pos.x() < w * 0.25) {
            splitPane(Qt::Horizontal, targetUrl);
            event->acceptProposedAction();
        } else if (pos.y() > h * 0.75 || pos.y() < h * 0.25) {
            splitPane(Qt::Vertical, targetUrl);
            event->acceptProposedAction();
        } else {
            event->acceptProposedAction();
        }
        return;
    }

    if (event->mimeData()->hasUrls()) {
        QStringList paths;
        for (const QUrl& url : event->mimeData()->urls()) {
            paths << url.toLocalFile();
        }
        onPathsDropped(paths, QModelIndex());
        event->acceptProposedAction();
    }
}

void ContentPanel::startVisibleTimer() {
    if (m_visibleTimer) {
        m_visibleTimer->start();
    }
}

void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    onCustomContextMenuRequested(view, pos);
}

void ContentPanel::onCustomContextMenuRequested(QAbstractItemView* view, const QPoint& pos) {
    if (!view) view = activeItemView();
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}

void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ColumnView) {
        m_currentPath = path;
        setIsRecursive(recursive);
        if (m_columnView) {
            if (!m_columnView->containsPath(path)) {
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

QString ContentPanel::activePath() const {
    if (m_currentViewMode == ColumnView && m_columnView) {
        ColumnViewPane* pane = m_columnView->activePane();
        if (pane && !pane->currentPath().isEmpty()) {
            return pane->currentPath();
        }
    }
    return m_currentPath;
}

bool ContentPanel::canPaste(const QString& targetOverride) const {
    QString target = !targetOverride.isEmpty() ? targetOverride : activePath();
    return ClipboardService::instance().canPaste(target);
}

void ContentPanel::performCopy(bool cutMode) {
    if (cutMode) ClipboardService::instance().cutItems(getSelectedPaths());
    else ClipboardService::instance().copyItems(getSelectedPaths());
}

void ContentPanel::performPaste() {
    QString target = activePath();
    if (canPaste(target)) ClipboardService::instance().executePaste(target, this);
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
    if (m_currentViewMode == ListView && m_listCanvas) {
        m_listCanvas->toggleFolderSectionCollapse();
    } else if ((m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) && m_gridCanvas) {
        m_gridCanvas->toggleFolderSectionCollapse();
    } else if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
    }
}

void ContentPanel::setViewMode(ViewMode mode) {
    if (m_currentViewMode == mode) {
        return;
    }

    // 🚀【闭环防线 1：第 0 毫秒时序校正与权威路径提取】
    // 若原视图为列视图，在采集选区快照前，优先提取最右侧列（最新展开的真实目录）作为权威路径
    if (m_currentViewMode == ColumnView && m_columnView) {
        ColumnViewPane* rPane = m_columnView->rightmostPane();
        if (!rPane) rPane = m_columnView->activePane();
        if (rPane && !rPane->currentPath().isEmpty()) {
            m_currentPath = rPane->currentPath();
        }
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
        m_viewStack->setCurrentWidget(m_listCanvas);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            // 🚀【闭环防线 2：防过度重建与闪烁】
            // 仅在列视图尚未包含当前路径时才执行 setRootPath 构建新列栈；
            // 若已包含（例如切去网格后切回），完好保留既有展开列栈与滚动条位置，零销毁、零闪烁！
            if (!m_columnView->containsPath(m_currentPath)) {
                m_columnView->setRootPath(m_currentPath);
            }
            m_viewStack->setCurrentWidget(m_columnView);
            m_columnView->scrollToRightmostPane();
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView);
        if (fjv) fjv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridCanvas);
    }

    // 🚀【闭环防线 3：切出列视图时无条件载入数据，并驱动全局导航原子对齐】
    if (oldMode == ColumnView && mode != ColumnView) {
        if (!m_currentPath.isEmpty()) {
            loadDirectory(m_currentPath, m_isRecursive);
            emit directorySelected(m_currentPath);
        }
    }

    // 2. 消费 SelectionState 真理源同步恢复选区
    restoreSelections();

    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    updateGridSize();
    emit viewModeChanged(mode);
    emit zoomLevelChanged(m_zoomLevel);

    if (m_visibleTimer) m_visibleTimer->start();

    if (m_viewCoordinator) {
        m_viewCoordinator->installActivationFilters();
    }
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
    if (m_gridCanvas) m_gridCanvas->updateZoom(m_zoomLevel);
    if (m_listCanvas) m_listCanvas->updateZoom(m_zoomLevel);
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
    auto pushFilter = [this](FilterProxyModel* p, bool wantFolders) {
        if (!p) return;
        FilterState s = m_currentFilter;
        s.showFolders = wantFolders;
        s.showFiles = !wantFolders;
        p->currentFilter = s;
        p->updateFilter();
    };
    pushFilter(m_folderProxyModel, true);
    pushFilter(m_fileProxyModel, false);
    pushFilter(m_gridFolderProxyModel, true);
    pushFilter(m_gridFileProxyModel, false);
    if (m_columnView) m_columnView->applyFilterState(m_currentFilter);
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
    QElapsedTimer timer;
    timer.start();
    QModelIndexList selected = getSelectedIndexes();
    qint64 tIndexes = timer.elapsed();

    QStringList paths;
    paths.reserve(selected.size());
    for (const auto& idx : selected) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) paths << p;
        }
    }
    qint64 tPaths = timer.elapsed();

    emit selectionChanged(paths);
    qint64 tEmit = timer.elapsed();

    // 消除重复调用：直接复用已有选区尺寸，禁止重复二次查询
    updateStatusBarStats(selected.size());
    qint64 tTotal = timer.elapsed();

    Logger::log(QString("[Perf] ContentPanel::emitSelectionChangedSignal: getSelectedIndexes=%1ms, parsePaths(%2)=%3ms, emitSignal=%4ms, updateStatus=%5ms, total=%6ms")
                .arg(tIndexes).arg(paths.size()).arg(tPaths - tIndexes).arg(tEmit - tPaths).arg(tTotal - tEmit).arg(tTotal));
}

void ContentPanel::updateStatusBarStats(int cachedSelectedCount) {
    FilterProxyModel* folderProxy = (m_currentViewMode == ListView && m_listCanvas) ? m_listCanvas->folderProxyModel() : (m_gridCanvas ? m_gridCanvas->folderProxyModel() : m_folderProxyModel);
    FilterProxyModel* fileProxy = (m_currentViewMode == ListView && m_listCanvas) ? m_listCanvas->fileProxyModel() : (m_gridCanvas ? m_gridCanvas->fileProxyModel() : m_fileProxyModel);

    int folderCount = folderProxy ? folderProxy->rowCount() : 0;
    int fileCount = fileProxy ? fileProxy->rowCount() : 0;
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
    if (m_currentViewMode == ListView && m_listCanvas) {
        m_listCanvas->refreshVisibleThumbnails(m_model);
    } else if ((m_currentViewMode == GridView || m_currentViewMode == JustifiedViewMode) && m_gridCanvas) {
        m_gridCanvas->refreshVisibleThumbnails(m_model);
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
    return m_viewCoordinator ? m_viewCoordinator->getActiveProxyModel() : nullptr;
}

QStringList ContentPanel::getSelectedPaths() const {
    return m_viewCoordinator ? m_viewCoordinator->getSelectedPaths() : QStringList();
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
    if (m_currentViewMode == ListView && m_listCanvas) {
        return m_listCanvas->activeItemView();
    }
    if (m_gridCanvas) {
        return m_gridCanvas->activeItemView();
    }
    return m_gridView;
}

QModelIndexList ContentPanel::getSelectedIndexes() const {
    if (m_currentViewMode == ColumnView) {
        QModelIndexList res;
        if (m_columnView && m_columnView->activePane()) {
            for (auto* view : {m_columnView->activePane()->folderListView(), m_columnView->activePane()->listView()}) {
                if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
                    for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                        if (idx.column() == 0) res.append(idx);
                    }
                }
            }
        }
        return res;
    }
    if (m_currentViewMode == ListView && m_listCanvas) {
        return m_listCanvas->getSelectedIndexes();
    }
    if (m_gridCanvas) {
        return m_gridCanvas->getSelectedIndexes();
    }
    return {};
}

void ContentPanel::restoreActiveView() {
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else if (m_currentViewMode == ListView) {
        m_viewStack->setCurrentWidget(m_listCanvas);
    } else {
        m_viewStack->setCurrentWidget(m_gridCanvas);
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
    if (m_currentViewMode == ListView && m_listCanvas) {
        if (m_listCanvas->folderView()) views << m_listCanvas->folderView();
        if (m_listCanvas->fileView()) views << m_listCanvas->fileView();
    } else if (m_gridCanvas) {
        if (m_gridCanvas->folderView()) views << m_gridCanvas->folderView();
        if (m_gridCanvas->fileView()) views << m_gridCanvas->fileView();
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