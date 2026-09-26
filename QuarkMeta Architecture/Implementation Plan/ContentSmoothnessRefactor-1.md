# Implementation Plan - Comprehensive Performance & Smoothness Refactor (Restoring Version-Old-8 Spec)

## Overview
This implementation plan restores the extreme smoothness and responsiveness of `Version-Old-8` across viewport thumbnail sampling and model data change notifications.

### Key Bottlenecks Identified & Fixed
1. **Viewport Edge Miss & Y-Axis Shift Bug (`DualSectionPanel::refreshVisibleThumbnails`)**:
   - Previously, sampling at `x=10` missed items on the right side of wide rows (e.g., 14 items in a row).
   - Furthermore, fallback offset loops incremented Y coordinates downward (`clampedTopY + offset`), miscalculating top-row items as out-of-viewport and causing scroll lag.
   - **Fix**: Clamp `clampedTopX` to start at `16px` (safely past the 6px view margin) to hit top-left items instantly. Clamp `clampedBtmX` to `view->width() - 16` for bottom-right items. Never offset along the Y axis in fallback loops.

2. **Artificial 80ms Deferred Batch Refresh Bug (`DiskItemModel::loadThumbnailsForRows`)**:
   - An 80ms timer (`m_thumbBatchTimer`) and queue batching held back finished thumbnails, adding forced visual delay even when read-only disk cache hits occurred in <2ms.
   - **Fix**: Remove `m_thumbBatchTimer` and `m_pendingThumbRows`. Emit `dataChanged` instantly on the main thread via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` upon thumbnail retrieval.

3. **HasThumbnailRole Strict Lock Bug (`DiskItemModel::data`)**:
   - `HasThumbnailRole` checked `m_iconCache.contains(path)`, causing card delegates to briefly fallback to extension badges before thumbnails were cached in memory.
   - **Fix**: Directly return `true` for all graphics files (`UiHelper::isGraphicsFile(ext)`), providing a steady SSOT state for card delegates and eliminating badge flickering.

## Modified Files List
- `src/ui/DualSectionPanel.cpp`
- `src/ui/models/DiskItemModel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/DualSectionPanel.cpp`
Update viewport sampling logic to use 16px horizontal margin alignment and X-axis-only fallback offsets.

<<<<<<< SEARCH
        int clampedTopX = qBound(0, topPoint.x(), view->width() - 1);
        int clampedTopY = qBound(0, topPoint.y(), view->height() - 1);

        int clampedBtmX = qBound(0, btmPoint.x(), view->width() - 1);
        int clampedBtmY = qBound(0, btmPoint.y(), view->height() - 1);

        QModelIndex topIdx = view->indexAt(QPoint(clampedTopX, clampedTopY));
        if (!topIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !topIdx.isValid(); offset += 10)
                topIdx = view->indexAt(QPoint(clampedTopX + offset, clampedTopY + offset));
        }
        QModelIndex btmIdx = view->indexAt(QPoint(clampedBtmX, clampedBtmY));
        if (!btmIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !btmIdx.isValid(); offset += 10)
                btmIdx = view->indexAt(QPoint(clampedBtmX - offset, clampedBtmY - offset));
        }
=======
        int clampedTopX = qBound(16, topPoint.x(), view->width() - 1);
        int clampedTopY = qBound(0, topPoint.y(), view->height() - 1);

        int clampedBtmX = qBound(0, btmPoint.x(), qMax(0, view->width() - 16));
        int clampedBtmY = qBound(0, btmPoint.y(), view->height() - 1);

        QModelIndex topIdx = view->indexAt(QPoint(clampedTopX, clampedTopY));
        if (!topIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !topIdx.isValid(); offset += 10)
                topIdx = view->indexAt(QPoint(qMin(view->width() - 1, clampedTopX + offset), clampedTopY));
        }
        QModelIndex btmIdx = view->indexAt(QPoint(clampedBtmX, clampedBtmY));
        if (!btmIdx.isValid()) {
            for (int offset = 10; offset <= 100 && !btmIdx.isValid(); offset += 10)
                btmIdx = view->indexAt(QPoint(qMax(0, clampedBtmX - offset), clampedBtmY));
        }
>>>>>>> REPLACE

---

### 2. `src/ui/models/DiskItemModel.cpp`
Restore instant `dataChanged` emission upon thumbnail load and simplify `HasThumbnailRole` for graphics files.

<<<<<<< SEARCH
            auto it = weakThis->m_pathToIndex.find(path);
            if (it != weakThis->m_pathToIndex.end()) {
                int rIdx = it->second;
                QMetaObject::invokeMethod(weakThis, [weakThis, rIdx]() {
                    if (!weakThis) return;
                    weakThis->m_pendingThumbRows.insert(rIdx);
                    if (weakThis->m_thumbBatchTimer && !weakThis->m_thumbBatchTimer->isActive()) {
                        weakThis->m_thumbBatchTimer->start();
                    }
                    emit weakThis->thumbnailLoaded(rIdx);
                }, Qt::QueuedConnection);
            }
=======
            auto it = weakThis->m_pathToIndex.find(path);
            if (it != weakThis->m_pathToIndex.end()) {
                int rIdx = it->second;
                QMetaObject::invokeMethod(weakThis, [weakThis, rIdx]() {
                    if (!weakThis) return;
                    emit weakThis->dataChanged(
                        weakThis->index(rIdx, 0),
                        weakThis->index(rIdx, weakThis->columnCount() - 1),
                        {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole}
                    );
                    emit weakThis->thumbnailLoaded(rIdx);
                }, Qt::QueuedConnection);
            }
>>>>>>> REPLACE

<<<<<<< SEARCH
    } else if (role == HasThumbnailRole) {
        if (record.isDir || record.thumbStatus == 1) return false;
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        QString ext = record.suffix.toLower();
        if (!UiHelper::isGraphicsFile(ext) || iconOnlyExts.contains(ext)) return false;
        if (m_iconCache.contains(path) || (m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0)) return true;
        if (record.width > 0 && record.height > 0) return true;
        return false;
=======
    } else if (role == HasThumbnailRole) {
        if (record.isDir || record.thumbStatus == 1) return false;
        static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
        QString ext = record.suffix.toLower();
        if (iconOnlyExts.contains(ext)) return false;
        if (UiHelper::isGraphicsFile(ext)) return true;
        if (record.width > 0 && record.height > 0) return true;
        return m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0;
>>>>>>> REPLACE

## Build & Verification Steps
1. Rebuild application using CMake and MSVC compiler:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. Launch application and navigate to a folder containing images/vectors.
3. Scroll rapidly through GridView.
4. Verify that:
   - All items across the full row width (left to right) are loaded without missing thumbnails.
   - Finished thumbnails pop in instantly with 0ms delay without waiting for 80ms batch intervals.
   - Cards do not flicker back and forth between extension badges and thumbnails.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses standard Qt Model-View `dataChanged` signals and `refreshVisibleThumbnails` contracts.

## Header API Signature Verification
- `DualSectionPanel::refreshVisibleThumbnails(ItemModelBase* model, QWidget* hostViewport)` -> `src/ui/DualSectionPanel.h`
- `DiskItemModel::loadThumbnailsForRows(const QList<int>& rows)` -> `src/ui/models/DiskItemModel.h`
