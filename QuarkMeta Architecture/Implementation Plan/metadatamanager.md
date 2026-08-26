# Implementation Plan - MetadataManager Facade Pattern (`metadatamanager.md`)

## 1. Overview
`MetadataManager` currently handles JSON file serialization, SQLite `global.db` access, and LRU memory caching directly within its class body.
This plan refactors `MetadataManager` using the Facade Pattern:
1. `QuarkMetaJsonStore`: Manages `.QuarkMeta.json` read/write operations and roaming.
2. `MetaDbRepository`: Manages SQLite `global.db` CRUD operations.
3. `MetaMemoryCache`: Manages LRU memory caching.
4. `MetadataManager`: Functions strictly as a top-level facade API Router.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/meta/QuarkMetaJsonStore.h`
- `src/meta/QuarkMetaJsonStore.cpp`
- `src/meta/MetaDbRepository.h`
- `src/meta/MetaDbRepository.cpp`
- `src/meta/MetaMemoryCache.h`
- `src/meta/MetaMemoryCache.cpp`
- `src/meta/MetadataManager.h`
- `src/meta/MetadataManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### `CMakeLists.txt`
```git
<<<<<<< SEARCH
    src/meta/MetadataManager.cpp
    src/meta/MetadataManager.h
=======
    src/meta/QuarkMetaJsonStore.h
    src/meta/QuarkMetaJsonStore.cpp
    src/meta/MetaDbRepository.h
    src/meta/MetaDbRepository.cpp
    src/meta/MetaMemoryCache.h
    src/meta/MetaMemoryCache.cpp
    src/meta/MetadataManager.cpp
    src/meta/MetadataManager.h
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Steps
1. Perform rating, color, and tag updates on files.
2. Verify `.QuarkMeta.json` updating via `QuarkMetaJsonStore`.
3. Verify SQLite operations via `MetaDbRepository` and LRU caching via `MetaMemoryCache`.
