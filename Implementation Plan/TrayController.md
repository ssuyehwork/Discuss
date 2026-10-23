# Implementation Plan - TrayController (Monochrome SVG Icons)

## 1. Overview
This implementation plan specifies the changes required to equip all actions in the system tray context menu (`TrayController.cpp`) with neutral monochrome (`#EEEEEE`) SVG icons.

---

## 2. Modified Files List
- `src/ui/TrayController.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in System Tray Context Menu (`src/ui/TrayController.cpp`)

```
<<<<<<< SEARCH
    QAction* showAction = m_trayMenu->addAction("显示主界面");
    m_trayMenu->addSeparator();
    QAction* quitAction = m_trayMenu->addAction("退出 QuarkMeta");
=======
    QAction* showAction = m_trayMenu->addAction(UiHelper::getIcon("monitor", QColor("#EEEEEE"), 18), "显示主界面");
    m_trayMenu->addSeparator();
    QAction* quitAction = m_trayMenu->addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 18), "退出 QuarkMeta");
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Verification**:
   - Right-click on the system tray icon in the OS taskbar notification area.
   - Verify that "显示主界面" and "退出 QuarkMeta" both feature neutral monochrome (`#EEEEEE`) SVG icons.
