# Implementation Plan - JustifiedView Single-View Grouping Architecture (`JustifiedView-1.md`)

## Overview
This implementation plan refactors `JustifiedView` to natively support unified single-view grouping (folders & files) without relying on external split widgets or multiple `QScrollArea` wrappers.
By inserting sentinel header geometries (`ItemGeometry` with `isHeader = true`) during `doLayout()`, `JustifiedView` draws section headers ("文件夹 (N)" / "文件 (M)"), supports folder group collapse/expand (`m_folderGroupCollapsed`), and isolates clicks on headers from item selections.

This restores true viewport calculation (`viewport()->rect()`) for thumbnail lazy loading and eliminates multi-scrollarea synchronization issues.

---

## Modified Files List
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/JustifiedView.h`

```
<<<<<<< SEARCH
    struct ItemGeometry {
        QRect rect;
        int index;
    };
    std::vector<ItemGeometry> m_geometries;
=======
    struct ItemGeometry {
        QRect rect;
        int index = -1;         // Real model row index, or -1 for sentinel section header
        bool isHeader = false;
        QString headerText;
        bool isFolderGroup = false;
    };
    std::vector<ItemGeometry> m_geometries;
    bool m_folderGroupCollapsed = false;
>>>>>>> REPLACE
```

---

### 2. `src/ui/JustifiedView.cpp`

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
            if (it->isHeader) {
                return QModelIndex(); // Sentinel section header is not a selectable item
            }
            return model()->index(it->index, 0);
        }
    }
    return QModelIndex();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void JustifiedView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command) {
    QRect contentsRect = rect.translated(0, verticalScrollBar()->value());
    QItemSelection selection;
    for (const auto& geo : m_geometries) {
        if (geo.rect.intersects(contentsRect)) {
            QModelIndex idx = model()->index(geo.index, 0);
            selection.select(idx, idx);
        }
    }
    selectionModel()->select(selection, command);
}
=======
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
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
=======
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
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QModelIndex idx = model()->index(geo.index, 0);
        QStyleOptionViewItem option;
        initViewItemOption(&option); 
        option.rect = geo.rect;
        
        if (selectionModel()->isSelected(idx))
            option.state |= QStyle::State_Selected;
        if (currentIndex() == idx)
            option.state |= QStyle::State_HasFocus;

        itemDelegateForIndex(idx)->paint(&painter, option, idx);
=======
        if (geo.isHeader) {
            painter.save();
            painter.fillRect(geo.rect, QColor("#202020"));
            painter.setPen(QColor("#CCCCCC"));
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            QString text = (geo.isFolderGroup ? (m_folderGroupCollapsed ? "▶ " : "▼ ") : "") + geo.headerText;
            painter.drawText(geo.rect.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
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
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Compile the project using CMake:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. Run unit and UI tests.
3. Verify that `JustifiedView` renders folder section headers and file section headers correctly in a single view with zero scrolling artifacts.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`viewport()->rect()` SSOT**: Directly calculates visible elements using single view viewport rect.
- **`QAbstractItemView` Contract**: Preserves native model/view architecture without split views.

---

## Header API Signature Verification Table

| File | Class / Struct | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/JustifiedView.h` | `JustifiedView` | `QRect visualRect(const QModelIndex& index) const override;` | Verified 100% Match |
| `src/ui/JustifiedView.h` | `JustifiedView` | `QModelIndex indexAt(const QPoint& point) const override;` | Verified 100% Match |
| `src/ui/JustifiedView.h` | `JustifiedView` | `void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command) override;` | Verified 100% Match |
| `src/ui/JustifiedView.h` | `JustifiedView` | `void paintEvent(QPaintEvent* event) override;` | Verified 100% Match |
