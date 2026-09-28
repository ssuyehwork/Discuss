#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "JustifiedView.h"
#include "UiHelper.h"
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
    }
    QAbstractItemView::setModel(model);
    if (model) {
        connect(model, &QAbstractItemModel::rowsRemoved, this, [this]() {
            scheduleLayout();
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
    if (!index.isValid()) return QRect();
    int targetRow = index.row();
    for (const auto& geo : m_geometries) {
        if (!geo.isHeader && geo.index == targetRow) {
            QRect r = geo.rect;
            r.translate(0, -verticalScrollBar()->value());
            return r;
        }
    }
    return QRect();
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
            if (it->isHeader) {
                return QModelIndex();
            }
            return model()->index(it->index, 0);
        }
    }
    return QModelIndex();
}

void JustifiedView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
    if (roles.contains(m_aspectRatioRole)) {
        scheduleLayout();
    } else {
        viewport()->update();
    }
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
}

void JustifiedView::rowsInserted(const QModelIndex& parent, int start, int end) {
    scheduleLayout();
    QAbstractItemView::rowsInserted(parent, start, end);
}

void JustifiedView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
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
        if (!geo.isHeader && geo.rect.intersects(contentsRect)) {
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
        int y = event->pos().y() + verticalScrollBar()->value();
        for (const auto& geo : m_geometries) {
            if (geo.isHeader && geo.rect.contains(event->pos().x(), y)) {
                if (geo.isFolderGroup) {
                    m_folderGroupCollapsed = !m_folderGroupCollapsed;
                    scheduleLayout();
                }
                event->accept();
                return;
            }
        }

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

void JustifiedView::paintEvent(QPaintEvent*) {
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
    int vHeight = viewport()->height();
    painter.translate(0, -scrollY);
    
    auto startIt = std::lower_bound(m_geometries.begin(), m_geometries.end(), scrollY,
        [](const ItemGeometry& geo, int targetY) {
            return geo.rect.bottom() < targetY;
        });

    for (auto it = startIt; it != m_geometries.end(); ++it) {
        const auto& geo = *it;
        if (geo.rect.top() > scrollY + vHeight) break;

        if (geo.isHeader) {
            painter.save();
            // 组头完全透明，直接透出画板底色 `#1E1E1E`
            
            const int iconSize = 12;
            const int marginX = 10;
            const QColor headerColor("#3498db");

            QFont font("Microsoft YaHei", 9, QFont::Bold);
            painter.setFont(font);
            painter.setPen(headerColor);

            if (geo.isFolderGroup) {
                // 1. Render group title text on the left
                QFontMetrics fm(font);
                int textWidth = fm.horizontalAdvance(geo.headerText);
                QRect textRect(geo.rect.left() + marginX, geo.rect.top(), textWidth, geo.rect.height());
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, geo.headerText);

                // 2. Render SVG vector collapse/expand arrow icon to the right of text
                const QString iconName = m_folderGroupCollapsed ? "scroll-008.svg" : "scroll-010.svg";
                QPixmap arrowPixmap = UiHelper::getIcon(iconName, headerColor, iconSize).pixmap(iconSize, iconSize);
                int iconX = textRect.right() + 6;
                int iconY = geo.rect.top() + (geo.rect.height() - iconSize) / 2;
                painter.drawPixmap(iconX, iconY, arrowPixmap);
            } else {
                QRect textRect = geo.rect.adjusted(marginX, 0, -marginX, 0);
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, geo.headerText);
            }

            painter.restore();
        } else {
            QModelIndex idx = model()->index(geo.index, 0);
            QStyleOptionViewItem option;
            initViewItemOption(&option); 
            option.rect = geo.rect;
            
            if (selectionModel()->isSelected(idx))
                option.state |= QStyle::State_Selected;
            if (currentIndex() == idx)
                option.state |= QStyle::State_HasFocus;

            itemDelegateForIndex(idx)->paint(&painter, option, idx);
        }
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
    m_layoutDirty = false;
    m_geometries.clear();
    if (!model()) return;
    int count = model()->rowCount();
    
    if (count == 0) {
        m_totalHeight = 0;
        updateGeometries();
        viewport()->update();
        return;
    }

    const int margin = 10;
    const int spacing = 5;
    const int headerHeight = 28;
    
    int scrollBarW = (verticalScrollBar() && verticalScrollBar()->isVisible()) ? verticalScrollBar()->width() : 0;
    int containerWidth = width() - scrollBarW - (margin * 2);
    if (containerWidth <= 0) return;

    int currentY = margin; 

    const int cardPadding = CardLayoutEngine::totalPaddingHorizontal();
    const int extraHeight = CardLayoutEngine::extraHeight();

    std::vector<int> folderIndices;
    std::vector<int> fileIndices;
    for (int r = 0; r < count; ++r) {
        QModelIndex idx = model()->index(r, 0);
        bool isDir = (model()->data(idx, TypeRole).toString() == "folder");
        if (isDir) {
            folderIndices.push_back(r);
        } else {
            fileIndices.push_back(r);
        }
    }

    auto layoutGridGroup = [&](const std::vector<int>& indices) {
        if (indices.empty()) return;
        int itemWidth = m_targetRowHeight + cardPadding;
        int itemHeight = m_targetRowHeight + extraHeight;

        int maxNumInRow = (containerWidth + spacing) / (itemWidth + spacing);
        if (maxNumInRow <= 0) maxNumInRow = 1;

        int standardSpacing = spacing;
        if (maxNumInRow > 1) {
            standardSpacing = (containerWidth - (maxNumInRow * itemWidth)) / (maxNumInRow - 1);
        }

        int idxCount = static_cast<int>(indices.size());
        int i = 0;
        while (i < idxCount) {
            int rowStart = i;
            int numInRow = std::min(maxNumInRow, idxCount - i);

            int currentX = margin;
            if (maxNumInRow == 1) {
                currentX = margin + std::max(0, (containerWidth - itemWidth) / 2);
            }

            for (int j = 0; j < numInRow; ++j) {
                int modelIdx = indices[rowStart + j];
                ItemGeometry itemGeo;
                itemGeo.rect = QRect(currentX, currentY, itemWidth, itemHeight);
                itemGeo.index = modelIdx;
                itemGeo.isHeader = false;
                m_geometries.push_back(itemGeo);
                currentX += itemWidth + standardSpacing;
            }
            i += numInRow;
            currentY += itemHeight + spacing;
        }
    };

    auto layoutJustifiedGroup = [&](const std::vector<int>& indices) {
        if (indices.empty()) return;
        int idxCount = static_cast<int>(indices.size());
        int i = 0;
        while (i < idxCount) {
            int rowStart = i;

            double rowAspectRatioSum = 0;
            std::vector<double> aspectRatios;

            while (i < idxCount) {
                int modelIdx = indices[i];
                QModelIndex idx = model()->index(modelIdx, 0);
                double ar = model()->data(idx, m_aspectRatioRole).toDouble();
                if (ar <= 0) ar = 1.0;

                aspectRatios.push_back(ar);
                rowAspectRatioSum += ar;

                int numInRow = (int)aspectRatios.size();
                double estimatedWidth = (rowAspectRatioSum * m_targetRowHeight) + (cardPadding * numInRow) + (spacing * (numInRow - 1));
                if (estimatedWidth > containerWidth) {
                    if (numInRow > 1) {
                        aspectRatios.pop_back();
                        rowAspectRatioSum -= ar;
                    } else {
                        i++;
                    }
                    break;
                }
                i++;
            }

            int rowEnd = i;
            int numInRow = rowEnd - rowStart;
            if (numInRow <= 0) break;

            int actualHeight = m_targetRowHeight;
            bool isLastRow = (i == idxCount);
            bool rowIsJustified = !isLastRow;

            int availableImageWidth = containerWidth - (spacing * (numInRow - 1)) - (cardPadding * numInRow);

            if (rowIsJustified) {
                actualHeight = qRound(availableImageWidth / rowAspectRatioSum);
                actualHeight = std::max(actualHeight, (int)(m_targetRowHeight * 0.75));
                actualHeight = std::min(actualHeight, (int)(m_targetRowHeight * 1.5));
            }

            int currentX = margin;

            for (int j = 0; j < numInRow; ++j) {
                int modelIdx = indices[rowStart + j];
                int itemWidth;

                if (j == numInRow - 1 && rowIsJustified) {
                    itemWidth = (containerWidth + margin) - currentX;
                } else {
                    itemWidth = qRound(aspectRatios[j] * actualHeight) + cardPadding;
                }

                ItemGeometry itemGeo;
                itemGeo.rect = QRect(currentX, currentY, itemWidth, actualHeight + extraHeight);
                itemGeo.index = modelIdx;
                itemGeo.isHeader = false;
                m_geometries.push_back(itemGeo);
                currentX += itemWidth + spacing;
            }
            currentY += actualHeight + extraHeight + spacing;
        }
    };

    if (!folderIndices.empty()) {
        ItemGeometry folderHeaderGeo;
        folderHeaderGeo.rect = QRect(margin, currentY, containerWidth, headerHeight);
        folderHeaderGeo.index = -1;
        folderHeaderGeo.isHeader = true;
        folderHeaderGeo.headerText = QString("文件夹 (%1)").arg(folderIndices.size());
        folderHeaderGeo.isFolderGroup = true;
        m_geometries.push_back(folderHeaderGeo);
        currentY += headerHeight + spacing;

        if (!m_folderGroupCollapsed) {
            if (m_layoutMode == GridMode) {
                layoutGridGroup(folderIndices);
            } else {
                layoutJustifiedGroup(folderIndices);
            }
        }
    }

    if (!fileIndices.empty()) {
        if (!folderIndices.empty()) {
            ItemGeometry fileHeaderGeo;
            fileHeaderGeo.rect = QRect(margin, currentY, containerWidth, headerHeight);
            fileHeaderGeo.index = -1;
            fileHeaderGeo.isHeader = true;
            fileHeaderGeo.headerText = QString("文件 (%1)").arg(fileIndices.size());
            fileHeaderGeo.isFolderGroup = false;
            m_geometries.push_back(fileHeaderGeo);
            currentY += headerHeight + spacing;
        }

        if (m_layoutMode == GridMode) {
            layoutGridGroup(fileIndices);
        } else {
            layoutJustifiedGroup(fileIndices);
        }
    }

    int oldHeight = m_totalHeight;
    m_totalHeight = currentY;
    updateGeometries();
    viewport()->update();

    if (oldHeight != m_totalHeight) {
        emit totalHeightChanged(m_totalHeight);
    }
}

} // namespace QuarkMeta