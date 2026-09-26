# Implementation Plan - DiskItemModel Zero-Latency Thumbnail Refresh Fix (Restore Version-Old-8 Smoothness Spec)

## Overview
This implementation plan restores the `Version-Old-8` zero-latency thumbnail UI refresh behavior in `DiskItemModel.cpp`.

### Root Cause
In previous revisions, an artificial 80ms timer delay (`m_thumbBatchTimer`) and queue batching logic (`m_pendingThumbRows`) were introduced into `DiskItemModel::loadThumbnailsForRows`. When an asynchronous thumbnail load completed, `dataChanged` was suppressed and deferred until the 80ms timer fired.

Even when thumbnails were retrieved from the read-only disk cache in 1~2ms, cards were held back for 80ms before updating the UI viewport. Additionally, strict checks inside `HasThumbnailRole` forced card delegates to briefly fallback to extension badges, causing visible card flickering and lag during scrolling.

### Fix
1. Remove `m_thumbBatchTimer` and `m_pendingThumbRows` 80ms delay mechanism.
2. In `loadThumbnailsForRows` callback, immediately emit `dataChanged` for the loaded row to achieve zero-latency UI card updates.
3. Align `HasThumbnailRole` logic with `Version-Old-8` so graphics files directly return `true`.

## Modified Files List
- `src/ui/models/DiskItemModel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/models/DiskItemModel.cpp`
In `DiskItemModel::loadThumbnailsForRows` callback and `DiskItemModel::data`:

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
1. Rebuild application using CMake and MSVC compiler.
2. Scroll through folders with thumbnail cache built.
3. Confirm thumbnails load and update in real-time with zero 80ms latency.
4. Verify smooth card rendering without visual stuttering or badge fallback flickering.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses standard Qt Model-View `dataChanged` signal and `HasThumbnailRole` contract without creating redundant timer threads.

## Header API Signature Verification
- `DiskItemModel::dataChanged` -> `void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>())`
