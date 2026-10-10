# Implementation Plan - AddCopyNameActionToFavorites.md

## Overview
Add a "复制名称" (Copy Name) action to the right-click context menu of items in `FavoritePanel` using the existing SSOT method `ContextMenuFactory::buildCopyNameAction`.

## Solution Strategy
In `FavoritePanel::onFavoriteContextMenu` (`src/ui/FavoritePanel.cpp`):
1. Immediately after `ContextMenuFactory::buildIconPickerMenu(...)` (or before "重命名"), call `ContextMenuFactory::buildCopyNameAction(&menu, {path}, this)`.
2. For virtual categories or real file/folder items, `buildCopyNameAction` will extract the name or path's file name and copy it to the clipboard using the SSOT `ContextMenuFactory::copyNamesToClipboard` helper.

## Modified Files List
- `src/ui/FavoritePanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
    // 3. 重命名 (直接唤起行内编辑框)
    QAction* renameAct = menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE")), "重命名");
=======
    // 复制名称 (SSOT 入口)
    ContextMenuFactory::buildCopyNameAction(&menu, {path}, this);

    // 3. 重命名 (直接唤起行内编辑框)
    QAction* renameAct = menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE")), "重命名");
>>>>>>> REPLACE
```

## Build & Verification
1. Open FavoritePanel.
2. Right-click any favorite folder or file.
3. Verify "复制名称" action appears in the menu with icon and text.
4. Click "复制名称" and verify clipboard content and overlay prompt.
