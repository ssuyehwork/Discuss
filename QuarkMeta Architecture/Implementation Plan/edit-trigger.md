# Implementation Plan - Restrict View EditTriggers to EditKeyPressed (`edit-trigger.md`)

## 1. Overview
In `ContentPanel.cpp`, `m_gridView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed)` caused double-clicking on folders/files to accidentally launch the inline `FileNameLineEdit` editor box before opening the directory.

This implementation plan removes `QAbstractItemView::DoubleClicked` from `setEditTriggers` in both `initGridView()` and `initListView()`, ensuring double-clicks are 100% dedicated to folder navigation and file QuickLook preview, while inline renaming remains accessible via F2 or context menu.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
    m_gridView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed); 
=======
    m_gridView->setEditTriggers(QAbstractItemView::EditKeyPressed); 
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build application with CMake / Ninja.
2. Double-click any folder in GridView or ListView.
3. Verify the folder opens immediately without triggering an inline line-edit widget.
4. Press F2 on any item or right-click -> "重命名" to confirm inline editing still works as expected.
