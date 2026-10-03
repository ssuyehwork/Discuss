#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "JustifiedView.h"
#include "CardLayoutEngine.h"
#include "../core/ModelContract.h"
#include <QPainter>
#include <QScrollBar>
#include <QResizeEvent>
#include <QStyleOptionViewItem>
#include <QAbstractItemDelegate>
#include <QTimer>
#include <algorithm>

namespace QuarkMeta {

JustifiedView::JustifiedView(QWidget* parent) : QAbstractItemView(parent) {
    setFrameShape(QFrame::NoFrame);
    m_layoutTimer = new QTimer(this);
    m_layoutTimer->setSingleShot(true);
    m_layoutTimer->setInterval(50);
    connect(m_layoutTimer, &QTimer::timeout, this, &JustifiedView::onLayoutTimerTimeout);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setSingleStep(20);
    
    setAutoFillBackground(true);
    viewport()->setAutoFillBackground(true);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    
    QPalette pal = viewport()->palette();
    pal.setColor(QPalette::Window, QColor("#1E1E1E"));
    viewport()->setPalette(pal);
    setPalette(pal);
}

void JustifiedView::setLayoutMode(LayoutMode mode) {
    if (m_layoutMode != mode) {
        m_layoutMode = mode;
        scheduleLayout();
    }
}

JustifiedView::LayoutMode JustifiedView::layoutMode() const {
    return m_layoutMode;
}

void JustifiedView::setFoldersCollapsed(bool collapsed) {
    if (m_foldersCollapsed != collapsed) {
        m_foldersCollapsed = collapsed;
        scheduleLayout();
    }
}

void JustifiedView::setTargetRowHeight(int h) {
    if (m_targetRowHeight != h) {
        m_targetRowHeight = h;
        scheduleLayout();
    }
}

void JustifiedView::setAspectRatioRole(int role) {
    if (m_aspectRatioRole != role) {
        m_aspectRatioRole = role;
        scheduleLayout();
    }
}

void JustifiedView::reset() {
    QAbstractItemView::reset();
    scheduleLayout();
}

void JustifiedView::doItemsLayout() {
    scheduleLayout();
}

void JustifiedView::setModel(QAbstractItemModel* model) {
    if (this->model()) {
        disconnect(this->model(), &QAbstractItemModel::rowsRemoved, this, nullptr);
        disconnect(this->model(), &QAbstractItemModel::modelReset, this, nullptr);
        disconnect(this->model(), &QAbstractItemModel::layoutChanged, this, nullptr);
    }
    QAbstractItemView::setModel(model);
    if (model) {
        connect(model, &QAbstractItemModel::rowsRemoved, this, [this]() {
            doLayout();
        });
        connect(model, &QAbstractItemModel::modelReset, this, [this]() {
            doLayout();
        });
        connect(model, &QAbstractItemModel::layoutChanged, this, [this]() {
            doLayout();
        });
    }
}

void JustifiedView::scheduleLayout() {
    m_layoutDirty = true;
    if (m_layoutTimer && !m_layoutTimer->isActive()) {
        m_layoutTimer->start();
    }
}

void JustifiedView::onLayoutTimerTimeout() {
    if (m_layoutDirty) {
        doLayout();
    }
}

QRect JustifiedView::visualRect(const QModelIndex& index) const {
    if (!index.isValid() || index.row() >= (int)m_geometries.size()) return QRect();
    QRect r = m_geometries[index.row()].rect;
    r.translate(0, -verticalScrollBar()->value());
    return r;
}

void JustifiedView::scrollTo(const QModelIndex& index, ScrollHint hint) {
    QRect rect = visualRect(index);
    if (rect.isEmpty()) return;
    
    int viewportHeight = viewport()->height();
    int scrollValue = verticalScrollBar()->value();
    
    if (hint == EnsureVisible) {
        if (rect.top() < 0) verticalScrollBar()->setValue(scrollValue + rect.top());
        else if (rect.bottom() > viewportHeight) verticalScrollBar()->setValue(scrollValue + rect.bottom() - viewportHeight);
    }
}

QModelIndex JustifiedView::indexAt(const QPoint& point) const {
    if (m_geometries.empty()) return QModelIndex();
    int y = point.y() + verticalScrollBar()->value();

    auto it = std::lower_bound(m_geometries.begin(), m_geometries.end(), y,
        [](const ItemGeometry& geo, int targetY) {
            return geo.rect.bottom() < targetY;
        });

    for (; it != m_geometries.end(); ++it) {
        if (it->rect.top() > y) break;
        if (it->rect.contains(point.x(), y)) {
            if (it->isHeader) return QModelIndex();
            return model()->index(it->index, 0);
        }
    }
    return QModelIndex();
}

void JustifiedView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
    if (roles.isEmpty() || roles.contains(m_aspectRatioRole) || roles.contains(Qt::DecorationRole) || roles.contains(ColorRole)) {
        scheduleLayout();
    }
    viewport()->update();
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
}

