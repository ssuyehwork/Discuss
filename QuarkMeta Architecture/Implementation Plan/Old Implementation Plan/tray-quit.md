# Implementation Plan - System Tray Exit Lifecycle Refactoring

## 1. Overview
This implementation plan addresses the issue where clicking "退出 QuarkMeta" (Quit QuarkMeta) from the system tray context menu failed to close the application or terminate the process. Because `setQuitOnLastWindowClosed(false)` is enabled in `main.cpp`, calling `QApplication::quit()` asynchronously without explicitly closing `m_mainWindow` prevented the main event loop from terminating cleanly.

By explicitly closing `m_mainWindow` (which triggers `MainWindow::closeEvent` to safely flush geometry and splitter configurations to disk) and calling `QApplication::exit(0)`, the event loop exits immediately and cleanly triggers `onApplicationAboutToQuit` to flush database transactions and release mutexes.

## 2. Modified Files List
- `src/ui/TrayController.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/TrayController.cpp`
```git
<<<<<<< SEARCH
void TrayController::onQuitApp() {
    if (m_trayIcon) m_trayIcon->hide();
    // 严禁在此处调用 DatabaseManager::shutdown()，统一交给 main.cpp 集中调度
    QApplication::quit();
}
=======
void TrayController::onQuitApp() {
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    if (m_mainWindow) {
        m_mainWindow->close();
    }
    // 强制通知 Qt 主事件循环以状态码 0 顺畅退出，触发现发 aboutToQuit 数据库与线程池清场机制
    QApplication::exit(0);
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure and build the project using CMake & MSVC:
   `cmake --build build --config Release`
2. Launch `QuarkMeta.exe`.
3. Right click the system tray icon and click "退出 QuarkMeta".
4. Verify that:
   - `m_mainWindow` closes and saves geometry settings.
   - The application process terminates cleanly with exit code 0.
