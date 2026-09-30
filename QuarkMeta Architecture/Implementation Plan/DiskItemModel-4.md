# Implementation Plan - DiskItemModel-4

## Overview
This implementation plan connects `DiskItemModel::incrementGeneration()` to `ThumbnailPipelineService::instance().cancelAll()` to guarantee instant cancellation of old thumbnail tasks and background processes when navigating, switching folders, or clearing items.

## Modified Files List
- `src/ui/models/DiskItemModel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/models/DiskItemModel.cpp`

```
<<<<<<< SEARCH
void DiskItemModel::incrementGeneration() {
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
}
=======
void DiskItemModel::incrementGeneration() {
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
    ThumbnailPipelineService::instance().cancelAll();
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile `src/ui/models/DiskItemModel.cpp`.
2. Verify that navigating between directories triggers `cancelAll()` and aborts all queued and running background extraction tasks.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `ThumbnailPipelineService::instance().cancelAll()` without creating separate cancellation logic.

## Header API Signature Verification
- `ThumbnailPipelineService::cancelAll()` -> exact signature in `ThumbnailPipelineService.h`.

## Header Inclusion Chain Check
- Verify `DiskItemModel.cpp` includes `"ThumbnailPipelineService.h"`.
