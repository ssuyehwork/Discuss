# ColumnItemDelegate In-Place Renaming Unification Implementation Plan

This implementation plan details the precise changes required to harmonize the in-place renaming editor experience in `ColumnItemDelegate` with QuarkMeta's application-wide standards (`QuarkMeta-Architecture-Planning.md`).

## Overview
Currently, `ColumnItemDelegate` relies on Qt's default `QStyledItemDelegate::createEditor` (which creates a standard `QLineEdit`). This causes three major issues:
1. **Lack of Extension Protection**: When renaming a file in Column View, the entire filename including extension is selected, leading to accidental deletion of file extensions.
2. **Native Right-Click Menu Violation**: Default `QLineEdit` presents Windows/Qt default context menus instead of QuarkMeta's dark-themed exclusive context menu.
3. **Inconsistent Navigation Keys**: Up/Down and Left/Right key behaviors during in-place editing do not match the smart cursor positioning and navigation guards present in Grid/Tree views.

This plan integrates `FileNameLineEdit` into `ColumnItemDelegate` and handles precise geometry and model data synchronization.

---

## Modified Files List
- `src/ui/ColumnItemDelegate.h`

---

## Detailed Line-by-Line Changes

### File: `src/ui/ColumnItemDelegate.h`

In `src/ui/ColumnItemDelegate.h`, `createEditor`, `updateEditorGeometry`, `setEditorData`, `setModelData`, and `eventFilter` are implemented to integrate `FileNameLineEdit` and standardize keyboard navigation and cursor positioning.

---

## Build & Verification Steps

1. **CMake Build Verification**:
   ```bash
   cmake --build build --config Debug
   ```
2. **Functional Verification**:
   - Launch QuarkMeta application and switch to Column View mode (Miller Columns).
   - Select a file in any active column and press `F2` (or click "Rename" from right-click context menu).
   - Verify that:
     - Only the base filename is selected (extension is protected and unselected).
     - Up/Down direction keys do not cause view selection drift while editing.
     - Pressing Left arrow moves cursor to beginning of filename; pressing Right arrow moves cursor directly before the extension dot.
     - Right-clicking inside the active in-place edit box invokes QuarkMeta's exclusive dark context menu.
