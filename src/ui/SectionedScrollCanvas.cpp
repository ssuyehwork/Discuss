#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SectionedScrollCanvas.h"
#include "FolderSectionWidget.h"
#include "ContentHeaderWidget.h"
#include "DropTreeView.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
#include "JustifiedView.h"
#include "DropJustifiedView.h"
#include "models/ItemModelBase.h"
#include "../core/NavigationService.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

namespace QuarkMeta {

SectionedScrollCanvas::SectionedScrollCanvas(CanvasType type, FilterProxyModel* proxyModel, QObject* eventFilter, QWidget* parent)
    : QScrollArea(parent), m_type(type), m_proxyModel(proxyModel) {
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);

    m_containerWidget = new QWidget(this);
    m_containerLayout = new QVBoxLayout(m_containerWidget);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);

    m_folderHeader = new FolderSectionHeaderBar(m_containerWidget);
    m_folderHeader->hide();
    m_containerLayout->addWidget(m_folderHeader);

    m_unifiedView = createUnifiedView(eventFilter);
    m_containerLayout->addWidget(m_unifiedView);

    m_fileHeader = new FileSectionHeaderBar(m_containerWidget);
    m_fileHeader->hide();
    m_containerLayout->addWidget(m_fileHeader);

    setWidget(m_containerWidget);

    if (eventFilter) {
        installEventFilter(eventFilter);
        if (viewport()) viewport()->installEventFilter(eventFilter);
        m_containerWidget->installEventFilter(eventFilter);
    }

    setupConnections();

    m_scrollThumbTimer = new QTimer(this);
    m_scrollThumbTimer->setSingleShot(true);
    m_scrollThumbTimer->setInterval(60);
    connect(m_scrollThumbTimer, &QTimer::timeout, this, [this]() {
        // 定时器扫描逻辑
    });

    if (verticalScrollBar()) {
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            triggerVisibleScan();
        });
    }
}

QAbstractItemView* SectionedScrollCanvas::createUnifiedView(QObject* eventFilter) {
    QAbstractItemView* view = nullptr;
    if (m_type == CanvasType::Grid) {
        auto* jv = new DropJustifiedView();
        jv->setFrameShape(QFrame::NoFrame);
        jv->setSelectionMode(QAbstractItemView::ExtendedSelection);
        jv->setContextMenuPolicy(Qt::CustomContextMenu);
        jv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        jv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        jv->setModel(m_proxyModel);
        jv->setAspectRatioRole(AspectRatioRole);
        auto* delegate = new ThumbnailDelegate(this);
        delegate->setHasThumbnailRole(HasThumbnailRole);
        delegate->setRatingRole(RatingRole);
        delegate->setPathRole(PathRole);
        delegate->setPinnedRole(PinnedRole);
        delegate->setTypeRole(TypeRole);
        delegate->setIsEmptyRole(IsEmptyRole);
        delegate->setColorRole(ColorRole);
        jv->setItemDelegate(delegate);
        view = jv;
    } else {
        auto* tv = new DropTreeView();
        tv->setFrameShape(QFrame::NoFrame);
        tv->setAlternatingRowColors(true);
        tv->setSortingEnabled(true);
        tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        tv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tv->setContextMenuPolicy(Qt::CustomContextMenu);
        tv->setSelectionMode(QAbstractItemView::ExtendedSelection);
        tv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tv->setRootIsDecorated(false);
        tv->setItemDelegate(new TreeItemDelegate(this, true, true));
        tv->setModel(m_proxyModel);
        tv->header()->setFixedHeight(32);
        tv->header()->setMinimumSectionSize(0);
        tv->applyColumnPolicies();
        view = tv;
    }
    if (eventFilter && view) {
        view->installEventFilter(eventFilter);
        if (view->viewport()) view->viewport()->installEventFilter(eventFilter);
    }
    return view;
}

