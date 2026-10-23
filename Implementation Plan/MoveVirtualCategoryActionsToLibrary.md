# Implementation Plan - Move Virtual Category Actions to Library Tab Exclusively

## Overview
This plan cleans up the right-click context menu in `FavoritePanel.cpp` by removing virtual category management actions ("新建文件夹", "新建子文件夹", "设置预设标签"), while making sure that virtual category management and preset tags reside exclusively under the "库" (`LibraryPanel`) tab.

---

## Modified Files List
- `src/ui/FavoritePanel.cpp`
- `src/ui/LibraryPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. Update `src/ui/FavoritePanel.cpp`

Remove virtual category creation and preset tag dialog actions from `FavoritePanel::onFavoriteContextMenu`:

```diff
<<<<<<< SEARCH
    if (!index.isValid()) {
        // 空白处右键：新建文件夹 (直接进入行内编辑)
        QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建文件夹");
        connect(newCatAct, &QAction::triggered, this, [this]() {
            createAndEditCategory(0);
        });

        auto* sortMenu = menu.addMenu(UiHelper::getIcon("list_ul", QColor("#AAAAAA")), "排列");
        UiHelper::applyMenuStyle(sortMenu);
        QAction* sortAsc = sortMenu->addAction("按名称 (A→Z)");
        connect(sortAsc, &QAction::triggered, this, [this]() { sortItemsByName(true); });
        QAction* sortDesc = sortMenu->addAction("按名称 (Z→A)");
        connect(sortDesc, &QAction::triggered, this, [this]() { sortItemsByName(false); });

        menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));
        return;
    }

    QString path = index.data(Qt::UserRole + 1).toString();
    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();
    int nodeId = index.data(Qt::UserRole + 6).toInt();
    bool isVirtual = index.data(Qt::UserRole + 7).toBool();

    if (curIconKey.isEmpty()) curIconKey = "folder_filled";
    if (curColorHex.isEmpty()) curColorHex = "#888888";

    QFileInfo fi(path);
    bool isFolder = isVirtual ? true : fi.isDir();
    bool isItemRemoved = false;

    // 1. 新建文件夹与新建子文件夹（直接创建并唤起行内编辑）
    QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建文件夹");
    connect(newCatAct, &QAction::triggered, this, [this]() {
        createAndEditCategory(0);
    });

    if (isFolder) {
        QAction* newSubCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建子文件夹");
        connect(newSubCatAct, &QAction::triggered, this, [this, nodeId]() {
            createAndEditCategory(nodeId);
        });
    }

    menu.addSeparator();

    // 2. 设置预设标签（自动标签对话框 PresetTagsDialog）
    QAction* presetTagAct = menu.addAction(UiHelper::getIcon("tag_filled", QColor("#9B59B6")), "设置预设标签");
    connect(presetTagAct, &QAction::triggered, this, [this, nodeId]() {
        PresetTagsDialog dlg(nodeId, this);
        dlg.exec();
    });
=======
    if (!index.isValid()) {
        auto* sortMenu = menu.addMenu(UiHelper::getIcon("list_ul", QColor("#AAAAAA")), "排列");
        UiHelper::applyMenuStyle(sortMenu);
        QAction* sortAsc = sortMenu->addAction("按名称 (A→Z)");
        connect(sortAsc, &QAction::triggered, this, [this]() { sortItemsByName(true); });
        QAction* sortDesc = sortMenu->addAction("按名称 (Z→A)");
        connect(sortDesc, &QAction::triggered, this, [this]() { sortItemsByName(false); });

        menu.exec(m_favoriteView->viewport()->mapToGlobal(pos));
        return;
    }

    QString path = index.data(Qt::UserRole + 1).toString();
    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();
    int nodeId = index.data(Qt::UserRole + 6).toInt();
    bool isVirtual = index.data(Qt::UserRole + 7).toBool();

    if (curIconKey.isEmpty()) curIconKey = "folder_filled";
    if (curColorHex.isEmpty()) curColorHex = "#888888";

    QFileInfo fi(path);
    bool isFolder = isVirtual ? true : fi.isDir();
    bool isItemRemoved = false;
>>>>>>> REPLACE
```

---

### 2. Update `src/ui/LibraryPanel.cpp`

Add "新建子分类" and "设置预设标签" actions to `LibraryPanel::onCategoryContextMenu`:

```diff
<<<<<<< SEARCH
    if (index.isValid()) {
        int nodeId = index.data(Qt::UserRole + 1).toInt();
        QAction* renameAct = menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE")), "重命名");
        connect(renameAct, &QAction::triggered, this, [this, index]() {
            if (m_treeView) m_treeView->edit(index);
        });

        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "删除分类");
        connect(removeAct, &QAction::triggered, this, [this, nodeId]() {
            LibraryService::instance().removeCategory(nodeId);
        });
    }
=======
    if (index.isValid()) {
        int nodeId = index.data(Qt::UserRole + 1).toInt();

        QAction* newSubCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建子分类");
        connect(newSubCatAct, &QAction::triggered, this, [this, nodeId]() {
            createAndEditCategory(nodeId);
        });

        QAction* presetTagAct = menu.addAction(UiHelper::getIcon("tag_filled", QColor("#9B59B6")), "设置预设标签");
        connect(presetTagAct, &QAction::triggered, this, [this, nodeId]() {
            PresetTagsDialog dlg(nodeId, this);
            dlg.exec();
        });

        menu.addSeparator();

        QAction* renameAct = menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE")), "重命名");
        connect(renameAct, &QAction::triggered, this, [this, index]() {
            if (m_treeView) m_treeView->edit(index);
        });

        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "删除分类");
        connect(removeAct, &QAction::triggered, this, [this, nodeId]() {
            LibraryService::instance().removeCategory(nodeId);
        });
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Recompile standard C++ build target.
2. Right click inside "收藏夹" (Bookmarks) tab: verify that "新建文件夹", "新建子文件夹", and "设置预设标签" are absent.
3. Right click inside "库" (Library) tab: verify "新建库分类", "新建子分类", and "设置预设标签" are present and functional.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `PresetTagsDialog` and `LibraryService::createCategory` SSOT APIs.

---

## Header API Signature Verification
- No API signature changes.

---

## Header Inclusion Chain & Type Completeness Check
- `LibraryPanel.cpp`: Include `"PresetTagsDialog.h"`.
