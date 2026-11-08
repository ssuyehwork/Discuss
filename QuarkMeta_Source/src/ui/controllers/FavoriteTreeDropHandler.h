#pragma once

#include <QObject>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QStandardItemModel>
#include "../DropTreeView.h"

namespace QuarkMeta {

class FavoriteTreeDropHandler : public QObject {
    Q_OBJECT
public:
    explicit FavoriteTreeDropHandler(QObject* parent = nullptr);
    ~FavoriteTreeDropHandler() override = default;

    static bool validateDrop(const QList<QPersistentModelIndex>& sources, 
                             const QModelIndex& target, 
                             InternalDropZone zone, 
                             QStandardItemModel* model);

    static bool executeMove(const QList<QPersistentModelIndex>& sources, 
                            const QModelIndex& target, 
                            InternalDropZone zone, 
                            QStandardItemModel* model, 
                            DropTreeView* view);
};

} // namespace QuarkMeta
