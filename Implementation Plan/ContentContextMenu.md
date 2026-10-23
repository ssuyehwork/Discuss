# Implementation Plan - ContentContextMenu

## 1. Overview
This implementation plan specifies the exact changes required to equip **all context menu items** in `ContentContextMenu.cpp` with semantically matching SVG icons, ensuring 0% unadorned text actions and strict adherence to the global 10px icon-text spacing and dark theme rules.

The plan registers required new SVG icon keys in `src/ui/SvgIcons.h` and updates all action/sub-menu creation calls in `src/ui/controllers/ContentContextMenu.cpp`.

---

## 2. Modified Files List
- `src/ui/SvgIcons.h`
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 New Semantic SVG Icons in `src/ui/SvgIcons.h`

Add missing semantic SVG icons (`open`, `launch`, `folder_search`, `paste`, `cut`, `edit`, `link`, `more_horizontal`, `shield`, `lock`, `unlock`, `key`, `sort`, `plus_circle`, `paste_tag`, `repeat`) into the `s_svgMap` registry in `src/ui/SvgIcons.h`:

```
<<<<<<< SEARCH
        {"archive", R"svg(<?xml version="1.0" encoding="utf-8"?><!-- Uploaded to: SVG Repo, www.svgrepo.com, Generator: SVG Repo Mixer Tools --> <svg fill="currentColor" width="800px" height="800px" viewBox="0 0 1920 1920" xmlns="http://www.w3.org/2000/svg"> <path d="M533.333 560v160H240l.008 453.33 209.066 213.34H1470.93l209.06-213.34L1680 720h-293.33V560h352c55.96 0 101.33 45.368 101.33 101.333v511.997h16c35.35 0 64 28.66 64 64V1856c0 35.35-28.65 64-64 64H64c-35.346 0-64-28.65-64-64v-618.67c0-35.34 28.654-64 64-64h16V661.333C80 605.368 125.369 560 181.333 560h352ZM1040 0v958.86l183.43-183.429 113.14 113.138L960 1265.14 583.431 888.569l113.138-113.138L880 958.86V0h160Z"/> </svg>)svg"},
=======
        {"archive", R"svg(<?xml version="1.0" encoding="utf-8"?><!-- Uploaded to: SVG Repo, www.svgrepo.com, Generator: SVG Repo Mixer Tools --> <svg fill="currentColor" width="800px" height="800px" viewBox="0 0 1920 1920" xmlns="http://www.w3.org/2000/svg"> <path d="M533.333 560v160H240l.008 453.33 209.066 213.34H1470.93l209.06-213.34L1680 720h-293.33V560h352c55.96 0 101.33 45.368 101.33 101.333v511.997h16c35.35 0 64 28.66 64 64V1856c0 35.35-28.65 64-64 64H64c-35.346 0-64-28.65-64-64v-618.67c0-35.34 28.654-64 64-64h16V661.333C80 605.368 125.369 560 181.333 560h352ZM1040 0v958.86l183.43-183.429 113.14 113.138L960 1265.14 583.431 888.569l113.138-113.138L880 958.86V0h160Z"/> </svg>)svg"},
        {"open", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/><polyline points="15 3 21 3 21 9"/><line x1="10" y1="14" x2="21" y2="3"/></svg>)svg"},
        {"launch", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/><polyline points="15 3 21 3 21 9"/><line x1="10" y1="14" x2="21" y2="3"/></svg>)svg"},
        {"folder_search", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/><circle cx="11" cy="13" r="3"/><line x1="13.1" y1="15.1" x2="16" y2="18"/></svg>)svg"},
        {"paste", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 4h2a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h2"/><rect x="8" y="2" width="8" height="4" rx="1" ry="1"/></svg>)svg"},
        {"cut", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="6" cy="6" r="3"/><circle cx="6" cy="18" r="3"/><line x1="20" y1="4" x2="8.12" y2="11.88"/><line x1="14.47" y1="14.48" x2="20" y2="20"/><line x1="8.12" y1="12.12" x2="12" y2="16"/></svg>)svg"},
        {"edit", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>)svg"},
        {"link", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"/><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"/></svg>)svg"},
        {"more_horizontal", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="1"/><circle cx="19" cy="12" r="1"/><circle cx="5" cy="12" r="1"/></svg>)svg"},
        {"shield", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>)svg"},
        {"lock", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>)svg"},
        {"unlock", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 9.9-1"/></svg>)svg"},
        {"key", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 2l-2 2m-7.61 7.61a5.5 5.5 0 1 1-7.778 7.778 5.5 5.5 0 0 1 7.778-7.778zm0 0L15.5 7.5m0 0l3 3L22 7l-3-3m-3.5 3.5L19 4"/></svg>)svg"},
        {"sort", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="12" y1="5" x2="12" y2="19"/><polyline points="19 12 12 19 5 12"/></svg>)svg"},
        {"paste_tag", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z"/><line x1="7" y1="7" x2="7.01" y2="7"/></svg>)svg"},
        {"repeat", R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="17 1 21 5 17 9"/><path d="M3 11V9a4 4 0 0 1 4-4h14"/><polyline points="7 23 3 19 7 15"/><path d="M21 13v2a4 4 0 0 1-4 4H3"/></svg>)svg"},
>>>>>>> REPLACE
```

---

### 3.2 Context Menu Actions with Semantic Icons in `src/ui/controllers/ContentContextMenu.cpp`

Update menu creation in `ContentContextMenu.cpp` to attach semantic icons to all actions and sub-menus:

```
<<<<<<< SEARCH
        if (isDriveRoot) {
            menu.addAction("打开")->setData(ContentPanel::ActionOpen);
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);

            bool isFavDrive = FavoriteDao::containsPath(path);
            menu.addAction(isFavDrive ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);

            menu.addSeparator();

            QAction* actItemPaste = menu.addAction("粘贴");
            actItemPaste->setData(ContentPanel::ActionPaste);
            actItemPaste->setEnabled(m_panel->canPaste(path));

            menu.addAction("复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction("复制路径")->setData(ContentPanel::ActionCopyPath);

            QMenu* moreMenuDrive = menu.addMenu("更多");
            UiHelper::applyMenuStyle(moreMenuDrive);
=======
        if (isDriveRoot) {
            menu.addAction(UiHelper::getIcon("open", QColor("#EEEEEE"), 18), "打开")->setData(ContentPanel::ActionOpen);
            menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            menu.addAction(UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", isPinned ? Style::ActiveOrange : QColor("#EEEEEE"), 18), isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);

            bool isFavDrive = FavoriteDao::containsPath(path);
            menu.addAction(UiHelper::getIcon(isFavDrive ? "close" : "star_filled", isFavDrive ? QColor("#e74c3c") : QColor("#FDB70A"), 18), isFavDrive ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);

            menu.addSeparator();

            QAction* actItemPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴");
            actItemPaste->setData(ContentPanel::ActionPaste);
            actItemPaste->setEnabled(m_panel->canPaste(path));

            menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制路径")->setData(ContentPanel::ActionCopyPath);

            QMenu* moreMenuDrive = menu.addMenu(UiHelper::getIcon("more_horizontal", QColor("#EEEEEE"), 18), "更多");
            UiHelper::applyMenuStyle(moreMenuDrive);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   Build the application using CMake:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual & Behavioral Verification**:
   - Right-click on drive roots, files, folders, and empty space in `ContentPanel`.
   - Verify visually that every context menu action contains a semantically accurate icon aligned to the left of the text label.
   - Verify that all sub-menus (`更多`, `移动到`, `外壳保护`, `新建...`, `排序`, `删除`) display an icon on their entrance action.
