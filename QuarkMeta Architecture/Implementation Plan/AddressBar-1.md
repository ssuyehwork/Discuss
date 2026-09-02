# AddressBar & Navigation Bar Layout Refactoring Implementation Plan (AddressBar-1.md)

## 1. Core Objectives & Architectural Answers

### Question 1: Single Source of Truth (SSOT)
- **Navigation Bar Layout State**: `MainWindow` / `m_navBarWidget` controls the responsive 1-row vs. 2-row layout mode based on container width thresholds.
- **Path Truncation & Nodes**: `BreadcrumbBar` remains the SSOT for rendering path levels and calculating visible nodes versus truncated trailing nodes ("...").
- **Hover Path Tooltip**: `AddressBar` provides hover event interception and delegates to `ToolTipOverlay::instance()` to render the full, un-truncated absolute physical path.

### Question 2: Encapsulation Integrity
- `AddressBar` encapsulates internal switching between `BreadcrumbBar` and `QLineEdit`.
- `BreadcrumbBar` manages level buttons and dynamic layout truncation without exposing raw child widget structures.
- `MainWindow` manages top-level navigation container (`m_navBarWidget`) geometry and responsive rearrangement without altering `AddressBar` or `SearchController` internal behaviors.

### Question 3: Root Cause vs. Surface Phenomenon
- **Surface Phenomenon**: Fixed-height single-row navigation bar causes `SearchEdit` and `AddressBar` to squeeze each other on narrow windows, clipping controls or overflowing breadcrumb buttons.
- **Root Cause**: `m_navBarLayout` used a static horizontal box layout with fixed height `42px`, lacking dynamic layout restructuring or responsive line wrapping under narrow window widths. `BreadcrumbBar` added all path level buttons into `m_layout` with `addStretch()` without clipping or truncating overflow items when horizontal width was insufficient.

---

## 2. Technical Implementation Details

### Rule 1: Responsive Navigation Bar Line Wrap (`MainWindow` / `m_navBarWidget`)
- In `MainWindow.cpp`, implement dynamic layout adjustment or custom layout event handling on `m_navBarWidget` (or `resizeEvent`):
  - When total available width for navigation controls is insufficient (e.g. width < threshold, or when `m_addressBar` width drops below minimum effective width):
    - **Row 1**: `[Back] [Forward] [Up] [AddressBar]` (AddressBar expands horizontally to take 100% remaining row 1 space).
    - **Row 2**: `[SearchController]` (Independent row, width expands 100% to fill container width).
    - Container height dynamically adjusts (e.g. `78px` in 2-row mode vs `42px` in 1-row mode).
  - When window width is sufficient:
    - Automatically restores to single-row side-by-side layout: `[Back] [Forward] [Up] [AddressBar] [SearchController]`, with `m_navBarWidget` height set back to `42px`.

### Rule 2: Head-Retaining Path Truncation (`BreadcrumbBar`)
- In `BreadcrumbBar.cpp`, update `setPath` and resize event/layout recalculation logic:
  - Keep drive/root and initial head path components visible.
  - Calculate total width required by level buttons. If visible width is exceeded, retain the head items (e.g., `C:\` or `此电脑` + first N folders) and append a trailing `...` button / indicator.
  - Clicking `...` or blank area switches to `QLineEdit` path editing mode.

### Rule 3: Hover ToolTipOverlay (`AddressBar`)
- In `AddressBar.cpp`:
  - Install event filter on `m_breadcrumbBar` and `m_addressContainer` for `QEvent::HoverEnter` / `QEvent::Enter` and `QEvent::HoverLeave` / `QEvent::Leave`.
  - On hover enter, query `m_currentPath` (converting `computer://` to "此电脑" or native physical path) and invoke `ToolTipOverlay::instance()->showText(QCursor::pos(), fullPhysicalPath, 0)`.
  - On hover leave or mouse click, call `ToolTipOverlay::hideTip()`.

---

## 3. Implementation Plan Self-Check Routine (Section 5.6)

1. **Public Contract Preservation**: All existing public methods (`AddressBar::setPath`, `AddressBar::currentPath`, `BreadcrumbBar::setPath`) and signals (`pathChanged`, `refreshRequested`) are preserved without signature modification.
2. **Code Search Accuracy**: Verified all usages of `m_navBarWidget`, `AddressBar`, `BreadcrumbBar`, and `SearchController` across `MainWindow.cpp`, `AddressBar.cpp`, and `BreadcrumbBar.cpp`.
3. **CMakeLists Registration**: No new source or header files created; modifying existing `src/ui/AddressBar.h/cpp`, `src/ui/BreadcrumbBar.h/cpp`, `src/ui/MainWindow.h/cpp`, `src/ui/SearchController.h/cpp`.
4. **File Naming Rule Enforcement**: Implementation plan saved directly to `QuarkMeta Architecture/Implementation Plan/AddressBar-1.md`.

---
