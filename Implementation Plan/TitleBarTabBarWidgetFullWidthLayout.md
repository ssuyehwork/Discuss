# Implementation Plan - TitleBar TabBarWidget Full-Width Elastic Stretch Layout

## 1. Overview
This implementation plan adjusts the layout in `TitleBarWidget` so that `TabBarWidget` stretches across all available horizontal space up to `SizeSlider`:
- **Problem**: Previously, `m_tabBar` was added to `m_layout` (`QHBoxLayout`) with stretch factor 0, followed by `m_layout->addStretch()`. As a result, `TabBarWidget` remained fixed at a narrow width (~324px for two tabs), leaving blank unused space in the middle of the title bar instead of extending directly to the scale slider (`m_sizeSlider`).
- **Fix**: Updates `TitleBarWidget::initUi` to add `m_tabBar` with a stretch factor of `1` (`m_layout->addWidget(m_tabBar, 1)`), and removes the intermediate `m_layout->addStretch()`. This allows `TabBarWidget` to dynamically fill the entire remaining horizontal area between `m_logoLabel` and `m_sizeSlider`.

---

## 2. Modified Files List
- `src/ui/TitleBarWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Changes in `src/ui/TitleBarWidget.cpp`

```
<<<<<<< SEARCH
    m_tabBar = new TabBarWidget(this, hoverFilter);
    m_layout->addWidget(m_tabBar);
    m_layout->addStretch();
=======
    m_tabBar = new TabBarWidget(this, hoverFilter);
    m_layout->addWidget(m_tabBar, 1);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. Verify syntax in `src/ui/TitleBarWidget.cpp`.
2. Ensure `CMakeLists.txt` builds `src/ui/TitleBarWidget.cpp`.
3. Verify visual layout in UI / window resize:
   - `TabBarWidget` fills the horizontal space between the logo on the left and `m_sizeSlider` on the right.
   - When resizing the main window horizontally, `TabBarWidget` stretches and shrinks elastically while keeping right-aligned title bar control buttons (`m_sizeSlider`, `m_btnViewMenu`, etc.) anchored to the right.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- **SSOT Reuse**: Directly reuses existing `QHBoxLayout::addWidget` parameter signatures without introducing extra wrapper widgets or custom layout managers.
- **Anti-Redundancy**: Directly removes the redundant `addStretch()` spacer in `m_layout`.

---

## 6. Header API Signature Verification

| Class / Struct | Method / Member Signature | Status |
| :--- | :--- | :--- |
| `TitleBarWidget` | `TabBarWidget* m_tabBar` | Existing member preserved |
| `TitleBarWidget` | `QHBoxLayout* m_layout` | Existing member preserved |

---

## 7. Header Inclusion Chain & Type Completeness Check

- Verified `#include "TitleBarWidget.h"` and `#include "TabBarWidget.h"` in `src/ui/TitleBarWidget.cpp`.
