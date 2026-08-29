# Implementation Plan - FavoritePanel-10

This implementation plan unifies all default folder icon keys in `FavoritePanel.cpp` to solid folder icon `folder_filled`, eliminating legacy hollow `folder` icons.

## 1. Overview
- **Unify Default Icon Key**: Change default fallback icon key in `loadFavorites()` and `addFavoriteItem()` from hollow `"folder"` to solid `"folder_filled"`.
- **Automatic Migration**: Convert legacy `"folder"` data to `"folder_filled"` at load time.

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```diff
<<<<<<< SEARCH
        QIcon icon = UiHelper::getIcon(rec.iconKey.isEmpty() ? "folder" : rec.iconKey, itemColor, 18);
        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
        item->setData(rec.path, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
=======
        QString iconKey = rec.iconKey.isEmpty() ? "folder_filled" : rec.iconKey;
        if (iconKey == "folder") iconKey = "folder_filled";

        QIcon icon = UiHelper::getIcon(iconKey, itemColor, 18);
        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
        item->setData(rec.path, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    FavoriteDao::addFavorite(cleanPath, "folder", "#FDB70A");

    QIcon icon = UiHelper::getIcon("folder", QColor("#FDB70A"), 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
    item->setData(cleanPath, Qt::UserRole + 1);
    item->setData("folder", Qt::UserRole + 2);
    item->setData("#FDB70A", Qt::UserRole + 3);
=======
    FavoriteDao::addFavorite(cleanPath, "folder_filled", "#FDB70A");

    QIcon icon = UiHelper::getIcon("folder_filled", QColor("#FDB70A"), 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
    item->setData(cleanPath, Qt::UserRole + 1);
    item->setData("folder_filled", Qt::UserRole + 2);
    item->setData("#FDB70A", Qt::UserRole + 3);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project using CMake:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. Test `FavoritePanel` loading:
   - Verify newly added and default favorite folders display solid golden `folder_filled` SVG icons.
