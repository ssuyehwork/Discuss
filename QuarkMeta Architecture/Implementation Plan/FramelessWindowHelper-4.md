# FramelessWindowHelper WM_NCHITTEST Fallback Implementation Plan

## Overview
Previously, `FramelessWindowHelper::handleNativeEvent` unconditionally intercepted all `WM_NCHITTEST` messages in the client area, returning `*result = HTCLIENT; return true;`. Unconditionally consuming `WM_NCHITTEST` for the client area bypassed default Qt / Win32 non-client event handling, which caused `MainWindow` to lose native edge resizing, titlebar dragging, and cursor updates after closing child overlays or dialogs (`TagSelectorOverlay` / `TagManagerDialog`).

This plan updates `WM_NCHITTEST` in `FramelessWindowHelper` to return `false` when the hit test is neither on an 8px resize edge (`HTTOP`, `HTLEFT`, etc.) nor on the titlebar (`HTCAPTION`), allowing Qt and Win32 `DefWindowProcW` to handle normal client area hit testing and cursor updates natively.

## Modified Files List
- `src/ui/FramelessWindowHelper.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FramelessWindowHelper.cpp`
```diff
<<<<<<< SEARCH
        *result = HTCLIENT;
        return true;
    }
=======
        return false;
    }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Apply the diff to `src/ui/FramelessWindowHelper.cpp`.
2. Build and verify:
   ```bash
   cmake -B build
   cmake --build build
   ```
3. Test opening and closing `TagManagerDialog` and `TagSelectorOverlay`, and verify that `MainWindow` preserves edge resizing (double-headed arrow cursors), titlebar dragging, and active widget cursor updates upon window closure.
