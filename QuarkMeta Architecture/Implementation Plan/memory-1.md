# Pure Disk Mode Memory Remnants Purge Plan (Iterative Version 1)

## Overview
According to `QuarkMeta-Architecture-Planning.md`, QuarkMeta operates strictly in pure disk directory direct-connect mode. All file and folder metadata must be written exclusively to discrete `.QuarkMeta.json` files via `QuarkMetaJson`.

This iterative implementation plan (`memory-1.md`) focuses exclusively on **newly identified, unremoved** memory-mode remnants in `StatisticsService`, specifically purging semi-static library counts (`libraryCounts`) and dynamic user category counts (`userCategoryCounts`):

1. **`StatisticsService.h`**: Clean removal of `libraryCounts` (`QMap<int, int>`) and `userCategoryCounts` (`QMap<int, int>`) map containers.
2. **`StatisticsService.cpp`**: Elimination of `libraryCounts` and `userCategoryCounts` incrementing, decrementing, and checking logic during asset operations.

---

## Modified Files List
- `src/meta/StatisticsService.h`
- `src/meta/StatisticsService.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/StatisticsService.h`
Remove legacy memory-mode category counting containers `libraryCounts` and `userCategoryCounts`.

```
<<<<<<< SEARCH
    // 2. 半静态托管库计数 (categoryId -> count)
    QMap<int, int> libraryCounts;
    // 3. 全动态用户分类计数 (categoryId -> count)
    QMap<int, int> userCategoryCounts;
=======
>>>>>>> REPLACE
```

---

### 2. `src/meta/StatisticsService.cpp`
Remove category count incrementing logic in `notifyAssetAdded`.

```
<<<<<<< SEARCH
    if (targetCatId > 0) {
        m_cachedSnapshot.userCategoryCounts[targetCatId]++;
    }
=======
>>>>>>> REPLACE
```

Remove legacy library and user category count decrementing logic in `purgeAsset`.

```
<<<<<<< SEARCH
    // 1. 托管库分类扣减
    if (libraryCatId > 0 && m_cachedSnapshot.libraryCounts.contains(libraryCatId)) {
        if (m_cachedSnapshot.libraryCounts[libraryCatId] > 0) {
            m_cachedSnapshot.libraryCounts[libraryCatId]--;
        }
    }

    // 2. 所有挂载过的用户分类全量扣减
    for (int userCatId : userCatIds) {
        if (m_cachedSnapshot.userCategoryCounts.contains(userCatId) && m_cachedSnapshot.userCategoryCounts[userCatId] > 0) {
            m_cachedSnapshot.userCategoryCounts[userCatId]--;
        }
    }
=======
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. Build Verification
Verify compilation:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 2. Verification Checklist
- [x] Semi-static library counts (`libraryCounts`) and user category counts (`userCategoryCounts`) are cleanly specified for removal.
- [x] Only newly discovered, unremoved memory-mode remnants are included in this iterative plan.
- [x] All Search/Replace Git Merge Diff blocks match verbatim context in `StatisticsService.h` and `StatisticsService.cpp`.
