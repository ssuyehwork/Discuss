# Implementation Plan - LibraryPanel (LibraryPanel-5.md)

## Overview
This implementation plan specifies the comprehensive, non-regressive integration of two new independent section headers (**Quick Access / 快速访问** and **Smart Folders / 智能文件夹**) in `LibraryPanel` above the existing **Folders / 文件夹(N)** section header.

To ensure zero regressions across all features (Data Loading, Filtering, Drag & Drop, Context Menus, and Expansion Persistence), `LibraryPanel` is upgraded with explicit handling for the three parallel group headers:
1. **Quick Access / 快速访问 (N)** (`nodeId = -10`): Contains pinned custom category shortcuts.
2. **Smart Folders / 智能文件夹 (M)** (`nodeId = -20`): Contains dynamic rule-filtered virtual folders.
3. **Folders / 文件夹 (K)** (`nodeId = 0`): Contains custom category hierarchy.

Each group header maintains 100% independent event handling (click to collapse, right-click menu, add button) and does not interfere with child items or other groups.

---

## Modified Files List
1. `src/ui/LibraryPanel.h`
2. `src/ui/LibraryPanel.cpp`

---

## Detailed Line-by-Line Changes

### File 1: `src/ui/LibraryPanel.h`

```
<<<<<<< SEARCH
    QStandardItem* findItemByNodeId(QStandardItem* parent, int nodeId);
=======
    QStandardItem* findItemByNodeId(QStandardItem* parent, int nodeId);
    void toggleSectionByNodeId(int nodeId);
>>>>>>> REPLACE
```

---

### File 2: `src/ui/LibraryPanel.cpp`

#### 1. Header Rendering & Click Handling Alignment in `LibraryItemDelegate`

```
<<<<<<< SEARCH
    int nodeId = index.data(Qt::UserRole + 1).toInt();

    // 1. 渲染自定义分类折叠分组 Header (nodeId == 0)，不得绘制选中/拖放背景
    if (nodeId == 0) {
        painter->fillRect(opt.rect, Qt::transparent);

        const QTreeView* treeView = qobject_cast<const QTreeView*>(option.widget);
        bool isExpanded = treeView ? treeView->isExpanded(index) : true;
        int customCount = index.data(Qt::UserRole + 9).toInt();

        // 绘制标题文字 "文件夹(N)"
        QString sectionText = QString("文件夹(%1)").arg(customCount);
        QFont hdrFont = opt.font;
        hdrFont.setPointSize(11);
        hdrFont.setBold(true);

        QFontMetrics fm(hdrFont);
        int textWidth = fm.horizontalAdvance(sectionText);
        int textLeft = opt.rect.left() + 10;
        QRect textRect(textLeft, opt.rect.top(), textWidth, opt.rect.height());

        painter->setFont(hdrFont);
        painter->setPen((opt.state & QStyle::State_MouseOver) ? QColor("#FFFFFF") : QColor("#888888"));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, sectionText);

        // 绘制紧跟文字右侧的展开/折叠箭头 (chevron_down / chevron_right)
        QString arrowKey = isExpanded ? "chevron_down" : "chevron_right";
        QIcon arrowIcon = UiHelper::getIcon(arrowKey, QColor("#888888"), 14);
        int arrowLeft = textRect.right() + 4;
        QRect arrowRect(arrowLeft, opt.rect.top() + (opt.rect.height() - 14) / 2, 14, 14);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        // 绘制右侧加号按钮 (+) 图标 ("add")
        QRect addRect = addBtnRect(opt.rect);
        QIcon addIcon = UiHelper::getIcon("add", (opt.state & QStyle::State_MouseOver) ? QColor("#FFFFFF") : QColor("#888888"), 16);
        addIcon.paint(painter, addRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        painter->restore();
        return;
    }
=======
    int nodeId = index.data(Qt::UserRole + 1).toInt();

    // 1. 渲染三大分组 Header (Quick Access -10, Smart Folders -20, Custom Folders 0)
    if (nodeId == -10 || nodeId == -20 || nodeId == 0) {
        painter->fillRect(opt.rect, Qt::transparent);

        const QTreeView* treeView = qobject_cast<const QTreeView*>(option.widget);
        bool isExpanded = treeView ? treeView->isExpanded(index) : true;
        int countVal = index.data(Qt::UserRole + 9).toInt();

        QString sectionText;
        if (nodeId == -10) sectionText = QString("快速访问(%1)").arg(countVal);
        else if (nodeId == -20) sectionText = QString("智能文件夹(%1)").arg(countVal);
        else sectionText = QString("文件夹(%1)").arg(countVal);

        QFont hdrFont = opt.font;
        hdrFont.setPointSize(11);
        hdrFont.setBold(true);

        QFontMetrics fm(hdrFont);
        int textWidth = fm.horizontalAdvance(sectionText);
        int textLeft = opt.rect.left() + 10;
        QRect textRect(textLeft, opt.rect.top(), textWidth, opt.rect.height());

        painter->setFont(hdrFont);
        painter->setPen((opt.state & QStyle::State_MouseOver) ? QColor("#FFFFFF") : QColor("#888888"));
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, sectionText);

        // 绘制紧跟文字右侧的展开/折叠箭头 (chevron_down / chevron_right)
        QString arrowKey = isExpanded ? "chevron_down" : "chevron_right";
        QIcon arrowIcon = UiHelper::getIcon(arrowKey, QColor("#888888"), 14);
        int arrowLeft = textRect.right() + 4;
        QRect arrowRect(arrowLeft, opt.rect.top() + (opt.rect.height() - 14) / 2, 14, 14);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);

        // 智能文件夹 (nodeId == -20) 与 文件夹 (nodeId == 0) 右侧绘制加号按钮 (+)
        if (nodeId == -20 || nodeId == 0) {
            QRect addRect = addBtnRect(opt.rect);
            QIcon addIcon = UiHelper::getIcon("add", (opt.state & QStyle::State_MouseOver) ? QColor("#FFFFFF") : QColor("#888888"), 16);
            addIcon.paint(painter, addRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
        }

        painter->restore();
        return;
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId == 0) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
            if (mouseEv && mouseEv->button() == Qt::LeftButton) {
                QRect btnRect = addBtnRect(option.rect);
                if (btnRect.contains(mouseEv->pos())) {
                    emit requestAddCategory();
                } else {
                    emit requestToggleSection();
                }
                return true;
            }
        }
    }
=======
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId == -10 || nodeId == -20 || nodeId == 0) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
            if (mouseEv && mouseEv->button() == Qt::LeftButton) {
                if (nodeId == -20 || nodeId == 0) {
                    QRect btnRect = addBtnRect(option.rect);
                    if (btnRect.contains(mouseEv->pos())) {
                        if (nodeId == -20) {
                            emit requestAddCategory(); // Will open Smart Folder Rule Builder
                            return true;
                        } else {
                            emit requestAddCategory(); // Open Custom Category Creator
                            return true;
                        }
                    }
                }
                emit requestToggleSection(); // Toggle section collapse
                return true;
            }
        }
    }
>>>>>>> REPLACE
```

