# Implementation Plan - NavPanel (Monochrome SVG Icons)

## 1. Overview
This implementation plan specifies the changes required to ensure all context menu actions in `NavPanel.cpp` (trash root context menu, folder/drive favorite context menu) use neutral monochrome (`#EEEEEE`) SVG icons.

---

## 2. Modified Files List
- `src/ui/NavPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in Navigation Context Menu (`src/ui/NavPanel.cpp`)

```
<<<<<<< SEARCH
    if (path == "trash_root") {
        QMenu menu(this);
        UiHelper::applyMenuStyle(&menu);

        QAction* actRestore = menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原全部");
        QAction* actEmpty = menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "清空回收站");
=======
    if (path == "trash_root") {
        QMenu menu(this);
        UiHelper::applyMenuStyle(&menu);

        QAction* actRestore = menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "还原全部");
        QAction* actEmpty = menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "清空回收站");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    bool isFav = FavoriteDao::containsPath(path);
    QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#e74c3c"), 18) : UiHelper::getIcon("star_filled", QColor("#FDB70A"), 18);
    QAction* actFavorite = menu.addAction(favIcon, isFav ? "从收藏夹移除" : "添加至收藏夹");
=======
    bool isFav = FavoriteDao::containsPath(path);
    QIcon favIcon = isFav ? UiHelper::getIcon("close", QColor("#EEEEEE"), 18) : UiHelper::getIcon("star_filled", QColor("#EEEEEE"), 18);
    QAction* actFavorite = menu.addAction(favIcon, isFav ? "从收藏夹移除" : "添加至收藏夹");
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Verification**:
   - Right-click on the "回收站" item and folder/drive items in `NavPanel`.
   - Verify that all context menu actions display neutral monochrome (`#EEEEEE`) icons.
