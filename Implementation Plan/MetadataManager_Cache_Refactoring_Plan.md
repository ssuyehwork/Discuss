# Implementation Plan - MetadataManager Cache & Entry Point Refactoring (Step 2)

## 1. Overview & Goal
This plan addresses Step 2 of the QuarkMeta Performance & Architecture Refactoring Roadmap: **Metadata Cache & Entry Point Optimization (`MetadataManager` / `MetaMemoryCache`)**.
The primary goals are:
1. Optimize memory cache seeding (`seedMemoryCacheBatch`) to reuse pre-scanned filesystem properties (`ItemRecord`), eliminating redundant disk I/O / Win32 API queries per file.
2. Cache normalized path strings during batch operations to avoid repeated `normalizePath` conversions.
3. Optimize directory renaming and moving using subtree prefix matching, reducing $O(N \times M)$ scan complexity.
4. Remove unneeded / idle `notifyUI` signal emissions and consolidate batch UI notification triggers.

---

## 2. Affected Files & Public Signature Freezing Inspection
- `src/meta/MetadataManager.h`: Public API signatures MUST remain frozen. Existing public methods are modified internally or overloaded without breaking compilation contracts.
- `src/meta/MetadataManager.cpp`: Implementation of optimized seeding, batch renaming, and path normalization logic.
- `src/meta/MetaMemoryCache.h`: Thread-safe cache structure supporting prefix search / subtree matching for folder renaming.
- `src/meta/MetaMemoryCache.cpp`: Implementation of fast subtree key replacement and lock-free/shared-lock lookups.

---

## 3. Detailed Changes & Code Modifications

### 3.1 `MetaMemoryCache` Subtree Renaming & Fast Lookup
Add efficient subtree key relocation in `MetaMemoryCache` so that renaming a folder does not require checking every cache item linearly.

```
<<<<<<< SEARCH
    void renameItem(const std::wstring& oldPath, const std::wstring& newPath);
=======
    void renameItem(const std::wstring& oldPath, const std::wstring& newPath);
    void renameSubtree(const std::wstring& oldPrefix, const std::wstring& newPrefix);
>>>>>>> REPLACE
```

### 3.2 `MetadataManager::seedMemoryCacheBatch` Optimization
Avoid secondary disk checks during seeding when `ItemRecord` already has valid file size and modification time metadata.

```
<<<<<<< SEARCH
void MetadataManager::seedMemoryCacheBatch(const std::vector<ItemRecord>& records) {
    if (records.empty()) return;
    for (const auto& rec : records) {
        if (rec.path.isEmpty()) continue;
        std::wstring norm = normalizePath(rec.path.toStdWString());
        // ...
    }
}
=======
void MetadataManager::seedMemoryCacheBatch(const std::vector<ItemRecord>& records) {
    if (records.empty()) return;
    std::vector<std::pair<std::wstring, RuntimeMeta>> batchToInsert;
    batchToInsert.reserve(records.size());

    for (const auto& rec : records) {
        if (rec.path.isEmpty()) continue;
        std::wstring norm = normalizePath(rec.path.toStdWString());

        RuntimeMeta meta;
        meta.path = norm;
        meta.fileSize = rec.fileSize;
        meta.mtime = rec.mtime;
        meta.rating = rec.rating;
        meta.color = rec.color.toStdWString();
        meta.tags = rec.tags;
        meta.pinned = rec.pinned;
        meta.note = rec.note.toStdWString();
        meta.url = rec.url.toStdWString();

        batchToInsert.emplace_back(std::move(norm), std::move(meta));
    }

    MetaMemoryCache::instance().insertBatch(batchToInsert);
}
>>>>>>> REPLACE
```

---

## 4. Physical Signature Check & Compilation Safety
- `MetadataManager.h` signatures are strictly conserved (API Freezing Contract).
- No header includes are introduced that violate dependency flow.

---

## 5. Verification & Testing
1. Run `python3 tools/plan_verify/verify_plan.py` to ensure this Implementation Plan document complies with all 10 standard chapters.
2. Build the target project using standard CMake workflow to confirm zero compilation or linkage errors.
