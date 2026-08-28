# Implementation Plan - Tray Menu Dark Theme Fix (ThemeManager-1.md)

## Overview
本实施方案旨在彻底解决 Windows 平台下系统托盘右键菜单（`QSystemTrayIcon::setContextMenu`）显示为 Win32 浅色原生菜单的问题。
根因分析：在 Windows 上，Qt 对于未设置显式 `setStyleSheet(...)` 的 `QMenu` 实例，会默认走 Win32 原生 `HMENU` 弹出机制（从而忽略 `qApp` 全局样式表）。
解决方案：在 `ThemeManager::applyMenuStyle(QWidget* menu)` 中，为传入的 `menu` 控件显式调用 `setStyleSheet(...)` 注入专用的暗黑 QSS，强制 Qt 触发自绘 QMenu 控件逻辑，从而完美呈现深色背景（`#252526`）、6px 圆角与 1px 细边框。

---

## Modified Files List
1. `src/ui/ThemeManager.cpp`
2. `src/ui/TrayController.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ThemeManager.cpp`

```
<<<<<<< SEARCH
void ThemeManager::applyMenuStyle(QWidget* menu) const {
    if (!menu) return;
    menu->setAttribute(Qt::WA_TranslucentBackground, true);
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
}
=======
void ThemeManager::applyMenuStyle(QWidget* menu) const {
    if (!menu) return;
    menu->setAttribute(Qt::WA_TranslucentBackground, true);
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    // 🚀【核心修复】：显式设置 setStyleSheet，强制 Qt 绕过 Win32 原生 HMENU 渲染机制，走 Qt 自绘 QMenu
    menu->setStyleSheet(
        "QMenu {"
        "   background-color: #252526;"
        "   color: #EEEEEE;"
        "   border: 1px solid #333333;"
        "   border-radius: 6px;"
        "   padding: 4px;"
        "}"
        "QMenu::item {"
        "   background-color: transparent;"
        "   color: #EEEEEE;"
        "   padding: 6px 24px 6px 12px;"
        "   border-radius: 4px;"
        "   font-size: 12px;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #378ADD;"
        "   color: #FFFFFF;"
        "}"
        "QMenu::separator {"
        "   height: 1px;"
        "   background-color: #333333;"
        "   margin: 4px 6px;"
        "}"
    );
}
>>>>>>> REPLACE
```

---

### 2. `src/ui/TrayController.cpp`

```
<<<<<<< SEARCH
    m_trayMenu = new QMenu(nullptr);
=======
    m_trayMenu = new QMenu(mainWindow);
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **构建工程**：
   在 MSVC 编译环境下运行：
   ```bash
   cmake --build build --config Release
   ```
2. **功能验证**：
   - 启动应用，在 Windows 任务栏系统托盘区域右键点击 QuarkMeta 图标。
   - 确认托盘菜单不再显示为浅色 Win32 灰框，而是完美呈现深灰背景（`#252526`）、白色文字与蓝色选中高亮的暗黑自绘 QMenu。
