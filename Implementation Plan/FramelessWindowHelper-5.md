# FramelessWindowHelper DWM Maximize Flicker Elimination Implementation Plan

## 1. Overview
When maximizing a frameless window (`MainWindow`), Windows DWM sends `WM_ERASEBKGND` to erase the background. Without intercepting `WM_ERASEBKGND`, the operating system paints the unrendered extended screen area with the default black brush before Qt finishes its layout recalculation and paint event, resulting in a black frame flashing artifact around the window during the maximization transition.

This plan intercepts `WM_ERASEBKGND` in `FramelessWindowHelper::handleNativeEvent` and returns `TRUE` (`*result = 1`), preventing the default OS background erasure and eliminating the black border flicker during window maximization and restoration.

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FramelessWindowHelper.cpp`

```diff
<<<<<<< SEARCH
    // 0. 尺寸与位置变动原生分发：第一时间校准标题栏最大化/还原图标
    if (msg->message == WM_SIZE || msg->message == WM_WINDOWPOSCHANGED) {
        if (m_titleBar) {
            QMetaObject::invokeMethod(m_titleBar, "setWindowMaximized", Q_ARG(bool, isMax));
        }
    }
=======
    // 0. 抑制 Windows 默认背景擦除：彻底消除 DWM 最大化/还原拉伸瞬间的黑框闪烁
    if (msg->message == WM_ERASEBKGND) {
        *result = 1;
        return true;
    }

    // 0b. 尺寸与位置变动原生分发：第一时间校准标题栏最大化/还原图标
    if (msg->message == WM_SIZE || msg->message == WM_WINDOWPOSCHANGED) {
        if (m_titleBar) {
            QMetaObject::invokeMethod(m_titleBar, "setWindowMaximized", Q_ARG(bool, isMax));
        }
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Compile the application using CMake / MSVC build pipeline.
2. Launch QuarkMeta and click the maximize button on the title bar or double-click the title bar.
3. Observe the window maximization transition.
4. Verify that no black borders or dark background flashes appear around the window frame during maximization or restoration.