void SectionedScrollCanvas::setupConnections() {
    if (m_unifiedView && m_unifiedView->selectionModel()) {
        connect(m_unifiedView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SectionedScrollCanvas::selectionChanged);
    }

    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_unifiedView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_unifiedView) {
                    m_unifiedView->setFixedHeight(height);
                }
            });
            connect(jv, &JustifiedView::layoutFinished, this, &SectionedScrollCanvas::triggerVisibleScan);
        }
    }

    if (m_proxyModel) {
        auto onModelChanged = [this]() {
            updateSectionCounts();
            triggerVisibleScan();
        };
        connect(m_proxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
        connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
    }

    if (m_unifiedView) {
        connect(m_unifiedView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);
        connect(m_unifiedView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    }

    connect(this, &QScrollArea::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);

    if (m_type == CanvasType::Grid) {
        if (auto* dropJv = qobject_cast<DropJustifiedView*>(m_unifiedView)) {
            connect(dropJv, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_proxyModel);
            });
        }
    } else {
        if (auto* dropTv = qobject_cast<DropTreeView*>(m_unifiedView)) {
            connect(dropTv, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_proxyModel);
            });
        }
    }
}

void SectionedScrollCanvas::updateSectionCounts() {
    if (!m_proxyModel) return;

    int folderCount = 0;
    int fileCount = 0;
    int total = m_proxyModel->rowCount();

    for (int i = 0; i < total; ++i) {
        QModelIndex idx = m_proxyModel->index(i, 0);
        bool isDir = idx.data(TypeRole).toString() == "folder" || idx.data(Qt::UserRole + 2).toBool();
        if (isDir) {
            folderCount++;
        } else {
            fileCount++;
        }
    }

    if (m_folderHeader) {
        m_folderHeader->setCount(folderCount);
        m_folderHeader->setVisible(folderCount > 0);
    }

    if (m_fileHeader) {
        m_fileHeader->setCount(fileCount);
        m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
    }

    if (m_unifiedView && total > 0) {
        if (m_type == CanvasType::Grid) {
            if (auto* jv = qobject_cast<JustifiedView*>(m_unifiedView)) {
                m_unifiedView->setFixedHeight(jv->totalHeight());
            }
        } else {
            auto* tv = static_cast<QTreeView*>(m_unifiedView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            m_unifiedView->setFixedHeight(total * rowH + hdrH + 2);
            m_unifiedView->updateGeometry();
        }
    }
}

void SectionedScrollCanvas::updateZoom(int zoomLevel) {
    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_unifiedView)) jv->setTargetRowHeight(zoomLevel);
    } else {
        QSize iconSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8));
        if (auto* tv = qobject_cast<DropTreeView*>(m_unifiedView)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(tv->header())) {
                hdr->setZoomLevel(zoomLevel);
            }
            tv->setIconSize(iconSize);
            tv->doItemsLayout();
        }
        updateSectionCounts();
    }
}

void SectionedScrollCanvas::toggleFolderSectionCollapse() {
}

QAbstractItemView* SectionedScrollCanvas::activeItemView() const {
    return m_unifiedView;
}

QModelIndexList SectionedScrollCanvas::getSelectedIndexes() const {
    if (m_unifiedView && m_unifiedView->selectionModel()) {
        return m_unifiedView->selectionModel()->selectedIndexes();
    }
    return {};
}

void SectionedScrollCanvas::refreshVisibleThumbnails(ItemModelBase*) {
}

bool SectionedScrollCanvas::eventFilter(QObject* obj, QEvent* event) {
    return QScrollArea::eventFilter(obj, event);
}

void SectionedScrollCanvas::triggerVisibleScan() {
    if (m_scrollThumbTimer) {
        m_scrollThumbTimer->start();
    }
}

void SectionedScrollCanvas::resizeEvent(QResizeEvent* event) {
    QScrollArea::resizeEvent(event);
    updateSectionCounts();
    triggerVisibleScan();
}

void SectionedScrollCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_unifiedView && m_unifiedView->selectionModel()) {
            m_unifiedView->selectionModel()->clearSelection();
        }
    }
    QScrollArea::mousePressEvent(event);
}

void SectionedScrollCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        NavigationService::instance().goUp();
        event->accept();
        return;
    }
    QScrollArea::mouseDoubleClickEvent(event);
}

void SectionedScrollCanvas::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QScrollArea::dragEnterEvent(event);
    }
}

void SectionedScrollCanvas::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QScrollArea::dragMoveEvent(event);
    }
}

void SectionedScrollCanvas::dropEvent(QDropEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        QStringList paths;
        for (const QUrl& url : event->mimeData()->urls()) {
            QString localPath = url.toLocalFile();
            if (!localPath.isEmpty()) paths << localPath;
        }
        if (!paths.isEmpty()) {
            emit pathsDropped(paths, QModelIndex(), m_proxyModel);
            event->acceptProposedAction();
            return;
        }
    }
    QScrollArea::dropEvent(event);
}

} // namespace QuarkMeta
