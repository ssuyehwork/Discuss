# Implementation Plan: PresetTagsDialog Tag Container Event Filter Fix

## 1. Overview
This implementation plan fixes an event propagation issue in `PresetTagsDialog` where clicking inside the "自动添加标签" container (`m_tagContainer`) failed to open `TagSelectorOverlay`.

### Key Improvements:
1. **Event Filter Ancestor Check**: Updated `PresetTagsDialog::eventFilter` to listen for mouse click events on `m_tagContainer` and any of its ancestor/child layout widgets, excluding `TagPill` instances.
2. **Window Hierarchy Binding**: Updated `TagSelectorOverlay` constructor call in `onTagContainerClicked` to use `window()` as parent instead of `nullptr`, ensuring popups display correctly over modal dialogs.

---

## 2. Modified Files List
1. `src/ui/PresetTagsDialog.cpp`

---

## 3. Build & Verification Steps
1. Open `PresetTagsDialog` from `FavoritePanel` context menu.
2. Click anywhere inside the red-bordered "自动添加标签" container box.
3. Observe `TagSelectorOverlay` popping up directly below the container.
