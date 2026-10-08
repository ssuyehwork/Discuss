# SvgIconsAndSortMenuRefactor.md Implementation Plan

## 1. Overview
This implementation plan covers 5 specific icon and menu refactoring tasks:
1. Remove old `"refresh"` icon from `SvgIcons.h`.
2. Rename `"sync"` icon entry in `SvgIcons.h` to `"refresh"`.
3. Update all existing C++ references from `"sync"` to `"refresh"`.
4. Change `"重新提取缩略图"` menu action icon from `"sync"`/`"refresh"` to `"repeat"`.
5. Dynamically update the `"排序"` menu icon based on `m_panel->currentSortOrder()`: `"arrow_up_long"` for ascending (`Qt::AscendingOrder`) and `"arrow_down_long"` for descending (`Qt::DescendingOrder`).

## 2. Modified Files List
- `src/ui/SvgIcons.h`
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/AddressBar.cpp`
- `src/ui/PanelLayoutManager.cpp`
- `src/ui/dialogs/FramelessConflictDialog.cpp`
- `src/ui/NavPanel.cpp`
- `src/ui/QuickLookWindow.cpp`

## 3. Detailed Line-by-Line Changes

```path
src/ui/SvgIcons.h
```

<<<<<<< SEARCH
        {"trash", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="3 6 5 6 21 6" /><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" /><line x1="10" y1="11" x2="10" y2="17" /><line x1="14" y1="11" x2="14" y2="17" /></svg>)svg"},
        {"refresh", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21.5 2v6h-6"></path><path d="M2.5 22v-6h6"></path><path d="M21.5 8A10 10 0 0 0 6 3.5l-3.5 4"></path><path d="M2.5 16A10 10 0 0 0 18 20.5l3.5-4"></path><circle cx="12" cy="12" r="1.5" fill="currentColor"></circle></svg>)svg"},
        {"search", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>)svg"},
=======
        {"trash", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="3 6 5 6 21 6" /><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" /><line x1="10" y1="11" x2="10" y2="17" /><line x1="14" y1="11" x2="14" y2="17" /></svg>)svg"},
        {"search", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>)svg"},
>>>>>>> REPLACE

<<<<<<< SEARCH
        {"scan", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 7V5a2 2 0 0 1 2-2h2"></path><path d="M17 3h2a2 2 0 0 1 2 2v2"></path><path d="M21 17v2a2 2 0 0 1-2 2h-2"></path><path d="M7 21H5a2 2 0 0 1-2-2v-2"></path><line x1="7" y1="12" x2="17" y2="12"></line></svg>)svg"},
        {"sync", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 2v6h-6"/><path d="M3 12a9 9 0 0 1 15-6.7L21 8"/><path d="M3 22v-6h6"/><path d="M21 12a9 9 0 0 1-15 6.7L3 16"/></svg>)svg"},
        {"camera", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"></path><circle cx="12" cy="13" r="4"></circle></svg>)svg"},
=======
        {"scan", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 7V5a2 2 0 0 1 2-2h2"></path><path d="M17 3h2a2 2 0 0 1 2 2v2"></path><path d="M21 17v2a2 2 0 0 1-2 2h-2"></path><path d="M7 21H5a2 2 0 0 1-2-2v-2"></path><line x1="7" y1="12" x2="17" y2="12"></line></svg>)svg"},
        {"refresh", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 2v6h-6"/><path d="M3 12a9 9 0 0 1 15-6.7L21 8"/><path d="M3 22v-6h6"/><path d="M21 12a9 9 0 0 1-15 6.7L3 16"/></svg>)svg"},
        {"camera", R"svg(<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"></path><circle cx="12" cy="13" r="4"></circle></svg>)svg"},
>>>>>>> REPLACE

```path
src/ui/controllers/ContentContextMenu.cpp
```

<<<<<<< SEARCH
    if (isTrashView) {
        if (onItem) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "还原")->setData(ContentPanel::ActionRestore);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
            menu.addSeparator();
        }
        menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "还原全部")->setData(ContentPanel::ActionRestoreAll);
=======
    if (isTrashView) {
        if (onItem) {
            menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "还原")->setData(ContentPanel::ActionRestore);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("trash", QColor("#EEEEEE"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
            menu.addSeparator();
        }
        menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "还原全部")->setData(ContentPanel::ActionRestoreAll);
>>>>>>> REPLACE

<<<<<<< SEARCH
            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);

                QMenu* cryptoMenu = menu.addMenu(UiHelper::getIcon("shield", QColor("#EEEEEE"), 18), "外壳保护");
=======
            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("repeat", QColor("#EEEEEE"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);

                QMenu* cryptoMenu = menu.addMenu(UiHelper::getIcon("shield", QColor("#EEEEEE"), 18), "外壳保护");
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 排序二级子菜单
    QMenu* sortMenu = menu.addMenu(UiHelper::getIcon("sort", QColor("#EEEEEE"), 18), "排序");
    UiHelper::applyMenuStyle(sortMenu);
=======
    // 排序二级子菜单
    bool isAscending = (m_panel->currentSortOrder() == Qt::AscendingOrder);
    QString sortIconKey = isAscending ? "arrow_up_long" : "arrow_down_long";
    QMenu* sortMenu = menu.addMenu(UiHelper::getIcon(sortIconKey, QColor("#EEEEEE"), 18), "排序");
    UiHelper::applyMenuStyle(sortMenu);
>>>>>>> REPLACE

```path
src/ui/AddressBar.cpp
```

<<<<<<< SEARCH
    m_btnRefresh = new QPushButton(m_addressContainer);
    m_btnRefresh->setFixedSize(30, 30);
    m_btnRefresh->setIcon(UiHelper::getIcon("sync", QColor("#CCCCCC"), 16));
    m_btnRefresh->setProperty("tooltipText", "刷新 (F5)");
=======
    m_btnRefresh = new QPushButton(m_addressContainer);
    m_btnRefresh->setFixedSize(30, 30);
    m_btnRefresh->setIcon(UiHelper::getIcon("refresh", QColor("#CCCCCC"), 16));
    m_btnRefresh->setProperty("tooltipText", "刷新 (F5)");
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (obj == m_btnRefresh) {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) {
            m_btnRefresh->setIcon(UiHelper::getIcon("sync", Qt::white, 16));
            QString text = m_btnRefresh->property("tooltipText").toString();
            if (!text.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            }
        } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave) {
            m_btnRefresh->setIcon(UiHelper::getIcon("sync", QColor("#CCCCCC"), 16));
            ToolTipOverlay::hideTip();
        }
    }
=======
    if (obj == m_btnRefresh) {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) {
            m_btnRefresh->setIcon(UiHelper::getIcon("refresh", Qt::white, 16));
            QString text = m_btnRefresh->property("tooltipText").toString();
            if (!text.isEmpty()) {
                ToolTipOverlay::instance()->showText(QCursor::pos(), text, 0);
            }
        } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave) {
            m_btnRefresh->setIcon(UiHelper::getIcon("refresh", QColor("#CCCCCC"), 16));
            ToolTipOverlay::hideTip();
        }
    }
>>>>>>> REPLACE

```path
src/ui/PanelLayoutManager.cpp
```

<<<<<<< SEARCH
    QAction* resetAct = menu->addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "重置分栏");
=======
    QAction* resetAct = menu->addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "重置分栏");
>>>>>>> REPLACE

```path
src/ui/dialogs/FramelessConflictDialog.cpp
```

<<<<<<< SEARCH
    btnAutoRename->setIcon(UiHelper::getIcon("sync", QColor("#EEEEEE"), 14));
=======
    btnAutoRename->setIcon(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 14));
>>>>>>> REPLACE

```path
src/ui/NavPanel.cpp
```

<<<<<<< SEARCH
        QAction* actRestore = menu.addAction(UiHelper::getIcon("sync", QColor("#EEEEEE"), 18), "还原全部");
=======
        QAction* actRestore = menu.addAction(UiHelper::getIcon("refresh", QColor("#EEEEEE"), 18), "还原全部");
>>>>>>> REPLACE

```path
src/ui/QuickLookWindow.cpp
```

<<<<<<< SEARCH
    QAction* actRotate = menu.addAction(UiHelper::getIcon("sync", QColor("#FFFFFF"), 18), "旋转");
=======
    QAction* actRotate = menu.addAction(UiHelper::getIcon("refresh", QColor("#FFFFFF"), 18), "旋转");
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. Run CMake build / compile script to compile the project.
2. Launch the application and right-click on any item/blank space in ContentPanel.
3. Verify that:
   - AddressBar refresh button displays the double-arrow circle sync icon (`refresh`).
   - "重新提取缩略图" displays the repeat icon (`repeat`).
   - "排序" displays `arrow_up_long` when current sort order is Ascending, and `arrow_down_long` when Descending.
   - All other places ("重置分栏", "旋转", "还原全部") correctly display `refresh`.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `UiHelper::getIcon` SSOT lookup mechanism.
- Reused existing `m_panel->currentSortOrder()` to dynamically fetch sort direction.

## 6. Header API Signature Verification
- `ContentPanel::currentSortOrder() const`: Returns `Qt::SortOrder` defined in `ContentPanel.h`.
- `UiHelper::getIcon(const QString& key, const QColor& color, int size)`: Static method in `UiHelper.h`.

## 7. Header Inclusion Chain & Type Completeness Check
- All modified `.cpp` files already include `UiHelper.h` and `<QMenu>`.
- Type completeness for `Qt::SortOrder` and `UiHelper` remains intact.