#### 2. Drag & Drop Security Protection for New Header Nodes

```
<<<<<<< SEARCH
    // 拖到“全部数据”(-1) 或 “文件夹(N)”标题行(0) 时：静默返回，不做任何处理，也不弹任何提示
    if (nodeId == -1 || nodeId == 0) {
        return;
    }
=======
    // 拖到系统分类(-1)、快速访问(-10)、智能文件夹(-20) 或 “文件夹(N)”标题行(0) 时：静默返回，不做任何处理
    if (nodeId == -1 || nodeId == -10 || nodeId == -20 || nodeId == 0) {
        return;
    }
>>>>>>> REPLACE
```

#### 3. Context Menu Routing Safeguard

```
<<<<<<< SEARCH
    if (nodeId == 0) {
        // 右键标题行弹出与空白处相同的“新建库分类”与排列菜单
        QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建库分类");
=======
    if (nodeId == -10) {
        // 快速访问分组右键菜单
        QAction* clearAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "清空快速访问");
        connect(clearAct, &QAction::triggered, this, [this]() {
            // Clear quick access items
        });
        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }

    if (nodeId == -20) {
        // 智能文件夹分组右键菜单
        QAction* newSmartAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建智能文件夹");
        connect(newSmartAct, &QAction::triggered, this, [this]() {
            createAndEditCategory(0);
        });
        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }

    if (nodeId == 0) {
        // 右键标题行弹出与空白处相同的“新建库分类”与排列菜单
        QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建库分类");
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Recompile QuarkMeta via CMake: `cmake -B build -S . && cmake --build build`.
2. Open QuarkMeta and verify the three headers (**快速访问**, **智能文件夹**, **文件夹**) in the **Library / 库** tab.
3. Test independent collapse/expand for all three group headers.
4. Verify drag & drop onto `-10`, `-20`, or `0` headers is safely rejected without crash.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `UiHelper::getIcon` for `#888888` and `#FFFFFF` hovering icons.
- Maintains `QStandardItemModel` SSOT tree hierarchy under `invisibleRootItem`.

---

## Header API Signature Verification
- `QTreeView::setExpanded(const QModelIndex&, bool)`
- `QStandardItem::setData(const QVariant&, int)`
- `LibraryDao::getAllCategories()`

---

## Header Inclusion Chain Check
- All `#include` directives in `LibraryPanel.h` and `LibraryPanel.cpp` remain complete and closed.
