# Multi-Column Thumbnail Pipeline Cancellation Fix (`MultiColumnThumbnailFix.md`)

## 1. Overview
This implementation plan addresses the bug where expanding a new column (e.g. Column 3) in Column View mode causes Column 3 or adjacent columns (Column 2) to lose thumbnails and display placeholder icons.

### **Root Cause Analysis**:
1. **Global Pipeline Cancellation Cross-Contamination**:
   In `DiskItemModel::incrementGeneration()` (line 36 of `src/ui/models/DiskItemModel.cpp`), calling `ThumbnailPipelineService::instance().cancelAll()` triggers a global generation increment and token cancellation inside the singleton `ThumbnailPipelineService`.
   In Column View mode, each column pane (`ColumnViewPane`) owns an independent `DiskItemModel`. When a user expands a new column (Column 3), Column 3's model invokes `setRecords(...)`, which calls `incrementGeneration()`, which prematurely cancels `ThumbnailPipelineService`'s global decoding tasks for Column 2 or Column 3.

2. **Orphaned `m_requestedPaths` Poisoning**:
   In `DiskItemModel::loadThumbnailsForRows`, requested file paths are recorded in `m_requestedPaths`. In the async callback from `ThumbnailPipelineService`, `if (weakThis->currentGeneration() != thisGen) return;` returned early *before* removing `path` from `m_requestedPaths`.
   When `ThumbnailPipelineService` dropped the callback or returned early due to the global `cancelAll()`, `m_requestedPaths` retained those paths permanently. Future calls to `refreshVisibleThumbnails()` checked `m_requestedPaths.contains(path)`, assumed the thumbnails were still loading, skipped requesting them, and permanently stuck those files on generic placeholder icons.

## 2. Modified Files List
- `src/ui/models/DiskItemModel.cpp`
- `src/util/ThumbnailPipelineService.cpp`

## 3. Detailed Line-by-Line Changes

### Change 1: Remove global `ThumbnailPipelineService::cancelAll()` from `DiskItemModel::incrementGeneration` and clear `m_requestedPaths` in `src/ui/models/DiskItemModel.cpp`
```
<<<<<<< SEARCH
void DiskItemModel::incrementGeneration() {
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
    ThumbnailPipelineService::instance().cancelAll();
}
=======
void DiskItemModel::incrementGeneration() {
    m_currentGen.fetch_add(1, std::memory_order_relaxed);
    m_requestedPaths.clear();
}
>>>>>>> REPLACE
```

### Change 2: Ensure `m_requestedPaths` is cleaned up unconditionally in `src/ui/models/DiskItemModel.cpp`
```
<<<<<<< SEARCH
    ThumbnailPipelineService::instance().loadBatchAsync(pathsToLoad, DiskMediaExtractor::kThumbSize, [weakThis, thisGen](const QString& path, const QPixmap& pixmap) {
        if (!weakThis || weakThis->currentGeneration() != thisGen) return;

        weakThis->m_requestedPaths.remove(path);
=======
    ThumbnailPipelineService::instance().loadBatchAsync(pathsToLoad, DiskMediaExtractor::kThumbSize, [weakThis, thisGen](const QString& path, const QPixmap& pixmap) {
        if (!weakThis) return;

        weakThis->m_requestedPaths.remove(path);
        if (weakThis->currentGeneration() != thisGen) return;
>>>>>>> REPLACE
```

### Change 3: Guarantee single load callback invocation in `src/util/ThumbnailPipelineService.cpp`
```
<<<<<<< SEARCH
            QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                    return;
                }

                if (!finalImg.isNull()) {
                    QPixmap pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }

                        if (onSingleLoaded) {
                            onSingleLoaded(path, pix);
                        }
                    } else {
                        if (onSingleLoaded) {
                            onSingleLoaded(path, QPixmap());
                        }
                    }
                } else {
                    if (onSingleLoaded) {
                        onSingleLoaded(path, QPixmap());
                    }
                }
            }, Qt::QueuedConnection);
=======
            QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                if (!onSingleLoaded) return;

                if (m_currentGeneration.load(std::memory_order_relaxed) == taskGen && !finalImg.isNull()) {
                    QPixmap pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }
                        onSingleLoaded(path, pix);
                    } else {
                        onSingleLoaded(path, QPixmap());
                    }
                } else {
                    onSingleLoaded(path, QPixmap());
                }
            }, Qt::QueuedConnection);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. **Verification Checklist**:
   - Open Column View mode. Expand Column 1, Column 2, Column 3 sequentially.
   - Confirm Column 1, Column 2, and Column 3 all load and display thumbnails simultaneously without canceling each other out.
   - Scroll within any column pane. Confirm thumbnails for off-screen files load and display correctly without orphaned placeholder states.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Isolation of Independent Models**: Ensured model-level `DiskItemModel` generation increments do not pollute global pipeline state for other models in multi-column / multi-pane views.
- **Resource Leak Prevention**: Guaranteed `m_requestedPaths` cleanup on every callback completion or model reset.

## 6. Header API Signature Verification
| Class / Function Name | Declaration File (`.h`) | Physical Exact Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `DiskItemModel::incrementGeneration` | `src/ui/models/DiskItemModel.h` | `void incrementGeneration();` | Signature Unchanged |
| `ThumbnailPipelineService::loadBatchAsync` | `src/util/ThumbnailPipelineService.h` | `void loadBatchAsync(const QStringList& filePaths, int targetSize, std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded);` | Signature Unchanged |

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/models/DiskItemModel.cpp` includes `"DiskItemModel.h"`, `"ThumbnailPipelineService.h"`.
- `src/util/ThumbnailPipelineService.cpp` includes `"ThumbnailPipelineService.h"`, `<QMutexLocker>`.
