# Implementation Plan - FavoritePanel-13

## Overview
Fix file item rendering in `FavoritePanel`. Previously, file items (`QFileInfo::isDir() == false`) were rendered using folder SVG icons (`folder_filled`) and custom folder colors instead of native system file icons and thumbnails. This change updates `loadFavorites()` and `addFavoriteItem()` in `FavoritePanel.cpp` to check `QFileInfo::isDir()` and load native system icons via `ShellIconManager::getFileIcon(path)` for files, while connecting `IconLoadNotifier::instance().iconLoaded` for asynchronous thumbnail update refresh.

## Modified Files List
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.h`
Include `ShellIconManager.h` if needed, or maintain constructor slot connections for `IconLoadNotifier`.

### `src/ui/FavoritePanel.cpp`
```cpp
<<<<<<< SEARCH
        QIcon icon = UiHelper::getIcon(iconKey, itemColor, 18);
        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
=======
        QIcon icon;
        if (fi.isDir()) {
            icon = UiHelper::getIcon(iconKey, itemColor, 18);
        } else {
            icon = ShellIconManager::getFileIcon(rec.path);
        }
        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    FavoriteDao::addFavorite(cleanPath, "folder_filled", "#FDB70A");

    QIcon icon = UiHelper::getIcon("folder_filled", QColor("#FDB70A"), 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
=======
    FavoriteDao::addFavorite(cleanPath, "folder_filled", "#FDB70A");

    QIcon icon;
    if (fi.isDir()) {
        icon = UiHelper::getIcon("folder_filled", QColor("#FDB70A"), 18);
    } else {
        icon = ShellIconManager::getFileIcon(cleanPath);
    }
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
>>>>>>> REPLACE
```

Also subscribe to `IconLoadNotifier::instance().iconLoaded`:
```cpp
<<<<<<< SEARCH
    // 信号绑定
    connect(m_favoriteView, &QTreeView::clicked, this, &FavoritePanel::onFavoriteClicked);
=======
    // 信号绑定
    connect(&IconLoadNotifier::instance(), &IconLoadNotifier::iconLoaded, this, [this](const QString& path) {
        Q_UNUSED(path);
        if (m_favoriteView && m_favoriteView->viewport()) {
            m_favoriteView->viewport()->update();
        }
    });
    connect(m_favoriteView, &QTreeView::clicked, this, &FavoritePanel::onFavoriteClicked);
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `FavoritePanel-13.md` exists and contains Git Merge Diff blocks.
2. Verify `FavoritePanel.cpp` compiles and properly displays file thumbnails/icons for favorited files.
