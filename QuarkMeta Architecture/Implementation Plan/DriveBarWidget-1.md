# Implementation Plan - DriveBarWidget Persistence Fix (DriveBarWidget-1.md)

## 1. Overview
This implementation plan resolves the missing persistence issue for the DriveBar (`DriveBarWidget`) visibility state.
When the user toggles the DriveBar via the title bar button (`chevrons_down` / `chevrons_up`), the new visibility state is saved to `AppConfig` under `MainWindow/DriveBarVisible`. Upon application startup, `MainWindow` and `TitleBarWidget` restore the saved visibility state and button icon state.

---

## 2. Modified Files List
- `src/ui/TitleBarWidget.h`
- `src/ui/TitleBarWidget.cpp`
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/TitleBarWidget.h`
Expose a public method `setDriveBarVisible(bool visible)` so upper-level controllers/MainWindow can sync the button check state and icon without triggering duplicate signals.

```
<<<<<<< SEARCH
    void setWindowMaximized(bool maximized);
    void setViewModeOption(ViewModeOption mode);
=======
    void setWindowMaximized(bool maximized);
    void setViewModeOption(ViewModeOption mode);
    void setDriveBarVisible(bool visible);
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/TitleBarWidget.cpp`
Implement `setDriveBarVisible(bool visible)` with `QSignalBlocker`.

```
<<<<<<< SEARCH
void TitleBarWidget::setViewModeOption(ViewModeOption mode) {
    m_currentViewMode = mode;
}
=======
void TitleBarWidget::setViewModeOption(ViewModeOption mode) {
    m_currentViewMode = mode;
}

void TitleBarWidget::setDriveBarVisible(bool visible) {
    if (!m_btnToggleDriveBar) return;
    QSignalBlocker blocker(m_btnToggleDriveBar);
    m_btnToggleDriveBar->setChecked(visible);
    m_btnToggleDriveBar->setIcon(UiHelper::getIcon(visible ? "chevrons_down" : "chevrons_up", QColor("#EEEEEE")));
}
>>>>>>> REPLACE
```

### 3.3 Update `src/ui/MainWindow.cpp`
Read `MainWindow/DriveBarVisible` state on initialization in `setupTopBars`, apply it to `m_titleBarWidget` and `m_driveBarWidget`, persist state when toggled, and save in `closeEvent`.

```
<<<<<<< SEARCH
    // 顶层子部件间的纯 UI 布局显隐联动
    connect(m_titleBarWidget, &TitleBarWidget::driveBarToggleRequested, this, [this](bool visible) {
        if (m_driveBarWidget) m_driveBarWidget->setVisible(visible);
    });
=======
    // 顶层子部件间的纯 UI 布局显隐联动与持久化恢复
    bool driveBarVis = AppConfig::instance().getValue("MainWindow/DriveBarVisible", true).toBool();
    m_titleBarWidget->setDriveBarVisible(driveBarVis);
    if (m_driveBarWidget) m_driveBarWidget->setVisible(driveBarVis);

    connect(m_titleBarWidget, &TitleBarWidget::driveBarToggleRequested, this, [this](bool visible) {
        if (m_driveBarWidget) m_driveBarWidget->setVisible(visible);
        AppConfig::instance().setValue("MainWindow/DriveBarVisible", visible);
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void MainWindow::closeEvent(QCloseEvent* event) {
    AppConfig::instance().setValue("MainWindow/LastPath", NavigationService::instance().currentUrl());
    AppConfig::instance().setValue("MainWindow/Geometry", saveGeometry());
    if (m_panelLayoutManager) {
        m_panelLayoutManager->saveLayoutState();
    }
    AppConfig::instance().sync();
    QMainWindow::closeEvent(event);
}
=======
void MainWindow::closeEvent(QCloseEvent* event) {
    AppConfig::instance().setValue("MainWindow/LastPath", NavigationService::instance().currentUrl());
    AppConfig::instance().setValue("MainWindow/Geometry", saveGeometry());
    if (m_driveBarWidget) {
        AppConfig::instance().setValue("MainWindow/DriveBarVisible", m_driveBarWidget->isVisible());
    }
    if (m_panelLayoutManager) {
        m_panelLayoutManager->saveLayoutState();
    }
    AppConfig::instance().sync();
    QMainWindow::closeEvent(event);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation**:
   ```bash
   cmake --build --preset x64-Release
   ```
2. **Functional Verification**:
   - Launch the application.
   - Click the "展开/收起盘符管理栏" button on the title bar to collapse/hide the DriveBar.
   - Close the application.
   - Relaunch the application and verify that:
     1. The DriveBar remains collapsed/hidden.
     2. The title bar toggle button icon accurately reflects the collapsed state (`chevrons_up`).
   - Click the button again to expand/show the DriveBar, close, and relaunch to verify persistent expansion (`chevrons_down`).
