# Implementation Plan - FavoritePanel-8

This implementation plan reflects the user's explicit instruction to simplify `FavoritePanel.cpp` tooltip handling. It removes `ToolTipOverlay` and `HoverEventFilter` dependencies for the icon grid buttons in `FavoritePanel`, restoring standard `btn->setToolTip(label)`.

## 1. Overview
- **Revert Custom ToolTip**: Per explicit user directive, remove `ToolTipOverlay` and `HoverEventFilter` from `FavoritePanel.cpp`.
- **Direct `setToolTip` Usage**: Set `btn->setToolTip(label)` directly on the 10 grid icon buttons.

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```diff
<<<<<<< SEARCH
#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ColorPicker.h"
#include "HoverEventFilter.h"
#include "ToolTipOverlay.h"
=======
#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ColorPicker.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
            btn->setIcon(UiHelper::getIcon(iconKey, catColor, 18));
            btn->setIconSize(QSize(18, 18));
            btn->setProperty("tooltipText", label);
            btn->installEventFilter(new HoverEventFilter(btn));

            pickerLayout->addWidget(btn, row, col);
=======
            btn->setIcon(UiHelper::getIcon(iconKey, catColor, 18));
            btn->setIconSize(QSize(18, 18));
            btn->setToolTip(label);

            pickerLayout->addWidget(btn, row, col);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project using CMake:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. Test right-click in `FavoritePanel`:
   - Verify icon grid buttons utilize `btn->setToolTip(label)` without error.
