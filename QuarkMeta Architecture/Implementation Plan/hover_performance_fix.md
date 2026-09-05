# AddressBar & DriveBar Hover Lag Fix Implementation Plan
(QuarkMeta Architecture/Implementation Plan/hover_performance_fix.md)

---

## 1. Overview

The application experiences severe UI lag / freezing when the user hovers over the AddressBar (Breadcrumb) or DriveBar (Tag Manager).
The investigation revealed three primary root causes:
1. **Unthrottled Hover Animations in `ToolTipOverlay`**: Calling `showText` on every `HoverEnter`/`Enter` creates and starts a `QPropertyAnimation` (150ms fade-in) repeatedly, causing paint events and event loop congestion.
2. **Duplicated & Unfiltered Event Filters**: `AddressBar` installs custom event filters on `m_addressContainer`, `m_breadcrumbBar`, `m_pathEdit`, etc., which compete with the global `HoverEventFilter`.
3. **Tooltip Window Overlay Re-Trigger Loop**: When `ToolTipOverlay` appears right near the cursor (`+15, +15`), cursor movement or boundary checking triggers immediate `HoverLeave` / `HoverEnter` flip-flop loops.

### Solution Strategy
1. **Add Show Debounce/Delay & Remove Animation Flashing**: Introduce a lightweight delay timer (e.g. 150ms) in `ToolTipOverlay` before displaying tooltip text to suppress brief transient hover events, and disable opacity property animation for high-frequency tooltips to keep rendering $O(1)$ and lightweight.
2. **Unify Hover Handling**: Consolidate tooltip triggering into property-based `tooltipText` handled through `HoverEventFilter`, or sanitize `AddressBar::eventFilter` to avoid redundant calls.
3. **Cursor Offset Safety**: Ensure `ToolTipOverlay` respects cursor positioning and screen bounds cleanly without overlapping the hovered control hotspot.

---

## 2. Modified Files List

1. `src/ui/ToolTipOverlay.h` & `src/ui/ToolTipOverlay.cpp`
2. `src/ui/AddressBar.cpp`
3. `src/ui/HoverEventFilter.cpp`

---

## 3. Detailed Line-by-Line Changes

### A. `src/ui/ToolTipOverlay.h` & `src/ui/ToolTipOverlay.cpp`
- Add `QTimer m_showDelayTimer` (150ms debounce timer) so tooltip text only renders when cursor stays still for 150ms.
- Disable opacity animation or make it optional so `showText` executes instantly without queuing background property animation tickers on every pixel movement.

### B. `src/ui/AddressBar.cpp`
- Remove redundant `m_addressContainer->installEventFilter(this)` and `m_breadcrumbBar->installEventFilter(this)` or simplify `AddressBar::eventFilter` to rely on standard `property("tooltipText")`.
- Set `tooltipText` on `BreadcrumbBar` / `AddressBar` container dynamically if needed.

### C. `src/ui/HoverEventFilter.cpp`
- Ensure `HoverEventFilter` handles `HoverEnter` / `HoverLeave` cleanly using `ToolTipOverlay::instance()->showText(...)` with debouncing.

---

## 4. Build & Verification Steps

1. Compile the project with CMake / C++ compiler.
2. Run the application and move the mouse rapidly over the AddressBar, Breadcrumb nodes, Refresh button, and Tag Manager button.
3. Verify that the UI remains completely fluid (60 FPS), with no lagging, frame drops, or freezing.
4. Verify that tooltip text still appears accurately when pausing over controls for >150ms and disappears immediately when leaving.
