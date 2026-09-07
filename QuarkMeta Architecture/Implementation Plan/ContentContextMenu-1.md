# Implementation Plan - ContentContextMenu (Monochrome SVG Icons & 10px Spacing)

## 1. Overview
This implementation plan specifies the exact changes required to equip **all context menu items** in `ContentContextMenu.cpp` with semantically matching SVG icons, strictly enforced as **neutral monochrome (`#EEEEEE`) with zero colored icons** (no red, yellow, blue, or green accents) and 10px icon-text spacing.

---

## 2. Modified Files List
- `src/ui/controllers/ContentContextMenu.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in Trash Context Menu (`ContentContextMenu.cpp`)

```
<<<<<<< SEARCH
    if (isTrashView) {
        if (onItem) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原")->setData(ContentPanel::ActionRestore);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
            menu.addSeparator();
        }
        menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原全部")->setData(ContentPanel::ActionRestoreAll);
        menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "清空回收站")->setData(ContentPanel::ActionEmptyTrash);
=======
    if (isTrashView) {
        if (onItem) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "还原")->setData(ContentPanel::ActionRestore);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
            menu.addSeparator();
        }
        menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "还原全部")->setData(ContentPanel::ActionRestoreAll);
        menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "清空回收站")->setData(ContentPanel::ActionEmptyTrash);
