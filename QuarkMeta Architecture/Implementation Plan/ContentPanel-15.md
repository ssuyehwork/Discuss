# Implementation Plan - ContentPanel-15.md

## 1. Overview
Fix the SSOT height calculation defect in `ContentPanel`'s list mode (`m_folderTreeView`):
Remove the hardcoded divide/multiply "guess-and-calculate" formula (`folderCount * 30 + 32`) used to calculate fixed height on `m_folderTreeView`. Replace it with row-height querying directly from the `QTreeView`'s delegate/sizeHint (`sizeHintForRow(0)` plus `header()->height()`), adhering to Single Source of Truth (SSOT) principles.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentPanel.cpp`
In `ContentPanel::initListView()`, update `updateListSectionCounts` to query exact row height from `m_folderTreeView` using `sizeHintForRow(0)` and `header()->height()`, replacing hardcoded `folderCount * 30 + 32`.

```
<<<<<<< SEARCH
        if (m_folderTreeView) {
            if (folderCount == 0) {
                m_folderTreeView->hide();
            } else {
                bool collapsed = m_listFolderHeader ? m_listFolderHeader->isCollapsed() : false;
                m_folderTreeView->setVisible(!collapsed);
                int folderH = qMax(32, folderCount * 30 + 32);
                m_folderTreeView->setFixedHeight(folderH);
            }
        }
=======
        if (m_folderTreeView) {
            if (folderCount == 0) {
                m_folderTreeView->hide();
            } else {
                bool collapsed = m_listFolderHeader ? m_listFolderHeader->isCollapsed() : false;
                m_folderTreeView->setVisible(!collapsed);
                int rowH = m_folderTreeView->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (m_folderTreeView->header() && m_folderTreeView->header()->isVisible()) ? m_folderTreeView->header()->height() : 0;
                int folderH = folderCount * rowH + hdrH + 2;
                m_folderTreeView->setFixedHeight(folderH);
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
2. Verify list view mode in `ContentPanel` displays folder section with exact fit height without extra vertical padding or cut-offs.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Uses `m_folderTreeView->sizeHintForRow(0)` and `header()->height()` (SSOT) instead of hardcoded magic numbers like `* 30 + 32`.

---

## 6. Header API Signature Verification
- `QTreeView::sizeHintForRow(int row) const` -> Qt `QTreeView` API
- `QHeaderView::height() const` -> Qt `QHeaderView` API
