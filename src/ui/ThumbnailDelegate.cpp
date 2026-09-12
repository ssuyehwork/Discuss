#include "ThumbnailDelegate.h"
#include "CardLayoutEngine.h"
#include "ContentPanel.h"
#include "CardPainterHelper.h"
#include "ElidedTextUtility.h"
#include "UiHelper.h"
#include "../core/ModelContract.h"

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QFileInfo>
#include <QLineEdit>
#include <QHelpEvent>

namespace QuarkMeta {

ThumbnailDelegate::ThumbnailDelegate(QObject* parent) : RenameCapableDelegate(parent) {}

void ThumbnailDelegate::setHasThumbnailRole(int role) { m_hasThumbnailRole = role; }
void ThumbnailDelegate::setRatingRole(int role) { m_ratingRole = role; }
void ThumbnailDelegate::setPathRole(int role) { m_pathRole = role; }
void ThumbnailDelegate::setPinnedRole(int role) { m_pinnedRole = role; }
void ThumbnailDelegate::setTypeRole(int role) { m_typeRole = role; }
void ThumbnailDelegate::setIsEmptyRole(int role) { m_isEmptyRole = role; }
void ThumbnailDelegate::setColorRole(int role) { m_colorRole = role; }

ThumbnailDelegate::Metrics ThumbnailDelegate::calculateMetrics(const QStyleOptionViewItem& option) const {
    CardLayout l = CardLayoutEngine::calculate(option.rect, option.decorationSize.width());
    Metrics m;
    m.cardRect = l.coverRect;
    m.textRect = l.textRect;
    m.banRect = l.banRect;
    m.ratingH = l.capsuleRect.height();
    m.ratingY = l.capsuleRect.top();
    m.starSize = l.starSize;
    m.starSpacing = l.starSpacing;
    m.starsStartX = l.starRects[0].left();
    return m;
}

void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;

    CardLayout l = CardLayoutEngine::calculate(option.rect, option.decorationSize.width());
    bool isSelected = (option.state & QStyle::State_Selected);

    bool hasThumb = index.data(m_hasThumbnailRole).toBool();
    QVariant decoData = index.data(Qt::DecorationRole);
    QPixmap thumb;
    if (decoData.canConvert<QPixmap>()) {
        thumb = decoData.value<QPixmap>();
    } else if (decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) thumb = icon.pixmap(l.coverRect.size());
    }

    bool isWaitingThumb = false;
    if (m_pathRole != -1 && thumb.isNull()) {
        QString path = index.data(m_pathRole).toString();
        QString ext = QFileInfo(path).suffix().toLower();
        if (UiHelper::isGraphicsFile(ext) || ext == "svg") isWaitingThumb = true;
    }

    bool isGrid = option.widget ? option.widget->property("gridMode").toBool() : false;

    // ① 绘制缩略图 Cover
    CardPainterHelper::drawCardCover(painter, l.coverRect, isSelected, hasThumb, thumb, 
                                     qvariant_cast<QIcon>(decoData), isGrid, isWaitingThumb);

    // ② 绘制卡片外边框
    CardPainterHelper::drawCardBorder(painter, l.coverRect, isSelected);

    // ③ 绘制置顶标记
    if (m_pinnedRole != -1) {
        bool isPinned = index.data(m_pinnedRole).toBool();
        CardPainterHelper::drawStatusIndicators(painter, l.coverRect, isPinned);
    }

    // ④ 绘制扩展名徽章
    if (m_pathRole != -1) {
        QString type = (m_typeRole != -1) ? index.data(m_typeRole).toString() : "";
        QString ext = (type == "folder") ? "DIR" : QFileInfo(index.data(m_pathRole).toString()).suffix().toUpper();
        if (ext.isEmpty()) ext = "FILE";
        CardPainterHelper::drawExtensionBadge(painter, l.coverRect, ext, hasThumb);
    }

    // ⑤ 绘制色标胶囊底色与星级 (精准 11 个参数完全对齐)
    if (m_ratingRole != -1) {
        int rating = index.data(m_ratingRole).toInt();
        QString colorStr = (m_colorRole != -1) ? index.data(m_colorRole).toString() : "";

        CardPainterHelper::drawRatingStars(
            painter,
            l.banRect,
            l.coverRect,
            l.starSize,
            l.starSpacing,
            l.capsuleRect.top(),
            l.capsuleRect.height(),
            l.starRects[0].left(),
            rating,
            colorStr,
            isSelected
        );
    }

    // ⑥ 绘制文件名 (限制2行)
    drawFileNameText(painter, l.textRect, isSelected, index, option);

    // ⑦ 空文件夹虚线边框
    if (!isSelected && m_isEmptyRole != -1 && m_typeRole != -1) {
        if (index.data(m_typeRole).toString() == "folder" && index.data(m_isEmptyRole).toBool()) {
            CardPainterHelper::drawEmptyFolderBorder(painter, l.coverRect);
        }
    }
}

void ThumbnailDelegate::drawFileNameText(QPainter* painter, const QRect& textRect, bool isSelected, const QModelIndex& index, const QStyleOptionViewItem& option) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QString name = index.data(Qt::DisplayRole).toString();
    painter->setPen(isSelected ? QColor("#3498db") : QColor("#EEEEEE"));

    QFont textFont = painter->font();
    textFont.setPointSize(8);
    painter->setFont(textFont);

    QString displayName = ElidedTextUtility::elideTwoLinesText(name, option.fontMetrics, textRect.width() - 8);
    painter->drawText(textRect.adjusted(4, 0, -4, 0), Qt::AlignCenter | Qt::TextWordWrap, displayName);
    painter->restore();
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    return QStyledItemDelegate::sizeHint(option, index);
}

void ThumbnailDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const { 
    CardLayout l = CardLayoutEngine::calculate(option.rect, option.decorationSize.width());
    editor->setGeometry(l.textRect.adjusted(1, 4, -1, -4)); 
} 

bool ThumbnailDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, 
                                const QStyleOptionViewItem& option, const QModelIndex& index) {
    return QStyledItemDelegate::helpEvent(event, view, option, index);
}

} // namespace QuarkMeta