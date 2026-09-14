# Implementation Plan - FramelessWindowHelper First-Frame White Flash Prevention

## 1. Overview
This implementation plan eliminates the first-frame "white flash" when `MainWindow` is first shown (`w.show()`).

The root causes resolved:
1. `FramelessWindowHelper::handleNativeEvent` now intercepts `WM_ERASEBKGND` Win32 native messages, returning `1` to prevent Windows OS from erasing/filling the window background with default opaque white brush prior to Qt dark UI rendering.
2. `MainWindow` sets `setAttribute(Qt::WA_TranslucentBackground)` in its constructor to instruct Qt's compositor to handle per-pixel alpha surfaces instead of filling default opaque OS background.

---

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.cpp`
- `src/ui/MainWindow.cpp`
- `Memories.md`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FramelessWindowHelper.cpp`
Interception of `WM_ERASEBKGND` in `FramelessWindowHelper::handleNativeEvent`:

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

### 3.2 `src/ui/MainWindow.cpp`
Set `Qt::WA_TranslucentBackground` attribute in constructor:

```cpp
<<<<<<< SEARCH
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setMinimumHeight(700); // 宽度由 PanelLayoutManager::updateDynamicMinimumSize() 动态管理
    setWindowTitle("QuarkMeta");
=======
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumHeight(700); // 宽度由 PanelLayoutManager::updateDynamicMinimumSize() 动态管理
    setWindowTitle("QuarkMeta");
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile QuarkMeta.
2. Launch QuarkMeta app on Windows.
3. Verify that the main window shows up directly with dark theme UI without any white frame or background flash on initial startup.
