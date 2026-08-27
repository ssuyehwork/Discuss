# Implementation Plan - TitleBar Double-Click & Drag Maximize/Restore (`titlebar-drag-max.md`)

## 1. Overview
In the frameless window implementation of `MainWindow`, double-clicking or dragging the top title bar (`m_titleBarWidget`, `m_appNameLabel`, `m_logoLabel`) needs to support standard desktop window interactions:
1. **Double-Click**: Toggles between maximized and normal window state (`isMaximized() ? showNormal() : showMaximized()`).
2. **Press & Drag**: When dragged while maximized, automatically restores the window to normal size and adjusts the window position so it follows the mouse cursor smoothly.

---

## 2. Modified Files List
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/MainWindow.cpp`
Install event filters on `m_titleBarWidget`, `m_appNameLabel`, and `m_logoLabel` in `setupSplitters()`, and update `eventFilter()` / mouse event handling.

```
<<<<<<< SEARCH
    m_titleBarWidget->installEventFilter(this);
    m_appNameLabel->installEventFilter(this);
    m_logoLabel->installEventFilter(this);
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build application using CMake / Ninja.
2. Double-click on the top title bar area to maximize or restore the window.
3. While maximized, press and drag the title bar down to verify that the window restores and follows the cursor smoothly.