void JustifiedView::rowsInserted(const QModelIndex& parent, int start, int end) {
    scheduleLayout();
    QAbstractItemView::rowsInserted(parent, start, end);
}

void JustifiedView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    doLayout();
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}

QModelIndex JustifiedView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers) {
    QModelIndex current = currentIndex();
    if (!current.isValid()) return model()->index(0, 0);
    
    int row = current.row();
    int count = (int)m_geometries.size();
    if (row < 0 || row >= count) return current;

    if (cursorAction == MoveLeft) {
        row = std::max(0, row - 1);
    } else if (cursorAction == MoveRight) {
        row = std::min(count - 1, row + 1);
    } else if (cursorAction == MoveUp || cursorAction == MoveDown) {
        QRect currentRect = m_geometries[row].rect;
        int centerX = currentRect.center().x();
        int bestIdx = -1;
        int minDistance = 1000000;

        for (int i = 0; i < count; ++i) {
            if (i == row) continue;
            QRect targetRect = m_geometries[i].rect;
            
            if (cursorAction == MoveUp && targetRect.bottom() < currentRect.top()) {
                int dy = currentRect.top() - targetRect.bottom();
                int dx = std::abs(targetRect.center().x() - centerX);
                int dist = dy * 100 + dx; 
                if (dist < minDistance) {
                    minDistance = dist;
                    bestIdx = i;
                }
            } else if (cursorAction == MoveDown && targetRect.top() > currentRect.bottom()) {
                int dy = targetRect.top() - currentRect.bottom();
                int dx = std::abs(targetRect.center().x() - centerX);
                int dist = dy * 100 + dx;
                if (dist < minDistance) {
                    minDistance = dist;
                    bestIdx = i;
                }
            }
        }
        if (bestIdx != -1) row = bestIdx;
    }
    
    return model()->index(row, 0);
}

int JustifiedView::horizontalOffset() const { return 0; }
int JustifiedView::verticalOffset() const { return verticalScrollBar()->value(); }
bool JustifiedView::isIndexHidden(const QModelIndex&) const { return false; }

void JustifiedView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command) {
    QRect contentsRect = rect.translated(0, verticalScrollBar()->value());
    QItemSelection selection;
    for (const auto& geo : m_geometries) {
        if (geo.isHeader) continue;
        if (geo.rect.intersects(contentsRect)) {
            QModelIndex idx = model()->index(geo.index, 0);
            selection.select(idx, idx);
        }
    }
    selectionModel()->select(selection, command);
}

QRegion JustifiedView::visualRegionForSelection(const QItemSelection& selection) const {
    QRegion region;
    for (const auto& range : selection) {
        for (int i = range.top(); i <= range.bottom(); ++i) {
            region += visualRect(model()->index(i, 0));
        }
    }
    return region;
}

