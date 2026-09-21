# Implementation Plan - ThumbnailStatusFilter

## Overview
This implementation plan fixes the thumbnail presence misjudgment in the Filter Panel (e.g., misclassifying unextracted `.ai` and `.eps` files as having thumbnails when they do not) and restores functional thumbnail presence filtering in `FilterProxyModel`.

The root cause consists of:
1. **Duplicated Guessing Logic (#1)**: `ContentStatsWorker::calculateStats` manually counted `hasThumbnailCount` by assuming any file with `UiHelper::isGraphicsFile(ext)` and `thumbStatus != 1` had a thumbnail without checking for physical thumbnail existence.
2. **Duplicated Guessing Logic (#2)**: `DiskItemModel::data(..., HasThumbnailRole)` manually assumed `UiHelper::isGraphicsFile(ext) == true` meant the item possessed a thumbnail.
3. **Filter Gate Disconnection**: `FilterProxyModel::filterAcceptsRow` omitted checking `currentFilter.thumbnailPresence`, rendering user selection of "Has Thumbnail" or "No Thumbnail" checkboxes ineffective.

To resolve these issues, a single truth source (`UiHelper::hasPhysicalThumbnail`) is introduced, the redundant guessing logic in `ContentStatsWorker` and `DiskItemModel` is replaced with this SSOT method, and filtering in `FilterProxyModel` is implemented.

---

## Modified Files List
- `src/ui/UiHelper.h`
- `src/ui/UiHelper.cpp`
- `src/ui/workers/ContentStatsWorker.cpp`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/models/FilterProxyModel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/UiHelper.h`
Add forward declaration or include for `ItemRecord` and method signature for `hasPhysicalThumbnail`.

```diff
<<<<<<< SEARCH
    static inline bool isGraphicsFile(const QString& ext) {
        return ColorPaletteEngine::isGraphicsFile(ext);
    }
=======
    static inline bool isGraphicsFile(const QString& ext) {
        return ColorPaletteEngine::isGraphicsFile(ext);
    }

    static bool hasPhysicalThumbnail(const ItemRecord& record);
>>>>>>> REPLACE
```

### 2. `src/ui/UiHelper.cpp`
Implement `UiHelper::hasPhysicalThumbnail`.

```diff
<<<<<<< SEARCH
#include "UiHelper.h"
#include "ThemeManager.h"
#include "SvgIcons.h"
=======
#include "UiHelper.h"
#include "ThemeManager.h"
#include "SvgIcons.h"
#include "../core/ItemRecord.h"
#include "../util/DiskMediaExtractor.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void UiHelper::setupLineEditContextMenu(QLineEdit* edit) {
    if (!edit) return;
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QLineEdit::customContextMenuRequested, edit, [edit](const QPoint& pos) {
        showLineEditContextMenu(edit, pos);
    });
}


} // namespace QuarkMeta
=======
void UiHelper::setupLineEditContextMenu(QLineEdit* edit) {
    if (!edit) return;
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QLineEdit::customContextMenuRequested, edit, [edit](const QPoint& pos) {
        showLineEditContextMenu(edit, pos);
    });
}

bool UiHelper::hasPhysicalThumbnail(const ItemRecord& record) {
    if (record.isDir) return false;
    if (record.thumbStatus == 1) return false;

    static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
    QString ext = record.suffix.toLower();
    if (iconOnlyExts.contains(ext)) return false;

    if (isStandardImage(ext) && record.width > 0 && record.height > 0) {
        return true;
    }

    QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(record.path);
    return QFile::exists(thumbPath);
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3. `src/ui/workers/ContentStatsWorker.cpp`
Replace redundant guessing logic with SSOT `UiHelper::hasPhysicalThumbnail`.

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
            if (UiHelper::hasPhysicalThumbnail(record)) {
                stats.hasThumbnailCount++;
            } else {
                stats.noThumbnailCount++;
            }
>>>>>>> REPLACE
```

### 4. `src/ui/models/DiskItemModel.cpp`
Delegate `HasThumbnailRole` to SSOT `UiHelper::hasPhysicalThumbnail`.

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
        return UiHelper::hasPhysicalThumbnail(record) ||
               (m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0) ||
               m_iconCache.contains(path);
>>>>>>> REPLACE
```

### 5. `src/ui/models/FilterProxyModel.cpp`
Add missing filter logic for `thumbnailPresence`.

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

    // 6.5 缩略图状态过滤
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
2. Launch QuarkMeta and navigate to a folder containing unextracted `.ai` / `.eps` or ungenerated thumbnail files.
3. Observe the right-side Filter Panel: verify `Has Thumbnail` count shows `0` and `No Thumbnail` count shows the correct number.
4. Toggle "Has Thumbnail" and "No Thumbnail" checkboxes in the Filter Panel to ensure the file view filters accordingly.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Channel Re-used**: Unified thumbnail check via `UiHelper::hasPhysicalThumbnail(record)` and `HasThumbnailRole`.
- **Anti-Redundancy**: Removed scattered guessing logic from `ContentStatsWorker` and `DiskItemModel`.
- **Zero-Value-Alteration**: Kept all existing color, size, and layout properties intact.

---

## Header API Signature Verification
- `UiHelper::hasPhysicalThumbnail(const ItemRecord& record)` -> Returns `bool`.
- `DiskItemModel::data(const QModelIndex& index, int role)` -> Returns `QVariant`.
- `FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent)` -> Returns `bool`.
