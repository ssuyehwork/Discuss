# Implementation Plan - SectionedScrollCanvas Smooth Performance & Layout Optimization

## Overview
This implementation plan optimizes `SectionedScrollCanvas` and `DualSectionPanel` to eliminate scrolling friction and layout vibration when rendering large file lists or filtered results.

### Problems Solved:
1. **Scrolling Friction & Layout Invalidation Loop**: Prevents `setFixedHeight` from being redundantly called during scrolling when heights haven't changed.
2. **Layout Structural Alignment**: Ensures `DualSectionPanel` layouts maintain clean alignment without triggering layout recalculation during scrolling.

---

## Modified Files List
- `src/ui/SectionedScrollCanvas.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/ui/SectionedScrollCanvas.cpp`

```
<<<<<<< SEARCH
    if (fileView && fileCount > 0) {
        if (m_type == CanvasType::Grid) {
            if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
                fileView->setFixedHeight(qMax(jv->totalHeight(), m_panel->fileViewMinHeight()));
            }
        } else {
            auto* tv = static_cast<QTreeView*>(fileView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            fileView->setFixedHeight(qMax(fileCount * rowH + hdrH + 2, m_panel->fileViewMinHeight()));
            fileView->updateGeometry();
        }
    }
=======
    if (fileView && fileCount > 0) {
        if (m_type == CanvasType::Grid) {
            if (auto* jv = qobject_cast<JustifiedView*>(fileView)) {
                int targetH = qMax(jv->totalHeight(), m_panel->fileViewMinHeight());
                if (fileView->height() != targetH) {
                    fileView->setFixedHeight(targetH);
                }
            }
        } else {
            auto* tv = static_cast<QTreeView*>(fileView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int targetH = qMax(fileCount * rowH + hdrH + 2, m_panel->fileViewMinHeight());
            if (fileView->height() != targetH) {
                fileView->setFixedHeight(targetH);
                fileView->updateGeometry();
            }
        }
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### Build Command:
```bash
cmake --build build --config Release
```

### Verification Methods:
1. Open a directory containing 2000+ files.
2. Rapidly drag the scrollbar up and down to verify smooth (60 FPS) scrolling performance.
3. Filter down to 1-2 items using `FilterPanel` to confirm compact top alignment.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **Reused Official Entrance**: `m_panel->refreshVisibleThumbnails(...)` is reused via `m_scrollThumbTimer` (60ms debounce) during scroll.
- **No Duplicate Layouts**: Avoids unneeded `setFixedHeight` calls when the target height has not changed.

---

## Header API Signature Verification
- `DualSectionPanel::fileViewMinHeight()` -> `int fileViewMinHeight() const` in `DualSectionPanel.h`
- `JustifiedView::totalHeight()` -> `int totalHeight() const` in `JustifiedView.h`
