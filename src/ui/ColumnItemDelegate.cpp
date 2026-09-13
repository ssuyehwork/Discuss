#include "ColumnItemDelegate.h"
#include "UiHelper.h"
#include "CardPainterHelper.h"
#include "../core/ModelContract.h"

#include <QPainter>
#include <QPainterPath>

namespace QuarkMeta {

ColumnItemDelegate::ColumnItemDelegate(QObject* parent)
    : RenameCapableDelegate(parent) {
}

QSize ColumnItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QSize sz = RenameCapableDelegate::sizeHint(option, index);
    sz.setHeight(32);
    return sz;
}

void ColumnItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    bool selected = (option.state & QStyle::State_Selected);
    bool hover = (option.state & QStyle::State_MouseOver);

    // 1. 背景绘制
    QColor bg;
    if (selected) {
        bg = QColor("#378ADD");
        bg.setAlphaF(0.18f);
    } else if (hover) {
        bg = QColor("#2A2D2E");
    } else {
        bg = QColor("#1E1E1E");
    }

    painter->setBrush(bg);
    painter->setPen(Qt::NoPen);
    painter->drawRect(option.rect);

    QRect rect = option.rect.adjusted(8, 0, -8, 0);

    // 2. 左侧图标 (文件 / 文件夹)
    bool isFolder = (index.data(TypeRole).toString() == "folder");
    bool isEmpty = index.data(IsEmptyRole).toBool();
    QVariant deco = index.data(Qt::DecorationRole);

    int iconSize = 18;
    QRect iconRect(rect.left(), rect.top() + (rect.height() - iconSize) / 2, iconSize, iconSize);

    if (deco.canConvert<QIcon>() && !deco.value<QIcon>().isNull()) {
        deco.value<QIcon>().paint(painter, iconRect, Qt::AlignCenter);
    } else if (deco.canConvert<QPixmap>() && !deco.value<QPixmap>().isNull()) {
        QPixmap pix = deco.value<QPixmap>();
        painter->drawPixmap(iconRect, pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        QIcon fallbackIcon = UiHelper::getIcon(isFolder ? "folder" : "file", QColor("#888888"), 18);
        fallbackIcon.paint(painter, iconRect, Qt::AlignCenter);
    }

    // 空文件夹虚线提示
    if (isFolder && isEmpty) {
        painter->save();
        painter->setPen(QPen(QColor("#41F2F2"), 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(iconRect.adjusted(-1, -1, 1, 1), 3, 3);
        painter->restore();
    }

    // 3. 右侧箭头 (如果是文件夹，保留 20px 专属区域)
    int rightReserved = isFolder ? 20 : 0;
    if (isFolder) {
        QRect arrowRect(rect.right() - 16, rect.top() + (rect.height() - 16) / 2, 16, 16);
        QIcon arrowIcon = UiHelper::getIcon("chevron_right", QColor("#888888"), 16);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter);
    }

    // 4. 文件/文件夹名称文本 (遵循 QuarkMeta-Architecture-Planning.md 第八章第三条：纯净单行渲染，不在分栏行内绘制星级/颜色标示)
    int textLeft = iconRect.right() + 8;
    int textWidth = rect.width() - (textLeft - rect.left()) - rightReserved;
    QRect textRect(textLeft, rect.top(), qMax(10, textWidth), rect.height());

    QString name = index.data(Qt::DisplayRole).toString();
    QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
    painter->setPen(textColor);
    painter->setFont(option.font);

    QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}

} // namespace QuarkMeta
