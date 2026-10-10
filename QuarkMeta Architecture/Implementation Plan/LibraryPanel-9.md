# Implementation Plan - LibraryPanel-9.md

## Overview
Fix line-inline editing failure when creating top-level categories and subcategories in `LibraryPanel`.

### Root Cause Analysis
1. When `createAndEditCategory(parentId)` calls `LibraryService::instance().createCategory(...)`, `LibraryService` inserts into SQLite and immediately emits the `libraryChanged` signal.
2. In `LibraryPanel::initUi()`, `libraryChanged` is connected to a `QTimer::singleShot(0, [this]() { loadLibrary(); });`.
3. Simultaneously, `createAndEditCategory(parentId)` sets `m_pendingEditNodeId = newId` and immediately calls `loadLibrary()`.
4. The synchronous `loadLibrary()` runs, builds items, finds `m_pendingEditNodeId`, and calls `m_treeView->edit(item->index())`. The inline QLineEdit editor appears.
5. Immediately afterwards on the same event loop pass, the pending `QTimer::singleShot(0, ...)` from `libraryChanged` fires!
6. That asynchronous `loadLibrary()` runs `m_model->clear()`, destroying all QStandardItems and closing/canceling the active line editor!
7. Furthermore, the search loop in `loadLibrary()` was only scanning top-level rows (`m_model->rowCount()`), so even if step 5/6 were mitigated, subcategories (`parentId > 0`) located inside parent `QStandardItem` children would never be found or edited.

### Solution
1. In `createAndEditCategory(parentId)`, set `m_pendingEditNodeId = newId`, but DO NOT synchronously invoke `loadLibrary()`. Let the `libraryChanged` signal trigger `loadLibrary()` cleanly via its `QTimer::singleShot(0, ...)` queue.
2. In `loadLibrary()`, use `itemMap` to lookup `m_pendingEditNodeId` across all tree depth levels (both top-level categories and subcategories).
3. If `m_pendingEditNodeId` is found, expand parent items, set current index, and invoke `m_treeView->edit(item->index())` via `QTimer::singleShot(0, ...)` so the edit trigger happens after tree view layout calculation completes.

## Modified Files List
- `src/ui/LibraryPanel.cpp`

## Detailed Changes

```
<<<<<<< SEARCH
    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        m_pendingEditNodeId = 0;

        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem* item = m_model->item(i);
            if (item && item->data(Qt::UserRole + 1).toInt() == targetNodeId) {
                m_treeView->setCurrentIndex(item->index());
                m_treeView->edit(item->index());
                break;
            }
        }
    }
=======
    if (m_pendingEditNodeId > 0 && m_treeView && itemMap.contains(m_pendingEditNodeId)) {
        int targetNodeId = m_pendingEditNodeId;
        m_pendingEditNodeId = 0;

        QStandardItem* targetItem = itemMap.value(targetNodeId);
        if (targetItem) {
            QModelIndex targetIdx = targetItem->index();
            m_treeView->setCurrentIndex(targetIdx);
            QTimer::singleShot(0, m_treeView, [this, targetIdx]() {
                if (m_treeView) {
                    m_treeView->edit(targetIdx);
                }
            });
        }
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void LibraryPanel::createAndEditCategory(int parentId) {
    int newId = LibraryService::instance().createCategory("新建分类", parentId);
    if (newId > 0) {
        m_pendingEditNodeId = newId;
        loadLibrary();
    }
}
=======
void LibraryPanel::createAndEditCategory(int parentId) {
    int newId = LibraryService::instance().createCategory("新建分类", parentId);
    if (newId > 0) {
        m_pendingEditNodeId = newId;
        // libraryChanged signal will trigger loadLibrary() asynchronously via QTimer
    }
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `m_pendingEditNodeId` handles both top-level and subcategories through `itemMap`.
2. Right-click blank space in Library panel ➔ "新建库分类" ➔ Verify inline edit QLineEdit appears.
3. Right-click an existing category ➔ "新建子分类" ➔ Verify subcategory expands and inline edit QLineEdit appears.

## SSOT & API Verification
- `itemMap` is already populated during `loadLibrary()` for all categories (root and child nodes).
- `LibraryService::createCategory` triggers `libraryChanged` which safely calls `loadLibrary()`.
