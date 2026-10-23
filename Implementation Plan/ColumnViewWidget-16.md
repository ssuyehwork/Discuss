# Implementation Plan - ColumnViewWidget-16.md

## 1. Overview
Fix the SSOT height calculation defect in `ColumnViewPane` (`ColumnViewWidget.cpp`):
Remove the hardcoded divide/multiply "guess-and-calculate" formula (`folderCount * 28 + 4` and `fileCount * 28 + 4`) used to set fixed heights on `m_folderListView` and `m_listView`. Instead, derive the exact layout height directly from Qt's model/view layout engine or `contentsSize().height()` (or `sizeHintForRow(0)`), ensuring 100% Single Source of Truth (SSOT) alignment without hardcoded pixel multiples.

---

## 2. Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ColumnViewWidget.cpp`
In `ColumnViewPane::initUi()`, replace the arbitrary `folderCount * 28 + 4` and `fileCount * 28 + 4` height guessing calculations in `updateSectionCountsAndHints` with SSOT row-height calculations derived directly from the view.

```
<<<<<<< SEARCH
        if (m_folderListView) {
            if (folderCount == 0) {
                m_folderListView->hide();
            } else {
                bool collapsed = m_folderHeader ? m_folderHeader->isCollapsed() : false;
                m_folderListView->setVisible(!collapsed);
                int folderH = qMax(28, folderCount * 28 + 4);
                m_folderListView->setFixedHeight(folderH);
            }
        }
        if (m_fileHeader) {
            m_fileHeader->setCount(fileCount);
            m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
        if (m_listView) {
            if (fileCount == 0) {
                m_listView->hide();
            } else {
                m_listView->show();
                int fileH = qMax(28, fileCount * 28 + 4);
                m_listView->setFixedHeight(fileH);
            }
        }
=======
        if (m_folderListView) {
            if (folderCount == 0) {
                m_folderListView->hide();
            } else {
                bool collapsed = m_folderHeader ? m_folderHeader->isCollapsed() : false;
                m_folderListView->setVisible(!collapsed);
                int rowH = m_folderListView->sizeHintForRow(0);
                if (rowH <= 0) rowH = 28;
                int folderH = folderCount * rowH + 2;
                m_folderListView->setFixedHeight(folderH);
            }
        }
        if (m_fileHeader) {
            m_fileHeader->setCount(fileCount);
            m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
        if (m_listView) {
            if (fileCount == 0) {
                m_listView->hide();
            } else {
                m_listView->show();
                int rowH = m_listView->sizeHintForRow(0);
                if (rowH <= 0) rowH = 28;
                int fileH = fileCount * rowH + 2;
                m_listView->setFixedHeight(fileH);
            }
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure and compile:
   ```bash
   cmake -B build -S .
   cmake --build build
   ```
2. Verify ColumnView panes display folder and file list items with 100% precise row bounds without extra vertical gaps or clipping.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Uses `sizeHintForRow(0)` directly from `QListView` (SSOT), eliminating arbitrary hardcoded magic numbers like `* 28 + 4`.

---

## 6. Header API Signature Verification
- `QListView::sizeHintForRow(int row) const` -> Qt `QListView` API
