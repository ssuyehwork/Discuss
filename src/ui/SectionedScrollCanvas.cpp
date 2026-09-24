#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SectionedScrollCanvas.h"
#include "DualSectionPanel.h"
#include "FolderSectionWidget.h"
#include "ContentHeaderWidget.h"
#include "DropJustifiedView.h"
#include "DropTreeView.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
#include "JustifiedView.h"
#include "models/ItemModelBase.h"
#include "../core/NavigationService.h"
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

namespace QuarkMeta {

SectionedScrollCanvas::SectionedScrollCanvas(CanvasType type, FilterProxyModel* folderProxy, FilterProxyModel* fileProxy, QObject* eventFilter, QWidget* parent)
    : QScrollArea(parent), m_type(type), m_folderProxyModel(folderProxy), m_fileProxyModel(fileProxy) {
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);

    QAbstractItemView* folderView = createFolderView(eventFilter);
    QAbstractItemView* fileView = createFileView(eventFilter);

    m_panel = new DualSectionPanel(folderView, fileView, m_folderProxyModel, m_fileProxyModel, this);
    m_panel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_panel->setFocusPolicy(Qt::StrongFocus);
    m_panel->setAcceptDrops(true);
    setWidget(m_panel);

    if (eventFilter) {
        installEventFilter(eventFilter);
        if (viewport()) viewport()->installEventFilter(eventFilter);
        m_panel->installEventFilter(eventFilter);
    }

    setupConnections();

    if (verticalScrollBar()) {
        connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            if (m_folderProxyModel && m_folderProxyModel->sourceModel()) {
                if (auto* diskModel = qobject_cast<ItemModelBase*>(m_folderProxyModel->sourceModel())) {
                    m_panel->refreshVisibleThumbnails(diskModel, viewport());
                }
            }
        });
    }
}

QAbstractItemView* SectionedScrollCanvas::createFolderView(QObject* eventFilter) {
    QAbstractItemView* view = nullptr;
    if (m_type == CanvasType::Grid) {
        auto* folderJv = new DropJustifiedView();
        folderJv->setFrameShape(QFrame::NoFrame);
        folderJv->setSelectionMode(QAbstractItemView::SingleSelection);
        folderJv->setContextMenuPolicy(Qt::CustomContextMenu);
        folderJv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        folderJv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
        view = folderJv;
    } else {
        auto* folderTv = new DropTreeView();
        folderTv->setFrameShape(QFrame::NoFrame);
        folderTv->setAlternatingRowColors(true);
        folderTv->setSortingEnabled(true);
        folderTv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        folderTv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        folderTv->setContextMenuPolicy(Qt::CustomContextMenu);
        folderTv->setSelectionMode(QAbstractItemView::SingleSelection);
        folderTv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        folderTv->setRootIsDecorated(false);
        folderTv->setItemDelegate(new TreeItemDelegate(this, true, true));
        folderTv->setModel(m_folderProxyModel);
        folderTv->header()->setFixedHeight(32);
        folderTv->header()->setMinimumSectionSize(0);
        folderTv->applyColumnPolicies();
        view = folderTv;
    }
    if (eventFilter && view) {
        view->installEventFilter(eventFilter);
        if (view->viewport()) view->viewport()->installEventFilter(eventFilter);
    }
    return view;
}

QAbstractItemView* SectionedScrollCanvas::createFileView(QObject* eventFilter) {
    QAbstractItemView* view = nullptr;
    if (m_type == CanvasType::Grid) {
        auto* fileJv = new DropJustifiedView();
        fileJv->setFrameShape(QFrame::NoFrame);
        fileJv->setSelectionMode(QAbstractItemView::ExtendedSelection);
        fileJv->setContextMenuPolicy(Qt::CustomContextMenu);
        fileJv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        fileJv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
        view = fileJv;
    } else {
        auto* fileTv = new DropTreeView();
        fileTv->setFrameShape(QFrame::NoFrame);
        fileTv->setAlternatingRowColors(true);
        fileTv->setSortingEnabled(true);
        fileTv->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        fileTv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        fileTv->setContextMenuPolicy(Qt::CustomContextMenu);
        fileTv->setSelectionMode(QAbstractItemView::ExtendedSelection);
        fileTv->setEditTriggers(QAbstractItemView::NoEditTriggers);
        fileTv->setRootIsDecorated(false);
        fileTv->setItemDelegate(new TreeItemDelegate(this, true, true));
        fileTv->setModel(m_fileProxyModel);
        fileTv->header()->setFixedHeight(32);
        fileTv->header()->setMinimumSectionSize(0);
        fileTv->applyColumnPolicies();
        view = fileTv;
    }
    if (eventFilter && view) {
        view->installEventFilter(eventFilter);
        if (view->viewport()) view->viewport()->installEventFilter(eventFilter);
    }
    return view;
}

QAbstractItemView* SectionedScrollCanvas::folderView() const { return m_panel->folderView(); }
QAbstractItemView* SectionedScrollCanvas::fileView() const { return m_panel->fileView(); }
FolderSectionHeaderBar* SectionedScrollCanvas::folderHeader() const { return m_panel->folderHeader(); }
FileSectionHeaderBar* SectionedScrollCanvas::fileHeader() const { return m_panel->fileHeader(); }

