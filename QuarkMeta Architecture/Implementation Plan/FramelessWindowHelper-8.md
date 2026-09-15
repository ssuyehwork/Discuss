# Implementation Plan - FramelessWindowHelper Native Message Handling Optimization & First-Frame White Flash Elimination

## 1. Overview
This implementation plan optimizes native Win32 message handling in `FramelessWindowHelper` and `MainWindow`:
1. `FramelessWindowHelper::handleNativeEvent` intercepts `WM_ERASEBKGND` Win32 messages and returns `1`, preventing Windows OS from erasing/filling the window background with default opaque white brush prior to Qt dark UI rendering.
2. `FramelessWindowHelper::handleNativeEvent` intercepts `WM_NCLBUTTONDOWN` when `wParam == HTCAPTION` and `isMax` is true to execute smooth maximized drag-to-restore ("Restore + Move") behavior.
3. `MainWindow` sets `setAttribute(Qt::WA_TranslucentBackground)` in its constructor to instruct Qt's compositor to handle per-pixel alpha surfaces instead of filling default opaque OS background.

---

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.cpp`
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FramelessWindowHelper.cpp`
Interception of `WM_ERASEBKGND` and `WM_NCLBUTTONDOWN` in `FramelessWindowHelper::handleNativeEvent`:

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

### 3.2 `src/ui/MainWindow.cpp`
Set `Qt::WA_TranslucentBackground` attribute in constructor:

```cpp
<<<<<<< SEARCH
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setMinimumHeight(700); // 宽度由 PanelLayoutManager::updateDynamicMinimumSize() 动态管理
=======
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumHeight(700); // 宽度由 PanelLayoutManager::updateDynamicMinimumSize() 动态管理
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile QuarkMeta.
2. Launch QuarkMeta app on Windows.
3. Verify that the main window shows up directly with dark theme UI without any white frame or background flash on initial startup.
4. Drag title bar when maximized to confirm smooth restore and movement.
