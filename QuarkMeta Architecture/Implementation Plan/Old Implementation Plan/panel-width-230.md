# Implementation Plan - Unify All Side Panel Minimum Widths & Splitter Sizes to 230px (`panel-width-230.md`)

## 1. Overview
This implementation plan standardizes the physical baseline width across all four side panels (`NavPanel`, `FavoritePanel`, `MetaPanel`, `FilterPanel`):
1. **FavoritePanel Minimum Width**: Update `FavoritePanel::setMinimumWidth(200)` to `setMinimumWidth(230)`.
2. **Main Splitter Initial Allocations**: Update default splitter layout sizes in `MainWindow.cpp` from `200 << 200 << 550 << 200 << 200` to `230 << 230 << 550 << 230 << 230`.

---

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FavoritePanel.cpp`
```
<<<<<<< SEARCH
    setMinimumWidth(200);
=======
    setMinimumWidth(230);
>>>>>>> REPLACE
```

### 3.2 `src/ui/MainWindow.cpp`
```
<<<<<<< SEARCH
        sizes << 200 << 200 << 550 << 200 << 200;
=======
        sizes << 230 << 230 << 550 << 230 << 230;
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build application with CMake / Ninja.
2. Launch `MainWindow` and verify all four sidebars (`NavPanel`, `FavoritePanel`, `MetaPanel`, `FilterPanel`) present a uniform 230px baseline width.
