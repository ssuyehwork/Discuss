# Implementation Plan: Fix DropTreeView Scroll Unit Mismatch and Conditional Bottom Margin

## 1. Overview
The previous implementation added `m_bottomMargin` (84) directly to `verticalScrollBar()->maximum()`. Because `QTreeView` defaults to `ScrollPerItem`, Qt treated `84` as **84 rows / items** (~2352 pixels), creating an unnaturally huge scroll area and massive empty space.

### Root Cause:
1. `QTreeView` vertical scrollbar range is measured in **rows/items** (`ScrollPerItem`), not pixels. Adding `84` added 84 virtual rows.
2. The bottom margin was applied unconditionally, even when items did not fill the viewport (`maximum() == 0`), forcing a scrollbar on short lists.

### Solution:
1. Set `setVerticalScrollMode(QAbstractItemView::ScrollPerPixel)` on `DropTreeView` for smooth pixel-based scrolling and pixel-unit scrollbar metrics.
2. In `DropTreeView::updateGeometries()`, check `if (bar->maximum() > 0)` so extra bottom margin is ONLY applied when item content actually exceeds viewport height and causes scrolling.
3. Convert pixel margin (`84px`) properly depending on `verticalScrollMode()`.

---

## 2. Modified Files List
1. `src/ui/DropTreeView.cpp`
2. `src/ui/DropTreeView.h`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/DropTreeView.cpp`

```
<<<<<<< SEARCH
DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setDragEnabled(true);
    setDropIndicatorShown(true);
    DragDropEventFilter::install(this);
=======
DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setDragEnabled(true);
    setDropIndicatorShown(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    DragDropEventFilter::install(this);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void DropTreeView::updateGeometries() {
    QTreeView::updateGeometries();
    if (m_bottomMargin > 0 && verticalScrollBar()) {
        QScrollBar* bar = verticalScrollBar();
        bar->setRange(bar->minimum(), bar->maximum() + m_bottomMargin);
    }
}
=======
void DropTreeView::updateGeometries() {
    QTreeView::updateGeometries();
    if (m_bottomMargin > 0 && verticalScrollBar()) {
        QScrollBar* bar = verticalScrollBar();
        if (bar->maximum() > 0) {
            int extra = (verticalScrollMode() == QAbstractItemView::ScrollPerPixel)
                        ? m_bottomMargin
                        : qMax(1, m_bottomMargin / 28);
            bar->setRange(bar->minimum(), bar->maximum() + extra);
        }
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Open FavoritePanel with few items (e.g. 5 items). Verify scrollbar is disabled (`maximum == 0`), items stay at top, and natural whitespace fills the bottom without forcing scroll.
2. Add many items/expand categories until content exceeds viewport height.
3. Scroll to the bottom and verify exactly 3 rows (~84px) of blank space appear past the last item.
4. Right-click on that 3-row blank space and verify the blank space context menu pops up.
