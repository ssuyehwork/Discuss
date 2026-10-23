# Thumbnail Buffer Expansion & Multi-Column Fix Implementation Plan (`ThumbnailBufferAndMultiColumnFix.md`)

## 1. Overview
This implementation plan addresses two critical thumbnail loading issues across all view modes and multi-column view:
1. **Pre-Load Buffer Expansion (15 Rows / Smooth Scrolling)**: Expanded the viewport pre-load buffer in `ContentViewCoordinator::calculateVisibleSourceRows` from 4 rows to **15 rows** (and 1.5x viewport height in `JustifiedView`). When scrolling fast, off-screen items 15 rows ahead are loaded in advance, ensuring instant thumbnail rendering without generic icon flickering.
2. **Multi-Column Cancellation Fix**: Removed the global `ThumbnailPipelineService::cancelAll()` call from `DiskItemModel::incrementGeneration()` and guaranteed `m_requestedPaths.remove(path)` executes unconditionally in callback completion, allowing all open column panes in Column View to load and display thumbnails simultaneously without cross-column cancellation.

## 2. Modified Files List
- `src/ui/controllers/ContentViewCoordinator.cpp`
- `src/ui/models/DiskItemModel.cpp`
- `src/util/ThumbnailPipelineService.cpp`

## 3. Detailed Line-by-Line Changes

### Change 1: Expand Pre-Load Buffer Zone in `src/ui/controllers/ContentViewCoordinator.cpp`
```
<<<<<<< SEARCH
        auto* jv = qobject_cast<JustifiedView*>(view);
        if (jv) {
            if (!jv->isLayoutReady()) return;
            int scrollY = jv->verticalScrollBar() ? jv->verticalScrollBar()->value() : 0;
            int vpH = jv->viewport()->height();
            QList<int> rangeRows = jv->rowsInRange(qMax(0, scrollY - vpH / 2), scrollY + vpH + vpH / 2);
            for (int r : rangeRows) {
                QModelIndex idx = model->index(r, 0);
                QModelIndex srcIdx = toSourceIndex(idx, m_panel->diskModel());
                if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
            }
            continue;
        }

        QSet<int> rows = calculateVisibleSourceRows(view, m_panel->diskModel());
        visibleRows.unite(rows);
    }

    if (!visibleRows.isEmpty()) {
        m_panel->diskModel()->loadThumbnailsForRows(visibleRows.values());
    }
}

QSet<int> ContentViewCoordinator::calculateVisibleSourceRows(QAbstractItemView* view, const QAbstractItemModel* targetDiskModel) {
    QSet<int> visibleRows;
    if (!view || !view->viewport() || !targetDiskModel) return visibleRows;
    QAbstractItemModel* model = view->model();
    if (!model || model->rowCount() == 0) return visibleRows;

    QRect vpRect = view->viewport()->rect();
    QModelIndex topIdx = view->indexAt(vpRect.topLeft());
    QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

    int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
    int bottom = btmIdx.isValid() ? qMin(model->rowCount() - 1, btmIdx.row() + 4) : model->rowCount() - 1;
=======
        auto* jv = qobject_cast<JustifiedView*>(view);
        if (jv) {
            if (!jv->isLayoutReady()) return;
            int scrollY = jv->verticalScrollBar() ? jv->verticalScrollBar()->value() : 0;
            int vpH = jv->viewport()->height();
            int bufferH = static_cast<int>(vpH * 1.5);
            QList<int> rangeRows = jv->rowsInRange(qMax(0, scrollY - bufferH), scrollY + vpH + bufferH);
            for (int r : rangeRows) {
                QModelIndex idx = model->index(r, 0);
                QModelIndex srcIdx = toSourceIndex(idx, m_panel->diskModel());
                if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
            }
            continue;
        }

        QSet<int> rows = calculateVisibleSourceRows(view, m_panel->diskModel());
        visibleRows.unite(rows);
    }

    if (!visibleRows.isEmpty()) {
        m_panel->diskModel()->loadThumbnailsForRows(visibleRows.values());
    }
}

QSet<int> ContentViewCoordinator::calculateVisibleSourceRows(QAbstractItemView* view, const QAbstractItemModel* targetDiskModel) {
    QSet<int> visibleRows;
    if (!view || !view->viewport() || !targetDiskModel) return visibleRows;
    QAbstractItemModel* model = view->model();
    if (!model || model->rowCount() == 0) return visibleRows;

    QRect vpRect = view->viewport()->rect();
    QModelIndex topIdx = view->indexAt(vpRect.topLeft());
    QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

    // 预载缓冲区扩展：由原先的 4 行扩展至 15 行（约 1-2 个屏幕高度），实现无感平滑滚动
    int top = topIdx.isValid() ? qMax(0, topIdx.row() - 15) : 0;
    int bottom = btmIdx.isValid() ? qMin(model->rowCount() - 1, btmIdx.row() + 15) : model->rowCount() - 1;
>>>>>>> REPLACE
```

### Change 2: Remove Global Cancellation and Clean Up `m_requestedPaths` in `src/ui/models/DiskItemModel.cpp`
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

### Change 3: Guarantee Notification Callback Completion in `src/util/ThumbnailPipelineService.cpp`
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
   - Scroll fast in list, grid, or column view mode. Confirm thumbnails for items coming into view 15 rows ahead are pre-loaded in advance and display instantly without generic icon flickering.
   - Open Column View mode. Expand Column 1, Column 2, Column 3 sequentially. Confirm all columns load and display thumbnails simultaneously without cross-column cancellation or orphaned placeholder states.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Buffer Pre-Load SSOT**: Buffer expansion logic is localized inside `ContentViewCoordinator::calculateVisibleSourceRows`, benefiting all 4 view modes without code duplication.
- **Model Isolation**: Preserves `DiskItemModel` generation safety without leaking cancellation signals to other views.

## 6. Header API Signature Verification
| Class / Function Name | Declaration File (`.h`) | Physical Exact Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `ContentViewCoordinator::calculateVisibleSourceRows` | `src/ui/controllers/ContentViewCoordinator.h` | `static QSet<int> calculateVisibleSourceRows(QAbstractItemView* view, const QAbstractItemModel* targetDiskModel);` | Matched 100% |
| `DiskItemModel::incrementGeneration` | `src/ui/models/DiskItemModel.h` | `void incrementGeneration();` | Matched 100% |
| `ThumbnailPipelineService::loadBatchAsync` | `src/util/ThumbnailPipelineService.h` | `void loadBatchAsync(const QStringList& filePaths, int targetSize, std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded);` | Matched 100% |

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/controllers/ContentViewCoordinator.cpp` includes `"ContentViewCoordinator.h"`, `"JustifiedView.h"`.
- `src/ui/models/DiskItemModel.cpp` includes `"DiskItemModel.h"`, `"ThumbnailPipelineService.h"`.
- `src/util/ThumbnailPipelineService.cpp` includes `"ThumbnailPipelineService.h"`, `<QMutexLocker>`.
