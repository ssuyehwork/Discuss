#include "DropListView.h"
#include "../core/ModelContract.h"
#include "ColumnItemDelegate.h"
#include "ViewDragDropHelper.h"
#include <QMouseEvent>

namespace QuarkMeta {

DropListView::DropListView(QWidget* parent) : QListView(parent) {
    setDragEnabled(true);
    setAcceptDrops(true);
}

void DropListView::dragEnterEvent(QDragEnterEvent* event) {
    if (!ViewDragDropHelper::handleDragEnter(this, event)) {
        QListView::dragEnterEvent(event);
    }
}

void DropListView::dragMoveEvent(QDragMoveEvent* event) {
    QModelIndex hoverIdx = indexAt(event->position().toPoint());
    if (m_currentHoverDropIdx != hoverIdx) {
        clearDropHighlight();
        if (hoverIdx.isValid()) {
            bool isFolder = (hoverIdx.data(TypeRole).toString() == "folder") || hoverIdx.data(Qt::UserRole + 2).toBool();
            if (isFolder) {
                m_currentHoverDropIdx = hoverIdx;
                if (model()) {
                    const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                    viewport()->update();
                }
            }
        }
    }

    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QListView::dragMoveEvent(event);
    }
}

void DropListView::dragLeaveEvent(QDragLeaveEvent* event) {
    clearDropHighlight();
    QListView::dragLeaveEvent(event);
}

void DropListView::clearDropHighlight() {
    if (m_currentHoverDropIdx.isValid() && model()) {
        const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, false, IsDropTargetRole);
        m_currentHoverDropIdx = QModelIndex();
        viewport()->update();
    }
}

void DropListView::dropEvent(QDropEvent* event) {
    clearDropHighlight();
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        QListView::dropEvent(event);
    }
}

void DropListView::startDrag(Qt::DropActions supportedActions) {
    ViewDragDropHelper::executeStartDrag(this, supportedActions);
}

void DropListView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event && event->button() == Qt::LeftButton) {
        QModelIndex idx = indexAt(event->pos());
        if (!idx.isValid()) {
            emit blankSpaceDoubleClicked();
            event->accept();
            return;
        }
    }
    QListView::mouseDoubleClickEvent(event);
}

} // namespace QuarkMeta
