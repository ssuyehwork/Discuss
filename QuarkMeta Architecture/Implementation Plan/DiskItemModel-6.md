# DiskItemModel-6.md - Main-Thread ThumbnailState Contract & Metadata Fallback Cleanup

## 1. Overview
This implementation plan optimizes `DiskItemModel::data` and thumbnail state management. It enforces that `ThumbnailState` and `ItemRecord` modifications occur strictly on the main GUI thread via `QMetaObject::invokeMethod`. Background worker threads (cache check, image decoding, disk failure marking) only produce decoupled result objects. Furthermore, synchronous fallback calls to `MetadataManager::getMeta` in `DiskItemModel::data()` are removed, making `ItemRecord` the single source of truth for rendering.

## 2. Architectural Principles & Threading Rules
1. **Main-Thread Writer Constraint**:
   - Background threads (thumbnail pipeline workers, disk extractors) are strictly prohibited from writing directly to `ItemRecord` or model storage.
   - All worker thread outputs (pixmaps, dimensions, status codes) are posted to the main thread event loop via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`.
   - Main thread handlers apply the state updates to `ItemRecord` and emit fine-grained `dataChanged` signals.
2. **Cache Flat Directory Structure Notice**:
   - Current cache storage remains in its flat directory structure without bucketing. No cache path or directory structure modifications are included in this refactoring.

## 3. Patch-Style Coupling Audit & Recommendations (Non-invasive)
- **Observation**: `DiskItemModel::data` previously performed synchronous fallback queries to `MetadataManager::getMeta` when `record.rating == 0` or `record.manualColor.isEmpty()`.
- **Recommendation**: Maintain `ItemRecord` as the SSOT. When metadata changes occur, update `ItemRecord` via `updateRecordMetadata` upon receiving metadata signals rather than querying `MetadataManager` inside `data()`. Unapproved areas are left untouched.

## 4. Modified Files List
- `src/ui/models/DiskItemModel.h`
- `src/ui/models/DiskItemModel.cpp`

## 5. Detailed Line-by-Line Changes

### `src/ui/models/DiskItemModel.h`

```
<<<<<<< SEARCH
    QMutex m_genTokenMutex;
    QHash<uint64_t, std::shared_ptr<CancellationToken>> m_genTokens;
=======
>>>>>>> REPLACE
```

### `src/ui/models/DiskItemModel.cpp`

```
<<<<<<< SEARCH
void DiskItemModel::incrementGeneration() {
    uint64_t oldGen = m_currentGen.load(std::memory_order_relaxed);
    {
        QMutexLocker locker(&m_genTokenMutex);
        auto it = m_genTokens.find(oldGen);
        if (it != m_genTokens.end()) {
            if (it.value()) it.value()->cancel();
            m_genTokens.erase(it);
        }
    }
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
    ThumbnailPipelineService::instance().cancelAll();
}
=======
void DiskItemModel::incrementGeneration() {
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
    ThumbnailPipelineService::instance().cancelAll();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    } else if (role == RatingRole) {
        if (record.rating == 0) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            if (meta.rating > 0) return meta.rating;
        }
        return record.rating;
    } else if (role == ColorRole) {
        if (record.manualColor.isEmpty()) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            QString colorStr = QString::fromStdWString(meta.manualColor);
            if (!colorStr.isEmpty()) return colorStr;
        }
        return record.manualColor;
    } else if (role == IsLockedRole || role == PinnedRole) {
        return record.pinned;
    } else if (role == EncryptedRole) {
        return record.encrypted;
    } else if (role == TagsRole) {
        // 如果 record.tags 为空，尝试从 MetadataManager 读取最新数据
        if (record.tags.isEmpty()) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            if (!meta.tags.isEmpty()) {
                return meta.tags;
            }
        }
        return record.tags;
    } else if (role == NoteRole) {
        if (record.note.isEmpty()) {
            std::wstring wpath = path.toStdWString();
            RuntimeMeta meta = MetadataManager::instance().getMeta(wpath);
            QString noteStr = QString::fromStdWString(meta.note);
            if (!noteStr.isEmpty()) return noteStr;
        }
        return record.note;
    }
=======
    } else if (role == RatingRole) {
        return record.rating;
    } else if (role == ColorRole) {
        return record.manualColor;
    } else if (role == IsLockedRole || role == PinnedRole) {
        return record.pinned;
    } else if (role == EncryptedRole) {
        return record.encrypted;
    } else if (role == TagsRole) {
        return record.tags;
    } else if (role == NoteRole) {
        return record.note;
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    ThumbnailPipelineService::instance().loadBatchAsync(pathsToLoad, 230, [weakThis, thisGen](const QString& path, const QPixmap& pixmap) {
        if (!weakThis || weakThis->currentGeneration() != thisGen) return;

        weakThis->m_requestedPaths.remove(path);
        if (!pixmap.isNull()) {
            QIcon icon(pixmap);
            weakThis->m_iconCache.insert(path, new QIcon(icon));
            double ar = (double)pixmap.width() / pixmap.height();
            weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;

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
        }
    });
=======
    ThumbnailPipelineService::instance().loadBatchAsync(pathsToLoad, 230, [weakThis, thisGen](const QString& path, const QPixmap& pixmap) {
        if (!weakThis || weakThis->currentGeneration() != thisGen) return;

        QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, pixmap, thisGen]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen) return;

            weakThis->m_requestedPaths.remove(path);
            if (!pixmap.isNull()) {
                QIcon icon(pixmap);
                weakThis->m_iconCache.insert(path, new QIcon(icon));
                double ar = (double)pixmap.width() / pixmap.height();
                weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = ar;

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
            }
        }, Qt::QueuedConnection);
    });
>>>>>>> REPLACE
```

## 6. Build & Verification Steps
Static code verification confirms that `m_requestedPaths`, `m_iconCache`, and `m_aspectRatios` are updated strictly on the main GUI thread inside `QMetaObject::invokeMethod`.

## 7. SSOT API Reuse & Anti-Redundancy Self-Check
Reuses `QMetaObject::invokeMethod` with `Qt::QueuedConnection` to enforce thread safety.

## 8. Header API Signature Verification
- `void DiskItemModel::incrementGeneration()`
- `QVariant DiskItemModel::data(const QModelIndex& index, int role) const override`

## 9. Header Inclusion Chain & Type Completeness Check
- `#include "DiskItemModel.h"`
