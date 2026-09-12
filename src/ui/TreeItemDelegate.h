#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QMouseEvent>
#include <QLineEdit>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include "ContentPanel.h"
#include "RenameCapableDelegate.h"
#include "RatingBarLayout.h"
#include "RowLayoutEngine.h"
#include "../meta/MetadataManager.h"
#include "../core/ModelContract.h"
#include "UiHelper.h"
#include "CardPainterHelper.h"
#include "StyleLibrary.h"
using namespace QuarkMeta::Style;

namespace QuarkMeta {

/**
 * @brief 通用树形视图代理，提供圆角高亮效果
 */
class TreeItemDelegate : public RenameCapableDelegate {
public:
    explicit TreeItemDelegate(QObject* parent = nullptr, bool showStatus = true, bool drawMiniCards = false)
        : RenameCapableDelegate(parent), m_drawMiniCards(drawMiniCards) { Q_UNUSED(showStatus); }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QSize sz = QStyledItemDelegate::sizeHint(option, index);
        const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(option.widget);
        int zoom = view ? view->iconSize().height() + 8 : 30;
        sz.setHeight(RowLayoutEngine::calculateRowHeight(zoom));
        return sz;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (!index.isValid()) return;


        bool selected = option.state & QStyle::State_Selected;
        bool hover = option.state & QStyle::State_MouseOver;

        // 🚀【行底色彻底统一与防穿透自绘】：直接根据选中/悬停/行号奇偶绘制底色，贯穿整个单元格矩形
        painter->save();
        QColor bg;
        bool useAlternate = false;
        if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
            useAlternate = view->alternatingRowColors();
        }

        if (selected) {
            bg = QColor("#378ADD");
            bg.setAlphaF(0.15f);
        } else if (hover) {
            bg = QColor("#2A2D2E");
        } else {
            // 根据控件是否开启斑马纹与行号奇偶精准赋值底色
            bg = (useAlternate && index.row() % 2 == 1) ? QColor("#252526") : QColor("#1E1E1E");
        }
        painter->setBrush(bg);
        painter->setPen(Qt::NoPen);
        painter->drawRect(option.rect);
        painter->restore();

        QStyleOptionViewItem opt = option;
        if (index.column() >= 1) {
            opt.displayAlignment = Qt::AlignCenter;
        }

        opt.state &= ~QStyle::State_Selected;
        opt.state &= ~QStyle::State_MouseOver;
        opt.features &= ~QStyleOptionViewItem::Alternate;
        opt.backgroundBrush = QBrush();
        
        if (selected) {
            opt.palette.setColor(QPalette::Text, Qt::white);
        }

        // 2026-06-16 按照 8 列架构重构：第 1, 2, 3 列由代理独立绘制；第 0 列作为名称列，具有微型圆角卡片预览（最左侧看片）
        int col = index.column();
        if (col == 0 && m_drawMiniCards) {
            // 自定义绘制名称列与最左侧圆角卡片
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);

            RowLayout layout = RowLayoutEngine::calculate(option.rect, option.rect.height());
            QRect squareRect = layout.cardRect;
            QRect textRect   = layout.textRect;

