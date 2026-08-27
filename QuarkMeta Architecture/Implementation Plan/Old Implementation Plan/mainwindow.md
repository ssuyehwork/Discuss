# Implementation Plan - MainWindow Decoupling (`mainwindow.md`)

## 1. Overview
`MainWindow` currently handles multiple responsibilities including top-level UI layout, frameless window resizing, global keyboard shortcut dispatching, and cross-panel signal coordination.
This plan refactors `MainWindow` into dedicated modules:
1. `GlobalShortcutController`: Intercepts and dispatches global keyboard shortcuts (e.g. `Space` for QuickLook, `Ctrl+F` for Search, `F2` for Rename).
2. `PanelMediator`: Mediates signal interactions between `ContentPanel`, `MetaPanel`, `FavoritePanel`, and `NavPanel`.
3. `MainWindow`: Retains only shell UI initialization, QSS stylesheet loading, and layout management.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/GlobalShortcutController.h`
- `src/ui/GlobalShortcutController.cpp`
- `src/ui/PanelMediator.h`
- `src/ui/PanelMediator.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### `CMakeLists.txt`
```git
<<<<<<< SEARCH
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
=======
    src/ui/GlobalShortcutController.h
    src/ui/GlobalShortcutController.cpp
    src/ui/PanelMediator.h
    src/ui/PanelMediator.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
>>>>>>> REPLACE
```

### `src/ui/MainWindow.h`
```git
<<<<<<< SEARCH
    // 全局快捷键与手势响应
    void setupShortcuts();
=======
    // 由 PanelMediator 与 GlobalShortcutController 分管
    void setupShortcuts();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Steps
1. Launch application and verify top-level UI rendering and QSS styles.
2. Test global shortcuts (`Ctrl+F`, `F2`, `Space`) through `GlobalShortcutController`.
3. Verify seamless cross-panel updates (e.g. selection in `ContentPanel` driving `MetaPanel`) managed via `PanelMediator`.
