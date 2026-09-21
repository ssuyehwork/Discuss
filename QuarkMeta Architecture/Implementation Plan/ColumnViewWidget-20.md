# Implementation Plan - ColumnViewWidget-20 (Column View Dynamic Stretch Handover)

## Overview
This implementation plan applies the dynamic viewport stretch handover logic to `ColumnViewPane` in `ColumnViewWidget.cpp` to resolve the coverage gap in Miller Columns view when a pane contains only subfolders (`folderCount > 0 && fileCount == 0`).

### Architectural Fix
1. **Column View Stretch Handover**:
   - In `ColumnViewPane`, when `fileCount == 0`, `m_listView` (file list) is hidden.
   - `m_folderListView` dynamically receives the stretch handover via `qMax(folderH, m_panel->fileViewMinHeight())`, ensuring `m_folderListView` stretches to cover 100% of the pane viewport remaining height.
   - This ensures rubber-band dragging and context menu actions in the lower area of a folder-only column pane land directly on `m_folderListView`'s viewport.

---

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`
Update height calculation in `updateSectionCountsAndHints` and `ColumnViewPane::resizeEvent`.

```diff
<<<<<<< SEARCH
        if (m_folderListView && folderCount > 0) {
            int rowH = m_folderListView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int folderH = folderCount * rowH + 2;
            m_folderListView->setFixedHeight(folderH);
        }

        if (m_listView && fileCount > 0) {
            int rowH = m_listView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int fileH = fileCount * rowH + 2;
            m_listView->setFixedHeight(qMax(fileH, m_panel->fileViewMinHeight()));
        }
=======
        if (m_folderListView && folderCount > 0) {
            int rowH = m_folderListView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int folderH = folderCount * rowH + 2;
            if (fileCount == 0) {
                m_folderListView->setFixedHeight(qMax(folderH, m_panel->fileViewMinHeight()));
            } else {
                m_folderListView->setFixedHeight(folderH);
            }
        }

        if (m_listView && fileCount > 0) {
            int rowH = m_listView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int fileH = fileCount * rowH + 2;
            m_listView->setFixedHeight(qMax(fileH, m_panel->fileViewMinHeight()));
        }
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_panel && m_paneScrollArea && m_paneScrollArea->viewport()) {
        int viewportH = m_paneScrollArea->viewport()->height();
        m_panel->updateSectionCounts(viewportH);
        if (m_listView && m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
            int fileCount = m_fileProxyModel->rowCount();
            int rowH = m_listView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int fileH = fileCount * rowH + 2;
            m_listView->setFixedHeight(qMax(fileH, m_panel->fileViewMinHeight()));
        }
    }
    update();
}
=======
void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_panel && m_paneScrollArea && m_paneScrollArea->viewport()) {
        int viewportH = m_paneScrollArea->viewport()->height();
        m_panel->updateSectionCounts(viewportH);
        int folderCount = m_folderProxyModel ? m_folderProxyModel->rowCount() : 0;
        int fileCount = m_fileProxyModel ? m_fileProxyModel->rowCount() : 0;
        if (m_folderListView && folderCount > 0) {
            int rowH = m_folderListView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int folderH = folderCount * rowH + 2;
            if (fileCount == 0) {
                m_folderListView->setFixedHeight(qMax(folderH, m_panel->fileViewMinHeight()));
            } else {
                m_folderListView->setFixedHeight(folderH);
            }
        }
        if (m_listView && fileCount > 0) {
            int rowH = m_listView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int fileH = fileCount * rowH + 2;
            m_listView->setFixedHeight(qMax(fileH, m_panel->fileViewMinHeight()));
        }
    }
    update();
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Perform CMake build to compile the project.
2. Switch to Column View mode and navigate to a column containing only subfolders (`fileCount == 0`).
3. Drag selection in the bottom area of the column pane: verify rubber-band selection works smoothly across the full column pane height.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Channel Re-used**: Re-used `m_panel->fileViewMinHeight()` for calculating column viewport remaining height.
- **Zero-Value-Alteration**: Preserved all existing row heights and column layout margins.

---

## Header API Signature Verification
- `DualSectionPanel::fileViewMinHeight()` -> Returns `int`.
- `ColumnViewPane::resizeEvent(QResizeEvent* event)` -> Returns `void`.
