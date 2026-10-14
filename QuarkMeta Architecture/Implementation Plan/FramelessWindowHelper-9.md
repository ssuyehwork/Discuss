# Implementation Plan - FramelessWindowHelper Native Win32 WM_NCHITTEST Architecture Optimization

## 1. Overview
This implementation plan outlines the native Win32 `WM_NCHITTEST` window management architecture for `FramelessWindowHelper`:
1. Eliminates legacy manual `grabMouse` / `setGeometry` resizing loops in favor of hardware-accelerated Windows DWM compositor resizing via precise native hit-testing (`HTTOPLEFT`, `HTTOPRIGHT`, `HTBOTTOMLEFT`, `HTBOTTOMRIGHT`, `HTLEFT`, `HTRIGHT`, `HTTOP`, `HTBOTTOM`).
2. Fixes border resize vs. title bar drag conflicts (`HTCAPTION` vs. `HTTOP`) and restores Windows native Aero Snap features.
3. Correctly handles `WM_NCCALCSIZE` and `WM_GETMINMAXINFO` for multi-monitor work area alignment without taskbar overlap or left-offset issues.
4. Uses `IsZoomed(hwnd)` as the Win32 single source of truth (SSOT) to prevent state desynchronization with Qt's delayed `isMaximized()` status.
5. Suppresses window initialization white flashes via `WM_ERASEBKGND` interception (`*result = 1`).
6. Implements smooth maximized drag-to-restore ("Restore + Move") via native `WM_NCLBUTTONDOWN` handling when `wParam == HTCAPTION` and window is zoomed.

---

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.h`
- `src/ui/FramelessWindowHelper.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FramelessWindowHelper.h`
Ensure `FramelessWindowHelper` header signature remains clean and backward compatible:

```cpp
<<<<<<< SEARCH
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override = default;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;

    static constexpr int kBaseResizeMargin = 8;
};
=======
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override = default;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;

    static constexpr int kBaseResizeMargin = 8;
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/FramelessWindowHelper.cpp`
Enhance native Win32 message handling in `handleNativeEvent` to include background erase suppression and maximized drag-to-restore:

```cpp
<<<<<<< SEARCH
    // 0. 尺寸与位置变动原生分发：第一时间校准标题栏最大化/还原图标
    if (msg->message == WM_SIZE || msg->message == WM_WINDOWPOSCHANGED) {
=======
    // -1. 拦截 WM_ERASEBKGND：彻底消除无边框窗口首次 show() 显示时的“秒闪白”默认背景填充
    if (msg->message == WM_ERASEBKGND) {
        *result = 1;
        return true;
    }

    // 0. 尺寸与位置变动原生分发：第一时间校准标题栏最大化/还原图标
    if (msg->message == WM_SIZE || msg->message == WM_WINDOWPOSCHANGED) {
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    // 5. 原生双击标题栏最大化 / 还原（通过 Win32 消息总线响应）
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (msg->wParam == HTCAPTION) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, isMax ? SC_RESTORE : SC_MAXIMIZE, 0);
            *result = 0;
            return true;
        }
    }
=======
    // 5. 原生双击标题栏最大化 / 还原（通过 Win32 消息总线响应）
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (msg->wParam == HTCAPTION) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, isMax ? SC_RESTORE : SC_MAXIMIZE, 0);
            *result = 0;
            return true;
        }
    }

    // 6. 最大化状态下拖拽标题栏：自动还原并跟随鼠标移动 (Restore + Move)
    if (msg->message == WM_NCLBUTTONDOWN && msg->wParam == HTCAPTION && isMax) {
        ::SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        POINT pt;
        GetCursorPos(&pt);
        RECT rc;
        GetWindowRect(hwnd, &rc);
        int winWidth = rc.right - rc.left;
        int newX = pt.x - winWidth / 2;
        int newY = pt.y - 10;
        SetWindowPos(hwnd, nullptr, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        ReleaseCapture();
        ::SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, msg->lParam);
        *result = 0;
        return true;
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute CMake build in sandbox or terminal:
   `cmake --build build`
2. Run unit and UI tests.
3. Verify frameless window behaviors on Windows:
   - Window border dragging resizes properly on all 4 borders and 4 corners.
   - Title bar dragging moves the window smoothly; dragging when maximized restores and continues moving.
   - Double clicking title bar toggles between maximized and restored states.
   - No white background flash occurs upon window initialization.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Win32 SSOT**: `::IsZoomed(hwnd)` is used as the single authoritative source of truth for window maximized state, avoiding stale Qt `isMaximized()` status.
- **No Parallel Resizing**: Reuses native DWM window resizing via `WM_NCHITTEST` return codes (`HTTOP`, `HTLEFT`, etc.) without installing redundant global mouse event filters or manual `grabMouse` loops.

---

## 6. Header API Signature Verification
| Class / Function Signature | Declared Header File | Signature Match Verification |
| :--- | :--- | :--- |
| `static FramelessWindowHelper* apply(QWidget* window, QWidget* titleBar = nullptr)` | `src/ui/FramelessWindowHelper.h` | 100% Exact Match |
| `static void setAlwaysOnTop(QWidget* window, bool onTop)` | `src/ui/FramelessWindowHelper.h` | 100% Exact Match |
| `static bool isAlwaysOnTop(QWidget* window)` | `src/ui/FramelessWindowHelper.h` | 100% Exact Match |
| `bool handleNativeEvent(void* message, qintptr* result)` | `src/ui/FramelessWindowHelper.h` | 100% Exact Match |
| `static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window)` | `src/ui/FramelessWindowHelper.h` | 100% Exact Match |

---

## 7. Header Inclusion Chain & Type Completeness Check
| Inclusion / Type Name | File | Purpose & Completeness Check |
| :--- | :--- | :--- |
| `#include <QObject>` | `src/ui/FramelessWindowHelper.h` | Provides base class `QObject` definition. |
| `#include <QWidget>` | `src/ui/FramelessWindowHelper.h` | Provides `QWidget` complete type definition. |
| `#include <QPointer>` | `src/ui/FramelessWindowHelper.h` | Provides `QPointer` smart pointer template. |
| `<windows.h>`, `<windowsx.h>` | `src/ui/FramelessWindowHelper.cpp` | Provides Win32 APIs (`IsZoomed`, `SetWindowPos`, `WM_NCHITTEST`, etc.). Guarded by `#ifdef Q_OS_WIN`. |
