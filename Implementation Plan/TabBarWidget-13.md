# Implementation Plan - TabBarWidget Redundant Tab Click Refresh Elimination

## 1. Overview
This implementation plan eliminates redundant disk re-scans and UI refreshes caused by tab clicking:
- **Problem**: Previously, clicking any tab (including the currently active tab) passed `forceNotify = true` to `setCurrentIndex(idx, true)`. This unconditionally emitted `currentTabChanged`, causing `PanelMediator` to invoke `restoreSplitState` and `NavigationService::navigateTo(url)`, which re-scanned the directory from disk, reset scroll positions, and caused unnecessary UI flicker.
- **Fix**: Changes the `tabClicked` handler connection in `TabBarWidget::rebuildTabsUi()` to call `setCurrentIndex(idx, false)`. When clicking the already active tab (`m_currentIndex == idx`), `indexChanged` evaluates to `false`, preventing the emission of `currentTabChanged` and eliminating redundant disk re-scans while maintaining proper tab switching when a different tab is clicked.

---

## 2. Modified Files List
- `src/ui/TabBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes in `src/ui/TabBarWidget.cpp`

```
<<<<<<< SEARCH
        connect(tabItem, &TabItemButton::tabClicked, this, [this](int idx) {
            setCurrentIndex(idx, true);
        });
=======
        connect(tabItem, &TabItemButton::tabClicked, this, [this](int idx) {
            setCurrentIndex(idx, false);
        });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. Verify syntax in `src/ui/TabBarWidget.cpp`.
2. Confirm `CMakeLists.txt` builds `src/ui/TabBarWidget.cpp`.
3. Test tab interaction:
   - Clicking a **different** tab triggers `currentTabChanged` and switches to the target tab path and split state.
   - Clicking the **currently active** tab does not trigger `currentTabChanged`, preserving view state and scroll position without performing a disk re-scan.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT Reuse**: Reuses existing `TabBarWidget::setCurrentIndex` signature and signal mechanisms without introducing duplicate tab event handlers.
- **Anti-Redundancy**: Directly removes redundant `forceNotify` signal broadcasts on active tab clicks.

---

## 6. Header API Signature Verification

| Class / Struct | Method / Member Signature | Status |
| :--- | :--- | :--- |
| `TabBarWidget` | `void setCurrentIndex(int index, bool forceNotify = false)` | Existing method maintained |
| `TabBarWidget` | `void currentTabChanged(int index, const QString& url)` | Existing signal maintained |

---

## 7. Header Inclusion Chain & Type Completeness Check

- Verified `#include "TabBarWidget.h"` in `src/ui/TabBarWidget.cpp`.
