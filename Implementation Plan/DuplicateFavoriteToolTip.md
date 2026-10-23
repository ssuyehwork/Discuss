# Implementation Plan - Duplicate Favorite Warning ToolTipOverlay

## Overview
This plan prevents adding duplicate paths to favorites across all entry points (NavPanel, ContentPanel, AddressBar, QuickLookWindow, drag-and-drop onto FavoritePanel).
When a user attempts to add an already favorited directory/file, a warning ToolTipOverlay `该文件夹已在收藏夹中，请勿重复添加` is displayed with a red alert border (`#e81123`) and insertion is aborted.

---

## Modified Files List
- `src/meta/FavoriteService.cpp`
- `src/ui/PanelMediator.cpp`
- `src/ui/FavoritePanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. Update `src/meta/FavoriteService.cpp`

```diff
<<<<<<< SEARCH
    bool existsAlready = FavoriteDao::containsPath(cleanPath);
    if (existsAlready) {
        bool ok = FavoriteDao::addFavorite(cleanPath, parentId, "folder_filled", "#888888");
        if (ok) {
            emit favoritesReloaded();
        }
        return ok;
    }
=======
    bool existsAlready = FavoriteDao::containsPath(cleanPath);
    if (existsAlready) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
        return false;
    }
>>>>>>> REPLACE
```

### 2. Update `src/ui/PanelMediator.cpp`

```diff
<<<<<<< SEARCH
        if (favoritePanel) {
            connect(navPanel, &NavPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QString& path) {
                favoritePanel->addFavoriteItem(path);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
            });
            connect(navPanel, &NavPanel::requestRemoveFavorite, favoritePanel, [favoritePanel](const QString& path) {
                favoritePanel->removeFavoriteItem(path);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e74c3c"));
            });
        }
=======
        if (favoritePanel) {
            connect(navPanel, &NavPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QString& path) {
                if (FavoriteService::instance().isFavorite(path)) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
                } else {
                    favoritePanel->addFavoriteItem(path);
                    favoritePanel->saveFavorites();
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
                }
            });
            connect(navPanel, &NavPanel::requestRemoveFavorite, favoritePanel, [favoritePanel](const QString& path) {
                favoritePanel->removeFavoriteItem(path);
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e74c3c"));
            });
        }
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
        if (favoritePanel) {
            connect(panel, &ContentPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QStringList& paths) {
                for (const QString& p : paths) {
                    favoritePanel->addFavoriteItem(p);
                }
                favoritePanel->saveFavorites();
            });
            connect(panel, &ContentPanel::requestRemoveFavorite, favoritePanel, [favoritePanel](const QStringList& paths) {
                for (const QString& p : paths) {
                    favoritePanel->removeFavoriteItem(p);
                }
                favoritePanel->saveFavorites();
            });
        }
=======
        if (favoritePanel) {
            connect(panel, &ContentPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QStringList& paths) {
                int addedCount = 0;
                int duplicateCount = 0;
                for (const QString& p : paths) {
                    if (FavoriteService::instance().isFavorite(p)) {
                        duplicateCount++;
                    } else {
                        favoritePanel->addFavoriteItem(p);
                        addedCount++;
                    }
                }
                if (addedCount > 0) {
                    favoritePanel->saveFavorites();
                    if (duplicateCount == 0) {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
                    } else {
                        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已添加 %1 个项目 (其余 %2 个已在收藏夹中)").arg(addedCount).arg(duplicateCount), 2000, QColor("#3498db"));
                    }
                } else if (duplicateCount > 0) {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
                }
            });
            connect(panel, &ContentPanel::requestRemoveFavorite, favoritePanel, [favoritePanel](const QStringList& paths) {
                for (const QString& p : paths) {
                    favoritePanel->removeFavoriteItem(p);
                }
                favoritePanel->saveFavorites();
                ToolTipOverlay::instance()->showText(QCursor::pos(), "已从收藏夹移除", 1500, QColor("#e74c3c"));
            });
        }
>>>>>>> REPLACE
```

### 3. Update `src/ui/FavoritePanel.cpp`

```diff
<<<<<<< SEARCH
void FavoritePanel::onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target) {
    int parentId = 0;
    if (target.isValid()) {
        FavoriteRecord rec = target.data(Qt::UserRole + 1).value<FavoriteRecord>();
        if (rec.nodeType == FavoriteNodeType::VirtualCategory) {
            parentId = rec.id;
        } else {
            parentId = rec.parentId;
        }
    }

    for (const QString& path : paths) {
        addFavoriteItem(path, parentId);
    }
    saveFavorites();
}
=======
void FavoritePanel::onPathsDroppedToFavorite(const QStringList& paths, const QModelIndex& target) {
    int parentId = 0;
    if (target.isValid()) {
        FavoriteRecord rec = target.data(Qt::UserRole + 1).value<FavoriteRecord>();
        if (rec.nodeType == FavoriteNodeType::VirtualCategory) {
            parentId = rec.id;
        } else {
            parentId = rec.parentId;
        }
    }

    int addedCount = 0;
    int duplicateCount = 0;
    for (const QString& path : paths) {
        if (FavoriteService::instance().isFavorite(path)) {
            duplicateCount++;
        } else {
            addFavoriteItem(path, parentId);
            addedCount++;
        }
    }

    if (addedCount > 0) {
        saveFavorites();
        if (duplicateCount == 0) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已成功添加至收藏夹", 1500, QColor("#2ecc71"));
        } else {
            ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已添加 %1 个项目 (其余 %2 个已在收藏夹中)").arg(addedCount).arg(duplicateCount), 2000, QColor("#3498db"));
        }
    } else if (duplicateCount > 0) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "该文件夹已在收藏夹中，请勿重复添加", 2000, QColor("#e81123"));
    }
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Recompile standard C++ build target.
2. Attempt to add an already favorited folder to favorites via AddressBar star, NavPanel context menu, ContentPanel context menu, or Drag & Drop.
3. Confirm that `ToolTipOverlay` displays the red warning message "该文件夹已在收藏夹中，请勿重复添加" and does not create duplicate entries.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `FavoriteService::instance().isFavorite(...)` SSOT check.
- Reuses `ToolTipOverlay::instance()->showText(...)` SSOT tooltip overlay.

---

## Header API Signature Verification
- `FavoriteService::isFavorite(const QString& path) const`: verified signature in `src/meta/FavoriteService.h`.
- `ToolTipOverlay::showText(const QPoint& globalPos, const QString& text, int timeout, const QColor& borderColor, bool exactPosition, const QColor& backgroundColor)`: verified signature in `src/ui/ToolTipOverlay.h`.

---

## Header Inclusion Chain & Type Completeness Check
- `FavoriteService.cpp`: Includes `ToolTipOverlay.h` for `ToolTipOverlay::instance()`.
