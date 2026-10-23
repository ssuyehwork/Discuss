# TitleBar Maximize/Restore Synchronization Bug Fix Implementation Plan

## 1. Overview
This implementation plan resolves the issue where restarting the application when it was previously maximized causes the titlebar control button to display the "Maximize" icon instead of the "Restore" icon.

The root cause is a timing mismatch during startup: when `showEvent` is initially triggered after `restoreGeometry`, Windows DWM and the Win32 message queue have not finished processing the `WM_SIZE` (`SIZE_MAXIMIZED`) message, causing `::IsZoomed(winId())` to momentarily return `FALSE`. 

The solution adds an event-loop delayed re-calibration (`QTimer::singleShot(0, ...)`) in `MainWindow::showEvent`, and listens to `WM_SIZE` / `WM_WINDOWPOSCHANGED` Win32 native messages inside `FramelessWindowHelper::handleNativeEvent` to guarantee immediate icon synchronization across startup, double-clicks, and Windows system shortcuts (`Win + Up` / `Win + Down`).

## 2. Modified Files List
- `src/ui/MainWindow.cpp`
- `src/ui/FramelessWindowHelper.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/MainWindow.cpp`
Add asynchronous single-shot calibration in `showEvent` to catch the post-pump Win32 `IsZoomed` state.

```
<<<<<<< SEARCH
void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);

    // 关键修正 5：在实际展示事件中再次核实同步最大化图标
#ifdef Q_OS_WIN
    if (m_titleBarWidget) {
        m_titleBarWidget->setWindowMaximized(::IsZoomed(reinterpret_cast<HWND>(winId())));
    }
#endif

    if (!m_panelsInitialized) {
=======
void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);

    // 关键修正 5：在实际展示事件中再次核实同步最大化图标（结合 0ms 单次定时器处理 DWM 异步延迟）
#ifdef Q_OS_WIN
    if (m_titleBarWidget) {
        m_titleBarWidget->setWindowMaximized(::IsZoomed(reinterpret_cast<HWND>(winId())));
        QTimer::singleShot(0, this, [this]() {
            if (m_titleBarWidget) {
                m_titleBarWidget->setWindowMaximized(::IsZoomed(reinterpret_cast<HWND>(winId())));
            }
        });
    }
#endif

    if (!m_panelsInitialized) {
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/FramelessWindowHelper.cpp`
Listen to Win32 `WM_SIZE` and `WM_WINDOWPOSCHANGED` messages in `handleNativeEvent` to push immediate state updates to `TitleBarWidget`.

```
<<<<<<< SEARCH
    // 1. 客户区计算：彻底修复图二的左偏脱轨Bug
    if (msg->message == WM_NCCALCSIZE) {
=======
    // 0. 尺寸与位置变动原生分发：第一时间校准标题栏最大化/还原图标
    if (msg->message == WM_SIZE || msg->message == WM_WINDOWPOSCHANGED) {
        if (m_titleBar) {
            QMetaObject::invokeMethod(m_titleBar, "setWindowMaximized", Q_ARG(bool, isMax));
        }
    }

    // 1. 客户区计算：彻底修复图二的左偏脱轨Bug
    if (msg->message == WM_NCCALCSIZE) {
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**: Compile using CMake / MSVC build environment.
2. **Startup Verification**:
   - Maximize the application window and close it.
   - Relaunch the application.
   - Verify that the titlebar control button displays the **Restore** (`restore_line`) icon and tooltip ("还原") instead of "Maximize".
3. **Shortcut & Snap Verification**:
   - Press `Win + Down` / `Win + Up` or drag the titlebar to snap/unsnap.
   - Verify that the titlebar icon updates instantly to reflect the native Win32 window state.
