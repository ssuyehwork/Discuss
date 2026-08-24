# Implementation Plan - MetadataManager & QuarkMetaJson Legacy Purge

## Overview
Purge legacy in-memory directory tree (`m_parentToChildren`, `hasChildrenInCache`, `getChildrenFromCache`), dual-track trash methods (`markAsTrash`, `setTrash`, `deletePermanently`, `RuntimeMeta.isTrash`), obsolete JSON persistence keys (`file_id_128`, `encrypt_salt`, `encrypt_iv`, `encrypt_verify_hash`), and unused headers in `MetadataManager.cpp`.

## Modified Files List
- `src/meta/MetadataManager.h`
- `src/meta/MetadataManager.cpp`
- `src/meta/QuarkMetaJson.cpp`
- `src/meta/StatisticsService.h`
- `src/meta/StatisticsService.cpp`
- `src/meta/DuplicateDetectorService.cpp`
- `src/core/CoreEngine.cpp`
- `src/util/DiskIoService.h`

## Detailed Line-by-Line Changes

### 1. `src/meta/MetadataManager.h`
- Remove `hasChildrenInCache`, `getChildrenFromCache` method declarations.
- Remove `markAsTrash`, `setTrash`, `deletePermanently` method declarations.
- Remove `m_parentToChildren` member declaration.
- Remove `isTrash` field from `RuntimeMeta` struct.

### 2. `src/meta/MetadataManager.cpp`
- Remove unused headers (`<QCryptographicHash>`, `<QRegularExpression>`, `<QImageReader>`, `<QSvgRenderer>`).
- Remove implementations of `hasChildrenInCache`, `getChildrenFromCache`, `markAsTrash`, `setTrash`, `deletePermanently`.
- Remove references to `m_parentToChildren` and `meta.isTrash`.

### 3. `src/meta/QuarkMetaJson.cpp`
- Remove `file_id_128`, `encrypt_salt`, `encrypt_iv`, `encrypt_verify_hash` serialization & deserialization.

### 4. Other Files
- Update call-sites in `CoreEngine.cpp`, `DiskIoService.h`, `DuplicateDetectorService.cpp`, `StatisticsService.cpp` where `deletePermanently`, `isTrash`, or `notifyAssetTrashChanged` were used.

## Build & Verification Steps
1. Verify code changes across modified files.
2. Ensure full project consistency.
