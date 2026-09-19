#pragma once

#include <QAbstractItemView>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QStringList>
#include <QModelIndex>

namespace QuarkMeta {

/**
 * @brief Helper class for unified drag-and-drop behavior across item views.
 */
class ViewDragDropHelper {
public:
    static bool handleDragEnter(QAbstractItemView* view, QDragEnterEvent* event);
    static bool handleDragMove(QAbstractItemView* view, QDragMoveEvent* event);
    static bool handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx);
    static void executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions);
};

} // namespace QuarkMeta
