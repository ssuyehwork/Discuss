#include "FavoriteTreeDropHandler.h"
#include <QMap>
#include <functional>

namespace QuarkMeta {

FavoriteTreeDropHandler::FavoriteTreeDropHandler(QObject* parent)
    : QObject(parent) {
}

bool FavoriteTreeDropHandler::validateDrop(const QList<QPersistentModelIndex>& sources, 
                                          const QModelIndex& target, 
                                          InternalDropZone zone, 
                                          QStandardItemModel* model) 
{
    if (sources.isEmpty() || !model) return false;

    auto isSourceOrDescendant = [](const QModelIndex& idx, const QList<QPersistentModelIndex>& srcList) -> bool {
        QModelIndex cur = idx;
        while (cur.isValid()) {
            for (const QPersistentModelIndex& psrc : srcList) {
                if (psrc.isValid() && psrc == cur) return true;
            }
            cur = cur.parent();
        }
        return false;
    };

    // 1. 检查目标节点或其父节点是否属于被拖动节点或其后代
    if (zone == InternalDropZone::OnItem) {
        if (isSourceOrDescendant(target, sources)) return false;
    } else if (zone == InternalDropZone::Above || zone == InternalDropZone::Below) {
        for (const QPersistentModelIndex& psrc : sources) {
            if (psrc.isValid() && psrc == target) return false;
        }
        if (target.parent().isValid() && isSourceOrDescendant(target.parent(), sources)) return false;
    }

    // 2. 位置变化检查
    QStandardItem* targetParentItem = nullptr;
    int destRow = 0;

    if (zone == InternalDropZone::OnItem) {
        if (target.isValid()) {
            targetParentItem = model->itemFromIndex(target);
            if (targetParentItem) destRow = targetParentItem->rowCount();
        }
    } else if (zone == InternalDropZone::Above) {
        if (target.isValid()) {
            targetParentItem = target.parent().isValid() ? model->itemFromIndex(target.parent()) : model->invisibleRootItem();
            destRow = target.row();
        }
    } else if (zone == InternalDropZone::Below) {
        if (target.isValid()) {
            targetParentItem = target.parent().isValid() ? model->itemFromIndex(target.parent()) : model->invisibleRootItem();
            destRow = target.row() + 1;
        }
    } else if (zone == InternalDropZone::Blank) {
        targetParentItem = model->invisibleRootItem();
        destRow = targetParentItem->rowCount();
    }

    if (!targetParentItem) {
        targetParentItem = model->invisibleRootItem();
        destRow = targetParentItem->rowCount();
    }

    bool anyChanged = false;
    for (const QPersistentModelIndex& psrc : sources) {
        if (!psrc.isValid()) continue;
        QStandardItem* srcItem = model->itemFromIndex(psrc);
        if (!srcItem) continue;

        QStandardItem* srcParentItem = srcItem->parent() ? srcItem->parent() : model->invisibleRootItem();
        int srcRow = srcItem->row();

        if (srcParentItem != targetParentItem) {
            anyChanged = true;
            break;
        } else {
            if (zone == InternalDropZone::OnItem || zone == InternalDropZone::Blank) {
                if (srcRow != targetParentItem->rowCount() - 1) {
                    anyChanged = true;
                    break;
                }
            } else if (zone == InternalDropZone::Above) {
                if (srcRow != destRow && srcRow != destRow - 1) {
                    anyChanged = true;
                    break;
                }
            } else if (zone == InternalDropZone::Below) {
                if (srcRow != destRow && srcRow != destRow - 1) {
                    anyChanged = true;
                    break;
                }
            }
        }
    }

    return anyChanged;
}

bool FavoriteTreeDropHandler::executeMove(const QList<QPersistentModelIndex>& sources, 
                                         const QModelIndex& target, 
                                         InternalDropZone zone, 
                                         QStandardItemModel* model, 
                                         DropTreeView* view) 
{
    if (sources.isEmpty() || !model) return false;

    QStandardItem* parentItem = nullptr;
    int destRow = 0;

    if (zone == InternalDropZone::OnItem) {
        if (target.isValid()) {
            parentItem = model->itemFromIndex(target);
            if (parentItem) {
                destRow = parentItem->rowCount();
            }
        }
    } else if (zone == InternalDropZone::Above) {
        if (target.isValid()) {
            parentItem = target.parent().isValid() ? model->itemFromIndex(target.parent()) : model->invisibleRootItem();
            destRow = target.row();
        }
    } else if (zone == InternalDropZone::Below) {
        if (target.isValid()) {
            parentItem = target.parent().isValid() ? model->itemFromIndex(target.parent()) : model->invisibleRootItem();
            destRow = target.row() + 1;
        }
    } else if (zone == InternalDropZone::Blank) {
        parentItem = model->invisibleRootItem();
        destRow = parentItem->rowCount();
    }

    if (!parentItem) {
        parentItem = model->invisibleRootItem();
        destRow = parentItem->rowCount();
    }

    // 记录展开状态
    QMap<int, bool> expandedMap;
    std::function<void(QStandardItem*)> saveExpandedStates = [&](QStandardItem* pItem) {
        int rCount = pItem ? pItem->rowCount() : model->rowCount();
        for (int i = 0; i < rCount; ++i) {
            QStandardItem* child = pItem ? pItem->child(i) : model->item(i);
            if (!child) continue;
            int nodeId = child->data(Qt::UserRole + 6).toInt();
            if (nodeId > 0 && view) {
                expandedMap[nodeId] = view->isExpanded(child->index());
            }
            saveExpandedStates(child);
        }
    };
    saveExpandedStates(nullptr);

    // 转换 itemsToMove
    QList<QStandardItem*> itemsToMove;
    for (const QPersistentModelIndex& pidx : sources) {
        if (pidx.isValid()) {
            QStandardItem* item = model->itemFromIndex(pidx);
            if (item) itemsToMove.append(item);
        }
    }
    if (itemsToMove.isEmpty()) return false;

    QList<QStandardItem*> movedItemsList;
    for (QStandardItem* item : itemsToMove) {
        QStandardItem* srcParent = item->parent() ? item->parent() : model->invisibleRootItem();
        int srcRow = item->row();

        QList<QStandardItem*> rowItems = srcParent->takeRow(srcRow);
        if (rowItems.isEmpty()) continue;

        int insertRow = destRow;
        if (srcParent == parentItem && srcRow < insertRow) {
            insertRow--;
        }

        parentItem->insertRow(insertRow, rowItems);
        destRow = insertRow + 1;
        movedItemsList.append(rowItems.first());
    }

    // 恢复展开状态
    std::function<void(QStandardItem*)> restoreExpandedStates = [&](QStandardItem* pItem) {
        int rCount = pItem ? pItem->rowCount() : model->rowCount();
        for (int i = 0; i < rCount; ++i) {
            QStandardItem* child = pItem ? pItem->child(i) : model->item(i);
            if (!child) continue;
            int nodeId = child->data(Qt::UserRole + 6).toInt();
            if (nodeId > 0 && expandedMap.contains(nodeId) && view) {
                view->setExpanded(child->index(), expandedMap.value(nodeId));
            }
            restoreExpandedStates(child);
        }
    };
    restoreExpandedStates(nullptr);

    // 目标若是虚拟文件夹，展开它
    if (zone == InternalDropZone::OnItem && target.isValid() && target.data(Qt::UserRole + 7).toBool() && view) {
        view->setExpanded(target, true);
    }

    // 更新新父节点的 id (顶层为 0)
    int newParentNodeId = 0;
    if (parentItem != model->invisibleRootItem()) {
        newParentNodeId = parentItem->data(Qt::UserRole + 6).toInt();
    }
    for (QStandardItem* movedItem : movedItemsList) {
        movedItem->setData(newParentNodeId, Qt::UserRole + 8);
    }

    // 恢复选中状态
    if (view && view->selectionModel()) {
        view->selectionModel()->clearSelection();
        for (QStandardItem* movedItem : movedItemsList) {
            view->selectionModel()->select(movedItem->index(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
    }

    return true;
}

} // namespace QuarkMeta
