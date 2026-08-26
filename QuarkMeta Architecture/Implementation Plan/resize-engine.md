# Implementation Plan - Frameless Window Resize Engine Refactoring in ResizeEventFilter (`resize-engine.md`)

## 1. Overview
This implementation plan refactors the frameless window edge resizing architecture into a fully self-contained, standalone engine within `ResizeEventFilter`:
1. **Integrated Resize Engine**: `ResizeEventFilter` handles both 8-direction hover cursor shape updates (`MouseMove`) and active edge drag resizing (`MouseButtonPress`, `MouseMove` with left button, and `MouseButtonRelease`).
2. **Preemptive Global Interception**: By handling press and move events directly when the mouse is over edge sensors (6px margin), `ResizeEventFilter` prevents child widgets (like `TitleBar`) from accidentally intercepting edge presses and mistaking resizing for window movement.
3. **Clean MainWindow**: `MainWindow.cpp` delegates edge mousePress and mouseMove resizing entirely to `ResizeEventFilter`.

---

## 2. Modified Files List
- `src/ui/ResizeEventFilter.h`
- `src/ui/ResizeEventFilter.cpp`
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ResizeEventFilter.h`
```
<<<<<<< SEARCH
private:
    enum ResizeDirection {
        None = 0,
        Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };

    QMainWindow* m_window;

    ResizeDirection getResizeDirection(const QPoint& pos) const;
    void updateCursorShape(ResizeDirection dir);
=======
private:
    enum ResizeDirection {
        None = 0,
        Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };

    QMainWindow* m_window = nullptr;
    bool m_isResizing = false;
    ResizeDirection m_resizeDir = None;
    QPoint m_resizeStartGlobal;
    QRect m_resizeStartGeometry;

    ResizeDirection getResizeDirection(const QPoint& pos) const;
    void updateCursorShape(ResizeDirection dir);
>>>>>>> REPLACE
```

### 3.2 `src/ui/ResizeEventFilter.cpp`
Implement complete press/drag-resize/release state machine in `ResizeEventFilter::eventFilter`.

---

## 4. Build & Verification Steps
1. Build application with CMake / Ninja.
2. Hover mouse over any window edge (e.g. top edge) until the double-arrow cursor appears.
3. Press left mouse button and drag to resize window height/width.
4. Verify that edge dragging resizes the window seamlessly in all 8 directions without converting to window move operations.