void SectionedScrollCanvas::setupConnections() {
    connect(m_panel, &DualSectionPanel::selectionChanged, this, &SectionedScrollCanvas::selectionChanged);
    connect(m_panel, &DualSectionPanel::folderCollapseToggled, this, [this](bool) {
        updateSectionCounts();
    });

    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();

    if (m_type == CanvasType::Grid) {
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
            connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                    m_panel->folderView()->setFixedHeight(height);
                }
            });
        }
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
            connect(jv, &JustifiedView::totalHeightChanged, this, [this](int height) {
                if (m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                    m_panel->fileView()->setFixedHeight(qMax(height, m_panel->fileViewMinHeight()));
                }
            });
        }
    }

    auto onModelChanged = [this]() { updateSectionCounts(); };
    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, onModelChanged);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, onModelChanged);

    connect(folderView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);
    connect(fileView, &QAbstractItemView::doubleClicked, this, &SectionedScrollCanvas::doubleClicked);

    connect(this, &QScrollArea::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(m_panel, &QWidget::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(folderView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);
    connect(fileView, &QAbstractItemView::customContextMenuRequested, this, &SectionedScrollCanvas::customContextMenuRequested);

    if (m_type == CanvasType::Grid) {
        if (auto* dropFolder = qobject_cast<DropJustifiedView*>(folderView)) {
            connect(dropFolder, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropJustifiedView*>(fileView)) {
            connect(dropFile, &DropJustifiedView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
    } else {
        if (auto* dropFolder = qobject_cast<DropTreeView*>(folderView)) {
            connect(dropFolder, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_folderProxyModel);
            });
        }
        if (auto* dropFile = qobject_cast<DropTreeView*>(fileView)) {
            connect(dropFile, &DropTreeView::pathsDropped, this, [this](const QStringList& p, const QModelIndex& idx) {
                emit pathsDropped(p, idx, m_fileProxyModel);
            });
        }
    }
}

void SectionedScrollCanvas::updateSectionCounts() {
    m_panel->updateSectionCounts(viewport()->height());
    if (!m_folderProxyModel || !m_fileProxyModel) return;

    int folderCount = m_folderProxyModel->rowCount();
    int fileCount = m_fileProxyModel->rowCount();
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();

    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                int baseH = fjv->totalHeight();
                if (fileCount == 0) {
                    folderView->setFixedHeight(qMax(baseH, m_panel->folderViewMinHeight()));
                } else {
                    folderView->setFixedHeight(baseH);
                }
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int baseH = folderCount * rowH + hdrH + 2;
            if (fileCount == 0) {
                folderView->setFixedHeight(qMax(baseH, m_panel->folderViewMinHeight()));
            } else {
                folderView->setFixedHeight(baseH);
            }
            folderView->updateGeometry();
        }
    }

    if (fileView && fileCount > 0) {
        if (m_type == CanvasType::Grid) {
            if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
                fileView->setFixedHeight(qMax(jv->totalHeight(), m_panel->fileViewMinHeight()));
            }
        } else {
            auto* tv = static_cast<QTreeView*>(fileView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            fileView->setFixedHeight(qMax(fileCount * rowH + hdrH + 2, m_panel->fileViewMinHeight()));
            fileView->updateGeometry();
        }
    }
}

void SectionedScrollCanvas::updateZoom(int zoomLevel) {
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();
    if (m_type == CanvasType::Grid) {
        if (auto* jv = qobject_cast<JustifiedView*>(fileView)) jv->setTargetRowHeight(zoomLevel);
        if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) fjv->setTargetRowHeight(zoomLevel);
    } else {
        QSize iconSize(qMax(16, zoomLevel - 8), qMax(16, zoomLevel - 8));
        if (auto* folderTree = qobject_cast<DropTreeView*>(folderView)) {
            folderTree->setIconSize(iconSize);
            folderTree->doItemsLayout();
        }
        if (auto* fileTree = qobject_cast<DropTreeView*>(fileView)) {
            if (auto* hdr = qobject_cast<ContentHeaderView*>(fileTree->header())) {
                hdr->setZoomLevel(zoomLevel);
            }
            fileTree->setIconSize(iconSize);
            fileTree->doItemsLayout();
        }
        updateSectionCounts();
    }
}

void SectionedScrollCanvas::toggleFolderSectionCollapse() {
    m_panel->toggleFolderSectionCollapse();
}

QAbstractItemView* SectionedScrollCanvas::activeItemView() const {
    return m_panel->activeItemView();
}

QModelIndexList SectionedScrollCanvas::getSelectedIndexes() const {
    return m_panel->getSelectedIndexes();
}

void SectionedScrollCanvas::refreshVisibleThumbnails(ItemModelBase* model) {
    m_panel->refreshVisibleThumbnails(model, viewport());
}

void SectionedScrollCanvas::resizeEvent(QResizeEvent* event) {
    QScrollArea::resizeEvent(event);
    updateSectionCounts();
}

void SectionedScrollCanvas::mousePressEvent(QMouseEvent* event) {
    auto* folderView = m_panel->folderView();
    auto* fileView = m_panel->fileView();
    if (folderView && folderView->selectionModel()) folderView->selectionModel()->clearSelection();
    if (fileView && fileView->selectionModel()) fileView->selectionModel()->clearSelection();
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
            emit pathsDropped(paths, QModelIndex(), m_fileProxyModel);
            event->acceptProposedAction();
            return;
        }
    }
    QScrollArea::dropEvent(event);
}

} // namespace QuarkMeta
