# Pure Disk Mode Memory Remnants Purge Plan (Iterative Version 2)

## Overview
According to `QuarkMeta-Architecture-Planning.md`, QuarkMeta operates strictly in pure disk directory direct-connect mode. All file and folder metadata must be written exclusively to discrete `.QuarkMeta.json` files via `QuarkMetaJson`.

Following iterative plan `memory-1.md`, this plan (`memory-2.md`) focuses strictly on **newly identified, unremoved** memory-mode remnants in MetaPanel UI and DuplicateDetectorService:

1. **`MetaPanel` Category Pill Remnants**: Clean removal of legacy `setCategoryPills`, `bindCategoryRequested`, and `unbindCategoryRequested` declarations and implementations in `MetaPanel.h` and `MetaPanel.cpp`.
2. **`DuplicateDetectorService` Legacy `folderId` Assignment**: Removal of legacy `folderId` string assignment in `DuplicateDetectorService.cpp` and `DuplicateDetectorService.h`.

---

## Modified Files List
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`
- `src/meta/DuplicateDetectorService.h`
- `src/meta/DuplicateDetectorService.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/MetaPanel.h`
Remove legacy category binding and pill display method/signal declarations.

```
<<<<<<< SEARCH
    void setCategoryPills(const std::vector<std::pair<int, QString>>& categories);
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void unbindCategoryRequested(const QString& path, int categoryId);
    void bindCategoryRequested(const QString& path);
=======
>>>>>>> REPLACE
```

---

### 2. `src/ui/MetaPanel.cpp`
Remove legacy category pill UI rendering and event emitting logic.

```
<<<<<<< SEARCH
                emit unbindCategoryRequested(m_selectedPaths.first(), catId);
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            emit bindCategoryRequested(m_selectedPaths.first());
=======
>>>>>>> REPLACE
```

---

### 3. `src/meta/DuplicateDetectorService.h`
Remove legacy `folderId` member variable.

```
<<<<<<< SEARCH
    QString folderId;
=======
>>>>>>> REPLACE
```

---

### 4. `src/meta/DuplicateDetectorService.cpp`
Remove legacy `folderId` assignment statements.

```
<<<<<<< SEARCH
        group.existingItem.folderId = QString::fromStdString(meta.folderId);
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        group.existingItem.folderId = QString::fromStdString(meta.folderId);
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
- [x] Newly discovered memory-mode remnants in MetaPanel and DuplicateDetectorService are cleanly specified.
- [x] Does not duplicate items in previous plans (`memory.md` / `memory-1.md`).
- [x] All Search/Replace Git Merge Diff blocks match verbatim context in source files.
