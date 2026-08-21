# Pure Disk Mode Memory Remnants Purge Plan (Iterative Version 1)

## Overview
According to `QuarkMeta-Architecture-Planning.md`, QuarkMeta operates strictly in pure disk directory direct-connect mode. All file and folder metadata must be written exclusively to discrete `.QuarkMeta.json` files via `QuarkMetaJson`.

Previous implementation plans (such as `memory.md` and `memory_mode_purge.md`) covered `CategoryLockManager`, `NativeFolderWatcher`, `system_stats` progress keys, and `kSqlInsertMeta` bypasses. This iterative plan (`memory-1.md`) strictly focuses on **newly identified, previously unremoved** memory-mode remnants:

1. **`ItemRecord` Memory Fields Cleanup**: Removal of legacy `isCategory` and `folderId` fields from `ItemRecord.h`.
2. **`StatisticsService` Category Counts Cleanup**: Removal of `userCategoryCounts` map maintenance in `StatisticsService.h` and `StatisticsService.cpp`.
3. **`ContentPanel` Category Filter Remnants**: Removal of legacy `m_isCategoryRecursive` category-mode toggle logic in `ContentPanel.cpp`.

---

## Modified Files List
- `src/core/ItemRecord.h`
- `src/meta/StatisticsService.h`
- `src/meta/StatisticsService.cpp`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/core/ItemRecord.h`
Remove legacy memory-mode category card flag and Base36 folderId field.

```
<<<<<<< SEARCH
    bool isCategory = false;
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::string folderId;
=======
>>>>>>> REPLACE
```

---

### 2. `src/meta/StatisticsService.h`
Remove legacy category count container `userCategoryCounts` from `StatisticsSnapshot`.

```
<<<<<<< SEARCH
    QMap<int, int> userCategoryCounts;
=======
>>>>>>> REPLACE
```

---

### 3. `src/meta/StatisticsService.cpp`
Remove category count incrementing and decrementing logic.

```
<<<<<<< SEARCH
        m_cachedSnapshot.userCategoryCounts[targetCatId]++;
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (m_cachedSnapshot.userCategoryCounts.contains(userCatId) && m_cachedSnapshot.userCategoryCounts[userCatId] > 0) {
            m_cachedSnapshot.userCategoryCounts[userCatId]--;
        }
=======
>>>>>>> REPLACE
```

---

### 4. `src/ui/ContentPanel.cpp`
Remove category-mode recursive layer button sync logic.

```
<<<<<<< SEARCH
    bool isCategoryMode = (m_currentCategoryType == "user_category");
    m_btnLayers->setVisible(!isCategoryMode);
    m_btnLayersBlue->setVisible(isCategoryMode);

    if (isCategoryMode) {
        m_btnLayersBlue->setChecked(m_isCategoryRecursive);
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
- [x] Only newly discovered, unremoved memory-mode remnants are included in this plan.
- [x] Items already addressed in previous plans/commits are NOT repeated.
- [x] All Search/Replace Git Merge Diff blocks match verbatim context in the codebase.
