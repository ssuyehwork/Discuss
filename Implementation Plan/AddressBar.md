# Implementation Plan - AddressBar (Monochrome SVG Icons)

## 1. Overview
This implementation plan specifies the changes required to ensure all context menu actions in `AddressBar.cpp` (breadcrumb node right-click menu) use neutral monochrome (`#EEEEEE`) SVG icons.

---

## 2. Modified Files List
- `src/ui/AddressBar.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in Address Bar Context Menu (`src/ui/AddressBar.cpp`)

```
<<<<<<< SEARCH
        bool isFav = FavoriteDao::containsPath(nativePath);
        QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#e74c3c")) : UiHelper::getIcon("star_filled", QColor("#FDB70A"));
        QAction* actFavToggle = menu.addAction(favIcon, isFav ? "取消收藏" : "添加至收藏夹");
        QAction* actCopyPath = menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE")), "复制完整路径");
=======
        bool isFav = FavoriteDao::containsPath(nativePath);
        QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#EEEEEE"), 18) : UiHelper::getIcon("star_filled", QColor("#EEEEEE"), 18);
        QAction* actFavToggle = menu.addAction(favIcon, isFav ? "取消收藏" : "添加至收藏夹");
        QAction* actCopyPath = menu.addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制完整路径");
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Verification**:
   - Right-click on any path node in the top address bar breadcrumb.
   - Verify that all context menu actions display neutral monochrome (`#EEEEEE`) SVG icons.
