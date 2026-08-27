# Implementation Plan - TrayController Context Menu Refactoring (tray.md)

## 1. Overview
This implementation plan addresses the systemic issue where system tray context menu items ("显示主界面" / "退出 QuarkMeta") fail to activate, become un-clickable, or break main window visibility logic.

Root Cause Analysis:
1. **Ownership Collision**: `m_trayMenu = new QMenu(mainWindow)` bound the context menu's lifecycle and Qt event dispatching to `mainWindow`. When `mainWindow` is hidden (`hide()`), Qt suppresses action events on its child widgets, causing clicks on tray actions to be silently ignored.
2. **Win32 Focus Loss**: Windows tray guidelines require calling `SetForegroundWindow` before displaying a popup context menu. Without this, Windows drops mouse click messages on tray popup menus.
3. **Signal & ContextMenu Conflict**: Calling `setContextMenu(m_trayMenu)` simultaneously with `activated` signal handler created event collisions on Windows platform.
4. **Window Activation State**: Calling only `showNormal()` fails to restore un-minimized frameless windows to front Z-order.

Resolution:
- Change `m_trayMenu` to an unparented `QMenu(nullptr)` managed explicitly by `TrayController`.
- Remove `setContextMenu(m_trayMenu)` binding. Manually popup context menu on `QSystemTrayIcon::Context` activation reason after invoking `SetForegroundWindow` on Windows.
- Update `onShowMainWindow()` to handle un-minimization, `show()`, `raise()`, and `activateWindow()`.

---

## 2. Modified Files List
1. `src/ui/TrayController.h` (No member changes required)
2. `src/ui/TrayController.cpp` (Update menu parent, remove `setContextMenu`, update `onTrayActivated` for manual popup, update `onShowMainWindow`, add destructor cleanup)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/TrayController.cpp`

```diff
<<<<<<< SEARCH
#include "TrayController.h"
#include <QApplication>
#include <QIcon>
#include <QDebug>
#include <QProgressDialog>
#include "../meta/DatabaseManager.h"
#include "BatchProgressDialog.h"

namespace QuarkMeta {

TrayController::TrayController(QMainWindow* mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow) {
    m_trayIcon = new QSystemTrayIcon(this);

    // 2026-04-14 物理加固：锁定图标来源为 Qt 资源系统中的标准 ico
    m_trayIcon->setIcon(QIcon(":/app_icon.ico"));
    m_trayIcon->setToolTip("QuarkMeta");

    m_trayMenu = new QMenu(mainWindow);
    m_trayMenu->setStyleSheet(
        "QMenu { background-color: #2D2D2D; color: #EEE; border: 1px solid #444; padding: 4px; border-radius: 8px; }"
        "QMenu::item { padding: 6px 25px 6px 10px; border-radius: 4px; font-size: 12px; color: #EEE; }"
        "QMenu::item:selected { background-color: #3E3E42; color: white; }"
        "QMenu::item:disabled { color: #666666; background-color: transparent; }"
    );

    QAction* showAction = m_trayMenu->addAction("显示主界面");
    m_trayMenu->addSeparator();
    QAction* quitAction = m_trayMenu->addAction("退出 QuarkMeta");

    connect(showAction, &QAction::triggered, this, &TrayController::onShowMainWindow);
    connect(quitAction, &QAction::triggered, this, &TrayController::onQuitApp);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayController::onTrayActivated);
}

TrayController::~TrayController() {
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}
=======
#include "TrayController.h"
#include <QApplication>
#include <QIcon>
#include <QDebug>
#include <QCursor>
#include <QProgressDialog>
#include "../meta/DatabaseManager.h"
#include "BatchProgressDialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

TrayController::TrayController(QMainWindow* mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow) {
    m_trayIcon = new QSystemTrayIcon(this);

    // 2026-04-14 物理加固：锁定图标来源为 Qt 资源系统中的标准 ico
    m_trayIcon->setIcon(QIcon(":/app_icon.ico"));
    m_trayIcon->setToolTip("QuarkMeta");

    m_trayMenu = new QMenu(nullptr);
    m_trayMenu->setStyleSheet(
        "QMenu { background-color: #2D2D2D; color: #EEE; border: 1px solid #444; padding: 4px; border-radius: 8px; }"
        "QMenu::item { padding: 6px 25px 6px 10px; border-radius: 4px; font-size: 12px; color: #EEE; }"
        "QMenu::item:selected { background-color: #3E3E42; color: white; }"
        "QMenu::item:disabled { color: #666666; background-color: transparent; }"
    );

    QAction* showAction = m_trayMenu->addAction("显示主界面");
    m_trayMenu->addSeparator();
    QAction* quitAction = m_trayMenu->addAction("退出 QuarkMeta");

    connect(showAction, &QAction::triggered, this, &TrayController::onShowMainWindow);
    connect(quitAction, &QAction::triggered, this, &TrayController::onQuitApp);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayController::onTrayActivated);
}

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
void TrayController::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (m_mainWindow->isVisible()) {
            m_mainWindow->hide();
        } else {
            onShowMainWindow();
        }
    }
}

void TrayController::onShowMainWindow() {
    m_mainWindow->showNormal();
    m_mainWindow->activateWindow();
}
=======
void TrayController::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (m_mainWindow && m_mainWindow->isVisible() && !m_mainWindow->isMinimized()) {
            m_mainWindow->hide();
        } else {
            onShowMainWindow();
        }
    } else if (reason == QSystemTrayIcon::Context) {
        if (m_trayMenu && m_mainWindow) {
#ifdef Q_OS_WIN
            SetForegroundWindow(reinterpret_cast<HWND>(m_mainWindow->winId()));
#endif
            m_trayMenu->exec(QCursor::pos());
        }
    }
}

void TrayController::onShowMainWindow() {
    if (!m_mainWindow) return;
    if (m_mainWindow->isMinimized()) {
        m_mainWindow->showNormal();
    }
    m_mainWindow->show();
    m_mainWindow->raise();
    m_mainWindow->activateWindow();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 4.1 Build Verification
Execute standard CMake build:
```bash
cmake --build build --config Debug
```
Confirm zero compilation errors and zero MOC linkage errors.

### 4.2 Behavior Verification
1. Launch app, minimize or hide main window to tray.
2. Right-click tray icon: confirm popup menu appears reliably.
3. Click "显示主界面" (Show Main Window): confirm main window is immediately un-minimized, brought to front, and receives focus.
4. Right-click tray icon and click "退出 QuarkMeta": confirm tray icon hides and app terminates with code 0 while saving configuration to disk.
