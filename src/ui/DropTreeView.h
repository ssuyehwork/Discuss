#ifndef DROPTREEVIEW_H
#define DROPTREEVIEW_H

#include <QTreeView>
#include <QHeaderView>
#include <QPainter>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>
#include <vector>
#include "RowLayoutEngine.h"
#include "../core/ModelContract.h"

namespace QuarkMeta {

/**
 * @brief 列表视图列策略配置 (ColumnPolicy)
 * 声明式静态定义各列的尺寸、拉伸模式与响应式激活阈值。
 */
struct ColumnPolicy {
    FileListColumn column;
    int fixedWidth;                     // 固定宽度（Name 列为 0）
    QHeaderView::ResizeMode resizeMode; // Fixed 或 Interactive
    bool alwaysHidden;                  // 是否常态隐藏 (如 Status 列)
};

class ContentHeaderView : public QHeaderView {
    Q_OBJECT
public:
    explicit ContentHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QHeaderView(orientation, parent) {}

    void setZoomLevel(int zoom) {
        m_zoomLevel = zoom;
        if (viewport()) viewport()->update();
    }

protected:
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override {
        if (!rect.isValid()) return;

        painter->save();
        painter->fillRect(rect, QColor("#252525"));
        painter->setPen(QColor("#333333"));
        painter->drawLine(rect.topRight(), rect.bottomRight());

        QString title = model() ? model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString() : QString();
        bool isSorted = (sortIndicatorSection() == logicalIndex && logicalIndex >= 0);
        QColor textColor = isSorted ? QColor("#FFFFFF") : QColor("#B0B0B0");
        painter->setPen(textColor);
        painter->setFont(font());

        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(title);

        if (logicalIndex == 0) {
            int textStartX = rect.left() + RowLayoutEngine::calculateHeaderTextStartX(m_zoomLevel);
            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);

            if (isSorted) {
                int arrowX = textStartX + textWidth + 6;
                drawSortArrow(painter, arrowX, rect.center().y(), sortIndicatorOrder(), textColor);
            }
        } else {
            if (isSorted) {
                int arrowWidth = 7;
                int gap = 6;
                int totalWidth = textWidth + gap + arrowWidth;
                int startX = rect.left() + (rect.width() - totalWidth) / 2;

                QRect textRect(startX, rect.top(), textWidth + 2, rect.height());
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);

                int arrowX = startX + textWidth + gap;
                drawSortArrow(painter, arrowX, rect.center().y(), sortIndicatorOrder(), textColor);
            } else {
                painter->drawText(rect, Qt::AlignCenter, title);
            }
        }
        painter->restore();
    }

private:
    static void drawSortArrow(QPainter* painter, int x, int centerY, Qt::SortOrder order, const QColor& color) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);

        QPolygonF triangle;
        if (order == Qt::AscendingOrder) {
            triangle << QPointF(x, centerY + 2.5)
                     << QPointF(x + 7.0, centerY + 2.5)
                     << QPointF(x + 3.5, centerY - 3.5);
        } else {
            triangle << QPointF(x, centerY - 2.5)
                     << QPointF(x + 7.0, centerY - 2.5)
                     << QPointF(x + 3.5, centerY + 3.5);
        }
        painter->drawPolygon(triangle);
        painter->restore();
    }

private:
    int m_zoomLevel = 96;
};

class DropTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit DropTreeView(QWidget* parent = nullptr);

    // 🚀【关键修复】：声明极小 minimumSizeHint，防止在 QStackedWidget 后台顶死外层布局
    QSize minimumSizeHint() const override { return QSize(50, 50); }

    int rowHeight(const QModelIndex& index) const { return QTreeView::rowHeight(index); }
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }
    void setBottomMargin(int bottom) { m_bottomMargin = bottom; updateGeometries(); }

    void setModel(QAbstractItemModel* model) override;
    void updateGroupHeaderSpanning();
    void applyColumnPolicies();

signals:
    void notesDropped(const QList<int>& noteIds, const QModelIndex& targetIndex);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);

protected:
    void startDrag(Qt::DropActions supportedActions) override;

    void keyboardSearch(const QString& search) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;
    int m_bottomMargin = 0;
    bool m_isApplyingPolicies = false;
};

} // namespace QuarkMeta

#endif // DROPTREEVIEW_H