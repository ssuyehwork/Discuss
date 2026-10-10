# Implementation Plan - FixIconPickerCrash.md

## Overview
Fix access violation crash (`0xc0000005` in `QVariant::clear`) when changing category/folder icons or colors via right-click context menu in `FavoritePanel` and `LibraryPanel`.

## Root Cause Analysis
In `FavoritePanel.cpp` and `LibraryPanel.cpp`, the lambdas passed to `ContextMenuFactory::buildIconPickerMenu` capture `index` (`QModelIndex`).
When a user selects a new icon or color from the menu:
1. `QStandardItem* item = m_model->itemFromIndex(index);` is called.
2. If background events or model updates occurred while the menu was open, or if `item->setIcon(newIcon)` triggers `itemChanged` signal processing, `item` or `index` may become invalid or trigger re-entrancy.
3. `item->setIcon(newIcon)` internally calls `setData(newIcon, Qt::DecorationRole)`. Since `DecorationRole` already contains a previous `QIcon` (wrapped in a `QVariant`), Qt attempts to release the old `QVariant` data via `QVariant::clear()`.
4. If `itemChanged` signal handlers or wild pointers are involved during model updates, `QVariant::clear()` attempts to read a dangling pointer (`rbx+8`), throwing `c0000005 Access Violation`.

## Solution Strategy
1. Wrap `index` in a `QPersistentModelIndex` inside the lambda, or check `persistentIdx.isValid()` before calling `itemFromIndex`.
2. Temporarily block model signals (`m_model->blockSignals(true)`) while setting the new icon / color data to prevent re-entrant `itemChanged` signal loops during `setIcon()`.
3. Unblock model signals (`m_model->blockSignals(false)`) immediately after updating icon/color data.

## Modified Files List
- `src/ui/FavoritePanel.cpp`
- `src/ui/LibraryPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FavoritePanel.cpp`
```
<<<<<<< SEARCH
    if (isFolder) {
        ContextMenuFactory::buildIconPickerMenu(&menu, curIconKey, curColorHex,
            [this, index](const QString& iconKey) {
                QStandardItem* item = m_favoriteModel->itemFromIndex(index);
                if (!item) return;

                QString colorHex = item->data(Qt::UserRole + 3).toString();
                if (colorHex.isEmpty()) colorHex = "#888888";

                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
                item->setIcon(newIcon);
                item->setData(iconKey, Qt::UserRole + 2);

                if (m_favoriteView && m_favoriteView->viewport()) {
                    m_favoriteView->viewport()->update();
                }
            },
            [this, index](const QString& hexColor) {
                QStandardItem* item = m_favoriteModel->itemFromIndex(index);
                if (!item) return;

                QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
                QString iconKey = item->data(Qt::UserRole + 2).toString();
                if (iconKey.isEmpty()) iconKey = "folder_filled";
                QString targetPath = item->data(Qt::UserRole + 1).toString();

                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
                item->setIcon(newIcon);
                item->setData(finalColor, Qt::UserRole + 3);

                if (!targetPath.isEmpty() && !targetPath.startsWith("virtual_cat_")) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetColor;
                    cmd.targetPaths = {targetPath};
                    cmd.params["color"] = finalColor;
                    CoreEngine::instance().executeCommand(cmd);
                }

                if (m_favoriteView && m_favoriteView->viewport()) {
                    m_favoriteView->viewport()->update();
                }
            }
        );
    }
=======
    if (isFolder) {
        QPersistentModelIndex persistentIdx(index);
        ContextMenuFactory::buildIconPickerMenu(&menu, curIconKey, curColorHex,
            [this, persistentIdx](const QString& iconKey) {
                if (!persistentIdx.isValid()) return;
                QStandardItem* item = m_favoriteModel ? m_favoriteModel->itemFromIndex(persistentIdx) : nullptr;
                if (!item) return;

                QString colorHex = item->data(Qt::UserRole + 3).toString();
                if (colorHex.isEmpty()) colorHex = "#888888";

                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
                if (m_favoriteModel) m_favoriteModel->blockSignals(true);
                item->setIcon(newIcon);
                item->setData(iconKey, Qt::UserRole + 2);
                if (m_favoriteModel) m_favoriteModel->blockSignals(false);

                if (m_favoriteView && m_favoriteView->viewport()) {
                    m_favoriteView->viewport()->update();
                }
            },
            [this, persistentIdx](const QString& hexColor) {
                if (!persistentIdx.isValid()) return;
                QStandardItem* item = m_favoriteModel ? m_favoriteModel->itemFromIndex(persistentIdx) : nullptr;
                if (!item) return;

                QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
                QString iconKey = item->data(Qt::UserRole + 2).toString();
                if (iconKey.isEmpty()) iconKey = "folder_filled";
                QString targetPath = item->data(Qt::UserRole + 1).toString();

                QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
                if (m_favoriteModel) m_favoriteModel->blockSignals(true);
                item->setIcon(newIcon);
                item->setData(finalColor, Qt::UserRole + 3);
                if (m_favoriteModel) m_favoriteModel->blockSignals(false);

                if (!targetPath.isEmpty() && !targetPath.startsWith("virtual_cat_")) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetColor;
                    cmd.targetPaths = {targetPath};
                    cmd.params["color"] = finalColor;
                    CoreEngine::instance().executeCommand(cmd);
                }

                if (m_favoriteView && m_favoriteView->viewport()) {
                    m_favoriteView->viewport()->update();
                }
            }
        );
    }
>>>>>>> REPLACE
```

