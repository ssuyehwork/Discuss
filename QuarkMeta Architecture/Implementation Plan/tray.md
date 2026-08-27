# Implementation Plan - TrayController Menu Fix (tray.md)

## 1. Overview
This implementation plan addresses the issue where system tray context menu items ("Show Main Window" / "Exit QuarkMeta") become non-responsive or fail to activate properly when clicked.

Key causes and resolution:
1. **Menu Ownership Issue**: `m_trayMenu` was parented to `mainWindow` (`new QMenu(mainWindow)`). When `mainWindow` is hidden or has active event filters (`TitleBarEventFilter`/`ResizeEventFilter`), actions in `m_trayMenu` fail to dispatch `triggered` signals properly.
   - **Fix**: Remove `mainWindow` as `QMenu` parent (`new QMenu()`), making it a standalone context menu owned and cleaned up explicitly by `TrayController`.
2. **Window Activation Robustness**: `onShowMainWindow()` previously only called `showNormal()`, which fails to un-minimize or bring frameless windows to the front when hidden/minimized.
   - **Fix**: Update `onShowMainWindow()` to handle window state un-minimization (`setWindowState`), `show()`, `raise()`, and `activateWindow()`.
3. **Tray Icon Lifecycle**: Explicitly delete `m_trayMenu` in `TrayController` destructor to prevent memory leaks.

---

## 2. Modified Files List
1. `src/ui/TrayController.h` (No member changes needed, destructor cleanup)
2. `src/ui/TrayController.cpp` (Update `m_trayMenu` construction, cleanup in destructor, update `onShowMainWindow()` activation logic)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/TrayController.cpp`

```diff
<<<<<<< SEARCH
TrayController::TrayController(QMainWindow* mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow) {
    m_trayIcon = new QSystemTrayIcon(this);

    // 2026-04-14 物理加固：锁定图标来源为 Qt 资源系统中的标准 ico
    m_trayIcon->setIcon(QIcon(":/app_icon.ico"));
    m_trayIcon->setToolTip("QuarkMeta");

    m_trayMenu = new QMenu(mainWindow);
=======
TrayController::TrayController(QMainWindow* mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow) {
    m_trayIcon = new QSystemTrayIcon(this);

    // 2026-04-14 物理加固：锁定图标来源为 Qt 资源系统中的标准 ico
    m_trayIcon->setIcon(QIcon(":/app_icon.ico"));
    m_trayIcon->setToolTip("QuarkMeta");

    m_trayMenu = new QMenu();
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
TrayController::~TrayController() {
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}
=======
TrayController::~TrayController() {
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    if (m_trayMenu) {
        delete m_trayMenu;
        m_trayMenu = nullptr;
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void TrayController::onShowMainWindow() {
    m_mainWindow->showNormal();
    m_mainWindow->activateWindow();
}
=======
void TrayController::onShowMainWindow() {
    if (!m_mainWindow) return;
    if (m_mainWindow->isMinimized()) {
        m_mainWindow->showNormal();
    }
    m_mainWindow->show();
    m_mainWindow->setWindowState((m_mainWindow->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    m_mainWindow->raise();
    m_mainWindow->activateWindow();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 4.1 Build Verification
Execute CMake build:
```bash
cmake --build build --config Debug
```
Confirm clean build with zero warnings/errors.

### 4.2 Behavior Verification
1. Launch application and minimize or hide main window to system tray.
2. Right-click system tray icon to pop up context menu.
3. Click "显示主界面" (Show Main Window); verify main window immediately shows, restores from minimized state if necessary, and gains focus in foreground.
4. Right-click system tray icon and click "退出 QuarkMeta" (Exit QuarkMeta); verify tray icon hides immediately and application exits cleanly with code 0.