            // 1. 绘制微型卡片背景（严格保持 Version-1 / Version-2 的纯透明背景底板）
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::transparent);
            QPainterPath cardPath;
            cardPath.addRoundedRect(squareRect, 4, 4);
            painter->drawPath(cardPath);

            // 2. 图像/图标平滑居中绘制（严格保持 Version-1 / Version-2 的物理等比居中渲染模式）
            QVariant decoData = index.data(Qt::DecorationRole);
            bool hasThumb = index.data(HasThumbnailRole).toBool();

            if (hasThumb) {
                QPixmap thumb;
                if (decoData.canConvert<QPixmap>()) {
                    thumb = decoData.value<QPixmap>();
                } else if (decoData.canConvert<QIcon>()) {
                    QIcon icon = decoData.value<QIcon>();
                    if (!icon.isNull()) thumb = icon.pixmap(squareRect.size());
                }

                if (!thumb.isNull()) {
                    painter->save();
                    QPainterPath clipPath;
                    clipPath.addRoundedRect(squareRect, 4, 4);
                    painter->setClipPath(clipPath);

                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2;
                    int y = squareRect.center().y() - scaled.height() / 2;
                    painter->drawPixmap(x, y, scaled);

                    painter->restore();
                } else {
                    QIcon icon = qvariant_cast<QIcon>(decoData);
                    if (!icon.isNull()) {
                        int iconSize = qRound(squareRect.width() * 0.75);
                        QRect iconRect(squareRect.center().x() - iconSize / 2,
                                       squareRect.center().y() - iconSize / 2,
                                       iconSize, iconSize);
                        icon.paint(painter, iconRect, Qt::AlignCenter);
                    }
                }
            } else {
                QIcon icon = qvariant_cast<QIcon>(decoData);
                if (!icon.isNull()) {
                    int iconSize = qRound(squareRect.width() * 0.75);
                    QRect iconRect(squareRect.center().x() - iconSize / 2,
                                   squareRect.center().y() - iconSize / 2,
                                   iconSize, iconSize);
                    icon.paint(painter, iconRect, Qt::AlignCenter);
                }
            }

            // 3. 空文件夹绘制青蓝色虚线框 (#41F2F2 Qt::DashLine)
            bool isFolder = (index.data(TypeRole).toString() == "folder");
            bool isEmpty = index.data(IsEmptyRole).toBool();
            if (isFolder && isEmpty) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(QPen(QColor("#41F2F2"), 1, Qt::DashLine));
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(squareRect, 4, 4);
                painter->restore();
            }

            // 4. 文本排版向右偏移：使用统一文本矩形
            QString name = index.data(Qt::DisplayRole).toString();
            QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");

            painter->setPen(textColor);
            painter->setFont(option.font);

            QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideMiddle, textRect.width() - 10);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

            painter->restore();
        } else if (col == 1 || col == 2) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);

            QModelIndex idx0 = index.model()->index(index.row(), 0);

            if (col == 1) { // 🚨 物理修复 ①：状态列图标在单元格内部 100% 水平+垂直绝对居中！
                bool isPinned = idx0.data(IsLockedRole).toBool();

                int iconSize = 16;
                // 计算单元格物理中心坐标
                QRect centeredRect(option.rect.left() + (option.rect.width() - iconSize) / 2,
                                   option.rect.top() + (option.rect.height() - iconSize) / 2,
                                   iconSize, iconSize);

                if (isPinned) {
                    UiHelper::getIcon("pin_vertical", QColor("#FF551C"), 16).paint(painter, centeredRect, Qt::AlignCenter);
                }
            } else if (col == 2) { // 星级列
                int rating = idx0.data(RatingRole).toInt();
                bool isSelected = option.state & QStyle::State_Selected;
                QString colorName = idx0.data(ColorRole).toString();

                if (rating > 0 || isSelected || !colorName.isEmpty()) {
                    // 🚀【统一调用 RatingBarLayout】：彻底消灭绘制时的 18 / -4 / 12 硬编码！
                    RatingBarMetrics rm = RatingBarLayout::calculate(option.rect, RatingBarMode::TreeRow);

                    CardPainterHelper::drawRatingStars(painter, rm.banRect, option.rect, rm.starSize, rm.starSpacing, 
                                                      option.rect.top(), option.rect.height(), rm.starsStartX,
                                                      rating, colorName, isSelected);
                }
            }
            painter->restore();
        } else {
            QStyledItemDelegate::paint(painter, opt, index);
        }
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (index.column() == 0 && m_drawMiniCards) {
            RowLayout layout = RowLayoutEngine::calculate(option.rect, option.rect.height());
            editor->setGeometry(layout.editorRect);
        } else {
            QRect r = option.rect;
            r.adjust(6, 2, -6, -2);
            editor->setGeometry(r);
        }
    }

private:
    bool m_drawMiniCards;
};

} // namespace QuarkMeta
