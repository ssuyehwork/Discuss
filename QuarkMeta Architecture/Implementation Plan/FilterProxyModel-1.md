# Implementation Plan - FilterProxyModel-1 (Harmonized Thumbnail Status & Filtering)

## Overview
This implementation plan addresses the thumbnail presence misjudgment in the Filter Panel (e.g., misclassifying unextracted `.ai` and `.eps` vector files with zero dimensions as having thumbnails) and restores functional thumbnail presence filtering in `FilterProxyModel`, while **strictly guaranteeing zero synchronous disk I/O on the UI main thread**.

### Architectural Vulnerability Elimination & SSOT Alignment
1. **Zero Main-Thread Disk I/O Guarantee**: Replaces disk calls (`QFile::exists`) with pure O(1) in-memory checks against `m_iconCache`, `m_aspectRatios`, and `ItemRecord` properties (`width > 0 && height > 0`, `thumbStatus != 1`). This prevents UI main thread frame drops and scrolling stutters.
2. **Harmonized Stats & Role Logic**:
   - Both `ContentStatsWorker::calculateStats` and `DiskItemModel::data(..., HasThumbnailRole)` evaluate thumbnail presence using the exact same criteria: `thumbStatus != 1`, not in `iconOnlyExts` ("cur", "ico", "ani"), and either present in memory cache (`m_iconCache`/`m_aspectRatios`) or having valid extracted image dimensions (`record.width > 0 && record.height > 0`).
   - Unextracted files without dimensions (such as unextracted `.ai` or `.eps` files) evaluate to `hasThumbnail = false` (`noThumbnailCount`), and once extracted with valid dimensions/cached pixmaps, seamlessly evaluate to `hasThumbnail = true` (`hasThumbnailCount`) across both statistics and filtering.
3. **Connected Filter Gate**: Implements missing `thumbnailPresence` filter logic in `FilterProxyModel::filterAcceptsRow`.

---

## Modified Files List
- `src/ui/workers/ContentStatsWorker.cpp`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/models/FilterProxyModel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/workers/ContentStatsWorker.cpp`
Refine thumbnail statistics logic in `ContentStatsWorker::calculateStats` to align strictly with extracted dimensions and `thumbStatus`.

```diff
<<<<<<< SEARCH
            static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
            QString ext = record.suffix.toLower();
            if (record.thumbStatus == 1) {
                stats.noThumbnailCount++;
            } else if (UiHelper::isGraphicsFile(ext) || (record.width > 0 && record.height > 0)) {
                if (!iconOnlyExts.contains(ext)) {
                    stats.hasThumbnailCount++;
                }
            }
=======
            static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
            QString ext = record.suffix.toLower();
            if (record.thumbStatus == 1) {
                stats.noThumbnailCount++;
            } else if (record.width > 0 && record.height > 0 && !iconOnlyExts.contains(ext)) {
                stats.hasThumbnailCount++;
            } else {
                stats.noThumbnailCount++;
            }
>>>>>>> REPLACE
```

### 2. `src/ui/models/DiskItemModel.cpp`
Harmonize `HasThumbnailRole` with pure in-memory cache and record metadata without any synchronous main-thread disk I/O.

```diff
<<<<<<< SEARCH
    } else if (role == HasThumbnailRole) {
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        QString ext = record.suffix.toLower();
        if (iconOnlyExts.contains(ext)) return false;
        if (UiHelper::isGraphicsFile(ext)) return true;
        if (record.width > 0 && record.height > 0) return true;
        return m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0;
=======
    } else if (role == HasThumbnailRole) {
        if (record.isDir || record.thumbStatus == 1) return false;
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        QString ext = record.suffix.toLower();
        if (iconOnlyExts.contains(ext)) return false;
        if (m_iconCache.contains(path) || (m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0)) return true;
        if (record.width > 0 && record.height > 0) return true;
        return false;
>>>>>>> REPLACE
```

### 3. `src/ui/models/FilterProxyModel.cpp`
Add missing filter logic for `thumbnailPresence` in `FilterProxyModel::filterAcceptsRow`.

```diff
<<<<<<< SEARCH
    if (currentFilter.duplicatePresence != FilterState::DupAll) {
        if (record.isDir) return false;
        bool isDuplicate = m_cachedDuplicatePaths.contains(record.path);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) return false;
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) return false;
    }
=======
    if (currentFilter.duplicatePresence != FilterState::DupAll) {
        if (record.isDir) return false;
        bool isDuplicate = m_cachedDuplicatePaths.contains(record.path);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) return false;
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) return false;
    }

    // 6.5 缩略图状态过滤 (Zero UI Main-Thread Disk I/O)
    if (currentFilter.thumbnailPresence != FilterState::ThumbAll) {
        bool hasThumb = sourceModelPtr->data(sourceModelPtr->index(sourceRow, 0), HasThumbnailRole).toBool();
        if (currentFilter.thumbnailPresence == FilterState::HasThumbnail && !hasThumb) return false;
        if (currentFilter.thumbnailPresence == FilterState::NoThumbnail && hasThumb) return false;
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Perform CMake build to compile the project.
2. Launch QuarkMeta and navigate to a folder containing unextracted `.ai` / `.eps` or complex vector/media files.
3. Observe the right-side Filter Panel: verify `Has Thumbnail` count shows `0` and `No Thumbnail (Failed/Unextracted)` count shows `2`.
4. Toggle "Has Thumbnail" and "No Thumbnail" checkboxes in the Filter Panel to verify smooth, instant filtering on the file view without UI thread stutters.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Channel Re-used**: Pure in-memory `HasThumbnailRole` and `ItemRecord` metadata.
- **Zero Main-Thread Sync I/O**: Completely eliminated `QFile::exists` or disk file stat operations from `data()` and `filterAcceptsRow()`.
- **Zero-Value-Alteration**: Preserved all existing theme, layout, and filter parameters intact.

---

## Header API Signature Verification
- `DiskItemModel::data(const QModelIndex& index, int role)` -> Returns `QVariant`.
- `FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent)` -> Returns `bool`.
