#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QPixmap>
#include "UiHelper.h"
#include "../core/ModelContract.h"

namespace QuarkMeta {

class ColumnItemDelegate : public QStyledItemDelegate {
public:
    explicit ColumnItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(index);
        QSize sz = QStyledItemDelegate::sizeHint(option, index);
        sz.setHeight(32);
        return sz;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (!index.isValid()) return;

        bool selected = option.state & QStyle::State_Selected;
        bool hover = option.state & QStyle::State_MouseOver;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform);

        // 1. 绘制背景
        QColor bg;
        if (selected) {
            bg = QColor("#378ADD");
            bg.setAlphaF(0.25f);
        } else if (hover) {
            bg = QColor("#2A2D2E");
        } else {
            bg = QColor("#1E1E1E");
        }
        painter->setBrush(bg);
        painter->setPen(Qt::NoPen);
        painter->drawRect(option.rect);

        // 2. 绘制图标/缩略图
        QVariant deco = index.data(Qt::DecorationRole);
        QRect iconRect(option.rect.left() + 8, option.rect.top() + (option.rect.height() - 20) / 2, 20, 20);

        if (deco.canConvert<QPixmap>()) {
            QPixmap pm = deco.value<QPixmap>();
            if (!pm.isNull()) {
                QPixmap scaled = pm.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                int x = iconRect.center().x() - scaled.width() / 2;
                int y = iconRect.center().y() - scaled.height() / 2;
                painter->drawPixmap(x, y, scaled);
            }
        } else if (deco.canConvert<QIcon>()) {
            QIcon icon = deco.value<QIcon>();
            if (!icon.isNull()) {
                icon.paint(painter, iconRect, Qt::AlignCenter);
            }
        }

        // 3. 绘制文本
        QString name = index.data(Qt::DisplayRole).toString();
        QRect textRect = option.rect.adjusted(34, 0, -28, 0);
        QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
        painter->setPen(textColor);
        painter->setFont(option.font);
        QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

        // 4. 如果是文件夹，最右侧绘制向右箭头
        bool isDir = index.data(Qt::UserRole + 2).toBool();
        if (isDir) {
            QRect arrowRect(option.rect.right() - 20, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
            QColor arrowColor = selected ? QColor("#FFFFFF") : QColor("#888888");
            UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
        }

        painter->restore();
    }
};

} // namespace QuarkMeta