void JustifiedView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier) {
        QModelIndex idx = indexAt(event->pos());
        if (!idx.isValid()) {
            m_isDraggingSelection = true;
            m_dragStartPos = event->pos();
            m_selectionRect = QRect();
            selectionModel()->clearSelection();
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier)) {
        QModelIndex clicked = indexAt(event->pos());
        if (clicked.isValid() && m_anchorRow >= 0) {
            int anchorVisual = -1, clickedVisual = -1;
            for (int i = 0; i < (int)m_geometries.size(); ++i) {
                if (m_geometries[i].index == m_anchorRow)      anchorVisual = i;
                if (m_geometries[i].index == clicked.row())    clickedVisual = i;
            }
            if (anchorVisual >= 0 && clickedVisual >= 0) {
                int vFrom = std::min(anchorVisual, clickedVisual);
                int vTo   = std::max(anchorVisual, clickedVisual);
                QItemSelection sel;
                for (int v = vFrom; v <= vTo; ++v) {
                    QModelIndex idx = model()->index(m_geometries[v].index, 0);
                    sel.select(idx, idx);
                }
                selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect);
                selectionModel()->setCurrentIndex(clicked, QItemSelectionModel::NoUpdate);
                event->accept();
                viewport()->update();
                return;
            }
        }
    }

    QAbstractItemView::mousePressEvent(event);
    QModelIndex current = currentIndex();
    if (current.isValid()) {
        m_anchorRow = current.row();
    } else {
        m_anchorRow = -1;
    }
    viewport()->update();
}

void JustifiedView::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDraggingSelection) {
        m_selectionRect = QRect(m_dragStartPos, event->pos()).normalized();
        setSelection(m_selectionRect, QItemSelectionModel::ClearAndSelect);
        viewport()->update();
        event->accept();
        return;
    }
    QAbstractItemView::mouseMoveEvent(event);
}

void JustifiedView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isDraggingSelection) {
        m_isDraggingSelection = false;
        m_selectionRect = QRect();
        viewport()->update();
        event->accept();
        return;
    }
    QAbstractItemView::mouseReleaseEvent(event);
}

void JustifiedView::mouseDoubleClickEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (!idx.isValid()) {
        QAbstractItemView::mouseDoubleClickEvent(event);
        return;
    }

    // 核心架构意图：双击 = 打开 / 预览（绝无编辑副作用）
    // 无论是命中卡片封面、文字区域还是整卡，统一发射 doubleClicked 驱动打开/预览
    emit doubleClicked(idx);
}

void JustifiedView::paintEvent(QPaintEvent* event) {
    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), QColor("#1E1E1E"));

    if (m_geometries.empty()) {
        painter.save();
        painter.setPen(QColor("#888888"));
        painter.setFont(QFont("Microsoft YaHei", 12));
        painter.drawText(viewport()->rect(), Qt::AlignCenter, "没有可显示的项目");
        painter.restore();
        return;
    }
    
    painter.save();
    int scrollY = verticalScrollBar()->value();
    painter.translate(0, -scrollY);
    
    QRect dirtyRect = event->rect().translated(0, scrollY);
    int dirtyTop = dirtyRect.top();
    int dirtyBottom = dirtyRect.bottom();

    auto startIt = std::lower_bound(m_geometries.begin(), m_geometries.end(), dirtyTop,
        [](const ItemGeometry& geo, int targetY) {
            return geo.rect.bottom() < targetY;
        });

    for (auto it = startIt; it != m_geometries.end(); ++it) {
        const auto& geo = *it;
        if (geo.rect.top() > dirtyBottom) break;

        if (geo.isHeader) {
            painter.save();
            painter.setPen(QColor("#3498db"));
            painter.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
            painter.drawText(geo.rect, Qt::AlignLeft | Qt::AlignVCenter, geo.headerText);
            painter.restore();
            continue;
        }

        QModelIndex idx = model()->index(geo.index, 0);
        if (!idx.isValid()) continue;

        QStyleOptionViewItem option;
        initViewItemOption(&option); 
        option.rect = geo.rect;
        
        if (selectionModel()->isSelected(idx))
            option.state |= QStyle::State_Selected;
        if (currentIndex() == idx)
            option.state |= QStyle::State_HasFocus;

        itemDelegateForIndex(idx)->paint(&painter, option, idx);
    }
    painter.restore();

    if (m_isDraggingSelection && !m_selectionRect.isEmpty()) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        QColor highlightColor = QColor("#378ADD");
        QColor brushColor = highlightColor;
        brushColor.setAlpha(80);
        painter.setBrush(brushColor);
        painter.setPen(QPen(highlightColor, 1, Qt::SolidLine));
        painter.drawRect(m_selectionRect);
        painter.restore();
    }
}

