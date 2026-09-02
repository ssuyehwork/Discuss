#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ContentPanel.h"
#include "controllers/ContentContextMenu.h"
#include "controllers/ContentKeyHandler.h"
#include "controllers/ContentSortController.h"
#include "controllers/ContentDataLoader.h"
#include "controllers/ContentFileOpsHandler.h"
#include "workers/ContentStatsWorker.h"
#include "DropJustifiedView.h"
#include "DropTreeView.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
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

#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QScrollBar>
#include <QFileInfo>
#include <QDir>
#include <QApplication>

namespace QuarkMeta {

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

    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setDynamicSortFilter(true);

    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(60);
    connect(m_visibleTimer, &QTimer::timeout, this, &ContentPanel::refreshVisibleThumbnails);

    m_sortController = new ContentSortController(this);
    connect(m_sortController, &ContentSortController::sortCriteriaChanged, this, [this](SortType, Qt::SortOrder) {
        m_sortController->applySortToModel(m_proxyModel);
    });
    m_sortController->applySortToModel(m_proxyModel);

    m_dataLoader = new ContentDataLoader(this);
    m_fileOpsHandler = new ContentFileOpsHandler(this);
    m_statsWorker = new ContentStatsWorker(this);

    connect(m_statsWorker, &ContentStatsWorker::statsReady, this, [this](const ScanStats& stats) {
        auto* proxy = qobject_cast<FilterProxyModel*>(m_proxyModel);
        if (proxy) {
            proxy->setCachedDuplicatePaths(stats.duplicatePaths);
        }
        emit directoryStatsReady(stats);
    });

    m_zoomLevel = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt();
    m_showFolders = AppConfig::instance().getValue("ContentPanel/ShowFolders", true).toBool();
    m_showFiles = AppConfig::instance().getValue("ContentPanel/ShowFiles", true).toBool();
    m_showHidden = AppConfig::instance().getValue("ContentPanel/ShowHidden", false).toBool();

    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;

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
    QWidget* titleBar = new QWidget(this);
    titleBar->setObjectName("ContainerHeader");
    titleBar->setFixedHeight(32);

    QHBoxLayout* titleL = new QHBoxLayout(titleBar);
    titleL->setContentsMargins(15, 0, 5, 0);
    titleL->setSpacing(5);

    QLabel* iconLabel = new QLabel(titleBar);
    iconLabel->setPixmap(UiHelper::getIcon("eye", QColor("#41F2F2"), 18).pixmap(18, 18));
    titleL->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("内容", titleBar);
    titleLabel->setObjectName("ContentPanelTitleLabel");
    titleL->addWidget(titleLabel);
    titleL->addStretch();

    auto setupToggleBtn = [this, titleBar, titleL](QPushButton*& btn, const QString& icon, const QColor& actCol, bool checked, const QString& tip, auto slot) {
        btn = new QPushButton(titleBar);
        btn->setCheckable(true);
        btn->setFixedSize(24, 24);
        btn->setChecked(checked);
        btn->setIcon(UiHelper::getIcon(icon, checked ? actCol : QColor("#888888"), 16));
        btn->setProperty("tooltipText", tip);
        btn->installEventFilter(this);
        btn->setObjectName("ViewModeToolBtn");
        connect(btn, &QPushButton::clicked, this, slot);
        titleL->addWidget(btn, 0, Qt::AlignVCenter);
    };

    setupToggleBtn(m_btnToggleHidden, "eye", QColor("#3498db"), m_showHidden, "显示/隐藏隐藏项目", [this]() {
        m_showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowHidden", m_showHidden);
        m_currentFilter.showHidden = m_showHidden;
        applyFilters();
    });

    setupToggleBtn(m_btnToggleFolders, "folder_filled", QColor("#FDB70A"), m_showFolders, "显示/隐藏文件夹", [this]() {
        m_showFolders = m_btnToggleFolders->isChecked();
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFolders", m_showFolders);
        m_currentFilter.showFolders = m_showFolders;
        applyFilters();
    });

    setupToggleBtn(m_btnToggleFiles, "file", QColor("#2ecc71"), m_showFiles, "显示/隐藏文件", [this]() {
        m_showFiles = m_btnToggleFiles->isChecked();
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFiles", m_showFiles);
        m_currentFilter.showFiles = m_showFiles;
        applyFilters();
    });

