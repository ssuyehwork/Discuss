# Implementation Plan - TagSelectorOverlay-1 (Refactored Activation & Cursor Cleanup)

## 1. Overview
This implementation plan refactors `TagSelectorOverlay` to ensure zero cursor or focus pollution upon closing:
1. **Immediate Event Filter Unregistration**: Unregisters `qApp->removeEventFilter(this)` immediately inside `closeOverlay()` rather than waiting for deferred `deleteLater()` destruction.
2. **Global Mouse Grab Release**: Calls `QWidget::mouseGrabber()->releaseMouse()` to release any implicit button grabber inside the overlay.
3. **Explicit Window Activation**: Explicitly calls `topWin->activateWindow()` on closing to overcome Win32 `WS_EX_TOOLWINDOW` activation non-return behavior.
4. **FramelessWindowHelper Cleanup**: Removes unnecessary `FramelessWindowHelper` on small overlays to keep native window event handling pure and lightweight.

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/TagManagerDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Clean Overlay Closure in TagSelectorOverlay (`src/ui/TagSelectorOverlay.cpp`)

```
<<<<<<< SEARCH
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
    if (this->mouseGrabber() == this) {
        this->releaseMouse();
    }
    emit overlayClosed();
    close();
    QGuiApplication::restoreOverrideCursor();
    QCursor::setPos(QCursor::pos());
    deleteLater();
}
=======
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;

    if (qApp) {
        qApp->removeEventFilter(this);
    }

    if (QWidget::mouseGrabber()) {
        QWidget::mouseGrabber()->releaseMouse();
    }

    emit overlayClosed();
    close();

    QWidget* topWin = parentWidget() ? parentWidget()->window() : nullptr;
    if (topWin) {
        topWin->activateWindow();
    }

    QGuiApplication::restoreOverrideCursor();
    QCursor::setPos(QCursor::pos());
    deleteLater();
}
>>>>>>> REPLACE
```

---

### 3.2 Clean Dialog Deactivation in TagManagerDialog (`src/ui/TagManagerDialog.cpp`)

```
<<<<<<< SEARCH
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
        topParent->setFocus();
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

2. **Functional Verification**:
   - Open and close `TagSelectorOverlay` / `TagManagerDialog`.
   - Verify that `MainWindow` receives activation, mouse resize handles (`WM_NCHITTEST`) are fully active, and cursor is restored to arrow.
