#include "DropJustifiedView.h"
#include "ViewDragDropHelper.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

namespace RapidNotes {

DropJustifiedView::DropJustifiedView(QWidget* parent) : JustifiedView(parent) {
    setDragEnabled(true);
    DragDropEventFilter::install(this);
}

void DropJustifiedView::startDrag(Qt::DropActions supportedActions) {
    ViewDragDropHelper::executeStartDrag(this, supportedActions);
}

} // namespace RapidNotes
