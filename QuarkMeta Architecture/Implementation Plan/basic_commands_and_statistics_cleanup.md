# Implementation Plan - BasicCommands & StatisticsService Cleanup

## Overview
Purge orphan member (`m_isCapsule`), ghost fields (`m_categoryCounts`, `m_tagCounts`), legacy category tree parameters (`targetCatId`, `userCatIds`), pseudo-loop over single-element vector in `StatisticsService.cpp`, and redundant includes in `BasicCommands.h` and `CoreEngine.cpp`.

## Modified Files List
- `src/core/BasicCommands.h`
- `src/core/CoreEngine.cpp`
- `src/meta/StatisticsService.h`
- `src/meta/StatisticsService.cpp`
- `src/meta/MetadataManager.cpp`

## Detailed Line-by-Line Changes

### 1. `src/core/BasicCommands.h`
- Remove unused includes `#include "../meta/DatabaseManager.h"` and `#include "sqlite3.h"`.
- Remove orphan member `bool m_isCapsule;` in `BatchRenameCommand`.

### 2. `src/core/CoreEngine.cpp`
- Remove redundant includes `#include "../meta/QuarkMetaJson.h"` and `#include "../util/ShellHelper.h"`.

### 3. `src/meta/StatisticsService.h` & `src/meta/StatisticsService.cpp`
- Remove ghost fields `m_categoryCounts` and `m_tagCounts`.
- Simplify `notifyAssetAdded(bool hasTags)`, `notifyAssetRemoved(bool hadTags, bool wasTrash)`, and `purgeAsset(bool hasTags)`.
- Replace single-element vector iteration `std::vector<sqlite3*> dbs = { DatabaseManager::instance().getGlobalDb() }; for (sqlite3* db : dbs)` with direct usage of `DatabaseManager::instance().getGlobalDb()`.

### 4. `src/meta/MetadataManager.cpp`
- Update `purgeAsset` call-sites to match simplified signature.

## Build & Verification Steps
1. Verify code changes across modified files.
2. Ensure full project consistency.
