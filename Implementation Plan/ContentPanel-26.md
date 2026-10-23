# Implementation Plan - Fix Drag Overlay Z-Order Stacking in ContentPanel

## 1. Overview
When dragging a tab or file over `ContentPanel` to trigger split view (top, bottom, left, right 25% margins), `m_dragOverlayWidget` is shown to visually represent the split region.
Currently, when hovering over the top 25% boundary, `m_dragOverlayWidget->show()` is called, but `m_dragOverlayWidget->raise()` is not called. As a result, the blue overlay is rendered behind higher z-order child widgets (such as `m_headerWidget` and `m_viewStack`), making the top overlay appear invisible or covered.
Adding `m_dragOverlayWidget->raise()` whenever the overlay is shown ensures the semi-transparent drag overlay is always brought to the absolute front of `ContentPanel`.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File: `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    if (pos.x() > w * 0.75) {
        m_dragOverlayWidget->setGeometry(w / 2, 0, w / 2, h);
        m_dragOverlayWidget->show();
    } else if (pos.x() < w * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w / 2, h);
        m_dragOverlayWidget->show();
    } else if (pos.y() > h * 0.75) {
        m_dragOverlayWidget->setGeometry(0, h / 2, w, h / 2);
        m_dragOverlayWidget->show();
    } else if (pos.y() < h * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w, h / 2);
        m_dragOverlayWidget->show();
    } else {
        hideDragOverlay();
    }
=======
    if (pos.x() > w * 0.75) {
        m_dragOverlayWidget->setGeometry(w / 2, 0, w / 2, h);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.x() < w * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w / 2, h);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.y() > h * 0.75) {
        m_dragOverlayWidget->setGeometry(0, h / 2, w, h / 2);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else if (pos.y() < h * 0.25) {
        m_dragOverlayWidget->setGeometry(0, 0, w, h / 2);
        m_dragOverlayWidget->show();
        m_dragOverlayWidget->raise();
    } else {
        hideDragOverlay();
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Verify that `m_dragOverlayWidget->raise()` is invoked in all four edge conditions (left, right, top, bottom).
2. Confirm that dragging over top 25% margin displays the blue overlay clearly on top of header and canvas views.
