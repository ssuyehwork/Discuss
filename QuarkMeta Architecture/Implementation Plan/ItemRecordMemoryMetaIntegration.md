# ItemRecordMemoryMetaIntegration Implementation Plan

This implementation plan details the precise changes required to ensure all created `ItemRecord` instances automatically populate their metadata directly from `MetadataManager` (SSOT in-memory cache) upon creation.

## Overview
When `DiskScanService` or `ContentDataLoader` scans a directory, it creates `ItemRecord` instances using `ItemRecord::create(path, nullptr)`. Previously, `ItemRecord::create` only fetched Win32 API file system attributes (size, times), leaving metadata fields (`rating`, `manualColor`, `tags`, `note`, `url`, `pinned`, `encrypted`) blank.

For Column View (Miller Columns), which loads directory records using `DiskScanService::scanDirectory` into isolated private models, this resulted in all items displaying with empty metadata (no rating stars, no color tags).

This plan modifies `ItemRecord::create` in `ItemRecord.cpp` to automatically query `MetadataManager::instance().getMeta(wPath)` and invoke `fromMetadata` to populate all in-memory metadata fields.

---

## Modified Files List
- `src/core/ItemRecord.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/core/ItemRecord.cpp`

In `ItemRecord::create`, when `providedMeta` is `nullptr`, `meta` is automatically fetched from `MetadataManager::instance().getMeta(wPath)`, and `fromMetadata(r, meta)` is called to populate metadata fields.

---

## Build & Verification Steps

1. **CMake Build Verification**:
   ```bash
   cmake --build build --config Debug
   ```
2. **Functional Verification**:
   - Switch application to Column View mode (Miller Columns).
   - Navigate to a directory containing files with existing metadata (e.g. rating stars, color tags, or tags).
   - Verify that:
     - All column view items display their rating stars and color tags immediately upon opening the column, populated directly from `MetadataManager` memory SSOT.
     - Rating stars and color tags render consistently across Grid View, List View, Justified View, and Column View without any missing data.