void JustifiedView::resizeEvent(QResizeEvent* event) {
    doLayout();
    QAbstractItemView::resizeEvent(event);
}

void JustifiedView::updateGeometries() {
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setRange(0, std::max(0, m_totalHeight - viewport()->height()));
    QAbstractItemView::updateGeometries();
}

void JustifiedView::doLayout() {
    m_geometries.clear();
    m_totalHeight = 0;

    if (!model() || model()->rowCount() == 0) {
        emit totalHeightChanged(m_totalHeight);
        emit layoutFinished();
        return;
    }

    int viewWidth = viewport()->width();
    if (viewWidth <= 0) viewWidth = width();
    if (viewWidth <= 0) viewWidth = 800;

    int spacing = 6;
    int currentY = spacing;
    int count = model()->rowCount();

    // 扫描分类：提取文件夹与文件索引
    std::vector<int> folderIndices;
    std::vector<int> fileIndices;

    for (int i = 0; i < count; ++i) {
        QModelIndex idx = model()->index(i, 0);
        bool isDir = idx.data(TypeRole).toString() == "folder" || idx.data(Qt::UserRole + 2).toBool();
        if (isDir) {
            folderIndices.push_back(i);
        } else {
            fileIndices.push_back(i);
        }
    }

    // 1. 文件夹区块布局
    if (!folderIndices.empty() && !m_foldersCollapsed) {
        for (int idx : folderIndices) {
            QModelIndex modelIdx = model()->index(idx, 0);
            double aspect = modelIdx.data(m_aspectRatioRole).toDouble();
            if (aspect <= 0.1) aspect = 1.0;
            int itemW = static_cast<int>(m_targetRowHeight * aspect);

            m_geometries.push_back({ QRect(spacing, currentY, itemW, m_targetRowHeight), idx, false, "", false });
            currentY += m_targetRowHeight + spacing;
        }
    }

    // 2. 文件区块布局
    if (!fileIndices.empty()) {
        for (int idx : fileIndices) {
            QModelIndex modelIdx = model()->index(idx, 0);
            double aspect = modelIdx.data(m_aspectRatioRole).toDouble();
            if (aspect <= 0.1) aspect = 1.0;
            int itemW = static_cast<int>(m_targetRowHeight * aspect);

            m_geometries.push_back({ QRect(spacing, currentY, itemW, m_targetRowHeight), idx, false, "", false });
            currentY += m_targetRowHeight + spacing;
        }
    }

    m_totalHeight = currentY + spacing;
    emit totalHeightChanged(m_totalHeight);
    emit layoutFinished();
}

bool JustifiedView::isLayoutReady() const {
    if (m_layoutDirty) return false;
    if (!model()) return true;
    return static_cast<int>(m_geometries.size()) >= model()->rowCount();
}

QList<int> JustifiedView::rowsInRange(int top, int bottom) const {
    QList<int> rows;
    if (!isLayoutReady() || m_geometries.empty()) return rows;

    auto startIt = std::lower_bound(m_geometries.begin(), m_geometries.end(), top,
        [](const ItemGeometry& geo, int targetY) {
            return geo.rect.bottom() < targetY;
        });

    for (auto it = startIt; it != m_geometries.end(); ++it) {
        const auto& geo = *it;
        if (geo.rect.top() > bottom) break;
        if (!geo.isHeader) {
            rows.append(geo.index);
        }
    }
    return rows;
}

} // namespace QuarkMeta