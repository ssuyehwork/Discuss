# PanelLayoutManager - Window Absolute Minimum Width Update Plan (PanelLayoutManager-3.md)

## 1. Overview
Updates the global absolute minimum window width (`kWindowAbsoluteMinWidth`) in `PanelLayoutManager.h` from `475px` to `710px`. 

This guarantees that the application window retains a minimum physical width of `710px` (corresponding to the standard 3-column physical width calculation: $3 \times 230\text{px} + 10\text{px} + 10\text{px} = 710\text{px}$), ensuring ample horizontal space for top controls (address bar, search input, actions) and Miller Columns (Column View) drill-down views even when side panels are hidden.

## 2. Modified Files List
- `src/ui/PanelLayoutManager.h`

## 3. Detailed Line-by-Line Changes

### `src/ui/PanelLayoutManager.h`
Update `kWindowAbsoluteMinWidth` constant definition.

```git
<<<<<<< SEARCH
    static constexpr int kWindowAbsoluteMinWidth = 475; // 顶栏与导航栏防重叠物理绝对下限
=======
    static constexpr int kWindowAbsoluteMinWidth = 710; // 顶栏与三栏视界物理绝对下限 (3 * 230px + 10px + 10px = 710px)
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation Verification**:
   ```bash
   cmake --build build
   ```
2. **Functional Verification**:
   - Run QuarkMeta and collapse all side panels (leaving only `ContentPanel`).
   - Try resizing `MainWindow` to its minimum possible width.
   - Verify that the window cannot be resized narrower than `710px`.
