# Implementation Plan - FavoritePanel-7

This implementation plan refines `HoverEventFilter` integration in `FavoritePanel.cpp`. It sets the `tooltipText` dynamic property on icon grid buttons and passes `btn` to `HoverEventFilter(btn)`, fixing MSVC C2661 constructor error.

## 1. Overview
- **Fix `HoverEventFilter` Syntax**: `HoverEventFilter` constructor takes a single `QObject* parent` argument. Dynamic property `btn->setProperty("tooltipText", label)` is used for message storage.
- **Strict Compliance**: Replaces native Qt tooltips with `ToolTipOverlay` driven by `HoverEventFilter`.

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```diff
<<<<<<< SEARCH
            btn->setIcon(UiHelper::getIcon(iconKey, catColor, 18));
            btn->setIconSize(QSize(18, 18));
            btn->setToolTip(label);

            pickerLayout->addWidget(btn, row, col);
=======
            btn->setIcon(UiHelper::getIcon(iconKey, catColor, 18));
            btn->setIconSize(QSize(18, 18));
            btn->setProperty("tooltipText", label);
            btn->installEventFilter(new HoverEventFilter(btn));

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
   - Hover over the 10 grid icon buttons and verify tooltips render via `ToolTipOverlay`.
