# Implementation Plan - SidebarBorderFix.md

## Overview
Fix border overlap issues (double 1px border lines appearing on top and left) in `SidebarContainerWidget` when the "收藏夹" (Favorite) tab is selected, making its border appearance 100% identical to the "库" (Library) tab and other main panels.

## Root Cause Analysis
1. **Outer Container Level (`m_sidebarContainer`)**:
   In `MainWindow.cpp` line 163:
   `m_sidebarContainer = new SidebarContainerWidget(this); m_sidebarContainer->setObjectName("FavoriteContainer");`
   In `style.qss` line 108:
   `#SidebarContainer, #FavoriteContainer, #ListContainer, #EditorContainer, #MetadataContainer, #FilterContainer { border: 1px solid #333333; ... }`
   Therefore, `m_sidebarContainer` receives an outer border of `1px solid #333333` on ALL 4 sides (top, bottom, left, right).

2. **Top Line Overlap (Double Top Border)**:
   Inside `SidebarContainerWidget`, `header` is created with `setObjectName("ContainerHeader")`.
   In `style.qss` line 127:
   `#ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; ... }`
   Inside `SidebarContainerWidget`, `m_stackedWidget` contains `FavoritePanel` and `LibraryPanel`.
   `FavoritePanel` has `setObjectName("FavoriteContainer")` in constructor (`FavoritePanel.cpp` line 107).
   Because `FavoritePanel` matches `#FavoriteContainer`, it draws its own 4-side `1px solid #333333` border.
   The TOP border of `FavoritePanel` sits directly against `ContainerHeader`'s `border-bottom`, creating a **2px thick line** between the header tab bar and the panel content!
   `LibraryPanel` has `setObjectName("LibraryContainer")`, which is NOT in `#FavoriteContainer` QSS, so `LibraryPanel` has `border: none` (transparent), drawing only 1px line from `ContainerHeader`'s `border-bottom`.

3. **Left Line Overlap (Double Left Border)**:
   `m_sidebarContainer` (`#FavoriteContainer`) draws 1px left border.
   `FavoritePanel` (`#FavoriteContainer`) ALSO draws 1px left border inside `m_sidebarContainer`!
   Because `FavoritePanel` fills `m_stackedWidget` inside `m_sidebarContainer` without margin/padding, `FavoritePanel`'s left border overlaps directly with `m_sidebarContainer`'s left border, creating a **2px thick line** on the left!
   `LibraryPanel` (`#LibraryContainer`) does NOT draw a border, so in Library tab mode, only `m_sidebarContainer`'s left border is drawn (1px).

4. **Summary of Overlapping Layers**:
   - **Top Overlap**: `ContainerHeader` (border-bottom: 1px) + `FavoritePanel` (border-top: 1px) = 2px.
   - **Left Overlap**: `m_sidebarContainer` (border-left: 1px) + `FavoritePanel` (border-left: 1px) = 2px.
   - **Right & Bottom Overlap**: `m_sidebarContainer` + `FavoritePanel` (double lines on right and bottom as well).

## Solution Strategy
1. **Unify Panel Object Names & Outer Responsibility**:
   - `m_sidebarContainer` (the outer host container widget in `MainWindow`) is the single entity responsible for drawing the 4-side outer frame (`1px solid #333333`).
   - Change `m_sidebarContainer->setObjectName("FavoriteContainer")` in `MainWindow.cpp` to `m_sidebarContainer->setObjectName("SidebarContainerWidget")` or give `#SidebarContainerWidget` the outer border rule in `style.qss`.
   - Alternatively, keep `m_sidebarContainer` matching `#SidebarContainerWidget` (or `#SidebarContainer`), so it draws the outer border.
2. **Remove Double Borders from Inner Panels (`FavoritePanel` & `LibraryPanel`)**:
   - `FavoritePanel` (`FavoriteContainer`) and `LibraryPanel` (`LibraryContainer`) are inner children embedded inside `SidebarContainerWidget`'s `m_stackedWidget`. They should NOT draw outer 4-side borders.
   - Remove `#FavoriteContainer` from `#SidebarContainer, #FavoriteContainer, #ListContainer...` outer border list in `style.qss`, OR explicitly define `#FavoriteContainer, #LibraryContainer { border: none; background: transparent; }`.
3. **Container Header Divider (`ContainerHeader`)**:
   - `ContainerHeader` continues to draw `border-bottom: 1px solid #333333;`, which serves as the clean 1px horizontal separator line between the tab bar and the stacked content panels for BOTH "收藏夹" and "库" tabs.
4. **Preserve All Other Main Containers**:
   - `#NavPanel` (`SidebarContainer`), `#ContentPanel` (`EditorContainer`), `#MetaPanel` (`MetadataContainer`), `#FilterPanel` (`FilterContainer`) retain their current `1px solid #333333` outer borders without any changes.

## Modified Files List
- `src/ui/MainWindow.cpp`
- `src/ui/SidebarContainerWidget.cpp`
- `resources/style.qss`

## Detailed Line-by-Line Changes

### 1. `src/ui/MainWindow.cpp`
Change object name of `m_sidebarContainer` to `"SidebarContainerWidget"` (it was previously named `"FavoriteContainer"` by mistake).

```
<<<<<<< SEARCH
    m_sidebarContainer = new SidebarContainerWidget(this); m_sidebarContainer->setObjectName("FavoriteContainer");
=======
    m_sidebarContainer = new SidebarContainerWidget(this); m_sidebarContainer->setObjectName("SidebarContainerWidget");
>>>>>>> REPLACE
```

### 2. `resources/style.qss`
Add `#SidebarContainerWidget` to the outer container border selector list so `m_sidebarContainer` draws the clean single 1px outer border.
Ensure `#FavoriteContainer` and `#LibraryContainer` have `border: none;`.

```
<<<<<<< SEARCH
#SidebarContainer, #FavoriteContainer, #ListContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 0px;
    color: #EEEEEE;
    margin: 0px;
    padding: 0px;
}
=======
#SidebarContainer, #SidebarContainerWidget, #ListContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 0px;
    color: #EEEEEE;
    margin: 0px;
    padding: 0px;
}

#FavoriteContainer, #LibraryContainer {
    background-color: #1E1E1E;
    border: none;
    margin: 0px;
    padding: 0px;
}
>>>>>>> REPLACE
```

## Build & Verification
1. Clean build of QuarkMeta.
2. Verify "库" (Library) tab: 1px top line under tab bar, 1px left, right, bottom outer border.
3. Verify "收藏夹" (Favorite) tab: 1px top line under tab bar, 1px left, right, bottom outer border.
4. Switch tabs back and forth: confirm ZERO shifting, ZERO line thickening, 100% visual consistency.
