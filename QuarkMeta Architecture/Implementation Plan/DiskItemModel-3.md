# Implementation Plan - DiskItemModel-3

## Overview
This implementation plan addresses critical context conflicts and performance bugs in thumbnail loading and data updates within `DiskItemModel`:
1. **Path Key Normalization**: Ensures `m_iconCache` and `m_aspectRatios` use unified `QDir::cleanPath` keys to fix cache lookup failures caused by slash/backslash mismatches in Windows environments.
2. **Dynamic Path Re-mapping on Async Callbacks**: Re-checks `m_pathToIndex` upon asynchronous thumbnail completion to dynamically resolve the latest valid row index, preventing signal loss when items drift after sorting or file changes.
3. **Aggregated `dataChanged` Notification**: Merges multiple single-row `dataChanged` signals in `flushPendingThumbDataChanged()` into a single range `dataChanged(minRow, maxRow)` signal, reducing Viewport re-render thrashing.

## Modified Files List
- `src/ui/models/DiskItemModel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/models/DiskItemModel.cpp`

```
<<<<<<< SEARCH
        QString cacheKey = path;
        QIcon* cached = m_iconCache.object(cacheKey);
        if (cached) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        
        if (isGraphic) return QIcon();
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        if (ShellIconManager::isIconCached(path, record.isDir, ext)) {
            m_iconCache.insert(cacheKey, new QIcon(icon));
        }
        return icon;
=======
        QString cleanKey = QDir::cleanPath(path);
        QIcon* cached = m_iconCache.object(cleanKey);
        if (!cached) {
            cached = m_iconCache.object(path);
        }
        if (cached) return *cached;

        QString ext = record.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        
        if (isGraphic) return QIcon();
        QIcon icon = ShellIconManager::getFileIconFast(path, record.isDir, ext);
        if (ShellIconManager::isIconCached(path, record.isDir, ext)) {
            m_iconCache.insert(cleanKey, new QIcon(icon));
        }
        return icon;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            auto it = weakThis->m_pathToIndex.find(path);
            if (it != weakThis->m_pathToIndex.end()) {
                int rIdx = it->second;
                QMetaObject::invokeMethod(weakThis, [weakThis, path, rIdx]() {
                    if (!weakThis) return;
                    if (rIdx >= 0 && rIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                        if (weakThis->m_allRecords[rIdx].path == path) {
                            weakThis->m_pendingThumbRows.insert(rIdx);
                            if (weakThis->m_thumbBatchTimer && !weakThis->m_thumbBatchTimer->isActive()) {
                                weakThis->m_thumbBatchTimer->start();
                            }
                            emit weakThis->thumbnailLoaded(rIdx);
                        }
                    }
                }, Qt::QueuedConnection);
            }
=======
            QMetaObject::invokeMethod(weakThis, [weakThis, path]() {
                if (!weakThis) return;
                auto it = weakThis->m_pathToIndex.find(path);
                if (it != weakThis->m_pathToIndex.end()) {
                    int currentIdx = it->second;
                    if (currentIdx >= 0 && currentIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                        if (weakThis->m_allRecords[currentIdx].path == path) {
                            weakThis->m_pendingThumbRows.insert(currentIdx);
                            if (weakThis->m_thumbBatchTimer && !weakThis->m_thumbBatchTimer->isActive()) {
                                weakThis->m_thumbBatchTimer->start();
                            }
                            emit weakThis->thumbnailLoaded(currentIdx);
                        }
                    }
                }
            }, Qt::QueuedConnection);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void DiskItemModel::flushPendingThumbDataChanged() {
    if (m_pendingThumbRows.isEmpty()) return;

    QSet<int> rowsToEmit = m_pendingThumbRows;
    m_pendingThumbRows.clear();

    for (int r : rowsToEmit) {
        if (r >= 0 && r < static_cast<int>(m_allRecords.size())) {
            emit dataChanged(index(r, 0), index(r, columnCount() - 1),
                              {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
        }
    }
}
=======
void DiskItemModel::flushPendingThumbDataChanged() {
    if (m_pendingThumbRows.isEmpty()) return;

    QSet<int> rowsToEmit = m_pendingThumbRows;
    m_pendingThumbRows.clear();

    int minRow = std::numeric_limits<int>::max();
    int maxRow = std::numeric_limits<int>::min();

    for (int r : rowsToEmit) {
        if (r >= 0 && r < static_cast<int>(m_allRecords.size())) {
            if (r < minRow) minRow = r;
            if (r > maxRow) maxRow = r;
        }
    }

    if (minRow <= maxRow) {
        emit dataChanged(index(minRow, 0), index(maxRow, columnCount() - 1),
                          {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
    }
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile `src/ui/models/DiskItemModel.cpp`.
2. Verify thumbnail rendering performance and ensure no UI stutter occurs during async thumbnail loading.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses existing `QDir::cleanPath` and `DiskItemModel` signals without introducing parallel redundant models.

## Header API Signature Verification
- `DiskItemModel::index(int row, int column, const QModelIndex &parent)` -> exact signature in Qt model.
- `QDir::cleanPath(const QString &path)` -> standard Qt Core API.

## Header Inclusion Chain Check
- Ensure `#include <QDir>` and `#include <limits>` are present in `DiskItemModel.cpp`.
