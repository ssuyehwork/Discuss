#pragma once

#include "RenameCapableDelegate.h"
#include <QPainter>

namespace QuarkMeta {

enum ColumnCustomRole {
    IsParentExpandedRole = Qt::UserRole + 200,
    IsDropTargetRole = Qt::UserRole + 201
};

/**
 * @brief 列视图专用 Delegate
 * 必须继承 RenameCapableDelegate，严禁重写 createEditor/setEditorData/setModelData。
 */
class ColumnItemDelegate : public RenameCapableDelegate {
    Q_OBJECT
public:
    explicit ColumnItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

} // namespace QuarkMeta
