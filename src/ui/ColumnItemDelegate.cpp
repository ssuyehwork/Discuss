#include "ColumnItemDelegate.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
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
    sz.setHeight(28);
    return sz;
}

void ColumnItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    bool selected = (option.state & QStyle::State_Selected);
    bool hover = (option.state & QStyle::State_MouseOver);
    bool isParentExpanded = index.data(IsParentExpandedRole).toBool();
    bool isDropTarget = index.data(IsDropTargetRole).toBool();

    // 1. 背景绘制
    QColor bg;
    if (isDropTarget) {
        bg = QColor("#005A9E");
    } else if (selected) {
        bg = QColor("#378ADD");
        bg.setAlphaF(0.18f);
    } else if (isParentExpanded) {
        bg = QColor("#334455");
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

    // 3. 读取元数据 (色标与星级)
    bool isDir = isFolder || index.data(Qt::UserRole + 2).toBool();
    int rating = index.data(RatingRole).toInt();
    QString colorName = index.data(ColorRole).toString();

    // 绘制 5px 宽度的左侧垂直色条
    if (!colorName.isEmpty()) {
        QString hexColor = Style::getColorHexByName(colorName);
        if (hexColor.startsWith("#")) {
            painter->setBrush(QColor(hexColor));
            painter->setPen(Qt::NoPen);
            QRect colorBarRect(option.rect.left(), option.rect.top(), 5, option.rect.height());
            painter->drawRect(colorBarRect);
        }
    }

    // 4. 绘制文字与右侧元数据 (动态调整星级与箭头宽度)
    int rightMargin = isDir ? 22 : 6;
    if (rating > 0) rightMargin += 32;

    QString name = index.data(Qt::DisplayRole).toString();
    QRect textRect = option.rect.adjusted(32, 0, -rightMargin, 0);
    QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
    painter->setPen(textColor);
    painter->setFont(option.font);
    QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    // 5. 绘制星级标示 (若 rating > 0)
    if (rating > 0) {
        int starRight = option.rect.right() - (isDir ? 22 : 6);
        int starSize = 12;
        int numWidth = 12;
        QRect starIconRect(starRight - numWidth - starSize, option.rect.top() + (option.rect.height() - starSize) / 2, starSize, starSize);
        QRect numRect(starRight - numWidth, option.rect.top() + (option.rect.height() - starSize) / 2, numWidth, starSize);

        QColor orangeColor = QColor("#FF551C");
        QIcon starIcon = UiHelper::getIcon("star_filled", orangeColor, starSize);
        starIcon.paint(painter, starIconRect, Qt::AlignCenter);

        painter->setPen(orangeColor);
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter->drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(rating));
    }

    // 6. 如果是文件夹，最右侧绘制向右箭头 chevron_right
    if (isDir) {
        QRect arrowRect(option.rect.right() - 20, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
        QColor arrowColor = selected ? QColor("#FFFFFF") : (isEmpty ? QColor("#41F2F2") : QColor("#888888"));
        UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
    }

    painter->restore();
}

} // namespace QuarkMeta
