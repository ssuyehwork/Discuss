# Implementation Plan - SearchController-4.md

## 1. Overview
This implementation plan outlines replacing the search icon key in `src/ui/SearchController.cpp` from `seach-3` to `seach-7`.
The `seach-7` icon exists in `src/ui/SvgIcons.h` (`s_svgIconMap`) and provides the updated visual style for the global search button in `SearchController`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/SearchController-4.md`.

## 2. Modified Files List
- `src/ui/SearchController.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/SearchController.cpp`
```diff
<<<<<<< SEARCH
    m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#CCCCCC"), 16));
=======
    m_btnSearch->setIcon(UiHelper::getIcon("seach-7", QColor("#CCCCCC"), 16));
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    if (watched == m_btnSearch) {
        if (event->type() == QEvent::Enter) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-3", Qt::white, 16));
        } else if (event->type() == QEvent::Leave) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#CCCCCC"), 16));
        }
    }
=======
    if (watched == m_btnSearch) {
        if (event->type() == QEvent::Enter) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-7", Qt::white, 16));
        } else if (event->type() == QEvent::Leave) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-7", QColor("#CCCCCC"), 16));
        }
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure CMake build directory if needed.
2. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
3. Run `QuarkMeta` and verify that the global search bar button shows the new `seach-7` SVG icon style, and that hovering over the button smoothly switches to the highlighted white icon (`seach-7`).

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: `UiHelper::getIcon("seach-7", color, size)` is used to retrieve the SVG icon, adhering to the single truth source for SVG icons in `SvgIcons.h`.
- **Zero Redundancy**: No new custom icon loading logic or inline styling is introduced.

## 6. Header API Signature Verification
- `UiHelper::getIcon(const QString& name, const QColor& color = QColor("#FFFFFF"), int size = 16)` in `src/ui/UiHelper.h` (Verified physical signature).
- `SvgIcons::s_svgIconMap` in `src/ui/SvgIcons.h` contains `"seach-7"`.

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "UiHelper.h"` is already present in `SearchController.cpp`.
- `QColor` and `Qt::white` are completely declared and accessible in `SearchController.cpp`.
- No header inclusion removal or breakage.
