# Implementation Plan - TagManagerDialog (Post-Exec HWND Activation & Cursor Refresh)

## 1. Overview
This implementation plan provides a physical-level HWND wakeup failsafe for `TagManagerDialog::showDialog`:
After `dlg.exec()` returns, it explicitly invokes `EnableWindow(hwnd, TRUE)`, `activateWindow()`, `restoreOverrideCursor()`, and `QCursor::setPos(QCursor::pos())` to guarantee that Windows DWM re-enables `WM_SETCURSOR` and `WM_NCHITTEST` message dispatch to `MainWindow`.

---

## 2. Modified Files List
- `src/ui/TagManagerDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Post-Exec HWND Wakeup in TagManagerDialog (`src/ui/TagManagerDialog.cpp`)

```
<<<<<<< SEARCH
void TagManagerDialog::showDialog(QWidget* parent, const QString& currentPath, bool isMirrorSource) {
    QWidget* topParent = parent ? parent->window() : nullptr;
    TagManagerDialog* dlg = new TagManagerDialog(currentPath, isMirrorSource, topParent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}
=======
void TagManagerDialog::showDialog(QWidget* parent, const QString& currentPath, bool isMirrorSource) {
    QWidget* topParent = parent ? parent->window() : nullptr;
    TagManagerDialog dlg(currentPath, isMirrorSource, topParent);
    dlg.exec();

    if (topParent) {
#ifdef Q_OS_WIN
        ::EnableWindow(reinterpret_cast<HWND>(topParent->winId()), TRUE);
#endif
        topParent->activateWindow();
        QGuiApplication::restoreOverrideCursor();
        QCursor::setPos(QCursor::pos());
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **HWND Verification**:
   - Open Tag Manager dialog, click close button while cursor is pointing hand.
   - Verify that upon closing, top-level window cursor immediately resets to arrow and edge resizing (`WM_NCHITTEST`) is 100% responsive.
