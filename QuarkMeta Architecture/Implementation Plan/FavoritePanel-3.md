# Implementation Plan - FavoritePanel-3

This implementation plan resolves the missing declaration errors (`saveFavorites`), fixes the blank file icon issue via `IconLoadNotifier` signal subscription, and enforces solid folder icons (`folder_filled`), folder/file dual-track rendering, and pure icon context menus without text labels.

## 1. Overview
- **Fix Declaration Errors**: Add `void saveFavorites();` private helper method to `FavoritePanel.h` to resolve compiler errors regarding `saveFavorites` not being a member of `FavoritePanel`.
- **Async Icon Refresh via `IconLoadNotifier`**: Subscribe to `IconLoadNotifier::instance().iconLoaded` in `FavoritePanel` constructor to trigger `m_favoriteView->viewport()->update()`. As soon as background threads finish extracting system icons for files (`.svg`, `.psd`, etc.), the view immediately updates and replaces placeholder icons with actual system thumbnails/icons.
- **Solid Folder Default**: Folder favorites default to `folder_filled` SVG key and `#FDB70A` color.
- **Dual-Track Item Rendering**:
  - **Folders (`QFileInfo::isDir() == true`)**: Rendered using `UiHelper::getIcon(iconKey, color, 18)`.
  - **Files (`QFileInfo::isDir() == false`)**: Rendered strictly using native system icons/thumbnails via `ShellIconManager::getFileIcon(path)`.
- **Pure Icon Context Menu (No Text Labels)**: Right-click "切换图标" and "切换色标" menus display **icons only with empty text strings `""`**.
- **Context Menu File Safeguard**: File items in favorites show only "取消收藏"; icon/color customization submenus are hidden for files.

## 2. Modified Files List
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.h`
```diff
<<<<<<< SEARCH
private:
    void initUi();

    QVBoxLayout* m_mainLayout = nullptr;

    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
};
=======
private:
    void initUi();
    void saveFavorites();

    QVBoxLayout* m_mainLayout = nullptr;

    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
};
>>>>>>> REPLACE
```

### `src/ui/FavoritePanel.cpp`
```diff
<<<<<<< SEARCH
FavoritePanel::FavoritePanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("ListContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("FavoritePanel { background-color: #1E1E1E; color: #EEEEEE; }");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadFavorites();
}
=======
FavoritePanel::FavoritePanel(QWidget* parent)
    : QFrame(parent) {
    setObjectName("ListContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);
    setStyleSheet("FavoritePanel { background-color: #1E1E1E; color: #EEEEEE; }");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadFavorites();

    // Subscribe to async system icon load notifications to update viewport when icons are extracted
    connect(&WindowsShellThumbnailProvider::instance(), &IconLoadNotifier::iconLoaded, this, [this]() {
        if (m_favoriteView && m_favoriteView->viewport()) {
            m_favoriteView->viewport()->update();
        }
    });
}
>>>>>>> REPLACE
```

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

    // Only folders support changing SVG icons and colors
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
            // Pure icon option - empty text label ""
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
            // Pure icon option - empty text label ""
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
1. Clean and build the project using CMake:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```
2. Run application and verify:
   - Ensure `saveFavorites()` compilation error is eliminated.
   - Add files (such as `.svg` or `.psd`) to FavoritePanel: when the background thread completes icon extraction, the viewport automatically refreshes and displays actual file system icons.
   - Verify folder items display `folder_filled` solid SVG icons by default.
   - Verify right-clicking folder items shows pure-icon menus without text labels.
   - Verify right-clicking file items shows only "取消收藏".
