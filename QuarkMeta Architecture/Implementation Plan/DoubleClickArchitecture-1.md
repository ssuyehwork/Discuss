# Double-Click Core Architecture Implementation Plan (DoubleClickArchitecture-1.md)

## 1. Core Objectives & Architectural Answers

### Question 1: Single Source of Truth (SSOT)
- **Double-Click Dispatching**: `ContentPanel::onDoubleClicked` is the single source of truth for routing double-click actions across all view modes (GridView, JustifiedMode, ListView).
- **Edit Triggers Isolation**: `QAbstractItemView::NoEditTriggers` (or strictly `EditKeyPressed` where double-click triggers are zero) enforces that double-clicking never invokes inline text editing in any view.

### Question 2: Encapsulation Integrity
- `JustifiedView::mouseDoubleClickEvent`: Eliminates `layout.isTextHit(...)` branching to `edit(idx)`. Double-clicking anywhere on a card (cover or text) emits `doubleClicked(idx)` for open/preview.
- `DropTreeView` & `DropJustifiedView`: Set `setEditTriggers(QAbstractItemView::NoEditTriggers)` during initialization.
- `ContentContextMenu` & `ContentKeyHandler`: Function as the exclusive entry points for inline renaming via `F2` and right-click '重命名' menu actions, preserving clean physical isolation between double-click navigation/preview and renaming.

### Question 3: Root Cause vs. Surface Phenomenon
- **Surface Phenomenon**: Double-clicking card text or list view name columns triggered inline renaming or flashed an editor box instead of opening/previewing the asset.
- **Root Cause**: `JustifiedView::mouseDoubleClickEvent` explicitly intercepted `layout.isTextHit(event->pos())` and invoked `edit(idx)`. Views allowed default double-click edit triggers to fire on double-click.

---

## 2. Technical Implementation Details

### Rule 1: Pure Double-Click Open/Preview
- In `JustifiedView.cpp`:
  - Update `JustifiedView::mouseDoubleClickEvent`:
    - On valid index, directly emit `doubleClicked(idx)` regardless of whether text or cover was hit.
    - Completely remove calls to `edit(idx)` inside `mouseDoubleClickEvent`.

### Rule 2: Unified Dispatching in `ContentPanel::onDoubleClicked`
- In `ContentPanel.cpp`:
  - Enforce `m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers)` in `initGridView()`.
  - Enforce `m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers)` in `initListView()`.
  - Verify `ContentPanel::onDoubleClicked` routes folders to `directorySelected(path)` and supported files to `RecordAccess` command + `requestQuickLook(path)`.

### Rule 3: Cross-View Consistency
- Ensure Grid Mode, Justified Mode, and List Mode all route double-click events through `ContentPanel::onDoubleClicked` without triggering delegate editors.

### Rule 4: Renaming Physical Isolation
- Preserve inline renaming strictly via `F2` (`ContentKeyHandler`) and right-click menu (`ContentContextMenu`), which explicitly invoke `view->edit(idx)`.

---

## 3. Implementation Plan Self-Check Routine (Section 5.6)

1. **Public Contract Preservation**: All existing public methods (`ContentPanel::onDoubleClicked`, signals `directorySelected`, `requestQuickLook`) are preserved without signature modification.
2. **Code Search Accuracy**: Verified all usages of `doubleClicked`, `edit(`, and `setEditTriggers` across `ContentPanel.cpp`, `JustifiedView.cpp`, `DropTreeView.cpp`, `ContentKeyHandler.cpp`, and `ContentContextMenu.cpp`.
3. **CMakeLists Registration**: No new source or header files created; modifying existing `src/ui/JustifiedView.cpp` and `src/ui/ContentPanel.cpp`.
4. **File Naming Rule Enforcement**: Implementation plan saved directly to `QuarkMeta Architecture/Implementation Plan/DoubleClickArchitecture-1.md`.

---
