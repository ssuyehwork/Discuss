# Implementation Plan - Three-Stage Hash Deduplication & Zero Main-Thread I/O Block (`duplicate-1.md`)

## 1. Overview
This implementation plan refactors the duplicate file detection mechanism to eliminate misidentifications and remove main-thread I/O bottlenecks:
1. **ScanStats Enhancement**: Added `std::unordered_set<QString> duplicatePaths;` to `ScanStats` struct to carry background deduplication results.
2. **DuplicateDetectorService Three-Stage Pipeline**: Implemented `findDuplicatePaths` with 1) File Size Bucketing, 2) FastHash (64KB Head+Tail SHA-256), and 3) Full SHA-256 Resolution.
3. **Async Background Computation**: `ContentPanel::recalculateAndEmitStats` delegates `findDuplicatePaths` to QtConcurrent background worker thread. Results are dispatched back to main UI thread `FilterProxyModel::setCachedDuplicatePaths`, giving $O(1)$ memory lookup during proxy filtering without blocking main thread.

---

## 2. Modified Files List
- `src/ui/ScanStats.h`
- `src/meta/DuplicateDetectorService.h`
- `src/meta/DuplicateDetectorService.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ScanStats.h`
```
<<<<<<< SEARCH
    int hasThumbnailCount = 0;
};
=======
    int hasThumbnailCount = 0;

    std::unordered_set<QString> duplicatePaths;
};
>>>>>>> REPLACE
```

### 3.2 `src/meta/DuplicateDetectorService.h`
```
<<<<<<< SEARCH
#include <vector>
#include "ItemRecord.h"
=======
#include <vector>
#include <unordered_set>
#include "ItemRecord.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    static std::vector<DuplicateConflictGroup> detectDuplicates(const QStringList& newImportedPaths);
=======
    static QString computeFastHash(const QString& filePath, qint64 fileSize = -1);
    static QString computeFullSha256(const QString& filePath);

    static std::unordered_set<QString> findDuplicatePaths(const std::vector<ItemRecord>& records);

    static std::vector<DuplicateConflictGroup> detectDuplicates(const QStringList& newImportedPaths);
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.h`
```
<<<<<<< SEARCH
    void updateFilter();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    void recomputeDuplicateCache();
    std::unordered_set<QString> m_cachedDuplicatePaths; // 缓存当前所有重复项的路径集合
=======
    void updateFilter();
    void setCachedDuplicatePaths(const std::unordered_set<QString>& paths);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    std::unordered_set<QString> m_cachedDuplicatePaths;
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Rebuild application using CMake/Ninja.
2. Navigate to a folder containing duplicate and unique files.
3. Verify deduplication filter counts in `FilterPanel` populate accurately without freezing the UI during folder scans.
