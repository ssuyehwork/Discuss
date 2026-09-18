#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SectionedScrollCanvas.h"
#include "FolderSectionWidget.h"
#include "ContentHeaderWidget.h"
#include "DropJustifiedView.h"
#include "DropTreeView.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
#include "JustifiedView.h"
#include "models/ItemModelBase.h"
#include "../core/CoreController.h"
#include <QHeaderView>
#include <QScrollBar>

namespace QuarkMeta {

SectionedScrollCanvas::SectionedScrollCanvas(CanvasType type, ItemModelBase* sourceModel, const FilterState& initialFilter, QObject* eventFilter, QWidget* parent)
    : QScrollArea(parent), m_type(type) {
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);

    m_containerWidget = new QWidget(this);
    m_layout = new QVBoxLayout(m_containerWidget);
    // 绝对照抄原数值：margins 0, spacing 0
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // 1. 文件夹代理模型 (仅文件夹)
    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(sourceModel);
    m_folderProxyModel->setFilterKeyColumn(0);
    m_folderProxyModel->setDynamicSortFilter(true);
    FilterState folderFilter = initialFilter;
    folderFilter.showFolders = true;
    folderFilter.showFiles = false;
    m_folderProxyModel->currentFilter = folderFilter;

    // 2. 文件代理模型 (仅文件)
    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(sourceModel);
    m_fileProxyModel->setFilterKeyColumn(0);
    m_fileProxyModel->setDynamicSortFilter(true);
    FilterState fileFilter = initialFilter;
    fileFilter.showFolders = false;
    fileFilter.showFiles = true;
    m_fileProxyModel->currentFilter = fileFilter;

    // 3. 标题栏
    m_folderHeader = new FolderSectionHeaderBar(m_containerWidget);
    m_folderHeader->hide();
    m_layout->addWidget(m_folderHeader);

    initViews(sourceModel, eventFilter);

    m_fileHeader = new FileSectionHeaderBar(m_containerWidget);
    m_fileHeader->hide();
    m_layout->addWidget(m_fileHeader);

    m_layout->addWidget(m_fileView);

    setWidget(m_containerWidget);

    setupConnections();
}

void SectionedScrollCanvas::initViews(ItemModelBase* /*sourceModel*/, QObject* eventFilter) {
    if (m_type == CanvasType::Grid) {
        auto* folderJv = new DropJustifiedView(m_containerWidget);
        folderJv->setFrameShape(QFrame::NoFrame);
        folderJv->setSelectionMode(QAbstractItemView::SingleSelection);
        folderJv->setContextMenuPolicy(Qt::CustomContextMenu);
        folderJv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        folderJv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 遵循 AllViewsCoExpansion
        folderJv->setModel(m_folderProxyModel);
        folderJv->setAspectRatioRole(AspectRatioRole);
        auto* fDelegate = new ThumbnailDelegate(this);
        fDelegate->setHasThumbnailRole(HasThumbnailRole);
        fDelegate->setRatingRole(RatingRole);
        fDelegate->setPathRole(PathRole);
        fDelegate->setPinnedRole(PinnedRole);
        fDelegate->setTypeRole(TypeRole);
        fDelegate->setIsEmptyRole(IsEmptyRole);
        fDelegate->setColorRole(ColorRole);
        folderJv->setItemDelegate(fDelegate);
        m_folderView = folderJv;
        m_folderView->hide();
        m_layout->addWidget(m_folderView);

        auto* fileJv = new DropJustifiedView(m_containerWidget);
        fileJv->setFrameShape(QFrame::NoFrame);
        fileJv->setSelectionMode(QAbstractItemView::ExtendedSelection);
        fileJv->setContextMenuPolicy(Qt::CustomContextMenu);
        fileJv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        fileJv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 遵循 AllViewsCoExpansion
        fileJv->setModel(m_fileProxyModel);
        fileJv->setAspectRatioRole(AspectRatioRole);
        auto* delegate = new ThumbnailDelegate(this);
        delegate->setHasThumbnailRole(HasThumbnailRole);
        delegate->setRatingRole(RatingRole);
        delegate->setPathRole(PathRole);
        delegate->setPinnedRole(PinnedRole);
        delegate->setTypeRole(TypeRole);
        delegate->setIsEmptyRole(IsEmptyRole);
        delegate->setColorRole(ColorRole);
        fileJv->setItemDelegate(delegate);
        m_fileView = fileJv;
    } else {
        auto* folderTv = new DropTreeView(m_containerWidget);
        folderTv->setFrameShape(QFrame::NoFrame);
        folderTv->setAlternatingRowColors(true);
        folderTv->setSortingEnabled(true);
        folderTv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        folderTv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 遵循 AllViewsCoExpansion
        folderTv->setContextMenuPolicy(Qt::CustomContextMenu);
        folderTv->setSelectionMode(QAbstractItemView::SingleSelection);
        folderTv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        folderTv->setRootIsDecorated(false);
        folderTv->setItemDelegate(new TreeItemDelegate(this, true, true));
        folderTv->setModel(m_folderProxyModel);
        // 绝对照抄原数值：表头高度 32，最小段尺寸 0
        folderTv->header()->setFixedHeight(32);
        folderTv->header()->setMinimumSectionSize(0);
        folderTv->applyColumnPolicies();
        m_folderView = folderTv;
        m_folderView->hide();
        m_layout->addWidget(m_folderView);

        auto* fileTv = new DropTreeView(m_containerWidget);
        fileTv->setFrameShape(QFrame::NoFrame);
        fileTv->setAlternatingRowColors(true);
        fileTv->setSortingEnabled(true);
        fileTv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        fileTv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 遵循 AllViewsCoExpansion
        fileTv->setContextMenuPolicy(Qt::CustomContextMenu);
        fileTv->setSelectionMode(QAbstractItemView::ExtendedSelection);
        fileTv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        fileTv->setRootIsDecorated(false);
        fileTv->setItemDelegate(new TreeItemDelegate(this, true, true));
        fileTv->setModel(m_fileProxyModel);
        // 绝对照抄原数值：表头高度 32，最小段尺寸 0
        fileTv->header()->setFixedHeight(32);
        fileTv->header()->setMinimumSectionSize(0);
        fileTv->applyColumnPolicies();
        m_fileView = fileTv;
    }

    if (eventFilter) {
        if (m_folderView) {
            m_folderView->installEventFilter(eventFilter);
            if (m_folderView->viewport()) m_folderView->viewport()->installEventFilter(eventFilter);
        }
        if (m_fileView) {
            m_fileView->installEventFilter(eventFilter);
            if (m_fileView->viewport()) m_fileView->viewport()->installEventFilter(eventFilter);
        }
    }
}

