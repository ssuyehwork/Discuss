# Implementation Plan - DualSectionPanel Viewport Sampling Repair (Restore Version-Old-8 Bottom-Right Sampling Spec)

## Overview
This implementation plan fixes a severe viewport thumbnail lazy-loading calculation bug inside `DualSectionPanel::refreshVisibleThumbnails`.

### Root Cause
In previous refactoring, `scanView` inside `DualSectionPanel::refreshVisibleThumbnails` sampled viewport bottom using a fixed X coordinate `QPoint(10, clampedBtmY)` instead of mapping the viewport's bottom-right corner (`vpRect.bottomRight()`).

In grid/waterfall layouts (`JustifiedView` / GridMode), items are arranged horizontally across the screen (e.g. 14 items per row). Sampling at `x=10` only hit the first item in the row (e.g. Row 0). As a result, `btmIdx.row()` returned Row 0 instead of the rightmost item (Row 13), causing the calculation algorithm to falsely discard items on the right side of the screen (e.g. 8 `.ai` files in the same visible row). The background `ThumbnailPipelineService` never received fetch requests for these visible items, resulting in missing thumbnails in the UI.

### Fix
Restore the `Version-Old-8` spec by mapping `hostViewport->mapToGlobal(vpRect.bottomRight())` to child views and clamping both X and Y coordinates within child view boundaries `[0, width() - 1]` and `[0, height() - 1]`.

## Modified Files List
- `src/ui/DualSectionPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/DualSectionPanel.cpp`
In `DualSectionPanel::refreshVisibleThumbnails`, update `scanView` sampling logic:
1. Map `vpRect.topLeft()` to `topPoint` and clamp `(clampedTopX, clampedTopY)` to `[0, view->width() - 1]` and `[0, view->height() - 1]`.
2. Map `vpRect.bottomRight()` to `btmPoint` and clamp `(clampedBtmX, clampedBtmY)` to `[0, view->width() - 1]` and `[0, view->height() - 1]`.
3. Perform `indexAt` sampling at `QPoint(clampedTopX, clampedTopY)` for `topIdx` and `QPoint(clampedBtmX, clampedBtmY)` for `btmIdx`.

<<<<<<< SEARCH
        QPoint topPoint = view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.topLeft()));
        QPoint btmPoint = view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.bottomRight()));

        if (topPoint.y() >= view->height() || btmPoint.y() <= 0) return;

        int clampedTopY = qBound(0, topPoint.y(), view->height());
        int clampedBtmY = qBound(0, btmPoint.y(), view->height());

        QModelIndex topIdx = view->indexAt(QPoint(10, clampedTopY));
        if (!topIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !topIdx.isValid(); offset += 10)
                topIdx = view->indexAt(QPoint(10, clampedTopY + offset));
        }
        QModelIndex btmIdx = view->indexAt(QPoint(10, clampedBtmY));
        if (!btmIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !btmIdx.isValid(); offset += 10)
                btmIdx = view->indexAt(QPoint(10, clampedBtmY - offset));
        }
=======
        QPoint topPoint = view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.topLeft()));
        QPoint btmPoint = view->mapFromGlobal(hostViewport->mapToGlobal(vpRect.bottomRight()));

        if (topPoint.y() >= view->height() || btmPoint.y() <= 0) return;

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
>>>>>>> REPLACE

## Build & Verification Steps
1. Rebuild application using CMake and MSVC compiler.
2. Open a folder containing multiple files arranged across a wide Grid view (e.g. 10~14 items in a single row).
3. Check debug log output for `refreshVisibleThumbnails calculated visible source rows`.
4. Confirm that all items on the right side of the row are correctly included in `visibleRows`.
5. Verify that thumbnails for items on the far right render immediately without missing or staying on extension badge placeholders.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses existing `refreshVisibleThumbnails(model, hostViewport)` contract without creating duplicate refresh loops.

## Header API Signature Verification
- `DualSectionPanel::refreshVisibleThumbnails` -> `void refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport)`
