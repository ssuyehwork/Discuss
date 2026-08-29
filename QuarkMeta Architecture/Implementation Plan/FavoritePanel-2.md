# Implementation Plan - FavoritePanel-2

This implementation plan refines `FavoritePanel` to enforce dual-track rendering between folders and files, default to `folder_filled` for folder favorites, strip text labels from context menu icon/color pickers, and prevent context menu icon/color customization for file favorites.

## 1. Overview
- **Solid Folder Icon Default**: Use `folder_filled` as the default SVG key for folder entries in the favorite panel.
- **Dual-Track Rendering**:
  - Folders (`QFileInfo::isDir() == true`): Rendered using `UiHelper::getIcon(iconKey, color, 18)`, supporting dynamic color and SVG icon key selection.
  - Files (`QFileInfo::isDir() == false`): Rendered strictly using native system icons/thumbnails via `ShellIconManager::getFileIcon(path)`.
- **Pure Icon Context Menu**: Right-click "切换图标" and "切换色标" sub-menus for folders will display **icons only (empty text string `""`)**, removing all textual labels.
- **Context Menu Safeguard**: File items in favorites will not display "切换图标" or "切换色标" sub-menus; right-clicking a file item will only show the "取消收藏" action.

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```diff
<<<<<<< SEARCH
void FavoritePanel::onFavoriteContextMenu(const QPoint& pos) {
    QModelIndex index = m_favoriteView->indexAt(pos);
    if (!index.isValid()) return;

    QString path = index.data(Qt::UserRole + 1).toString();
    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();
    if (curIconKey.isEmpty()) curIconKey = "folder";
    if (curColorHex.isEmpty()) curColorHex = "#FDB70A";

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder", QColor("#EEEEEE")), "切换图标");
    static const QPair<QString, QString> iconOptions[] = {
        { "folder", "标准文件夹" },
        { "star", "星号" },
        { "heart", "红心" },
        { "bookmark", "书签" },
        { "tag", "标签" }
    };
    for (const auto& opt : iconOptions) {
        QAction* act = iconMenu->addAction(UiHelper::getIcon(opt.first, QColor(curColorHex)), opt.second);
        connect(act, &QAction::triggered, this, [this, path, opt, curColorHex, index]() {
            FavoriteDao::updateFavorite(path, opt.first, curColorHex);
            QIcon newIcon = UiHelper::getIcon(opt.first, QColor(curColorHex), 18);
            m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
            m_favoriteModel->itemFromIndex(index)->setData(opt.first, Qt::UserRole + 2);
        });
    }

    QMenu* colorMenu = menu.addMenu(UiHelper::getIcon("circle_filled", QColor(curColorHex)), "切换色标");
    static const QPair<QString, QString> colorOptions[] = {
        { "#FDB70A", "金色" },
        { "#E24B4A", "红色" },
        { "#EF9F27", "橙色" },
        { "#639922", "绿色" },
        { "#1D9E75", "青色" },
        { "#378ADD", "蓝色" },
        { "#7F77DD", "紫色" }
    };
    for (const auto& opt : colorOptions) {
        QAction* act = colorMenu->addAction(UiHelper::getIcon("circle_filled", QColor(opt.first)), opt.second);
        connect(act, &QAction::triggered, this, [this, path, curIconKey, opt, index]() {
            FavoriteDao::updateFavorite(path, curIconKey, opt.first);
            QIcon newIcon = UiHelper::getIcon(curIconKey, QColor(opt.first), 18);
            m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
            m_favoriteModel->itemFromIndex(index)->setData(opt.first, Qt::UserRole + 3);
        });
    }

    menu.addSeparator();

    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "取消收藏");
    connect(removeAct, &QAction::triggered, this, [this, path, index]() {
        FavoriteDao::removeFavorite(path);
        m_favoriteModel->removeRow(index.row());
    });

    menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));
}
=======
void FavoritePanel::onFavoriteContextMenu(const QPoint& pos) {
    QModelIndex index = m_favoriteView->indexAt(pos);
    if (!index.isValid()) return;

    QString path = index.data(Qt::UserRole + 1).toString();
    QFileInfo fi(path);

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    // Only folders allow changing SVG icons and color tags
    if (fi.isDir()) {
        QString curIconKey = index.data(Qt::UserRole + 2).toString();
        QString curColorHex = index.data(Qt::UserRole + 3).toString();
        if (curIconKey.isEmpty()) curIconKey = "folder_filled";
        if (curColorHex.isEmpty()) curColorHex = "#FDB70A";

        QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "切换图标");
        static const QString iconKeys[] = {
            "folder_filled",
            "star_filled",
            "heart_filled",
            "bookmark_filled",
            "tag_filled"
        };
        for (const QString& key : iconKeys) {
            // Icon only - text label is empty string ""
            QAction* act = iconMenu->addAction(UiHelper::getIcon(key, QColor(curColorHex)), "");
            connect(act, &QAction::triggered, this, [this, path, key, curColorHex, index]() {
                FavoriteDao::updateFavorite(path, key, curColorHex);
                QIcon newIcon = UiHelper::getIcon(key, QColor(curColorHex), 18);
                m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
                m_favoriteModel->itemFromIndex(index)->setData(key, Qt::UserRole + 2);
            });
        }

        QMenu* colorMenu = menu.addMenu(UiHelper::getIcon("circle_filled", QColor(curColorHex)), "切换色标");
        static const QString colorHexes[] = {
            "#FDB70A",
            "#E24B4A",
            "#EF9F27",
            "#639922",
            "#1D9E75",
            "#378ADD",
            "#7F77DD"
        };
        for (const QString& hex : colorHexes) {
            // Icon only - text label is empty string ""
            QAction* act = colorMenu->addAction(UiHelper::getIcon("circle_filled", QColor(hex)), "");
            connect(act, &QAction::triggered, this, [this, path, curIconKey, hex, index]() {
                FavoriteDao::updateFavorite(path, curIconKey, hex);
                QIcon newIcon = UiHelper::getIcon(curIconKey, QColor(hex), 18);
                m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
                m_favoriteModel->itemFromIndex(index)->setData(hex, Qt::UserRole + 3);
            });
        }

        menu.addSeparator();
    }

    QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "取消收藏");
    connect(removeAct, &QAction::triggered, this, [this, path, index]() {
        FavoriteDao::removeFavorite(path);
        m_favoriteModel->removeRow(index.row());
    });

    menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_favoriteModel->clear();

    FavoriteDao::initTable();
    auto list = FavoriteDao::getAllFavorites();

    for (const auto& rec : list) {
        QFileInfo fi(rec.path);
        if (!fi.exists()) continue;

        QColor itemColor = QColor(rec.colorHex);
        if (!itemColor.isValid()) itemColor = QColor("#FDB70A");

        QIcon icon = UiHelper::getIcon(rec.iconKey.isEmpty() ? "folder" : rec.iconKey, itemColor, 18);
        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
        item->setData(rec.path, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);

        m_favoriteModel->appendRow(item);
    }
}
=======
void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_favoriteModel->clear();

    FavoriteDao::initTable();
    auto list = FavoriteDao::getAllFavorites();

    for (const auto& rec : list) {
        QFileInfo fi(rec.path);
        if (!fi.exists()) continue;

        QIcon icon;
        if (fi.isDir()) {
            QColor itemColor = QColor(rec.colorHex);
            if (!itemColor.isValid()) itemColor = QColor("#FDB70A");
            QString iconKey = rec.iconKey.isEmpty() ? "folder_filled" : rec.iconKey;
            icon = UiHelper::getIcon(iconKey, itemColor, 18);
        } else {
            icon = ShellIconManager::getFileIcon(rec.path);
        }

        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
        item->setData(rec.path, Qt::UserRole + 1);
        item->setData(rec.iconKey.isEmpty() ? "folder_filled" : rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex.isEmpty() ? "#FDB70A" : rec.colorHex, Qt::UserRole + 3);

        m_favoriteModel->appendRow(item);
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void FavoritePanel::addFavoriteItem(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty()) return;

    if (FavoriteDao::containsPath(cleanPath)) return;

    QFileInfo fi(cleanPath);
    if (!fi.exists()) return;

    FavoriteDao::addFavorite(cleanPath, "folder", "#FDB70A");

    QIcon icon = UiHelper::getIcon("folder", QColor("#FDB70A"), 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
    item->setData(cleanPath, Qt::UserRole + 1);
    item->setData("folder", Qt::UserRole + 2);
    item->setData("#FDB70A", Qt::UserRole + 3);

    m_favoriteModel->appendRow(item);
}
=======
void FavoritePanel::addFavoriteItem(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty()) return;

    if (FavoriteDao::containsPath(cleanPath)) return;

    QFileInfo fi(cleanPath);
    if (!fi.exists()) return;

    QIcon icon;
    QString iconKey = "folder_filled";
    QString colorHex = "#FDB70A";

    if (fi.isDir()) {
        FavoriteDao::addFavorite(cleanPath, iconKey, colorHex);
        icon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
    } else {
        FavoriteDao::addFavorite(cleanPath, "", "");
        icon = ShellIconManager::getFileIcon(cleanPath);
    }

    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
    item->setData(cleanPath, Qt::UserRole + 1);
    item->setData(iconKey, Qt::UserRole + 2);
    item->setData(colorHex, Qt::UserRole + 3);

    m_favoriteModel->appendRow(item);
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Clean and build the application:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```
2. Run the application and verify:
   - Add a folder to FavoritePanel: it displays a solid folder SVG icon (`folder_filled`).
   - Right-click the folder entry: "切换图标" and "切换色标" options display pure icons without text labels.
   - Add a file to FavoritePanel: it displays its native system icon/thumbnail.
   - Right-click the file entry: no "切换图标" or "切换色标" sub-menus appear; only "取消收藏" is available.
