# Implementation Plan - Restoring JustifiedView 10px Standard Margin Spec & Aligning Viewport Sampling

## Overview
This implementation plan restores the user-specified **`10px`** standard physical margin inside `JustifiedView.cpp` and aligns `DualSectionPanel::refreshVisibleThumbnails` viewport sampling coordinates accordingly.

### Root Cause
In previous revisions, the card layout margin in `JustifiedView.cpp` was improperly changed from the user-specified `10px` down to `6px` (`const int margin = 6;`). This parameter alteration caused cards to sit uncomfortably close to the left boundary of the content panel and, on high-DPI viewports, caused card covers to be clipped along the left edge.

To bypass `indexAt(QPoint(0, Y))` returning null on the `6px` margin, ad-hoc fallback logic like `QPoint(10, Y)` was introduced, introducing secondary bugs during grid row sampling.

### Fix
1. Restore `const int margin = 10;` inside `JustifiedView.cpp` to strictly enforce the user-specified 10px card margin.
2. Align `DualSectionPanel::refreshVisibleThumbnails` sampling bounds with the restored `10px` margin.

## Modified Files List
- `src/ui/JustifiedView.cpp`
- `src/ui/DualSectionPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/JustifiedView.cpp`
Restore `margin` constant from `6` to `10`.

<<<<<<< SEARCH
    const int margin = 6;
    const int spacing = 5;
=======
    const int margin = 10;
    const int spacing = 5;
>>>>>>> REPLACE

---

### 2. `src/ui/DualSectionPanel.cpp`
Align viewport sampling bounds with the 10px margin.

<<<<<<< SEARCH
        int clampedTopX = qBound(16, topPoint.x(), view->width() - 1);
        int clampedTopY = qBound(0, topPoint.y(), view->height() - 1);

        int clampedBtmX = qBound(0, btmPoint.x(), qMax(0, view->width() - 16));
        int clampedBtmY = qBound(0, btmPoint.y(), view->height() - 1);
=======
        int clampedTopX = qBound(12, topPoint.x(), view->width() - 1);
        int clampedTopY = qBound(0, topPoint.y(), view->height() - 1);

        int clampedBtmX = qBound(0, btmPoint.x(), qMax(0, view->width() - 12));
        int clampedBtmY = qBound(0, btmPoint.y(), view->height() - 1);
>>>>>>> REPLACE

## Build & Verification Steps
1. Rebuild application using CMake and MSVC compiler:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. Launch application in GridView / JustifiedView mode.
3. Verify that cards sit with a clean, comfortable 10px margin from the left edge of the content panel without left-edge clipping or truncated card covers.
4. Verify that viewport thumbnail lazy loading operates without missing items across rows.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `JustifiedView::doLayout()` and `DualSectionPanel::refreshVisibleThumbnails()` without introducing redundant layout passes.

## Header API Signature Verification
- `JustifiedView::doLayout()` -> `src/ui/JustifiedView.h`
- `DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport)` -> `src/ui/DualSectionPanel.h`
