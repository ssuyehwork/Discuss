#ifndef DROPTREEVIEW_H
#define DROPTREEVIEW_H

#include <QTreeView>
#include <QHeaderView>
#include <QPainter>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>

namespace QuarkMeta {

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
        painter->setPen(QColor("#B0B0B0"));
        painter->setFont(font());

        if (logicalIndex == 0) {
            int textStartX = rect.left() + 40; // 🚀 表头第 0 列名称固定从 40px 起始，与 TreeItemDelegate 40px 文本起点绝对物理居中垂直对齐

            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        } else {
            painter->drawText(rect, Qt::AlignCenter, title);
        }
        painter->restore();
    }

private:
    int m_zoomLevel = 96;
};

class DropTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit DropTreeView(QWidget* parent = nullptr);

    /**
     * @brief 物理辅助：暴露内部 rowHeight 接口以支持外部布局高度计算
     */
    int rowHeight(const QModelIndex& index) const { return QTreeView::rowHeight(index); }

    /**
     * @brief 设置空状态时的占位文本提示
     */
    void setEmptyHint(const QString& hint) { m_emptyHint = hint; }

signals:
    void notesDropped(const QList<int>& noteIds, const QModelIndex& targetIndex);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

    void keyboardSearch(const QString& search) override;
    void paintEvent(QPaintEvent* event) override;

private:
    // 2026-06-xx 物理辅助：拖拽悬停自动展开
    QTimer* m_autoExpandTimer = nullptr;
    QModelIndex m_hoverIndex;
    QString m_emptyHint;
};

} // namespace QuarkMeta

#endif // DROPTREEVIEW_H
