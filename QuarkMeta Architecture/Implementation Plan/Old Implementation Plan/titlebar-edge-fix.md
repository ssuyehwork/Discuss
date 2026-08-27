# Implementation Plan - Prioritize Edge Resizing Over TitleBar Dragging (`titlebar-edge-fix.md`)

## 1. Overview
When hovering over the top window edge, `ResizeEventFilter` displays a vertical double-arrow cursor. However, pressing the mouse left button previously caused `TitleBarEventFilter` to intercept `MouseButtonPress` and set `m_isDragging = true`, incorrectly turning vertical resizing into window movement.

This implementation plan adds edge Margin detection inside `TitleBarEventFilter`. When the mouse position falls within the window's 8-direction edge sensor zones (6px Margin), `TitleBarEventFilter` immediately yields (returns `false`), allowing `MainWindow::mousePressEvent` to handle edge resizing correctly.

---

## 2. Modified Files List
- `src/ui/TitleBarEventFilter.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/TitleBarEventFilter.cpp`
Check if `mouseEv->position()` is in top/edge margin (6px) before intercepting press/drag:

```
<<<<<<< SEARCH
    if (event->type() == QEvent::MouseButtonDblClick) {
=======
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
        QPoint localPos = m_window->mapFromGlobal(mouseEv->globalPosition().toPoint());
        int margin = 6;
        if (localPos.y() < margin || localPos.x() < margin || localPos.x() > m_window->width() - margin) {
            return false; // Yield to MainWindow edge resizing
        }
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build application with CMake / Ninja.
2. Hover mouse over the top edge of the window until the vertical double-arrow cursor appears.
3. Press left mouse button and drag vertically: verify that the window height resizes properly instead of moving the window.
4. Drag title bar from non-edge area: verify normal window moving and maximize-restore interactions remain smooth.
