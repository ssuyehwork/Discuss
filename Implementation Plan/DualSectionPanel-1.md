# Implementation Plan - DualSectionPanel Layout Align & Pure-Folder View MinHeight Fix

## Overview
This implementation plan addresses the layout anomalies (empty top gap ①, floating folder cards ②, empty bottom gap ③) when navigating to directories that contain only folders (`folderCount > 0 && fileCount == 0`).

### Root Causes
1. **Missing `Qt::AlignTop` Alignment**: `DualSectionPanel`'s `QVBoxLayout` did not set `m_layout->setAlignment(Qt::AlignTop)`. When total child height was smaller than `DualSectionPanel`'s height, Qt's `QVBoxLayout` evenly distributed remaining space above and below `folderView`, resulting in top gap ① and bottom gap ③.
2. **Circular Math Dependency**: `computeFileViewMinHeight` subtracted `m_folderView->height()`. When `fileCount == 0`, `fileView` was hidden, so `fileViewMinHeight` evaluated to 0. Meanwhile, `folderView` had no viewport-stretch formula, leaving `folderView` at its exact content height without filling the canvas area.

## Modified Files List
- `src/ui/DualSectionPanel.h`
- `src/ui/DualSectionPanel.cpp`
- `src/ui/SectionedScrollCanvas.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/DualSectionPanel.h`
Add `computeFolderViewMinHeight` helper declaration and inline getter.

<<<<<<< SEARCH
    int fileViewMinHeight() const { return computeFileViewMinHeight(m_lastHostViewportHeight); }
    int computeFileViewMinHeight(int hostViewportHeight) const;
=======
    int fileViewMinHeight() const { return computeFileViewMinHeight(m_lastHostViewportHeight); }
    int folderViewMinHeight() const { return computeFolderViewMinHeight(m_lastHostViewportHeight); }
    int computeFileViewMinHeight(int hostViewportHeight) const;
    int computeFolderViewMinHeight(int hostViewportHeight) const;
>>>>>>> REPLACE

---

### 2. `src/ui/DualSectionPanel.cpp`
In constructor, set `m_layout->setAlignment(Qt::AlignTop)` to pin all section bars and views strictly to the top.
Update `computeFileViewMinHeight` to return 0 when `fileCount == 0`.
Add `computeFolderViewMinHeight` implementation.

<<<<<<< SEARCH
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
=======
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->setAlignment(Qt::AlignTop);
>>>>>>> REPLACE

<<<<<<< SEARCH
int DualSectionPanel::computeFileViewMinHeight(int hostViewportHeight) const {
    int used = 0;
    if (m_folderHeader && m_folderHeader->isVisible()) used += m_folderHeader->height();
    if (m_folderView && m_folderView->isVisible()) used += m_folderView->height();
    if (m_fileHeader && m_fileHeader->isVisible()) used += m_fileHeader->height();
    return qMax(0, hostViewportHeight - used);
}
=======
int DualSectionPanel::computeFileViewMinHeight(int hostViewportHeight) const {
    if (m_fileProxyModel && m_fileProxyModel->rowCount() == 0) {
        return 0;
    }
    int used = 0;
    if (m_folderHeader && m_folderHeader->isVisible()) used += m_folderHeader->height();
    if (m_folderView && m_folderView->isVisible()) used += m_folderView->height();
    if (m_fileHeader && m_fileHeader->isVisible()) used += m_fileHeader->height();
    return qMax(0, hostViewportHeight - used);
}

int DualSectionPanel::computeFolderViewMinHeight(int hostViewportHeight) const {
    if (m_folderProxyModel && m_folderProxyModel->rowCount() == 0) {
        return 0;
    }
    int used = 0;
    if (m_folderHeader && m_folderHeader->isVisible()) used += m_folderHeader->height();
    return qMax(0, hostViewportHeight - used);
}
>>>>>>> REPLACE

---

### 3. `src/ui/SectionedScrollCanvas.cpp`
In `updateSectionCounts()`, when `fileCount == 0`, stretch `folderView` to occupy the remaining viewport height using `m_panel->folderViewMinHeight()`.

<<<<<<< SEARCH
    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                folderView->setFixedHeight(fjv->totalHeight());
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            folderView->setFixedHeight(folderCount * rowH + hdrH + 2);
            folderView->updateGeometry();
        }
    }
=======
    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                int contentH = fjv->totalHeight();
                int minH = (fileCount == 0) ? m_panel->folderViewMinHeight() : 0;
                folderView->setFixedHeight(qMax(contentH, minH));
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int contentH = folderCount * rowH + hdrH + 2;
            int minH = (fileCount == 0) ? m_panel->folderViewMinHeight() : 0;
            folderView->setFixedHeight(qMax(contentH, minH));
            folderView->updateGeometry();
        }
    }
>>>>>>> REPLACE

---

## Build & Verification Steps
1. Perform CMake configuration and build using standard MSVC/Qt toolchain.
2. Navigate to a folder containing folders only (`folderCount > 0 && fileCount == 0`).
3. Verify that:
   - Header `文件夹 (N)` is pinned strictly to top without top gap ①.
   - Folder cards start immediately below header without floating gap ②.
   - Left-click on canvas empty area correctly clears selection.
   - Right-click anywhere in the canvas area below folder cards correctly triggers the context menu.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses existing `folderViewMinHeight()` and `DualSectionPanel::updateSectionCounts()` without creating redundant duplicate layout panels.

## Header API Signature Verification
- `DualSectionPanel::folderViewMinHeight()` -> `int folderViewMinHeight() const`
- `DualSectionPanel::computeFolderViewMinHeight(int hostViewportHeight)` -> `int computeFolderViewMinHeight(int hostViewportHeight) const`
