# Implementation Plan: PresetTagsDialog Integration & Tag Dictionary Read-Only Safeguard

## 1. Overview
This implementation plan integrates `PresetTagsDialog` ("设置自动标签"对话框) with `FavoritePanel`'s "设置预设标签" context menu action.

### Key Rules & Requirements:
1. **Tag Dictionary Read-Only Safeguard**: The SQLite `tags` global lexicon table remains strictly read-only during tag selection. `TagSelectorOverlay` acts purely as a read-only picker and does not alter the global `tags` database.
2. **Context Menu Action**: Clicking "设置预设标签" in `FavoritePanel` opens `PresetTagsDialog`.
3. **Interactive Tag Container**: Clicking the tag container opens `TagSelectorOverlay`. Selected tags are displayed inside the container as interactive `TagPill` capsules.

---

## 2. Modified Files List
1. `src/ui/PresetTagsDialog.h`
2. `src/ui/PresetTagsDialog.cpp`
3. `src/ui/FavoritePanel.cpp`
4. `CMakeLists.txt`

---

## 3. Build & Verification Steps
1. Right-click a favorite item in `FavoritePanel` -> Click "设置预设标签".
2. Observe `PresetTagsDialog` ("设置自动标签") dialog popping up.
3. Click the empty area in "自动添加标签" container -> Observe `TagSelectorOverlay` appearing.
4. Select tags -> Confirm tags display as `TagPill` capsules inside the dialog container.
5. Confirm SQLite `tags` table remains un-modified.