void SectionedScrollCanvas::setupConnections() {
    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderView && m_folderHeader->count() > 0) {
            m_folderView->setVisible(!collapsed);
        }
    });

    if (m_type == CanvasType::Grid) {
        if (auto* fjv = qobject_cast<JustifiedView*>(m_folderView)) {
            connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_folderView && m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                    m_folderView->setFixedHeight(height);
                }
            });
        }
        if (auto* jv = qobject_cast<JustifiedView*>(m_fileView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_fileView && m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                    m_fileView->setFixedHeight(height);
                }
            });
        }
    }

    auto onModelChanged = [this]() { updateSectionCounts(); };
    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);

    connect(m_folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SectionedScrollCanvas::selectionChanged);
    connect(m_fileView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SectionedScrollCanvas::selectionChanged);

    connect(m_folderView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);
    connect(m_fileView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);

    connect(m_folderView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(m_fileView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);

    if (m_type == CanvasType::Grid) {
        if (auto* dropFolder = qobject_cast<DropJustifiedView*>(m_folderView)) {
            connect(dropFolder, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropJustifiedView*>(m_fileView)) {
            connect(dropFile, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
    } else {
        if (auto* dropFolder = qobject_cast<DropTreeView*>(m_folderView)) {
            connect(dropFolder, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropTreeView*>(m_fileView)) {
            connect(dropFile, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
    }
}

void SectionedScrollCanvas::setSourceModel(ItemModelBase* model) {
    if (m_folderProxyModel) m_folderProxyModel->setSourceModel(model);
    if (m_fileProxyModel) m_fileProxyModel->setSourceModel(model);
}

void SectionedScrollCanvas::applyFilter(const FilterState& filter) {
    if (m_folderProxyModel) {
        FilterState s = filter;
        s.showFolders = true;
        s.showFiles = false;
        m_folderProxyModel->currentFilter = s;
        m_folderProxyModel->updateFilter();
    }
    if (m_fileProxyModel) {
        FilterState s = filter;
        s.showFolders = false;
        s.showFiles = true;
        m_fileProxyModel->currentFilter = s;
        m_fileProxyModel->updateFilter();
    }
}

void SectionedScrollCanvas::applySort(int column, Qt::SortOrder order) {
    if (m_folderProxyModel) m_folderProxyModel->sort(column, order);
    if (m_fileProxyModel) m_fileProxyModel->sort(column, order);
}

void SectionedScrollCanvas::updateSectionCounts() {
    if (!m_folderProxyModel || !m_fileProxyModel) return;
    int folderCount = m_folderProxyModel->rowCount();
    int fileCount = m_fileProxyModel->rowCount();

    if (m_folderHeader) {
        m_folderHeader->setCount(folderCount);
        m_folderHeader->setVisible(folderCount > 0);
    }
    if (m_folderView) {
        if (folderCount == 0) {
            m_folderView->hide();
        } else {
            bool collapsed = m_folderHeader ? m_folderHeader->isCollapsed() : false;
            m_folderView->setVisible(!collapsed);
            if (m_type == CanvasType::Grid) {
                if (auto* fjv = qobject_cast<JustifiedView*>(m_folderView)) {
                    m_folderView->setFixedHeight(fjv->totalHeight());
                }
            } else {
                // 绝对照抄原数值：默认行高 30，边距 2
                auto* tv = static_cast<QTreeView*>(m_folderView);
                int rowH = tv->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
                m_folderView->setFixedHeight(folderCount * rowH + hdrH + 2);
            }
        }
    }

    if (m_fileHeader) {
        m_fileHeader->setCount(fileCount);
        m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
    }
    if (m_fileView) {
        if (fileCount == 0) {
            m_fileView->hide();
        } else {
            m_fileView->show();
            if (m_type == CanvasType::Grid) {
                if (auto* jv = qobject_cast<JustifiedView*>(m_fileView)) {
                    m_fileView->setFixedHeight(jv->totalHeight());
                }
            } else {
                // 遵循 AllViewsCoExpansion.md：绝对照抄原数值
                auto* tv = static_cast<QTreeView*>(m_fileView);
                int rowH = tv->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
                m_fileView->setFixedHeight(fileCount * rowH + hdrH + 2);
            }
        }
    }
}

void SectionedScrollCanvas::updateZoom(int zoomLevel) {
    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_fileView)) jv->setTargetRowHeight(zoomLevel);
        if (auto* fjv = qobject_cast<JustifiedView*>(m_folderView)) fjv->setTargetRowHeight(zoomLevel);
    } else {
        auto* tree = static_cast<DropTreeView*>(m_fileView);
        if (auto* hdr = qobject_cast<ContentHeaderView*>(tree->header())) {
            hdr->setZoomLevel(zoomLevel);
        }
        // 绝对照抄原数值：qMax(16, zoomLevel - 8)
        tree->setIconSize(QSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8)));
        tree->doItemsLayout();
    }
}

void SectionedScrollCanvas::toggleFolderSectionCollapse() {
    if (m_folderHeader && m_folderHeader->isVisible() && m_folderHeader->count() > 0) {
        m_folderHeader->setCollapsed(!m_folderHeader->isCollapsed());
    }
}

QAbstractItemView* SectionedScrollCanvas::activeItemView() const {
    if (m_folderView && (m_folderView->hasFocus() || (m_folderView->selectionModel() && m_folderView->selectionModel()->hasSelection()))) {
        return m_folderView;
    }
    return m_fileView;
}

QModelIndexList SectionedScrollCanvas::getSelectedIndexes() const {
    QModelIndexList res;
    for (auto* view : {m_folderView, m_fileView}) {
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

void SectionedScrollCanvas::refreshVisibleThumbnails(ItemModelBase* model) {
    if (!model || CoreController::isShuttingDown()) return;

    // 🚀【真实视口几何空间投影】：彻底根治全高撑开下的全量加载卡顿
    QRect vpRect = viewport()->rect();
    QSet<int> visibleRows;

    auto scanView = [&](QAbstractItemView* view, FilterProxyModel* proxy) {
        if (!view || !view->isVisible() || !proxy || proxy->rowCount() == 0) return;

        // 将 ScrollArea 外层可见物理矩形投影至子视图坐标系
        QPoint topPoint = view->mapFromGlobal(viewport()->mapToGlobal(vpRect.topLeft()));
        QPoint btmPoint = view->mapFromGlobal(viewport()->mapToGlobal(vpRect.bottomRight()));

        if (topPoint.y() >= view->height() || btmPoint.y() <= 0) return;

        int clampedTopY = qBound(0, topPoint.y(), view->height());
        int clampedBtmY = qBound(0, btmPoint.y(), view->height());

        QModelIndex topIdx = view->indexAt(QPoint(10, clampedTopY));
        QModelIndex btmIdx = view->indexAt(QPoint(10, clampedBtmY));

        // 绝对照抄原数值：缓冲前后 4 行
        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) {
                visibleRows.insert(srcIdx.row());
            }
        }
    };

    scanView(m_folderView, m_folderProxyModel);
    scanView(m_fileView, m_fileProxyModel);

    if (!visibleRows.isEmpty()) {
        model->loadThumbnailsForRows(visibleRows.values());
    }
}

} // namespace QuarkMeta
