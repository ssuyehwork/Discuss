# Implementation Plan - DualSectionPanel Ultra-Smooth Viewport Sampling Fix (Eliminate Y-Offset Shift)

## Overview
This implementation plan eliminates the viewport scroll lag caused by Y-axis offset shifting in `DualSectionPanel::refreshVisibleThumbnails`.

### Root Cause
In previous sampling logic, when `indexAt(clampedTopX, clampedTopY)` failed due to landing on the 6px view edge margin (`x < 6`), the fallback offset loop incremented both X and Y simultaneously: `QPoint(clampedTopX + offset, clampedTopY + offset)`.

This pushed the sampling point up to 100 pixels DOWNWARD into the viewport. Consequently, items in the top row(s) of the viewport were miscalculated as "out of visible range", causing missing thumbnail fetch requests and visible loading delay during scrolling.

### Fix
1. Clamp `clampedTopX` to start at `16px` (safely past the 6px margin) so `indexAt` immediately hits the top-left item without entering fallback loops.
2. In fallback loops, only offset along the `X` axis (`clampedTopX + offset, clampedTopY`), ensuring `Y` stays strictly pinned to the viewport top/bottom edge without shifting down.

## Modified Files List
- `src/ui/DualSectionPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/DualSectionPanel.cpp`
In `DualSectionPanel::refreshVisibleThumbnails`:

<<<<<<< SEARCH
        int clampedTopX = qBound(0, topPoint.x(), view->width() - 1);
        int clampedTopY = qBound(0, topPoint.y(), view->height() - 1);

        int clampedBtmX = qBound(0, btmPoint.x(), view->width() - 1);
        int clampedBtmY = qBound(0, btmPoint.y(), view->height() - 1);

        QModelIndex topIdx = view->indexAt(QPoint(clampedTopX, clampedTopY));
        if (!topIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !topIdx.isValid(); offset += 10)
                topIdx = view->indexAt(QPoint(clampedTopX + offset, clampedTopY + offset));
        }
        QModelIndex btmIdx = view->indexAt(QPoint(clampedBtmX, clampedBtmY));
        if (!btmIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !btmIdx.isValid(); offset += 10)
                btmIdx = view->indexAt(QPoint(clampedBtmX - offset, clampedBtmY - offset));
        }
=======
        int clampedTopX = qBound(16, topPoint.x(), view->width() - 1);
        int clampedTopY = qBound(0, topPoint.y(), view->height() - 1);

        int clampedBtmX = qBound(0, btmPoint.x(), qMax(0, view->width() - 16));
        int clampedBtmY = qBound(0, btmPoint.y(), view->height() - 1);

        QModelIndex topIdx = view->indexAt(QPoint(clampedTopX, clampedTopY));
        if (!topIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !topIdx.isValid(); offset += 10)
                topIdx = view->indexAt(QPoint(qMin(view->width() - 1, clampedTopX + offset), clampedTopY));
        }
        QModelIndex btmIdx = view->indexAt(QPoint(clampedBtmX, clampedBtmY));
        if (!btmIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !btmIdx.isValid(); offset += 10)
                btmIdx = view->indexAt(QPoint(qMax(0, clampedBtmX - offset), clampedBtmY));
        }
>>>>>>> REPLACE

## Build & Verification Steps
1. Rebuild application using CMake and MSVC compiler.
2. Scroll through a grid view containing multiple rows of cards.
3. Observe that items at the top-left edge of the viewport render immediately without lag.
4. Verify debug log `refreshVisibleThumbnails calculated visible source rows` includes the absolute top items starting from row 0 when scrolled to top.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses existing `refreshVisibleThumbnails` contract without creating redundant timer loops.

## Header API Signature Verification
- `DualSectionPanel::refreshVisibleThumbnails` -> `void refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport)`
