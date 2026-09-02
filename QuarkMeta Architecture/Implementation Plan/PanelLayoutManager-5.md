# Implementation Plan - PanelLayoutManager-5

This implementation plan enforces a hard physical minimum window width limit of 475px across all panel visibility states to prevent title bar controls, search input, and address bar from overlapping and clipping when hiding multiple side panels.

## 1. Overview & Root Cause Analysis
- **Root Cause**: When 4 side panels are hidden, `PanelLayoutManager::updateDynamicMinimumSize()` dynamically calculated a minimum width of ~240px (1 visible panel). This value is far below the physical width sum required by title bar and navigation bar components (~465px~475px), causing control overlapping, clipping, and layout bleed.
- **Physical Safety Lock**: Define `kWindowAbsoluteMinWidth = 475` in `PanelLayoutManager.h` and clamp calculated minimum window width with `std::max(kWindowAbsoluteMinWidth, calculatedMinW)` in `PanelLayoutManager.cpp` and `MainWindow.cpp`.

## 2. Modified Files List
- `src/ui/PanelLayoutManager.h`
- `src/ui/PanelLayoutManager.cpp`
- `src/ui/MainWindow.cpp`

## 3. Detailed Changes Plan

### `src/ui/PanelLayoutManager.h`
Add constant:
```cpp
static constexpr int kWindowAbsoluteMinWidth = 475; // 顶栏与导航栏防重叠物理绝对下限
```

### `src/ui/PanelLayoutManager.cpp`
Update `updateDynamicMinimumSize()`:
```cpp
int calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10;
int finalMinW = std::max(kWindowAbsoluteMinWidth, calculatedMinW);
m_mainWindow->setMinimumWidth(finalMinW);
```

### `src/ui/MainWindow.cpp`
Update constructor:
```cpp
setMinimumSize(475, 400);
```

## 4. Verification Routine
1. Verify compilation and symbol definitions.
2. Hide 1, 2, 3, or 4 side panels and drag main window horizontal border to minimum width.
3. Confirm window stops at exactly 475px without title bar or navigation controls overlapping.
