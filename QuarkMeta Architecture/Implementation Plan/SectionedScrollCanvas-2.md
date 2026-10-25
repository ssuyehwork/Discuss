# Implementation Plan - SectionedScrollCanvas Layout Align & Pure-Folder View MinHeight Fix

## Overview
This implementation plan addresses the layout stretching and min-height behavior in `SectionedScrollCanvas` (List View and Grid View) when navigating to directories containing only folders (`folderCount > 0 && fileCount == 0`).

### Root Causes
1. **Circular Formula Avoidance**: In `SectionedScrollCanvas::updateSectionCounts()`, when `fileCount == 0`, `folderView` must be stretched using `m_panel->folderViewMinHeight()`, rather than `m_panel->fileViewMinHeight()`.
2. **Formula Integrity**: `m_panel->folderViewMinHeight()` calculates $H_{\text{viewport}} - H_{\text{folderHeader}}$ without subtracting `folderView`'s own height, preventing circular math.

## Modified Files List
- `src/ui/SectionedScrollCanvas.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/SectionedScrollCanvas.cpp`
In `SectionedScrollCanvas::updateSectionCounts()`, use `m_panel->folderViewMinHeight()` to stretch `folderView` when `fileCount == 0`.

<<<<<<< SEARCH
    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                folderView->setFixedHeight(fjv->totalHeight());
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            folderView->setFixedHeight(folderCount * rowH + hdrH + 2);
            folderView->updateGeometry();
        }
    }
=======
    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                int contentH = fjv->totalHeight();
                int minH = (fileCount == 0) ? m_panel->folderViewMinHeight() : 0;
                folderView->setFixedHeight(qMax(contentH, minH));
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int contentH = folderCount * rowH + hdrH + 2;
            int minH = (fileCount == 0) ? m_panel->folderViewMinHeight() : 0;
            folderView->setFixedHeight(qMax(contentH, minH));
            folderView->updateGeometry();
        }
    }
>>>>>>> REPLACE

---

## Build & Verification Steps
1. Perform CMake configuration and build using standard MSVC/Qt toolchain.
2. Switch to Grid View or List View.
3. Navigate to a folder containing folders only (`folderCount > 0 && fileCount == 0`).
4. Verify that `folderView` stretches to fill the viewport height without collapsing or leaving empty gaps.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `m_panel->folderViewMinHeight()` from `DualSectionPanel` without duplicating min-height calculations.

## Header API Signature Verification
- `DualSectionPanel::folderViewMinHeight()` -> `int folderViewMinHeight() const`
