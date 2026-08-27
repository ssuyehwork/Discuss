# Implementation Plan - FilterPanel Decoupling (`filterpanel.md`)

## 1. Overview
`FilterPanel` currently manages UI widget rendering, `FilterState` filter criteria tracking, and `ScanStats` statistical aggregations.
This plan decouples `FilterPanel` into:
1. `FilterStateModel`: Manages filter state and emits configuration changes.
2. `ScanStatsEngine`: Aggregates file statistics asynchronously.
3. `FilterPanel`: Focuses strictly on UI section rendering and user interactions.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/FilterStateModel.h`
- `src/ui/FilterStateModel.cpp`
- `src/ui/ScanStatsEngine.h`
- `src/ui/ScanStatsEngine.cpp`
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### `CMakeLists.txt`
```git
<<<<<<< SEARCH
    src/ui/FilterPanel.cpp
    src/ui/FilterPanel.h
=======
    src/ui/FilterStateModel.h
    src/ui/FilterStateModel.cpp
    src/ui/ScanStatsEngine.h
    src/ui/ScanStatsEngine.cpp
    src/ui/FilterPanel.cpp
    src/ui/FilterPanel.h
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Verification Steps
1. Open `FilterPanel` and toggle various filters (Ratings, Tags, Thumbnail presence).
2. Verify that `FilterStateModel` accurately updates filter parameters.
3. Verify that `ScanStatsEngine` calculates counts and updates `FilterPanel` labels seamlessly.
