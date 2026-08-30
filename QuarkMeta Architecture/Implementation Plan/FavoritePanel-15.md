# Implementation Plan - FavoritePanel-15

## Overview
Fix file icon placeholder persistence in `FavoritePanel`. Previously, when `ShellIconManager::getFileIcon(path)` returned a default placeholder icon during async shell icon extraction, `FavoritePanel::initUi` listened to `IconLoadNotifier::instance().iconLoaded` but only called `viewport()->update()`. Because `QStandardItem` held the cached initial placeholder `QIcon`, viewport updates re-painted the placeholder. This change updates `IconLoadNotifier::iconLoaded` handler to iterate over `m_favoriteModel` rows, re-fetch resolved icons for files (`!QFileInfo(path).isDir()`), update `item->setIcon(realIcon)`, and refresh the view.

## Modified Files List
- `src/ui/FavoritePanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```cpp
<<<<<<< SEARCH
    connect(&IconLoadNotifier::instance(), &IconLoadNotifier::iconLoaded, this, [this]() {
        if (m_favoriteView && m_favoriteView->viewport()) {
            m_favoriteView->viewport()->update();
        }
    });
=======
    connect(&IconLoadNotifier::instance(), &IconLoadNotifier::iconLoaded, this, [this]() {
        if (!m_favoriteModel) return;
        for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
            QStandardItem* item = m_favoriteModel->item(i);
            if (!item) continue;
            QString path = item->data(Qt::UserRole + 1).toString();
            if (path.isEmpty()) continue;
            QFileInfo fi(path);
            if (!fi.isDir()) {
                QIcon realIcon = ShellIconManager::getFileIcon(path);
                item->setIcon(realIcon);
            }
        }
        if (m_favoriteView && m_favoriteView->viewport()) {
            m_favoriteView->viewport()->update();
        }
    });
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `FavoritePanel-15.md` exists and contains Git Merge Diff blocks.
2. Verify `FavoritePanel.cpp` updates `QStandardItem` icons dynamically upon asynchronous shell icon extraction completion.
