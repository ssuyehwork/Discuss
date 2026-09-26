# Implementation Plan - Restoring Version-Old-8 Flat UI Architecture (Purging DualSectionPanel Wrapper Layer)

## Overview
This implementation plan restores the clean, flat UI architecture of `Version-Old-8` by purging the `DualSectionPanel` and `SectionedScrollCanvas` intermediate wrapper layers, returning direct view control to `ContentPanel`.

### Architecture History & Problem
In `Version-Old-8`, `ContentPanel` directly managed `JustifiedView` (`m_gridView`), `DropTreeView` (`m_treeView`), and `DropListView` (`m_listView`). Each view owned its native scrollbar and independent `viewport()`. `refreshVisibleThumbnails()` directly queried `view->viewport()->rect()`, achieving zero-coordinate-offset and zero-latency virtual scrolling.

Later refactorings wrapped item views inside `DualSectionPanel` and `SectionedScrollCanvas`, disabled native scrollbars, forced `setFixedHeight(totalHeight)` on child views, and introduced complex `mapFromGlobal` coordinate mapping for viewport calculations. This destroyed native virtual scrolling, introduced sampling bugs, and degraded scrolling performance.

### Restoration Strategy
1. **Direct View Management**: Re-establish direct ownership of `DropJustifiedView`, `DropTreeView`, and `DropListView` inside `ContentPanel` (or `ContentViewCoordinator`).
2. **Native Scrollbars & Viewports**: Re-enable native scrollbars on item views (`Qt::ScrollBarAsNeeded`) and remove height force-expansions.
3. **Direct Viewport Lazy Loading**: Restore `refreshVisibleThumbnails()` to directly query `view->viewport()->rect()` without intermediate container coordinate transformations.

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentViewCoordinator.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.cpp`
Restore direct view initialization and remove `SectionedScrollCanvas` wrapper instantiations.

<<<<<<< SEARCH
    m_listCanvas = new SectionedScrollCanvas(SectionedScrollCanvas::CanvasType::List, this);
    m_gridCanvas = new SectionedScrollCanvas(SectionedScrollCanvas::CanvasType::Grid, this);

    m_stackedLayout->addWidget(m_listCanvas);
    m_stackedLayout->addWidget(m_gridCanvas);
=======
    m_treeView = new DropTreeView(this);
    m_gridView = new DropJustifiedView(this);

    m_stackedLayout->addWidget(m_treeView);
    m_stackedLayout->addWidget(m_gridView);
>>>>>>> REPLACE

---

### 2. `src/ui/controllers/ContentViewCoordinator.cpp`
Restore direct `refreshVisibleThumbnails()` viewport calculation using `view->viewport()->rect()` directly from `Version-Old-8`.

<<<<<<< SEARCH
void ContentViewCoordinator::refreshVisibleThumbnails() {
    if (!m_panel || !m_panel->model() || CoreController::isShuttingDown()) return;

    QList<QAbstractItemView*> views = currentActiveViews();
    QSet<int> visibleRows;

    for (auto* view : views) {
        if (!view || !view->viewport()) continue;
        auto* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy || proxy->rowCount() == 0) continue;

        QRect vpRect = view->viewport()->rect();
        QModelIndex topIdx = view->indexAt(vpRect.topLeft());
        QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    }

    if (!visibleRows.isEmpty()) {
        m_panel->model()->loadThumbnailsForRows(visibleRows.values());
    }
}
=======
void ContentViewCoordinator::refreshVisibleThumbnails() {
    if (!m_panel || !m_panel->model() || CoreController::isShuttingDown()) return;

    QList<QAbstractItemView*> views = currentActiveViews();
    QSet<int> visibleRows;

    for (auto* view : views) {
        if (!view || !view->viewport() || !view->isVisible()) continue;
        auto* proxy = qobject_cast<QSortFilterProxyModel*>(view->model());
        if (!proxy || proxy->rowCount() == 0) continue;

        QRect vpRect = view->viewport()->rect();

        // Exact Version-Old-8 specification: direct sampling on view's own native viewport
        QModelIndex topIdx = view->indexAt(vpRect.topLeft());
        QModelIndex btmIdx = view->indexAt(vpRect.bottomRight());

        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 20) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 100) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
    }

    if (!visibleRows.isEmpty()) {
        m_panel->model()->loadThumbnailsForRows(visibleRows.values());
    }
}
>>>>>>> REPLACE

## Build & Verification Steps
1. Rebuild application using CMake and MSVC compiler:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. Open application and switch between List, Grid, and Column views.
3. Verify that views scroll smoothly with native scrollbars.
4. Verify that `refreshVisibleThumbnails` operates without coordinate offset errors or layout storms.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `ContentPanel` and `ContentViewCoordinator` standard direct view delegation matching `Version-Old-8`.

## Header API Signature Verification
- `ContentPanel::m_gridView` -> `DropJustifiedView*`
- `ContentPanel::m_treeView` -> `DropTreeView*`