    m_btnLayers = new QPushButton(titleBar);
    m_btnLayers->setCheckable(true);
    m_btnLayers->setFixedSize(24, 24);
    m_btnLayers->setIcon(UiHelper::getIcon("layers", QColor("#2ecc71"), 18));
    m_btnLayers->setProperty("tooltipText", "显示子文件夹中的项目");
    m_btnLayers->installEventFilter(this);
    m_btnLayers->setObjectName("ViewModeToolBtn");
    connect(m_btnLayers, &QPushButton::clicked, this, [this]() {
        if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
            m_btnLayers->setChecked(false);
            return;
        }
        loadDirectory(m_currentPath, m_btnLayers->isChecked());
    });
    titleL->addWidget(m_btnLayers, 0, Qt::AlignVCenter);

    m_mainLayout->addWidget(titleBar);

    m_viewStack = new QStackedWidget(this);
    m_viewStack->setFrameShape(QFrame::NoFrame);
    initGridView();
    initListView();
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);
    m_viewStack->setCurrentWidget(m_gridView);

    m_mainLayout->addWidget(m_viewStack, 1);
}

void ContentPanel::initGridView() {
    m_gridView = new DropJustifiedView(this);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_gridView->setModel(m_proxyModel);

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
    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_gridView, SIGNAL(pathsDropped(QStringList,QModelIndex)), this, SLOT(onPathsDropped(QStringList,QModelIndex)));
}

void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
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

bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    if (m_keyHandler && m_keyHandler->handleEvent(obj, event)) return true;
    return QFrame::eventFilter(obj, event);
}

void ContentPanel::ensureSourceModelIsDiskModel() {
    if (m_model != m_diskModel) {
        m_model = m_diskModel;
        m_proxyModel->setSourceModel(m_model);
    }
}

void ContentPanel::applySort() {
    if (m_sortController) {
        m_sortController->applySortToModel(m_proxyModel);
    }
}

void ContentPanel::startVisibleTimer() {
    if (m_visibleTimer) {
        m_visibleTimer->start();
    }
}

void ContentPanel::onCustomContextMenuRequested(const QPoint& pos) {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    if (!view) view = (m_viewStack && m_viewStack->currentWidget() == m_gridView) ? m_gridView : m_treeView;
    if (!view) return;
    ContentContextMenu menuHandler(this);
    menuHandler.showMenu(view, pos);
}

void ContentPanel::loadDirectory(const QString& path, bool recursive) {
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

void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex) {
    if (m_fileOpsHandler) m_fileOpsHandler->onPathsDropped(paths, targetIndex);
}

void ContentPanel::onDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QString path = index.data(PathRole).toString();
    if (path.isEmpty()) return;
    if (QFileInfo(path).isDir()) {
        emit directorySelected(path);
    } else {
        AppCommand cmd;
        cmd.type = AppCommandType::RecordAccess;
        cmd.targetPaths << path;
        CoreEngine::instance().executeCommand(cmd);

        QString ext = QFileInfo(path).suffix().toLower();
        static const QSet<QString> whiteList = {
            "jpg", "jpeg", "png", "bmp", "webp", "gif", "ico", "cur", "ani", "psd", "ai", "eps", "pdf", "svg",
            "txt", "md", "markdown", "log", "cpp", "h", "hpp", "c", "py", "js", "css", "html", "json", "xml", "ini", "conf", "yaml", "yml"
        };
        if (whiteList.contains(ext)) emit requestQuickLook(path);
    }
}

void ContentPanel::setViewMode(ViewMode mode) {
    m_currentViewMode = mode;
    int minZoom = (mode == ListView) ? 30 : 93;
    m_zoomLevel = qBound(minZoom, m_zoomLevel, 230);

    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridView);
    }

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
    if (m_viewStack->currentWidget() == m_gridView) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
            jv->setTargetRowHeight(m_zoomLevel);
        }
    } else if (m_viewStack->currentWidget() == m_treeView) {
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
    m_currentFilter = state;
    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;
    applyFilters();
}

void ContentPanel::applyFilters() {
    auto* proxy = qobject_cast<FilterProxyModel*>(m_proxyModel);
    if (proxy) {
        proxy->currentFilter = m_currentFilter;
        proxy->updateFilter();
    }
    updateStatusBarStats();
}

void ContentPanel::search(const QString& query) {
    m_currentFilter.keyword = query;
    applyFilters();
}

void ContentPanel::refreshAll() {
    if (!m_currentPath.isEmpty() && m_currentPath != "computer://") loadDirectory(m_currentPath, m_isRecursive);
    else loadDirectory("computer://");
}

void ContentPanel::updateItemMetadata(const QString& path) { if (m_model) m_model->updateRecordMetadata(path); }
void ContentPanel::migrateModelCache(const QString& oldPath, const QString& newPath) { if (m_model) m_model->migrateCache(oldPath, newPath); }
void ContentPanel::clearFolderCache(const QString& folderPath) { if (m_model) m_model->clearCacheForFolder(folderPath); }

void ContentPanel::onSelectionChanged() {
    if (!m_selectionTimer) {
        m_selectionTimer = new QTimer(this);
        m_selectionTimer->setSingleShot(true);
        m_selectionTimer->setInterval(30);
        connect(m_selectionTimer, &QTimer::timeout, this, &ContentPanel::emitSelectionChangedSignal);
    }
    m_selectionTimer->start();
}

