# Implementation Plan - ThemeManager

## 1. Overview
This implementation plan specifies the exact changes required to unify and lock the physical spacing between icons and text across all right-click menus and context menus in QuarkMeta to **10 pixels (10px)**.

By adding the `QMenu::icon { margin-right: 10px; }` subcontrol rule to both the global stylesheet (`resources/style.qss`) and the programmatic menu style applicator (`ThemeManager::applyMenuStyle`), all QMenu instances in the application (including file list context menus, navigation panel menus, title bar menus, and system tray menus) will enforce a consistent 10px gap between the menu icon and the item text.

---

## 2. Modified Files List
- `resources/style.qss`
- `src/ui/ThemeManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes in `resources/style.qss`

```
<<<<<<< SEARCH
QMenu::item {
    background-color: transparent;
    color: #EEEEEE;
    padding: 6px 24px 6px 12px;
    border-radius: 4px;
    font-size: 12px;
}
=======
QMenu::icon {
    margin-right: 10px;
}
QMenu::item {
    background-color: transparent;
    color: #EEEEEE;
    padding: 6px 24px 6px 12px;
    border-radius: 4px;
    font-size: 12px;
}
>>>>>>> REPLACE
```

---

### 3.2 Changes in `src/ui/ThemeManager.cpp`

```
<<<<<<< SEARCH
        "QMenu::item {"
        "   background-color: transparent;"
        "   color: #EEEEEE;"
        "   padding: 6px 24px 6px 12px;"
        "   border-radius: 4px;"
        "   font-size: 12px;"
        "}"
=======
        "QMenu::icon {"
        "   margin-right: 10px;"
        "}"
        "QMenu::item {"
        "   background-color: transparent;"
        "   color: #EEEEEE;"
        "   padding: 6px 24px 6px 12px;"
        "   border-radius: 4px;"
        "   font-size: 12px;"
        "}"
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   Run the build script or CMake build command to ensure the application compiles without error:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Inspection Verification**:
   - Right-click on any file/folder item in `ContentPanel` to trigger `ContentContextMenu`.
   - Right-click on any item in `NavPanel` or `FavoritePanel`.
   - Verify visually that the physical gap between the icon subcontrol and the item text label is locked to 10 pixels across all context menus.
