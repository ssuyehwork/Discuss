# Implementation Plan - Context Menu Logic Normalization (ContextMenuNormalization-1.md)

## 1. Overview
Normalize right-click context menu execution and model modification logic across all UI panels (`ContentContextMenu`, `FavoritePanel`, `NavPanel`):
1. **Command Engine Integration**: Normalize right-click color marking, pinning/unpinning, and tag modifications in `ContentContextMenu` so they execute through `CoreEngine::executeCommand` / `MetadataManager` and notify via `CentralEventHub`. This ensures instant, unified state synchronization across Grid, List, Justified, and Column View panes.
2. **Path & Name Clipboard Operations Normalization**: Normalize copy path (`QDir::toNativeSeparators`) and copy name actions across `NavPanel`, `FavoritePanel`, and `ContentContextMenu`.
3. **Favorite Folder Color Sync**: Connect `FavoritePanel` folder color modifications to `CoreEngine::executeCommand(AppCommandType::SetColor)` so setting a folder color in Favorites updates the global folder color in Disk/MetadataManager and vice versa.

## 2. Modified Files List
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/FavoritePanel.cpp`
- `src/ui/NavPanel.cpp`

## 3. Detailed Line-by-Line Changes

### File: `src/ui/controllers/ContentContextMenu.cpp`
```
<<<<<<< SEARCH
            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto* model = view->model();
                if (!model) return;
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) model->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });
=======
            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                QStringList targetPaths;
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) {
                        QString p = idx.data(PathRole).toString();
                        if (!p.isEmpty()) targetPaths << p;
                    }
                }
                if (!targetPaths.isEmpty()) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetColor;
                    cmd.targetPaths = targetPaths;
                    cmd.params["color"] = hexColor;
                    CoreEngine::instance().executeCommand(cmd);
                    if (m_panel) {
                        for (const QString& p : targetPaths) m_panel->updateItemMetadata(p);
                    }
                }
                menu.close();
            });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            auto* model = view->model();
            auto* proxy = qobject_cast<QSortFilterProxyModel*>(model);
            auto indexes = view->selectionModel()->selectedIndexes();
            bool pin = (action == ContentPanel::ActionPin);
            for (const QModelIndex& idx : indexes) {
                if (idx.column() == 0 && model) {
                    model->setData(idx, pin, IsLockedRole);
                }
            }
            if (proxy) {
                proxy->invalidate();
                proxy->sort(0, proxy->sortOrder());
            }
            break;
        }
=======
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            auto indexes = view->selectionModel()->selectedIndexes();
            bool pin = (action == ContentPanel::ActionPin);
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << p;
                }
            }
            if (!targetPaths.isEmpty()) {
                for (const QString& p : targetPaths) {
                    MetadataManager::instance().setPinned(p.toStdWString(), pin);
                    if (m_panel) m_panel->updateItemMetadata(p);
                }
                if (m_panel) m_panel->refreshAll();
            }
            break;
        }
>>>>>>> REPLACE
```

### File: `src/ui/FavoritePanel.cpp`
```
<<<<<<< SEARCH
        // 🚀【持续改色 0ms 就地预览，绝对不调用 close()】
        connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, index, iconMenu, iconButtons](const QString& hexColor) {
            QStandardItem* item = m_favoriteModel->itemFromIndex(index);
            if (!item) return;

            QString finalColor = hexColor.isEmpty() ? "#FDB70A" : hexColor.toUpper();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            if (iconKey.isEmpty()) iconKey = "folder_filled";

            // 1. 实时就地刷新左侧收藏项
            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);

            // 2. 联动刷新子菜单自身的头部图标与内部 50 个小图标颜色
            iconMenu->setIcon(UiHelper::getIcon("folder_filled", QColor(finalColor)));
            for (const auto& btnPair : iconButtons) {
                btnPair.first->setIcon(UiHelper::getIcon(btnPair.second, QColor(finalColor), 18));
            }

            if (m_favoriteView && m_favoriteView->viewport()) {
                m_favoriteView->viewport()->update();
            }
        });
=======
        // 🚀【持续改色 0ms 就地预览，绝对不调用 close()】
        connect(colorPickerWidget, &ColorStripPicker::colorSelected, this, [this, index, iconMenu, iconButtons](const QString& hexColor) {
            QStandardItem* item = m_favoriteModel->itemFromIndex(index);
            if (!item) return;

            QString finalColor = hexColor.isEmpty() ? "#FDB70A" : hexColor.toUpper();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            if (iconKey.isEmpty()) iconKey = "folder_filled";
            QString targetPath = item->data(Qt::UserRole + 1).toString();

            // 1. 实时就地刷新左侧收藏项
            QIcon newIcon = UiHelper::getIcon(iconKey, QColor(finalColor), 18);
            item->setIcon(newIcon);
            item->setData(finalColor, Qt::UserRole + 3);

            // 2. 联动刷新子菜单自身的头部图标与内部 50 个小图标颜色
            iconMenu->setIcon(UiHelper::getIcon("folder_filled", QColor(finalColor)));
            for (const auto& btnPair : iconButtons) {
                btnPair.first->setIcon(UiHelper::getIcon(btnPair.second, QColor(finalColor), 18));
            }

            // 3. 全局 Command 发起设色，触发 MetadataManager 与 CentralEventHub
            if (!targetPath.isEmpty()) {
                AppCommand cmd;
                cmd.type = AppCommandType::SetColor;
                cmd.targetPaths = {targetPath};
                cmd.params["color"] = finalColor;
                CoreEngine::instance().executeCommand(cmd);
            }

            if (m_favoriteView && m_favoriteView->viewport()) {
                m_favoriteView->viewport()->update();
            }
        });
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Execute build:
   ```bash
   mkdir -p build && cd build && cmake .. && make -j$(nproc)
   ```
2. Verify that color marking and pinning via right-click context menu in ContentPanel / ColumnView / FavoritePanel emit CoreEngine commands and update all UI views simultaneously.
