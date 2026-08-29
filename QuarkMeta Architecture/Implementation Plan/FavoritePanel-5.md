# Implementation Plan - FavoritePanel-5

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
- **File Item Safeguard**: Context menu for file items (`QFileInfo::isDir() == false`) displays only "取消收藏", hiding icon and color selection submenus.

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

## 3. Detailed Line-by-Line Changes

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
#include "../core/AppConfig.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QWidgetAction>
#include <QGridLayout>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
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
    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();
    if (curIconKey.isEmpty()) curIconKey = "folder_filled";
    if (curColorHex.isEmpty()) curColorHex = "#FDB70A";

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    QFileInfo fi(path);
    bool isFolder = fi.isDir();

    if (isFolder) {
        // ColorStripPicker Action
        QWidgetAction* colorPickerAction = new QWidgetAction(&menu);
        ColorStripPicker* colorPickerWidget = new ColorStripPicker(curColorHex, &menu);
        colorPickerAction->setDefaultWidget(colorPickerWidget);
        menu.addAction(colorPickerAction);

        connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, path, curIconKey, index, &menu](const QString& hexColor) {
            QString finalColor = hexColor.isEmpty() ? "#FDB70A" : hexColor.toUpper();
            FavoriteDao::updateFavorite(path, curIconKey, finalColor);
            QIcon newIcon = UiHelper::getIcon(curIconKey, QColor(finalColor), 18);
            m_favoriteModel->itemFromIndex(index)->setIcon(newIcon);
            m_favoriteModel->itemFromIndex(index)->setData(finalColor, Qt::UserRole + 3);
            menu.close();
        });

        // Icon Grid Picker Submenu
        QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor(curColorHex)), "切换图标");
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

        QColor catColor = QColor(curColorHex);
        int row = 0;
        int col = 0;
        for (const auto& pair : builtInIcons) {
            QString label = pair.first;
            QString iconKey = pair.second;

            QPushButton* btn = new QPushButton(pickerWidget);
            btn->setFixedSize(28, 28);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(
                "QPushButton { "
                "  background-color: transparent; "
                "  border: 1px solid transparent; "
                "  border-radius: 4px; "
                "}"
                "QPushButton:hover { "
                "  background-color: #3E3E42; "
                "  border: 1px solid #555555; "
                "}"
                "QPushButton:pressed { "
                "  background-color: #4E4E52; "
                "}"
            );
            btn->setIcon(UiHelper::getIcon(iconKey, catColor, 18));
            btn->setIconSize(QSize(18, 18));
            btn->setToolTip(label);

            pickerLayout->addWidget(btn, row, col);

            connect(btn, &QPushButton::clicked, this, [this, path, iconKey, curColorHex, index, iconMenu]() {
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

## 4. Build & Verification Steps
1. Rebuild the project using CMake:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. Launch QuarkMeta and test right-click actions in `FavoritePanel`:
   - Right-click a favorite folder item: Verify `ColorStripPicker` line and "切换图标" 5x2 grid icon menu appear with zero right-hand black empty space.
   - Right-click a favorite file item: Verify only "取消收藏" is displayed.
