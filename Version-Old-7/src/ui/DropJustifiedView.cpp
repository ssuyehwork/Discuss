#include "DropJustifiedView.h"
#include "ViewDragDropHelper.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

namespace QuarkMeta {

DropJustifiedView::DropJustifiedView(QWidget* parent) : JustifiedView(parent) {
    setDragEnabled(true);
    setAcceptDrops(true);
}

void DropJustifiedView::dragEnterEvent(QDragEnterEvent* event) {
    if (!ViewDragDropHelper::handleDragEnter(this, event)) {
        JustifiedView::dragEnterEvent(event);
    }
}

void DropJustifiedView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        JustifiedView::dragMoveEvent(event);
    }
}

void DropJustifiedView::dropEvent(QDropEvent* event) {
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        JustifiedView::dropEvent(event);
    }
}

void DropJustifiedView::startDrag(Qt::DropActions supportedActions) {
    ViewDragDropHelper::executeStartDrag(this, supportedActions);
}

} // namespace QuarkMeta
