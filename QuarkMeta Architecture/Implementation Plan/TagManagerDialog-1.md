# Implementation Plan - TagManagerDialog & DriveBarWidget (HWND Activation & Non-Blocking Dialog Refactoring)

## 1. Overview
This implementation plan addresses the HWND activation and modal blocking defects in `TagManagerDialog` and `DriveBarWidget`:
1. **Root HWND Binding**: Corrects the `parent` parameter passed in `DriveBarWidget.cpp` from `this` (a child toolbar widget) to `window()` (top-level `MainWindow`), restoring proper Win32 `EnableWindow` and `SetActiveWindow` activation chains when closing the dialog.
2. **Flattened Event Loop & Non-Nested Dialogs**: Ensures `TagManagerDialog` parent binding uses top-level window references and avoids nested `exec()` loops during inner operations.

---

## 2. Modified Files List
- `src/ui/DriveBarWidget.cpp`
- `src/ui/TagManagerDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Correct Parent Binding in DriveBarWidget (`src/ui/DriveBarWidget.cpp`)

```
<<<<<<< SEARCH
    // 2. 创建各独立功能图标（纯 Icon，通过 ToolTipOverlay 提示）
    m_btnTagManager = createIconButton("tag", QColor("#1abc9c"), "标签管理");
    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(this, NavigationService::instance().currentUrl(), false);
    });
=======
    // 2. 创建各独立功能图标（纯 Icon，通过 ToolTipOverlay 提示）
    m_btnTagManager = createIconButton("tag", QColor("#1abc9c"), "标签管理");
    connect(m_btnTagManager, &QPushButton::clicked, this, [this]() {
        TagManagerDialog::showDialog(window(), NavigationService::instance().currentUrl(), false);
    });
>>>>>>> REPLACE
```

---

### 3.2 Ensure Top-Level Parent and Proper Dialog Deletion in TagManagerDialog (`src/ui/TagManagerDialog.cpp`)

```
<<<<<<< SEARCH
void TagManagerDialog::showDialog(QWidget* parent, const QString& currentPath, bool isMirrorSource) {
    TagManagerDialog* dlg = new TagManagerDialog(currentPath, isMirrorSource, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}
=======
void TagManagerDialog::showDialog(QWidget* parent, const QString& currentPath, bool isMirrorSource) {
    QWidget* topParent = parent ? parent->window() : nullptr;
    TagManagerDialog* dlg = new TagManagerDialog(currentPath, isMirrorSource, topParent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional & HWND Activation Verification**:
   - Open Tag Manager Dialog from DriveBar.
   - Close Tag Manager Dialog.
   - Verify that MainWindow edge resize cursors (`WM_NCHITTEST`), title bar dragging, and cursor shape (`Qt::ArrowCursor`) are perfectly restored and fully functional.
