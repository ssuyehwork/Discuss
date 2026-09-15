#include "DropListView.h"
#include "../core/ModelContract.h"
#include "ColumnItemDelegate.h"
#include "ViewDragDropHelper.h"
#include <QMouseEvent>

namespace QuarkMeta {

DropListView::DropListView(QWidget* parent) : QListView(parent) {
    setDragEnabled(true);
    setAcceptDrops(true);
}

void DropListView::dragEnterEvent(QDragEnterEvent* event) {
    if (!ViewDragDropHelper::handleDragEnter(this, event)) {
        QListView::dragEnterEvent(event);
    }
}

void DropListView::dragMoveEvent(QDragMoveEvent* event) {
    QModelIndex hoverIdx = indexAt(event->position().toPoint());
    if (m_currentHoverDropIdx != hoverIdx) {
        clearDropHighlight();
        if (hoverIdx.isValid()) {
            bool isFolder = (hoverIdx.data(TypeRole).toString() == "folder") || hoverIdx.data(Qt::UserRole + 2).toBool();
            if (isFolder) {
                m_currentHoverDropIdx = hoverIdx;
                if (model()) {
                    const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                    viewport()->update();
                }
            }
        }
    }

    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QListView::dragMoveEvent(event);
    }
}

void DropListView::dragLeaveEvent(QDragLeaveEvent* event) {
    clearDropHighlight();
    ViewDragDropHelper::clearHover(this);
    QListView::dragLeaveEvent(event);
}

void DropListView::clearDropHighlight() {
    if (m_currentHoverDropIdx.isValid() && model()) {
        const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, false, IsDropTargetRole);
        m_currentHoverDropIdx = QModelIndex();
        viewport()->update();
    }
}

void DropListView::dropEvent(QDropEvent* event) {
    clearDropHighlight();
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        QListView::dropEvent(event);
    }
}

void DropListView::startDrag(Qt::DropActions supportedActions) {
    ViewDragDropHelper::executeStartDrag(this, supportedActions);
}

void DropListView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_folderCount > 0 && m_folderHeaderRect.contains(event->pos())) {
            m_foldersCollapsed = !m_foldersCollapsed;
            if (model()) {
                int total = model()->rowCount();
                for (int i = 0; i < total; ++i) {
                    QModelIndex idx = model()->index(i, 0);
                    if (idx.data(TypeRole).toString() == "folder") {
                        setRowHidden(i, m_foldersCollapsed);
                    }
                }
            }
            viewport()->update();
            event->accept();
            return;
        }
    }
    QListView::mousePressEvent(event);
}

void DropListView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event && event->button() == Qt::LeftButton) {
        QModelIndex idx = indexAt(event->pos());
        if (!idx.isValid()) {
            emit blankSpaceDoubleClicked();
            event->accept();
            return;
        }
    }
    QListView::mouseDoubleClickEvent(event);
}

void DropListView::paintEvent(QPaintEvent* event) {
    m_folderCount = 0;
    m_folderHeaderRect = QRect();

    if (model()) {
        int total = model()->rowCount();
        for (int i = 0; i < total; ++i) {
            QModelIndex idx = model()->index(i, 0);
            bool isDir = (idx.data(TypeRole).toString() == "folder");
            if (isDir) {
                m_folderCount++;
                setRowHidden(i, m_foldersCollapsed);
            }
        }
    }

    QListView::paintEvent(event);

    if (m_folderCount > 0) {
        QPainter painter(viewport());
        painter.save();

        m_folderHeaderRect = QRect(0, 0, viewport()->width(), 26);
        painter.fillRect(m_folderHeaderRect, QColor("#222222"));

        painter.setPen(QColor("#A0A0A0"));
        QFont headerFont("Microsoft YaHei", 9, QFont::Bold);
        painter.setFont(headerFont);

        QString arrow = m_foldersCollapsed ? "▶" : "▼";
        QString headerText = QString("  %1  文件夹 (%2)").arg(arrow).arg(m_folderCount);
        painter.drawText(m_folderHeaderRect, Qt::AlignLeft | Qt::AlignVCenter, headerText);

        painter.restore();
    }
}

} // namespace QuarkMeta