>>>>>>> REPLACE
```

---

### 3.2 Neutral Monochrome Icons for Drive Root (`ContentContextMenu.cpp`)

```
<<<<<<< SEARCH
        if (isDriveRoot) {
            menu.addAction("打开")->setData(ContentPanel::ActionOpen);
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);

            bool isFavDrive = FavoriteDao::containsPath(path);
            menu.addAction(isFavDrive ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);

            menu.addSeparator();

            QAction* actItemPaste = menu.addAction("粘贴");
            actItemPaste->setData(ContentPanel::ActionPaste);
            actItemPaste->setEnabled(m_panel->canPaste(path));

            menu.addAction("复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction("复制路径")->setData(ContentPanel::ActionCopyPath);

            QMenu* moreMenuDrive = menu.addMenu("更多");
            UiHelper::applyMenuStyle(moreMenuDrive);
=======
        if (isDriveRoot) {
            menu.addAction(UiHelper::getIcon("open", QColor("#EEEEEE"), 18), "打开")->setData(ContentPanel::ActionOpen);
            menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            menu.addAction(UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", QColor("#EEEEEE"), 18), isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);

            bool isFavDrive = FavoriteDao::containsPath(path);
            menu.addAction(UiHelper::getIcon(isFavDrive ? "close" : "star_filled", QColor("#EEEEEE"), 18), isFavDrive ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);

            menu.addSeparator();

            QAction* actItemPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴");
            actItemPaste->setData(ContentPanel::ActionPaste);
            actItemPaste->setEnabled(m_panel->canPaste(path));

            menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制路径")->setData(ContentPanel::ActionCopyPath);

            QMenu* moreMenuDrive = menu.addMenu(UiHelper::getIcon("more_horizontal", QColor("#EEEEEE"), 18), "更多");
            UiHelper::applyMenuStyle(moreMenuDrive);
>>>>>>> REPLACE
```

---

### 3.3 Neutral Monochrome Icons for File & Folder Items (`ContentContextMenu.cpp`)

```
<<<<<<< SEARCH
        } else {
            menu.addAction(isFolder ? "打开文件夹" : "打开")->setData(ContentPanel::ActionOpen);
            if (!isFolder) {
                menu.addAction("用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            }
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);

            bool isFavItem = FavoriteDao::containsPath(path);
            menu.addAction(isFavItem ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);

            menu.addSeparator();

            menu.addAction("复制")->setData(ContentPanel::ActionCopy);
            menu.addAction("剪切")->setData(ContentPanel::ActionCut);

            if (!isComputerRoot && !currentPath.isEmpty()) {
                std::wstring volSerial = MetadataManager::getVolumeSerialNumber(path.toStdWString());
                QStringList recentFolders = NavigationHistoryService::getRecentVisitedFolders(volSerial);
                recentFolders.removeAll(currentPath);

                QMenu* moveMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor("#3498db"), 18), "移动到");
                UiHelper::applyMenuStyle(moveMenu);

                auto performMoveTo = [this](const QString& targetDir) {
                    QStringList selectedPaths = m_panel->getSelectedPaths();
                    if (selectedPaths.isEmpty()) return;

                    DiskIoContext ioCtx;
                    ioCtx.sources = selectedPaths;
                    ioCtx.destination = targetDir;
                    ioCtx.isMove = true;

                    QPointer<ContentPanel> weakPanel(m_panel);
                    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
                        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
                            if (weakPanel) {
                                if (success) {
                                    weakPanel->refreshAll();
                                    ToolTipOverlay::instance()->showText(QCursor::pos(), "文件移动成功", 1500, QColor("#2ecc71"));
                                } else {
                                    ToolTipOverlay::instance()->showText(QCursor::pos(), "移动失败：物理写入未能完成", 2000, QColor("#e81123"));
                                }
                            }
                        });
                    });
                };

                for (const QString& recentDir : recentFolders) {
                    QAction* actMove = moveMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 16), recentDir);
                    connect(actMove, &QAction::triggered, this, [performMoveTo, recentDir]() {
                        performMoveTo(recentDir);
                    });
                }

                if (!recentFolders.isEmpty()) {
                    moveMenu->addSeparator();
                }

                QAction* actBrowseMove = moveMenu->addAction("浏览选择文件夹...");
                connect(actBrowseMove, &QAction::triggered, this, [this, performMoveTo]() {
                    QString selectedDir = FramelessFileDialog::getExistingDirectory(m_panel, "选择移动的目标文件夹", m_panel->currentPath());
                    if (!selectedDir.isEmpty()) {
                        performMoveTo(selectedDir);
                    }
                });
            }

            QAction* actItemPaste = menu.addAction("粘贴");
            actItemPaste->setData(ContentPanel::ActionPaste);
            actItemPaste->setEnabled(m_panel->canPaste(isFolder ? path : currentPath));

            menu.addAction("复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction("复制路径")->setData(ContentPanel::ActionCopyPath);

            QMenu* moreMenu = menu.addMenu("更多");
            UiHelper::applyMenuStyle(moreMenu);
=======
        } else {
            menu.addAction(UiHelper::getIcon(isFolder ? "folder_open" : "open", QColor("#EEEEEE"), 18), isFolder ? "打开文件夹" : "打开")->setData(ContentPanel::ActionOpen);
            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("launch", QColor("#EEEEEE"), 18), "用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            }
            menu.addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, this, [this, view, &menu](const QString& hexColor) {
                auto indexes = view->selectionModel()->selectedIndexes();
                for (const auto& idx : indexes) {
                    if (idx.column() == 0) m_panel->getProxyModel()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(IsLockedRole).toBool();
            menu.addAction(UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", QColor("#EEEEEE"), 18), isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);

            bool isFavItem = FavoriteDao::containsPath(path);
            menu.addAction(UiHelper::getIcon(isFavItem ? "close" : "star_filled", QColor("#EEEEEE"), 18), isFavItem ? "取消收藏" : "添加至收藏夹")->setData(ContentPanel::ActionAddToFavorites);

            menu.addSeparator();

            menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE"), 18), "复制")->setData(ContentPanel::ActionCopy);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);

            if (!isComputerRoot && !currentPath.isEmpty()) {
                std::wstring volSerial = MetadataManager::getVolumeSerialNumber(path.toStdWString());
                QStringList recentFolders = NavigationHistoryService::getRecentVisitedFolders(volSerial);
                recentFolders.removeAll(currentPath);

                QMenu* moveMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18), "移动到");
                UiHelper::applyMenuStyle(moveMenu);

                auto performMoveTo = [this](const QString& targetDir) {
                    QStringList selectedPaths = m_panel->getSelectedPaths();
                    if (selectedPaths.isEmpty()) return;

                    DiskIoContext ioCtx;
                    ioCtx.sources = selectedPaths;
                    ioCtx.destination = targetDir;
                    ioCtx.isMove = true;

                    QPointer<ContentPanel> weakPanel(m_panel);
                    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
                        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
                            if (weakPanel) {
                                if (success) {
                                    weakPanel->refreshAll();
                                    ToolTipOverlay::instance()->showText(QCursor::pos(), "文件移动成功", 1500, QColor("#2ecc71"));
                                } else {
                                    ToolTipOverlay::instance()->showText(QCursor::pos(), "移动失败：物理写入未能完成", 2000, QColor("#e81123"));
                                }
                            }
                        });
                    });
                };

                for (const QString& recentDir : recentFolders) {
                    QAction* actMove = moveMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 16), recentDir);
                    connect(actMove, &QAction::triggered, this, [performMoveTo, recentDir]() {
                        performMoveTo(recentDir);
                    });
                }

                if (!recentFolders.isEmpty()) {
                    moveMenu->addSeparator();
                }

                QAction* actBrowseMove = moveMenu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "浏览选择文件夹...");
                connect(actBrowseMove, &QAction::triggered, this, [this, performMoveTo]() {
                    QString selectedDir = FramelessFileDialog::getExistingDirectory(m_panel, "选择移动的目标文件夹", m_panel->currentPath());
                    if (!selectedDir.isEmpty()) {
                        performMoveTo(selectedDir);
                    }
                });
            }

            QAction* actItemPaste = menu.addAction(UiHelper::getIcon("paste", QColor("#EEEEEE"), 18), "粘贴");
            actItemPaste->setData(ContentPanel::ActionPaste);
            actItemPaste->setEnabled(m_panel->canPaste(isFolder ? path : currentPath));

            menu.addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制路径")->setData(ContentPanel::ActionCopyPath);

            QMenu* moreMenu = menu.addMenu(UiHelper::getIcon("more_horizontal", QColor("#EEEEEE"), 18), "更多");
            UiHelper::applyMenuStyle(moreMenu);
