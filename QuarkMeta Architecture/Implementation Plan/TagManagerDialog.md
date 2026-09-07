# Implementation Plan - TagManagerDialog (Monochrome SVG Icons)

## 1. Overview
This implementation plan specifies the changes required to equip **all context menu items** in `TagManagerDialog.cpp` (group context menu and tag item context menu) with neutral monochrome (`#EEEEEE`) SVG icons.

---

## 2. Modified Files List
- `src/ui/TagManagerDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Neutral Monochrome Icons in Group Context Menu (`src/ui/TagManagerDialog.cpp`)

```
<<<<<<< SEARCH
void TagManagerDialog::showGroupContextMenu(int groupId, const QString& groupName, const QPoint& globalPos) {
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);
    menu.addAction("重命名分组")->setData(1);
    menu.addAction("删除分组")->setData(2);
=======
void TagManagerDialog::showGroupContextMenu(int groupId, const QString& groupName, const QPoint& globalPos) {
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);
    menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE"), 18), "重命名分组")->setData(1);
    menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "删除分组")->setData(2);
>>>>>>> REPLACE
```

---

### 3.2 Neutral Monochrome Icons in Tag Item Context Menu (`src/ui/TagManagerDialog.cpp`)

```
<<<<<<< SEARCH
void TagManagerDialog::showTagContextMenu(const QString& tagName, const QPoint& globalPos) {
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    // 1. 添加到分组子菜单
    QMenu* groupSubMenu = menu.addMenu("添加到分组...");
    UiHelper::applyMenuStyle(groupSubMenu);
    for (const auto& grp : m_allGroups) {
        if (grp.id <= 0) continue;
        QAction* actGrp = groupSubMenu->addAction(grp.name);
        connect(actGrp, &QAction::triggered, this, [this, tagName, grp]() {
            TagLexiconService::instance().moveTagToGroup(tagName, grp.id);
            refreshSidebar();
            refreshTags();
        });
    }

    // 2. 从当前组移出（仅在具体组视图有效）
    if (m_activeGroupId > 0) {
        menu.addAction("从当前组移出")->setData(1);
    }

    menu.addSeparator();
    menu.addAction("删除此标签")->setData(2);
=======
void TagManagerDialog::showTagContextMenu(const QString& tagName, const QPoint& globalPos) {
    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    // 1. 添加到分组子菜单
    QMenu* groupSubMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE"), 18), "添加到分组...");
    UiHelper::applyMenuStyle(groupSubMenu);
    for (const auto& grp : m_allGroups) {
        if (grp.id <= 0) continue;
        QAction* actGrp = groupSubMenu->addAction(UiHelper::getIcon("tag", QColor("#EEEEEE"), 16), grp.name);
        connect(actGrp, &QAction::triggered, this, [this, tagName, grp]() {
            TagLexiconService::instance().moveTagToGroup(tagName, grp.id);
            refreshSidebar();
            refreshTags();
        });
    }

    // 2. 从当前组移出（仅在具体组视图有效）
    if (m_activeGroupId > 0) {
        menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE"), 18), "从当前组移出")->setData(1);
    }

    menu.addSeparator();
    menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "删除此标签")->setData(2);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Visual Verification**:
   - Open Tag Manager dialog, right-click on groups and tag pills.
   - Confirm that all group/tag context menu options feature neutral monochrome (`#EEEEEE`) SVG icons.