### `src/ui/LibraryPanel.cpp`
```
<<<<<<< SEARCH
    QPersistentModelIndex persistentIdx(index);
    ContextMenuFactory::buildIconPickerMenu(&menu, curIconKey, curColorHex,
        [this, index](const QString& iconKey) {
            QStandardItem* item = m_model->itemFromIndex(index);
            if (!item) return;

            QString colorHex = item->data(Qt::UserRole + 3).toString();
            if (colorHex.isEmpty()) colorHex = "#888888";

            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
            item->setIcon(newIcon);
            item->setData(iconKey, Qt::UserRole + 2);

            int nodeId = item->data(Qt::UserRole + 1).toInt();
            LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, colorHex);
        },
        [this, index](const QString& hexColor) {
            QStandardItem* item = m_model->itemFromIndex(index);
            if (!item) return;

            QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            if (iconKey.isEmpty()) iconKey = "folder_filled";

            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);

            int nodeId = item->data(Qt::UserRole + 1).toInt();
            LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, finalColor);
        }
    );
=======
    QPersistentModelIndex persistentIdx(index);
    ContextMenuFactory::buildIconPickerMenu(&menu, curIconKey, curColorHex,
        [this, persistentIdx](const QString& iconKey) {
            if (!persistentIdx.isValid()) return;
            QStandardItem* item = m_model ? m_model->itemFromIndex(persistentIdx) : nullptr;
            if (!item) return;

            QString colorHex = item->data(Qt::UserRole + 3).toString();
            if (colorHex.isEmpty()) colorHex = "#888888";

            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
            if (m_model) m_model->blockSignals(true);
            item->setIcon(newIcon);
            item->setData(iconKey, Qt::UserRole + 2);
            if (m_model) m_model->blockSignals(false);

            int nodeId = item->data(Qt::UserRole + 1).toInt();
            LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, colorHex);
        },
        [this, persistentIdx](const QString& hexColor) {
            if (!persistentIdx.isValid()) return;
            QStandardItem* item = m_model ? m_model->itemFromIndex(persistentIdx) : nullptr;
            if (!item) return;

            QString finalColor = hexColor.isEmpty() ? "#888888" : hexColor.toUpper();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            if (iconKey.isEmpty()) iconKey = "folder_filled";

            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            if (m_model) m_model->blockSignals(true);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);
            if (m_model) m_model->blockSignals(false);

            int nodeId = item->data(Qt::UserRole + 1).toInt();
            LibraryDao::updateCategoryNode(nodeId, item->text(), iconKey, finalColor);
        }
    );
>>>>>>> REPLACE
```

## Build & Verification
1. Build QuarkMeta.
2. Open FavoritePanel / LibraryPanel, right-click category or folder.
3. Change icon / change color multiple times consecutively.
4. Verify no crash occurs and new icons render cleanly.
