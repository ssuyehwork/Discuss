# TitleBar Icons Remapping Implementation Plan

## 1. Overview
This implementation plan specifies the icon remapping for the view mode controls in `TitleBarWidget.cpp`:
- Replaces the "排列方式" button icon with `write_1.svg` (`"write_1"`).
- Replaces the "自适应" menu item icon with `resize2.svg` (`"resize2"`).
- Replaces the "网格" menu item icon with `extension.svg` (`"extension"`).

## 2. Modified Files List
- `src/ui/TitleBarWidget.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Update View Mode Button and Menu Icons in `TitleBarWidget.cpp`

```
<<<<<<< SEARCH
    m_btnViewMenu = createTitleBtn("grid", "排列方式");
=======
    m_btnViewMenu = createTitleBtn("write_1", "排列方式");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        QAction* actAdaptive = menu.addAction(UiHelper::getIcon("grid", QColor("#EEEEEE"), 18), "自适应(A)");
        QAction* actGrid = menu.addAction(UiHelper::getIcon("grid", QColor("#EEEEEE"), 18), "网格(G)");
        QAction* actList = menu.addAction(UiHelper::getIcon("list_ul", QColor("#EEEEEE"), 18), "列表(L)");
=======
        QAction* actAdaptive = menu.addAction(UiHelper::getIcon("resize2", QColor("#EEEEEE"), 18), "自适应(A)");
        QAction* actGrid = menu.addAction(UiHelper::getIcon("extension", QColor("#EEEEEE"), 18), "网格(G)");
        QAction* actList = menu.addAction(UiHelper::getIcon("list_ul", QColor("#EEEEEE"), 18), "列表(L)");
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**: Compile using CMake / MSVC build environment.
2. **Visual Verification**:
   - Inspect the TitleBar "排列方式" button: verify it displays the `write_1` icon.
   - Click "排列方式" to open the menu: verify "自适应" displays `resize2` and "网格" displays `extension`.
