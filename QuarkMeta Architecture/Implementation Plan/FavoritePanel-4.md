# Implementation Plan - FavoritePanel-4

This implementation plan refines `FavoritePanel` right-click context menus. It replaces plain `QAction` lists with `QWidgetAction` combined with a `QGridLayout` (5 columns × 2 rows) for a 10-item vector SVG icon picker, embeds `ColorStripPicker` for color selection, and completely eliminates empty black right-hand whitespace margins.

## 1. Overview
- **10-Item Vector Icon Grid (`builtInIcons`)**:
  - `folder_filled` (Solid Folder)
  - `category` (Category)
  - `image_filled` (Photo/Media)
  - `clock_filled` (Clock/History)
  - `star_filled` (Star/Favorite)
  - `heart_filled` (Heart/Common)
  - `lock_filled` (Lock/Secure)
  - `book` (Book/Doc)
  - `settings_filled` (Settings)
  - `globe_filled` (Globe/Network)
- **`QWidgetAction` + `QGridLayout` Grid Layout**: Icon picker sub-menu uses a 5-column grid of 28x28px compact buttons (`QPushButton`), completely eliminating right-side empty space.
- **Embedded `ColorStripPicker`**: Color selection submenu directly embeds `ColorStripPicker` widget displaying 9 color circles (no color + 8 standard colors).
- **File Item Safeguard**: Context menu for file items displays only "取消收藏", hiding icon and color selection submenus.

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
#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "../meta/FavoriteDao.h"
#include <QPainter>
#include "../core/AppConfig.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
=======
#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "ColorPicker.h"
#include "../meta/FavoriteDao.h"
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QWidgetAction>
#include <QGridLayout>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>
>>>>>>> REPLACE
```

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

    // Subscribe to async system icon load notifications to refresh viewport when icons finish extracting
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

    // Folders only: enable ColorStripPicker and QWidgetAction 10-icon grid
    if (fi.isDir()) {
        QString curIconKey = index.data(Qt::UserRole + 2).toString();
        QString curColorHex = index.data(Qt::UserRole + 3).toString();
        if (curIconKey.isEmpty()) curIconKey = "folder_filled";
        if (curColorHex.isEmpty()) curColorHex = "#FDB70A";

        // 1. ColorStripPicker sub-menu
        QMenu* colorMenu = menu.addMenu(UiHelper::getIcon("circle_filled", QColor(curColorHex)), "切换色标");
        UiHelper::applyMenuStyle(colorMenu);

        QWidgetAction* colorAction = new QWidgetAction(colorMenu);
        ColorStripPicker* colorPickerWidget = new ColorStripPicker(curColorHex, colorMenu);
        colorAction->setDefaultWidget(colorPickerWidget);
        colorMenu->addAction(colorAction);

        connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, path, curIconKey, index, colorMenu](const QString& hexColor) {
            QString finalColor = hexColor.isEmpty() ? "#FDB70A" : hexColor;
            FavoriteDao::updateFavorite(path, curIconKey, finalColor);
            QIcon newIcon = UiHelper::getIcon(curIconKey, QColor(finalColor), 18);
            m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
            m_favoriteModel->itemFromIndex(index)->setData(finalColor, Qt::UserRole + 3);
            colorMenu->close();
        });

        // 2. QWidgetAction 10-icon 5x2 grid sub-menu
        QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "切换图标");
        UiHelper::applyMenuStyle(iconMenu);

        QWidgetAction* pickerAction = new QWidgetAction(iconMenu);
        QWidget* pickerWidget = new QWidget(iconMenu);
        QGridLayout* pickerLayout = new QGridLayout(pickerWidget);
        pickerLayout->setContentsMargins(6, 6, 6, 6);
        pickerLayout->setSpacing(6);

        static const QList<QPair<QString, QString>> builtInIcons = {
            {"默认文件夹", "folder_filled"},
            {"层级分类", "category"},
            {"照片媒体", "image_filled"},
            {"时钟历史", "clock_filled"},
            {"星标收藏", "star_filled"},
            {"爱心常用", "heart_filled"},
            {"加密安全", "lock_filled"},
            {"图书文档", "book"},
            {"配置管理", "settings_filled"},
            {"网络球体", "globe_filled"}
        };

        QColor folderColor = QColor(curColorHex);
        if (!folderColor.isValid()) folderColor = QColor("#FDB70A");

        int row = 0, col = 0;
        for (const auto& item : builtInIcons) {
            QString label = item.first;
            QString iconKey = item.second;

            QPushButton* btnIcon = new QPushButton(pickerWidget);
            btnIcon->setFixedSize(28, 28);
            btnIcon->setCursor(Qt::PointingHandCursor);
            btnIcon->setStyleSheet(
                "QPushButton { "
                "  background-color: transparent; "
                "  border: 1px solid transparent; "
                "  border-radius: 4px; "
                "}"
                "QPushButton:hover { "
                "  background-color: #3E3E42; "
                "  border: 1px solid #555555; "
                "}"
            );
            btnIcon->setIcon(UiHelper::getIcon(iconKey, folderColor, 18));
            btnIcon->setIconSize(QSize(18, 18));
            btnIcon->setToolTip(label);

            pickerLayout->addWidget(btnIcon, row, col);

            connect(btnIcon, &QPushButton::clicked, this, [this, path, iconKey, curColorHex, index, iconMenu]() {
                FavoriteDao::updateFavorite(path, iconKey, curColorHex);
                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(curColorHex), 18);
                m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
                m_favoriteModel->itemFromIndex(index)->setData(iconKey, Qt::UserRole + 2);
                iconMenu->close();
            });

            col++;
            if (col >= 5) {
                col = 0;
                row++;
            }
        }

        pickerWidget->setLayout(pickerLayout);
        pickerAction->setDefaultWidget(pickerWidget);
        iconMenu->addAction(pickerAction);

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
1. Build with CMake:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```
2. Run application and verify:
   - Right-click favorite folder: check that "切换图标" displays a compact 5x2 grid of 10 vector SVG icons with zero right-hand whitespace.
   - Check that "切换色标" displays `ColorStripPicker` with 9 color circles.
   - Right-click favorite file: check that icon/color submenus are completely hidden.
