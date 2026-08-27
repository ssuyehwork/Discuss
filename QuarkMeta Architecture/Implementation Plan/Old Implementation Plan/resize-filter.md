# Implementation Plan - Unify Frameless Window Cursor Control via ResizeEventFilter (Option A) (`resize-filter.md`)

## 1. Overview
This implementation plan decouples mouse hover cursor shape management for frameless window resizing according to Option A:
1. **Single Source of Truth**: All window edge cursor shape updates are handled solely by `ResizeEventFilter`.
2. **MainWindow Clean Up**: Duplicate cursor calculation logic (`updateCursorShape` calls during mouse move) in `MainWindow.cpp` is removed, keeping `MainWindow` focused purely on window geometry updates (`move` / `setGeometry`).

---

## 2. Modified Files List
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/MainWindow.cpp`
```
<<<<<<< SEARCH
    if (!m_isDragging) {
        updateCursorShape(getResizeDirection(event->position().toPoint()));
    }
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build application with CMake / Ninja.
2. Hover mouse over any edge or corner of `MainWindow` to verify `ResizeEventFilter` correctly changes cursor shapes.
3. Drag window edges to resize and verify smooth window geometry scaling.
