# Implementation Plan - Step 4: Proxy Traversal & Debounced Thumbnail Scan Fixes

This implementation plan details fixing proxy model traversal across controllers, replacing raw `qobject_cast<QSortFilterProxyModel*>` with unified `toSourceIndex` mapping, and setting up debounced visible thumbnail scanning.

## 1. Overview
Currently, several controllers attempt to cast `view->model()` directly to `QSortFilterProxyModel*`. Because `view->model()` is now `SectionProxyModel`, these casts fail and return `nullptr`.
This step introduces `ContentViewCoordinator::toSourceIndex(const QModelIndex& idx, const QAbstractItemModel* targetModel)` to traverse the `mapToSource` proxy chain down to `DiskItemModel`.
It also unifies thumbnail scanning into a 60ms debounced `QTimer` (`m_visibleTimer`) in `ContentPanel`.

---

## 2. Modified Files List
1. `src/ui/controllers/ContentViewCoordinator.h` & `src/ui/controllers/ContentViewCoordinator.cpp`
2. `src/ui/controllers/ContentContextMenu.cpp`
3. `src/ui/controllers/ContentKeyHandler.cpp`
4. `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/controllers/ContentViewCoordinator.h`
<<<<<<< SEARCH
    static QModelIndex toSourceIndex(const QModelIndex& proxyIdx);
=======
    static QModelIndex toSourceIndex(const QModelIndex& idx, const QAbstractItemModel* targetModel);
>>>>>>> REPLACE

---

### 3.2 `src/ui/controllers/ContentViewCoordinator.cpp`
```cpp
QModelIndex ContentViewCoordinator::toSourceIndex(const QModelIndex& idx, const QAbstractItemModel* targetModel) {
    if (!idx.isValid()) return QModelIndex();

    QModelIndex curr = idx;
    while (curr.isValid()) {
        if (curr.model() == targetModel) {
            return curr;
        }
        const QAbstractProxyModel* proxy = qobject_cast<const QAbstractProxyModel*>(curr.model());
        if (!proxy) break;
        curr = proxy->mapToSource(curr);
    }

    return (curr.model() == targetModel) ? curr : QModelIndex();
}
```

---

### 3.3 `src/ui/ContentPanel.cpp` (Debounced Thumbnail Scanner)
```cpp
void ContentPanel::startVisibleScanTimer() {
    if (!m_visibleTimer) {
        m_visibleTimer = new QTimer(this);
        m_visibleTimer->setSingleShot(true);
        m_visibleTimer->setInterval(60);
        connect(m_visibleTimer, &QTimer::timeout, this, &ContentPanel::performVisibleThumbnailScan);
    }
    m_visibleTimer->start();
}
```

---

## 4. Build & Verification Steps
1. Apply changes to `ContentViewCoordinator.h/.cpp`, `ContentContextMenu.cpp`, `ContentKeyHandler.cpp`, `ContentPanel.h/.cpp`.
2. Compile project using CMake.
3. Test thumbnail loading on scroll, context menu on section headers vs items, and hotkey actions (star rating, color tagging) across Grid, List, and Column views.
