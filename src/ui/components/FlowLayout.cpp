#include "FlowLayout.h"
#include <QWidget>

namespace QuarkMeta {

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing) { setContentsMargins(margin, margin, margin, margin); }
FlowLayout::~FlowLayout() { QLayoutItem *item; while ((item = takeAt(0))) delete item; }
void FlowLayout::addItem(QLayoutItem *item) { itemList.append(item); invalidate(); }
int FlowLayout::horizontalSpacing() const { return m_hSpace >= 0 ? m_hSpace : 4; }
int FlowLayout::verticalSpacing() const { return m_vSpace >= 0 ? m_vSpace : 4; }
int FlowLayout::count() const { return itemList.size(); }
QLayoutItem *FlowLayout::itemAt(int index) const { return itemList.value(index); }
QLayoutItem *FlowLayout::takeAt(int index) { return (index >= 0 && index < itemList.size()) ? itemList.takeAt(index) : nullptr; }
Qt::Orientations FlowLayout::expandingDirections() const { return Qt::Orientations(); }
bool FlowLayout::hasHeightForWidth() const { return true; }
int FlowLayout::heightForWidth(int width) const { return doLayout(QRect(0, 0, width, 0), true); }
void FlowLayout::setGeometry(const QRect &rect) { QLayout::setGeometry(rect); doLayout(rect, false); }
QSize FlowLayout::sizeHint() const { return minimumSize(); }
QSize FlowLayout::minimumSize() const {
    QSize size;
    for (QLayoutItem *item : itemList) size = size.expandedTo(item->minimumSize());
    size += QSize(2 * contentsMargins().top(), 2 * contentsMargins().top());
    return size;
}
int FlowLayout::doLayout(const QRect &rect, bool testOnly) const {
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    QList<QLayoutItem*> rowItems;

    for (QLayoutItem *item : itemList) {
        int spaceX = horizontalSpacing();
        int spaceY = verticalSpacing();
        int itemW = item->sizeHint().width();
        int nextX = x + itemW + spaceX;

        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            if (!testOnly && !rowItems.isEmpty()) {
                int rowW = x - spaceX - effectiveRect.x();
                int xOffset = 0;
                if (m_alignment & Qt::AlignHCenter) {
                    xOffset = qMax(0, (effectiveRect.width() - rowW) / 2);
                } else if (m_alignment & Qt::AlignRight) {
                    xOffset = qMax(0, effectiveRect.width() - rowW);
                }
                if (xOffset > 0) {
                    for (QLayoutItem* rowItem : rowItems) {
                        QRect g = rowItem->geometry();
                        rowItem->setGeometry(g.translated(xOffset, 0));
                    }
                }
            }
            rowItems.clear();

            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + itemW + spaceX;
            lineHeight = 0;
        }

        if (!testOnly) {
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
            rowItems.append(item);
        }

        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }

    if (!testOnly && !rowItems.isEmpty()) {
        int rowW = x - horizontalSpacing() - effectiveRect.x();
        int xOffset = 0;
        if (m_alignment & Qt::AlignHCenter) {
            xOffset = qMax(0, (effectiveRect.width() - rowW) / 2);
        } else if (m_alignment & Qt::AlignRight) {
            xOffset = qMax(0, effectiveRect.width() - rowW);
        }
        if (xOffset > 0) {
            for (QLayoutItem* rowItem : rowItems) {
                QRect g = rowItem->geometry();
                rowItem->setGeometry(g.translated(xOffset, 0));
            }
        }
    }

    return y + lineHeight - rect.y() + bottom;
}

} // namespace QuarkMeta
