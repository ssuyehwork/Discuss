# Implementation Plan - QuickLookWindow (Monochrome SVG Icons)

## 1. Overview
This implementation plan specifies the exact changes required to equip **all context menu items** in `QuickLookWindow.cpp` with neutral monochrome (`#EEEEEE`) SVG icons.

---

## 2. Modified Files List
- `src/ui/QuickLookWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in QuickLook Context Menu (`src/ui/QuickLookWindow.cpp`)

```
<<<<<<< SEARCH
    // 14 项选项
    QAction* actPrev = menu.addAction("上一个");
    QAction* actNext = menu.addAction("下一个");
    menu.addSeparator();

    QAction* actRotate = menu.addAction("旋转");
    QAction* actFlip = menu.addAction("水平翻转");
    QAction* actOrig = menu.addAction("原始");
    QAction* actFit = menu.addAction("自适应");
    actOrig->setCheckable(true);
    actFit->setCheckable(true);
    bool isFit = m_graphicsView->isFitMode();
    actFit->setChecked(isFit);
    actOrig->setChecked(!isFit);
    menu.addSeparator();

    QAction* actOpenDefault = menu.addAction("用系统默认程序打开");
    QAction* actShowExplorer = menu.addAction("在”资源管理器”中显示");
    menu.addSeparator();

    QAction* actCopy = menu.addAction("复制");
    QAction* actCut = menu.addAction("剪切");
    QAction* actDel = menu.addAction("删除");
    menu.addSeparator();

    QAction* actCopyName = menu.addAction("复制文件名");
    QAction* actCopyPath = menu.addAction("复制路径");
    bool isFav = FavoriteDao::containsPath(m_currentPath);
    QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#e74c3c")) : UiHelper::getIcon("star_filled", QColor("#FDB70A"));
    QAction* actFavorite = menu.addAction(favIcon, isFav ? "取消收藏" : "添加至收藏夹");
    menu.addSeparator();

    QAction* actTextExtSettings = menu.addAction("文本扩展名设置...");
=======
    // 14 项选项
    QAction* actPrev = menu.addAction(UiHelper::getIcon("chevron_right", QColor("#EEEEEE"), 18), "上一个");
    QAction* actNext = menu.addAction(UiHelper::getIcon("chevron_right", QColor("#EEEEEE"), 18), "下一个");
    menu.addSeparator();

    QAction* actRotate = menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "旋转");
    QAction* actFlip = menu.addAction(UiHelper::getIcon("layers", QColor("#EEEEEE"), 18), "水平翻转");
    QAction* actOrig = menu.addAction(UiHelper::getIcon("image_picture", QColor("#EEEEEE"), 18), "原始");
    QAction* actFit = menu.addAction(UiHelper::getIcon("grid", QColor("#EEEEEE"), 18), "自适应");
    actOrig->setCheckable(true);
    actFit->setCheckable(true);
    bool isFit = m_graphicsView->isFitMode();
    actFit->setChecked(isFit);
    actOrig->setChecked(!isFit);
    menu.addSeparator();

    QAction* actOpenDefault = menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开");
    QAction* actShowExplorer = menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在”资源管理器”中显示");
    menu.addSeparator();

    QAction* actCopy = menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "复制");
    QAction* actCut = menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切");
    QAction* actDel = menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "删除");
    menu.addSeparator();

    QAction* actCopyName = menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制文件名");
    QAction* actCopyPath = menu.addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制路径");
    bool isFav = FavoriteDao::containsPath(m_currentPath);
    QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#EEEEEE"), 18) : UiHelper::getIcon("star_filled", QColor("#EEEEEE"), 18);
    QAction* actFavorite = menu.addAction(favIcon, isFav ? "取消收藏" : "添加至收藏夹");
    menu.addSeparator();

    QAction* actTextExtSettings = menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "文本扩展名设置...");
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Verification**:
   - Press Space on any file item to trigger QuickLook, then right-click inside the QuickLook preview window.
   - Confirm that all 14 menu actions feature neutral monochrome (`#EEEEEE`) SVG icons with 100% semantic matching.
