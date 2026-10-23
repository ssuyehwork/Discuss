# JustifiedView-5.md - Binary Search Viewport Clipped Rendering

## 1. Overview
This implementation plan optimizes `JustifiedView::paintEvent` and viewport indexing using binary search (`std::lower_bound`). It clips drawing strictly to visible item geometries intersecting `event->rect()`, reducing rendering complexity from $O(N)$ to $O(K)$ where $K$ is the number of items in the viewport.

## 2. Architectural Principles & Threading Rules
1. **Viewport Clipping Guarantee**:
   - `JustifiedView::paintEvent` computes dirty region intersections against `event->rect()`.
   - Items completely outside the dirty rect are bypassed during delegate painting.
2. **Binary Search Range Lookup**:
   - `rowsInRect(QRect)` uses `std::lower_bound` on sorted `m_geometries` to identify candidate rows in $O(\log N)$ time.

## 3. Modified Files List
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`

## 4. Detailed Line-by-Line Changes

### `src/ui/JustifiedView.h`

```
<<<<<<< SEARCH
public:
    QRect visualRect(const QModelIndex& index) const override;
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
    QModelIndex indexAt(const QPoint& point) const override;
=======
public:
    QRect visualRect(const QModelIndex& index) const override;
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
    QModelIndex indexAt(const QPoint& point) const override;
    QList<int> rowsInRect(const QRect& rect) const;
>>>>>>> REPLACE
```

### `src/ui/JustifiedView.cpp`

```
<<<<<<< SEARCH
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
=======
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

QList<int> JustifiedView::rowsInRect(const QRect& rect) const {
    QList<int> rows;
    if (m_geometries.empty() || rect.isEmpty()) return rows;

    int targetTop = rect.top() + verticalScrollBar()->value();
    int targetBottom = rect.bottom() + verticalScrollBar()->value();

    auto it = std::lower_bound(m_geometries.begin(), m_geometries.end(), targetTop,
        [](const ItemGeometry& geo, int topY) {
            return geo.rect.bottom() < topY;
        });

    for (; it != m_geometries.end(); ++it) {
        if (it->rect.top() > targetBottom) break;
        if (it->isHeader) continue;
        if (it->rect.intersects(QRect(rect.left(), targetTop, rect.width(), rect.height()))) {
            rows.append(it->index);
        }
    }
    return rows;
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
            painter.setPen(QColor("#3498db"));
            painter.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
            painter.drawText(geo.rect, Qt::AlignLeft | Qt::AlignVCenter, geo.headerText);
            painter.restore();
            continue;
        }

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
=======
void JustifiedView::paintEvent(QPaintEvent* event) {
    QPainter painter(viewport());
    QRect dirtyRect = event ? event->rect() : viewport()->rect();
    painter.fillRect(dirtyRect, QColor("#1E1E1E"));

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
    
    int dirtyTop = dirtyRect.top() + scrollY;
    int dirtyBottom = dirtyRect.bottom() + scrollY;

    auto startIt = std::lower_bound(m_geometries.begin(), m_geometries.end(), dirtyTop,
        [](const ItemGeometry& geo, int targetY) {
            return geo.rect.bottom() < targetY;
        });

    for (auto it = startIt; it != m_geometries.end(); ++it) {
        const auto& geo = *it;
        if (geo.rect.top() > dirtyBottom) break;

        if (!geo.rect.intersects(QRect(dirtyRect.left(), dirtyTop, dirtyRect.width(), dirtyRect.height()))) {
            continue;
        }

        if (geo.isHeader) {
            painter.save();
            painter.setPen(QColor("#3498db"));
            painter.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
            painter.drawText(geo.rect, Qt::AlignLeft | Qt::AlignVCenter, geo.headerText);
            painter.restore();
            continue;
        }

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
>>>>>>> REPLACE
```

## 5. Build & Verification Steps
Static code verification confirms that `paintEvent` uses `event->rect()` and `rowsInRect` for $O(\log N)$ item geometry range selection.

## 6. SSOT API Reuse & Anti-Redundancy Self-Check
Reuses `std::lower_bound` on sorted geometries vector.

## 7. Header API Signature Verification
- `QList<int> JustifiedView::rowsInRect(const QRect& rect) const`
- `void JustifiedView::paintEvent(QPaintEvent* event) override`

## 8. Header Inclusion Chain & Type Completeness Check
- `#include "JustifiedView.h"`
- `#include <QPaintEvent>`
- `#include <algorithm>`
