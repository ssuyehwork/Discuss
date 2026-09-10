# Implementation Plan - MainWindow Self-Healing Cursor Fix (MainWindow-16.md)

## 1. Overview
This implementation plan introduces a "Self-Healing Architecture" at the top-level host window (`MainWindow::changeEvent`).

When `MainWindow` regains focus (intercepted via `QEvent::ActivationChange` where `isActiveWindow() == true`), it automatically releases any orphaned Qt mouse grabbers, releases Win32 system mouse capture via `::ReleaseCapture()`, and unsets residual window-level cursors.

This ensures that regardless of how secondary popups, overlays, or menus exit, `MainWindow` cleanly heals its input capture and cursor state without requiring defensive cleanup code scattered across every child dialog or overlay.

---

## 2. Modified Files List
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/MainWindow.cpp`
Enhance `MainWindow::changeEvent` to intercept `QEvent::ActivationChange` and perform host self-healing.

```
<<<<<<< SEARCH
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_searchController && m_searchController->historyPanel()) {
            m_searchController->historyPanel()->hide();
        }
        if (m_titleBarWidget) {
#ifdef Q_OS_WIN
            m_titleBarWidget->setWindowMaximized(::IsZoomed(reinterpret_cast<HWND>(winId())));
#else
            m_titleBarWidget->setWindowMaximized(isMaximized());
#endif
        }
        if (m_bodyLayout) {
            m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin);
        }
    }
    QMainWindow::changeEvent(event);
}
=======
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_searchController && m_searchController->historyPanel()) {
            m_searchController->historyPanel()->hide();
        }
        if (m_titleBarWidget) {
#ifdef Q_OS_WIN
            m_titleBarWidget->setWindowMaximized(::IsZoomed(reinterpret_cast<HWND>(winId())));
#else
            m_titleBarWidget->setWindowMaximized(isMaximized());
#endif
        }
        if (m_bodyLayout) {
            m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin);
        }
    } else if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (QWidget::mouseGrabber()) {
                QWidget::mouseGrabber()->releaseMouse();
            }
#ifdef Q_OS_WIN
            if (testAttribute(Qt::WA_WState_Created)) {
                ::ReleaseCapture();
            }
#endif
            unsetCursor();
        }
    }
    QMainWindow::changeEvent(event);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional & Architecture Self-Healing Verification**:
   - Open any overlay or dialog (e.g. `TagSelectorOverlay`, `TagManagerDialog`, right-click context menu).
   - Dismiss or close the overlay/dialog.
   - Move the mouse cursor back onto `MainWindow` title bar or canvas.
   - Verify that the mouse cursor immediately restores to standard arrow / resize handle cursors without sticking in hand cursor (`Qt::PointingHandCursor`) or frozen hover states.
