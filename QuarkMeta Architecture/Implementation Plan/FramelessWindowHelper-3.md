# Strict Win32 SetWindowPos Window Pinning Implementation Plan

## 1. Overview
This implementation plan strictly enforces the rule that window pinning/always-on-top behavior **must exclusively use the Win32 native `SetWindowPos(HWND_TOPMOST / HWND_NOTOPMOST)` API**. All alternative methods (such as `setWindowFlags` or `Qt::WindowStaysOnTopHint`) are disabled and purged.

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Enforce Pure Win32 `SetWindowPos` Pinning in `FramelessWindowHelper::setAlwaysOnTop` and `isAlwaysOnTop`

```
<<<<<<< SEARCH
void FramelessWindowHelper::setAlwaysOnTop(QWidget* window, bool onTop) {
    if (!window) return;

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    SetWindowPos(hwnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
#else
    Qt::WindowFlags flags = window->windowFlags();
    if (onTop) flags |= Qt::WindowStaysOnTopHint;
    else flags &= ~Qt::WindowStaysOnTopHint;
    window->setWindowFlags(flags);
    window->show();
#endif
}

bool FramelessWindowHelper::isAlwaysOnTop(QWidget* window) {
    if (!window) return false;
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}
=======
void FramelessWindowHelper::setAlwaysOnTop(QWidget* window, bool onTop) {
    if (!window) return;

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    SetWindowPos(hwnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
#endif
}

bool FramelessWindowHelper::isAlwaysOnTop(QWidget* window) {
    if (!window) return false;
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    return (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
#else
    return false;
#endif
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**: Compile using CMake / MSVC build environment.
2. **Behavioral Verification**:
   - Toggle window pin on TitleBar or press `Alt + Q`.
   - Verify that window pinning exclusively uses `SetWindowPos(hwnd, HWND_TOPMOST, ...)` without invoking Qt `setWindowFlags` or causing window re-creation.
