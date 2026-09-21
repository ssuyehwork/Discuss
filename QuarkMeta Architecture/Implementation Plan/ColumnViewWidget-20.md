# Implementation Plan - ColumnViewWidget Layout Align & Pure-Folder View MinHeight Fix

## Overview
This implementation plan addresses the layout collapse and circular calculation dependency in `ColumnViewWidget` (Miller Column view) when navigating to directories containing only folders (`folderCount > 0 && fileCount == 0`).

### Root Causes
1. **Circular Math & Wrong Method Usage**: Previously, `ColumnViewPane` used `m_panel->fileViewMinHeight()` to stretch `folderView`. Since `fileViewMinHeight()` subtracts `m_folderView->height()`, using it to set `folderView` created a self-subtracting circular formula that collapsed the view height.
2. **Proper Method**: Must use `m_panel->folderViewMinHeight()`, which computes $H_{\text{viewport}} - H_{\text{folderHeader}}$ without self-subtraction.

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.cpp`
In `ColumnViewPane::updateSectionCounts()`, stretch `folderView` using `m_panel->folderViewMinHeight()` when `fileCount == 0`.

<<<<<<< SEARCH
        if (m_folderView && folderCount > 0 && m_folderView->isVisible()) {
            auto* tv = static_cast<QTreeView*>(m_folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int folderH = folderCount * rowH + hdrH + 2;
            m_folderView->setFixedHeight(folderH);
            m_folderView->updateGeometry();
        }
=======
        if (m_folderView && folderCount > 0 && m_folderView->isVisible()) {
            auto* tv = static_cast<QTreeView*>(m_folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int folderH = folderCount * rowH + hdrH + 2;
            int minH = (fileCount == 0) ? m_panel->folderViewMinHeight() : 0;
            m_folderView->setFixedHeight(qMax(folderH, minH));
            m_folderView->updateGeometry();
        }
>>>>>>> REPLACE

---

## Build & Verification Steps
1. Perform CMake configuration and build using standard MSVC/Qt toolchain.
2. Switch to Column View (Miller Column).
3. Navigate to a folder containing folders only (`folderCount > 0 && fileCount == 0`).
4. Verify that `folderView` stretches to fill the viewport height without collapsing or leaving empty gaps.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `m_panel->folderViewMinHeight()` from `DualSectionPanel` without duplicating min-height calculations.

## Header API Signature Verification
- `DualSectionPanel::folderViewMinHeight()` -> `int folderViewMinHeight() const`
