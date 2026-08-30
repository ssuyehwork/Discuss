# Implementation Plan - FavoritePanel-16

## Overview
Integrate `ThumbnailPipelineService` into `FavoritePanel` for favorited graphics files (e.g., `.jpg`, `.png`, `.webp`, `.ai`, `.psd`). Previously, `FavoritePanel` loaded file icons purely through `ShellIconManager::getFileIcon(path)`, which returned fixed extension-based program icons. This change updates `FavoritePanel` to check `UiHelper::isGraphicsFile(ext)`. For graphics files, `FavoritePanel` retrieves micro-thumbnails (64px) from `ThumbnailPipelineService::instance().getFromMemoryCache(path, 64)`. If uncached, it triggers `loadBatchAsync` to extract thumbnails asynchronously while displaying native program icons as temporary placeholders, updating `QStandardItem` icons when extraction completes.

## Modified Files List
- `src/ui/FavoritePanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`

1. Include `ThumbnailPipelineService.h`:
```cpp
<<<<<<< SEARCH
#include "ShellIconManager.h"
#include "ColorPicker.h"
=======
#include "ShellIconManager.h"
#include "../util/ThumbnailPipelineService.h"
#include "ColorPicker.h"
>>>>>>> REPLACE
```

2. Update `IconLoadNotifier::iconLoaded` handler in `initUi()`:
```cpp
<<<<<<< SEARCH
            if (!fi.isDir()) {
                QIcon realIcon = ShellIconManager::getFileIcon(path);
                item->setIcon(realIcon);
            }
=======
            if (!fi.isDir()) {
                QString ext = fi.suffix().toLower();
                QIcon realIcon;
                if (UiHelper::isGraphicsFile(ext)) {
                    QPixmap thumb = ThumbnailPipelineService::instance().getFromMemoryCache(path, 64);
                    if (!thumb.isNull()) {
                        realIcon = QIcon(thumb);
                    } else {
                        realIcon = ShellIconManager::getFileIcon(path);
                    }
                } else {
                    realIcon = ShellIconManager::getFileIcon(path);
                }
                item->setIcon(realIcon);
            }
>>>>>>> REPLACE
```

3. Update `loadFavorites()` and `addFavoriteItem()` to check `UiHelper::isGraphicsFile(ext)` and load or trigger thumbnail extraction via `ThumbnailPipelineService`:
```cpp
<<<<<<< SEARCH
        QIcon icon;
        if (fi.isDir()) {
            icon = UiHelper::getIcon(iconKey, itemColor, 18);
        } else {
            icon = ShellIconManager::getFileIcon(rec.path);
        }
=======
        QIcon icon;
        if (fi.isDir()) {
            icon = UiHelper::getIcon(iconKey, itemColor, 18);
        } else {
            QString ext = fi.suffix().toLower();
            if (UiHelper::isGraphicsFile(ext)) {
                QPixmap thumb = ThumbnailPipelineService::instance().getFromMemoryCache(rec.path, 64);
                if (!thumb.isNull()) {
                    icon = QIcon(thumb);
                } else {
                    icon = ShellIconManager::getFileIcon(rec.path);
                    ThumbnailPipelineService::instance().loadBatchAsync({rec.path}, 64);
                }
            } else {
                icon = ShellIconManager::getFileIcon(rec.path);
            }
        }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    QIcon icon;
    if (fi.isDir()) {
        icon = UiHelper::getIcon("folder_filled", QColor("#FDB70A"), 18);
    } else {
        icon = ShellIconManager::getFileIcon(cleanPath);
    }
=======
    QIcon icon;
    if (fi.isDir()) {
        icon = UiHelper::getIcon("folder_filled", QColor("#FDB70A"), 18);
    } else {
        QString ext = fi.suffix().toLower();
        if (UiHelper::isGraphicsFile(ext)) {
            QPixmap thumb = ThumbnailPipelineService::instance().getFromMemoryCache(cleanPath, 64);
            if (!thumb.isNull()) {
                icon = QIcon(thumb);
            } else {
                icon = ShellIconManager::getFileIcon(cleanPath);
                ThumbnailPipelineService::instance().loadBatchAsync({cleanPath}, 64);
            }
        } else {
            icon = ShellIconManager::getFileIcon(cleanPath);
        }
    }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `FavoritePanel-16.md` exists and contains Git Merge Diff blocks.
2. Verify `FavoritePanel.cpp` loads real content thumbnails for favorited graphics files (`AI-2.ai`, `.webp`, `.jpg`, etc.) using `ThumbnailPipelineService`.
