# Implementation Plan - DualSectionPanel Preload Buffer Expansion (100 Items Downward Pre-fetch)

## Overview
This implementation plan expands the thumbnail viewport lazy-loading buffer in `DualSectionPanel::refreshVisibleThumbnails` to pre-fetch 100 items ahead below the visible viewport.

### Technical Motivation
Previously, `refreshVisibleThumbnails` calculated visible bounds with a buffer of only 4 rows (`btmIdx.row() + 4`). When fast-scrolling through directories containing hundreds of images, the scrollbar would outpace the 4-row buffer, momentarily showing unrendered cards or extension badges.

By extending the downward pre-fetch buffer to 100 items (`btmIdx.row() + 100`), `ThumbnailPipelineService` retrieves read-only disk cache thumbnails well before they scroll into the visible viewport, achieving uninterrupted 60 FPS scrolling.

## Modified Files List
- `src/ui/DualSectionPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/DualSectionPanel.cpp`
In `DualSectionPanel::refreshVisibleThumbnails`, expand downward visible range from `+ 4` to `+ 100` and upward buffer from `- 4` to `- 20`:

<<<<<<< SEARCH
        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : qMin(proxy->rowCount() - 1, top + 20);
=======
        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 20) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 100) : qMin(proxy->rowCount() - 1, top + 100);
>>>>>>> REPLACE

## Build & Verification Steps
1. Rebuild application using CMake and MSVC compiler:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. Open a folder with 200+ images/vectors.
3. Fast-scroll down the viewport using mouse wheel or scrollbar drag.
4. Verify debug log `refreshVisibleThumbnails calculated visible source rows` shows 100+ rows queued for pre-fetching.
5. Observe that cards popping into view are pre-loaded without placeholders or white gaps.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses existing `refreshVisibleThumbnails` pipeline and `loadThumbnailsForRows` contract without introducing duplicate threads.

## Header API Signature Verification
- `DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport)` -> `src/ui/DualSectionPanel.h`