void ContentPanel::emitSelectionChangedSignal() {
    QList<QModelIndex> indexes = getSelectedIndexes();
    QStringList paths;
    paths.reserve(qMin(indexes.size(), 50));
    for (const auto& idx : indexes) {
        if (idx.isValid()) paths.append(idx.data(PathRole).toString());
        if (paths.size() >= 50) break;
    }
    emit selectionChanged(paths);
    updateStatusBarStats();
}

void ContentPanel::updateStatusBarStats() {
    if (m_proxyModel) emit statusBarStatsUpdated(0, 0, m_proxyModel->rowCount());
}

void ContentPanel::recalculateAndEmitStats() {
    if (!m_model || m_model->allRecords().empty()) return;
    if (m_statsWorker) {
        m_statsWorker->processAsync(m_model->allRecords(), m_currentFilter.showHidden);
    }
}

void ContentPanel::refreshVisibleThumbnails() {
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    if (!view || !m_model || CoreController::isShuttingDown()) return;

    int top = 0, bottom = m_proxyModel->rowCount() - 1;
    QModelIndex topIdx = view->indexAt(QPoint(10, 10));
    QModelIndex btmIdx = view->indexAt(QPoint(view->viewport()->width() - 10, view->viewport()->height() - 10));
    if (topIdx.isValid()) top = qMax(0, topIdx.row() - 4);
    if (btmIdx.isValid()) bottom = qMin(m_proxyModel->rowCount() - 1, btmIdx.row() + 4);

    QList<int> visibleRows;
    for (int r = top; r <= bottom; ++r) {
        QModelIndex proxyIdx = m_proxyModel->index(r, 0);
        QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        if (srcIdx.isValid()) visibleRows.append(srcIdx.row());
    }

    m_model->loadThumbnailsForRows(visibleRows);
}

void ContentPanel::selectAndScrollToPath(const QString& path) { selectAndScrollToItem(path); }
void ContentPanel::selectAndScrollToItem(const QString& path) {
    if (!m_proxyModel || path.isEmpty()) return;
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        QModelIndex proxyIdx = m_proxyModel->index(i, 0);
        if (proxyIdx.data(PathRole).toString() == path) {
            QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
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
    if (!m_proxyModel || m_proxyModel->rowCount() == 0) return QString();
    int curIdx = -1;
    for (int i = 0; i < m_proxyModel->rowCount(); ++i) {
        if (m_proxyModel->index(i, 0).data(PathRole).toString() == currentPath) { curIdx = i; break; }
    }
    if (curIdx == -1) return QString();
    int target = curIdx + delta;
    if (target < 0 || target >= m_proxyModel->rowCount()) return QString();
    return m_proxyModel->index(target, 0).data(PathRole).toString();
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

QModelIndexList ContentPanel::getSelectedIndexes() const {
    if (!m_viewStack) return {};
    bool isGrid = (m_viewStack->currentWidget() == m_gridView);
    QItemSelectionModel* sel = isGrid ? m_gridView->selectionModel() : m_treeView->selectionModel();
    if (!sel) return {};
    if (isGrid) {
        QModelIndexList res;
        for (const auto& idx : sel->selectedIndexes()) if (idx.column() == 0) res.append(idx);
        return res;
    }
    return sel->selectedRows(0);
}

void ContentPanel::restoreActiveView() {
    m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? static_cast<QWidget*>(m_treeView) : static_cast<QWidget*>(m_gridView));
}

void ContentPanel::restoreSelections() {
    if (m_pendingSelectNames.isEmpty()) return;
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(m_viewStack->currentWidget());
    if (view && view->selectionModel()) {
        QItemSelection sel;
        QModelIndex lastIdx;
        const auto& recs = m_model->allRecords();
        for (size_t i = 0; i < recs.size(); ++i) {
            if (m_pendingSelectNames.contains(QFileInfo(recs[i].path).fileName())) {
                QModelIndex pIdx = m_proxyModel->mapFromSource(m_model->index(static_cast<int>(i), 0));
                if (pIdx.isValid()) { sel.select(pIdx, pIdx); lastIdx = pIdx; }
            }
        }
        view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (m_isPendingEdit) view->edit(lastIdx); }
    }
    m_pendingSelectNames.clear();
}

void ContentPanel::setPendingSelectName(const QString& name, bool edit) {
    m_pendingSelectNames.clear();
    if (!name.isEmpty()) m_pendingSelectNames.insert(name);
    m_isPendingEdit = edit;
}

void ContentPanel::updateLayersButtonState() {
    if (!m_btnLayers) return;
    bool isComp = m_currentPath.isEmpty() || m_currentPath == "computer://";
    m_btnLayers->setEnabled(!isComp);
    m_btnLayers->setProperty("tooltipText", isComp ? "“此电脑”不支持递归显示" : "显示子文件夹中的项目");
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