>>>>>>> REPLACE
```

---

### 3.4 Neutral Monochrome Icons for Submenus, Renaming, Refresh & Delete (`ContentContextMenu.cpp`)

```
<<<<<<< SEARCH
            QAction* actCopyTags = menu.addAction("复制标签");
            actCopyTags->setData(ContentPanel::ActionCopyTags);
            actCopyTags->setEnabled(!cleanTags.isEmpty());

            QAction* actPasteTags = menu.addAction("粘贴标签");
            actPasteTags->setData(ContentPanel::ActionPasteTags);
            actPasteTags->setEnabled(ClipboardService::instance().hasCopiedTags());

            QAction* actRepeat = menu.addAction(LastOperationManager::instance().displayText());
            actRepeat->setData(ContentPanel::ActionRepeatLastOp);
            actRepeat->setEnabled(LastOperationManager::instance().hasOperation());

            menu.addAction("重命名")->setData(ContentPanel::ActionRename);

            menu.addSeparator();
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);

            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("sync", QColor("#3498db"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);

                QMenu* cryptoMenu = menu.addMenu("外壳保护");
                UiHelper::applyMenuStyle(cryptoMenu);
                cryptoMenu->addAction("执行外壳保护")->setData(ContentPanel::ActionEncrypt);
                cryptoMenu->addAction("解除保护")->setData(ContentPanel::ActionDecrypt);
                cryptoMenu->addAction("修改保护密码")->setData(ContentPanel::ActionChangePwd);
            }
=======
            QAction* actCopyTags = menu.addAction(UiHelper::getIcon("tag", QColor("#EEEEEE"), 18), "复制标签");
            actCopyTags->setData(ContentPanel::ActionCopyTags);
            actCopyTags->setEnabled(!cleanTags.isEmpty());

            QAction* actPasteTags = menu.addAction(UiHelper::getIcon("paste_tag", QColor("#EEEEEE"), 18), "粘贴标签");
            actPasteTags->setData(ContentPanel::ActionPasteTags);
            actPasteTags->setEnabled(ClipboardService::instance().hasCopiedTags());

            QAction* actRepeat = menu.addAction(UiHelper::getIcon("repeat", QColor("#EEEEEE"), 18), LastOperationManager::instance().displayText());
            actRepeat->setData(ContentPanel::ActionRepeatLastOp);
            actRepeat->setEnabled(LastOperationManager::instance().hasOperation());

            menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE"), 18), "重命名")->setData(ContentPanel::ActionRename);

            menu.addSeparator();
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "刷新")->setData(ContentPanel::ActionRefresh);

            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);

                QMenu* cryptoMenu = menu.addMenu(UiHelper::getIcon("shield", QColor("#EEEEEE"), 18), "外壳保护");
                UiHelper::applyMenuStyle(cryptoMenu);
                cryptoMenu->addAction(UiHelper::getIcon("lock", QColor("#EEEEEE"), 18), "执行外壳保护")->setData(ContentPanel::ActionEncrypt);
                cryptoMenu->addAction(UiHelper::getIcon("unlock", QColor("#EEEEEE"), 18), "解除保护")->setData(ContentPanel::ActionDecrypt);
                cryptoMenu->addAction(UiHelper::getIcon("key", QColor("#EEEEEE"), 18), "修改保护密码")->setData(ContentPanel::ActionChangePwd);
            }
>>>>>>> REPLACE
```

---

### 3.5 Neutral Monochrome Icons for Submenu Outers (`ContentContextMenu.cpp`)

```
<<<<<<< SEARCH
    // 排序二级子菜单
    QMenu* sortMenu = menu.addMenu("排序");
    UiHelper::applyMenuStyle(sortMenu);
=======
    // 排序二级子菜单
    QMenu* sortMenu = menu.addMenu(UiHelper::getIcon("sort", QColor("#EEEEEE"), 18), "排序");
    UiHelper::applyMenuStyle(sortMenu);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 删除子菜单
    if (onItem && !isDriveRoot) {
        menu.addSeparator();
        QMenu* delMenu = menu.addMenu("删除");
        UiHelper::applyMenuStyle(delMenu);
        delMenu->addAction("移入回收站")->setData(ContentPanel::ActionDelete);
        delMenu->addAction("永久删除")->setData(ContentPanel::ActionSecureDelete);
    }
=======
    // 删除子菜单
    if (onItem && !isDriveRoot) {
        menu.addSeparator();
        QMenu* delMenu = menu.addMenu(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "删除");
        UiHelper::applyMenuStyle(delMenu);
        delMenu->addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "移入回收站")->setData(ContentPanel::ActionDelete);
        delMenu->addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   Build the application using CMake:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual & Behavioral Verification**:
   - Right-click anywhere in `ContentPanel` (file, folder, drive root, blank area, trash view).
   - Confirm visually that **every single menu item** contains a neutral monochrome (`#EEEEEE`) SVG icon.
   - Confirm that zero colored icons (no red, yellow, blue, or green accents) appear in any right-click menu.
