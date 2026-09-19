#include "ViewDragDropHelper.h"
#include "../core/ModelContract.h"
#include <QMimeData>
#include <QDrag>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QItemSelectionModel>

namespace QuarkMeta {

DragDropEventFilter::DragDropEventFilter(QAbstractItemView* targetView, QObject* parent)
    : QObject(parent ? parent : targetView), m_targetView(targetView) {
}

void DragDropEventFilter::install(QAbstractItemView* view) {
    if (!view) return;
    view->setAcceptDrops(true);
    auto* filter = new DragDropEventFilter(view, view);
    view->installEventFilter(filter);
    if (view->viewport()) {
        view->viewport()->installEventFilter(filter);
    }

    // 🚀【核心修复】：将事件过滤器的 pathsDropped 动态信号直接桥接至宿主视图的 pathsDropped 信号
    QObject::connect(filter, SIGNAL(pathsDropped(QStringList,QModelIndex)),
                     view, SIGNAL(pathsDropped(QStringList,QModelIndex)));
}

void DragDropEventFilter::clearDropHighlight() {
    if (m_currentHoverDropIdx.isValid() && m_targetView && m_targetView->model()) {
        const_cast<QAbstractItemModel*>(m_targetView->model())->setData(m_currentHoverDropIdx, false, IsDropTargetRole);
        m_currentHoverDropIdx = QModelIndex();
        if (m_targetView->viewport()) m_targetView->viewport()->update();
    }
}

bool DragDropEventFilter::eventFilter(QObject* watched, QEvent* event) {
    if (!m_targetView) return QObject::eventFilter(watched, event);

    if (event->type() == QEvent::DragEnter) {
        auto* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (ViewDragDropHelper::handleDragEnter(m_targetView, dragEvent)) {
            return true;
        }
    } else if (event->type() == QEvent::DragMove) {
        auto* moveEvent = static_cast<QDragMoveEvent*>(event);
        QModelIndex hoverIdx = m_targetView->indexAt(moveEvent->position().toPoint());
        if (m_currentHoverDropIdx != hoverIdx) {
            clearDropHighlight();
            if (hoverIdx.isValid()) {
                bool isFolder = (hoverIdx.data(TypeRole).toString() == "folder") || hoverIdx.data(Qt::UserRole + 2).toBool();
                if (isFolder) {
                    m_currentHoverDropIdx = hoverIdx;
                    if (m_targetView->model()) {
                        const_cast<QAbstractItemModel*>(m_targetView->model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                        if (m_targetView->viewport()) m_targetView->viewport()->update();
                    }
                }
            }
        }
        if (ViewDragDropHelper::handleDragMove(m_targetView, moveEvent)) {
            return true;
        }
    } else if (event->type() == QEvent::DragLeave) {
        clearDropHighlight();
        ViewDragDropHelper::clearHover(m_targetView);
        return true;
    } else if (event->type() == QEvent::Drop) {
        clearDropHighlight();
        auto* dropEv = static_cast<QDropEvent*>(event);
        QStringList paths;
        QModelIndex targetIdx;
        if (ViewDragDropHelper::handleDrop(m_targetView, dropEv, paths, targetIdx)) {
            emit pathsDropped(paths, targetIdx);
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

QAbstractItemView* ViewDragDropHelper::s_hoverView = nullptr;
QPersistentModelIndex ViewDragDropHelper::s_hoverIndex;

bool ViewDragDropHelper::isDropTarget(const QAbstractItemView* view, const QModelIndex& index) {
    return view && s_hoverView == view && s_hoverIndex.isValid() && s_hoverIndex == index;
}

void ViewDragDropHelper::clearHover(QAbstractItemView* view) {
    if (view && s_hoverView != view) return;
    QAbstractItemView* oldView = s_hoverView;
    s_hoverView = nullptr;
    s_hoverIndex = QPersistentModelIndex();
    if (oldView && oldView->viewport()) {
        oldView->viewport()->update();
    }
}

bool ViewDragDropHelper::handleDragEnter(QAbstractItemView* /*view*/, QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return true;
    }
    return false;
}

bool ViewDragDropHelper::handleDragMove(QAbstractItemView* view, QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();

        if (view) {
            QModelIndex newHover = view->indexAt(event->position().toPoint());
            if (s_hoverView != view || s_hoverIndex != newHover) {
                QAbstractItemView* oldView = s_hoverView;
                s_hoverView = view;
                s_hoverIndex = newHover;

                if (oldView && oldView->viewport()) oldView->viewport()->update();
                if (view->viewport()) view->viewport()->update();
            }
        }
        return true;
    }
    return false;
}

bool ViewDragDropHelper::handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx) {
    outPaths.clear();
    outTargetIdx = QModelIndex();

    if (event->mimeData() && event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            QString localPath = url.toLocalFile();
            if (!localPath.isEmpty()) {
                outPaths.append(QDir::toNativeSeparators(localPath));
            }
        }
        outTargetIdx = view->indexAt(event->position().toPoint());
        if (!outPaths.isEmpty()) {
            event->acceptProposedAction();
            clearHover(view);
            return true;
        }
    }
    clearHover(view);
    return false;
}

void ViewDragDropHelper::executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions) {
    if (!view || !view->selectionModel()) return;
    QModelIndexList indexes = view->selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) return;

    QList<QUrl> urls;
    for (const QModelIndex& idx : indexes) {
        if (idx.column() != 0) continue;

        QString path = idx.data(PathRole).toString();
        if (path.isEmpty()) {
            path = idx.data(Qt::UserRole + 1).toString();
        }

        if (!path.isEmpty() && QFileInfo::exists(path)) {
            urls.append(QUrl::fromLocalFile(path));
        }
    }

    if (urls.isEmpty()) return;

    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls(urls);

    QDrag* drag = new QDrag(view);
    drag->setMimeData(mimeData);

    QPixmap pixmap(1, 1);
    pixmap.fill(Qt::transparent);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(0, 0));

    drag->exec(supportedActions | Qt::CopyAction, Qt::MoveAction);
}

} // namespace QuarkMeta
