# Pure Disk Mode Memory Remnants Purge Plan (Iterative Version 1)

## Overview
According to `QuarkMeta-Architecture-Planning.md`, QuarkMeta operates strictly in pure disk directory direct-connect mode. All file and folder metadata must be written exclusively to discrete `.QuarkMeta.json` files via `QuarkMetaJson`.

This iterative implementation plan (`memory-1.md`) focuses on newly identified, unremoved memory-mode remnants in data models and snapshot engines:

1. **`ItemRecord` Memory Fields Cleanup**: Removal of legacy `isCategory` and `folderId` fields from `ItemRecord.h` and `ItemRecord.cpp`.
2. **`ModelContract` Category Roles Cleanup**: Removal of `CategoryIdRole` and `CategoryKindRole` from `ModelContract.h`.
3. **`OperationSnapshotEngine` Category Enum Cleanup**: Removal of `AssignToCategory` action enum and `primaryCategoryCatId` field in `OperationSnapshotEngine.h`.
4. **`ContentPanel` Category Mode Remnants**: Clean removal of legacy `m_isCategoryRecursive` toggle and `isCategoryMode` layer button sync logic in `ContentPanel.cpp`.

---

## Modified Files List
- `src/core/ItemRecord.h`
- `src/core/ItemRecord.cpp`
- `src/core/ModelContract.h`
- `src/core/OperationSnapshotEngine.h`
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

### 2. `src/core/ItemRecord.cpp`
Remove folderId assignment logic in ItemRecord mapping.

```
<<<<<<< SEARCH
    if (!meta.folderId.empty()) {
        r.folderId = meta.folderId;
    }
=======
>>>>>>> REPLACE
```

---

### 3. `src/core/ModelContract.h`
Remove legacy category roles from ModelContract enum.

```
<<<<<<< SEARCH
    CategoryIdRole      = Qt::UserRole + 107, // 所属分类 ID
    CategoryKindRole    = Qt::UserRole + 110, // 分类类型 (0=User, 1=SystemLibrary)
=======
>>>>>>> REPLACE
```

---

### 4. `src/core/OperationSnapshotEngine.h`
Remove legacy category action type and category ID tracking field.

```
<<<<<<< SEARCH
    AssignToCategory   // 归类到...
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    int primaryCategoryCatId = 0; // 主分类 ID
=======
>>>>>>> REPLACE
```

---

### 5. `src/ui/ContentPanel.cpp`
Remove category-mode layer button toggling logic in `ContentPanel.cpp`.

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
- [x] Newly discovered memory-mode remnants in data models, contracts, and snapshot engines are cleanly specified.
- [x] Does not duplicate items already removed or covered in previous commits/plans.
- [x] All Search/Replace Git Merge Diff blocks match verbatim context in source files.
