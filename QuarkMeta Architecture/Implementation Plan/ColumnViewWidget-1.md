# ColumnViewWidget Implementation Plan - Drag & Drop Migration, Target Highlighting & Context Menu Paste

This implementation plan details the technical architecture and exact code changes required to restore full file migration capabilities, target folder drop-highlighting, right-click context menu "Paste" action, and parent folder secondary highlighting in Column View (Miller Columns).

---

## Overview of Issues & Solution Strategy

1. **Persistent Parent Folder Secondary Highlighting (父文件夹二级持久高亮)**
   - **Problem**: When navigating through column panes in Column View, clicking/selecting a file item in column N clears or suppresses the selection in column N-1 (parent column), causing the user to lose visual context of which parent folder spawned the active column.
   - **Solution**: Enhance `ColumnItemDelegate` and `ColumnViewWidget` state management to draw a dedicated secondary background highlight (using standard theme token `#2A2A2A` or distinct secondary active state color `#383838`) for parent folder items whose sub-directory is currently open in the adjacent child column, regardless of active item selections in child columns.

2. **Cross-Column Drag & Drop File Migration (跨列拖拽文件/文件夹迁移)**
   - **Problem**: `DropListView` delegates drag-and-drop to `ViewDragDropHelper`, which only emits `pathsDropped` with the target index relative to the single `DropListView` receiving the drop. In `ColumnViewPane`, `DropListView::pathsDropped` is currently not connected to `ContentPanel::onPathsDropped`, nor is the source pane/target pane directory context preserved.
   - **Solution**:
     - Connect `DropListView::pathsDropped` in `ColumnViewPane` to `ContentPanel::onPathsDropped`.
     - In `DropListView` / `ViewDragDropHelper` handle drop event target resolution: when items are dropped onto a specific subfolder row in a column pane, target index resolves to that subfolder; when dropped onto the blank space of a column pane, target index resolves to invalid, causing `ContentFileOpsHandler::onPathsDropped` to target the column pane's own `m_path`.

3. **Target Folder Hover Highlighting During Drag (拖拽移动时目标文件夹实时高亮反馈)**
   - **Problem**: Dragging over folder items in `DropListView` does not visually highlight the target item, leading to potential drag errors.
   - **Solution**: Enable drag drop indicators and custom hover state rendering in `DropListView` / `ColumnItemDelegate` during `dragMoveEvent` and clear target highlight on `dragLeaveEvent` / `dropEvent`.

4. **Context Menu "Paste" Action Enablement (右键菜单“粘贴”功能激活)**
   - **Problem**: Right-clicking inside a column pane invokes `ContentContextMenu::showMenu`. `ContentPanel::canPaste(targetOverride)` currently uses `m_currentPath` (root directory of column view), which might be empty or root, or when right-clicking on a subfolder/blank area in column pane K, `m_currentPath` did not match column pane K's actual path.
   - **Solution**: Update `ContentContextMenu::showMenu` so that when invoked from a `ColumnViewPane`'s `m_listView`, `currentPath` is resolved dynamically to `pane->currentPath()`, ensuring `canPaste()` accurately checks clipboard status for that specific column folder.

---

## Affected Files List

1. `src/ui/ColumnItemDelegate.h`
2. `src/ui/ColumnItemDelegate.cpp`
3. `src/ui/DropListView.h`
4. `src/ui/DropListView.cpp`
5. `src/ui/ColumnViewWidget.h`
6. `src/ui/ColumnViewWidget.cpp`
7. `src/ui/controllers/ContentContextMenu.cpp`

---

## Detailed Line-by-Line Search / Replace Diffs

### 1. `src/ui/ColumnItemDelegate.h` & `src/ui/ColumnItemDelegate.cpp`
Add secondary parent highlight role and drop target highlight role rendering support in `ColumnItemDelegate`.

```gdiff
<<<<<<< SEARCH
namespace QuarkMeta {

class ColumnItemDelegate : public QStyledItemDelegate {
=======
namespace QuarkMeta {

enum ColumnCustomRole {
    IsParentExpandedRole = Qt::UserRole + 200,
    IsDropTargetRole = Qt::UserRole + 201
};

class ColumnItemDelegate : public QStyledItemDelegate {
>>>>>>> REPLACE
```

```gdiff
<<<<<<< SEARCH
    if (option.state & QStyle::State_Selected) {
        bgColor = QColor("#0078D4");
        textColor = QColor("#FFFFFF");
    } else if (option.state & QStyle::State_MouseOver) {
        bgColor = QColor("#2A2A2A");
    }
=======
    bool isParentExpanded = index.data(IsParentExpandedRole).toBool();
    bool isDropTarget = index.data(IsDropTargetRole).toBool();

    if (isDropTarget) {
        bgColor = QColor("#005A9E"); // Highlighted target drop folder
        textColor = QColor("#FFFFFF");
    } else if (option.state & QStyle::State_Selected) {
        bgColor = QColor("#0078D4");
        textColor = QColor("#FFFFFF");
    } else if (isParentExpanded) {
        bgColor = QColor("#334455"); // Distinct persistent secondary parent highlight
        textColor = QColor("#FFFFFF");
    } else if (option.state & QStyle::State_MouseOver) {
        bgColor = QColor("#2A2A2A");
    }
>>>>>>> REPLACE
```

### 2. `src/ui/DropListView.h` & `src/ui/DropListView.cpp`
Handle hover drop target rendering and drag leave cleanup in `DropListView`.

```gdiff
<<<<<<< SEARCH
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
=======
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    QModelIndex m_currentHoverDropIdx;
    void clearDropHighlight();
>>>>>>> REPLACE
```

```gdiff
<<<<<<< SEARCH
void DropListView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QListView::dragMoveEvent(event);
    }
}

void DropListView::dropEvent(QDropEvent* event) {
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        QListView::dropEvent(event);
    }
}
=======
void DropListView::dragMoveEvent(QDragMoveEvent* event) {
    QModelIndex hoverIdx = indexAt(event->position().toPoint());
    if (m_currentHoverDropIdx != hoverIdx) {
        clearDropHighlight();
        m_currentHoverDropIdx = hoverIdx;
        if (m_currentHoverDropIdx.isValid() && model()) {
            const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
            viewport()->update();
        }
    }

    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QListView::dragMoveEvent(event);
    }
}

void DropListView::dragLeaveEvent(QDragLeaveEvent* event) {
    clearDropHighlight();
    QListView::dragLeaveEvent(event);
}

void DropListView::clearDropHighlight() {
    if (m_currentHoverDropIdx.isValid() && model()) {
        const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, false, IsDropTargetRole);
        m_currentHoverDropIdx = QModelIndex();
        viewport()->update();
    }
}

void DropListView::dropEvent(QDropEvent* event) {
    clearDropHighlight();
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        QListView::dropEvent(event);
    }
}
>>>>>>> REPLACE
```

### 3. `src/ui/ColumnViewWidget.cpp`
Connect `pathsDropped` from each column pane to `ContentPanel::onPathsDropped` and update parent folder secondary highlight flags.

```gdiff
<<<<<<< SEARCH
    if (m_contentPanel) {
        // 保留 installEventFilter 用于捕获按键快捷键 (m_keyHandler)
        m_listView->installEventFilter(m_contentPanel);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
    }
=======
    if (m_contentPanel) {
        // 保留 installEventFilter 用于捕获按键快捷键 (m_keyHandler)
        m_listView->installEventFilter(m_contentPanel);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
        connect(m_listView, &DropListView::pathsDropped, m_contentPanel, &ContentPanel::onPathsDropped);
    }
>>>>>>> REPLACE
```

```gdiff
<<<<<<< SEARCH
void ColumnViewWidget::updatePaneWidths() {
=======
void ColumnViewWidget::updateParentHighlights() {
    for (int i = 0; i < m_panes.size() - 1; ++i) {
        ColumnViewPane* parentPane = m_panes[i];
        ColumnViewPane* childPane = m_panes[i + 1];
        if (!parentPane || !childPane || !parentPane->proxyModel()) continue;

        QString childPath = QDir::toNativeSeparators(QDir::cleanPath(childPane->currentPath()));
        FilterProxyModel* model = parentPane->proxyModel();

        for (int r = 0; r < model->rowCount(); ++r) {
            QModelIndex idx = model->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
            bool isExpandedParent = (QString::compare(itemPath, childPath, Qt::CaseInsensitive) == 0);
            model->setData(idx, isExpandedParent, IsParentExpandedRole);
        }
    }
}

void ColumnViewWidget::updatePaneWidths() {
>>>>>>> REPLACE
```

### 4. `src/ui/controllers/ContentContextMenu.cpp`
Enable right-click menu context resolution for column panes so "Paste" is active when valid data exists in clipboard.

```gdiff
<<<<<<< SEARCH
    QString currentPath = m_panel->currentPath();
=======
    QString currentPath = m_panel->currentPath();

    // 如果上下文菜单产生自 ColumnViewPane 中的 DropListView，动态修正 currentPath 为该 ColumnViewPane 的物理目录
    if (view && view->objectName() == "ColumnViewPaneListView") {
        QWidget* parentWidget = view->parentWidget();
        while (parentWidget && parentWidget->objectName() != "ColumnViewPane") {
            parentWidget = parentWidget->parentWidget();
        }
        if (parentWidget) {
            QString panePath = parentWidget->property("panePath").toString();
            if (!panePath.isEmpty()) {
                currentPath = panePath;
            }
        }
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **Build Verification**:
   - Run `mkdir -p build && cd build && cmake .. && make -j$(nproc)` to ensure clean compilation without MOC or symbol errors.

2. **Functional & Visual Verification**:
   - **Cross-Column Drag & Drop**: Drag a file from Column ① to subfolder item X in Column ②. Verify that subfolder X turns blue (`#005A9E`) during drag over, and releasing moves the file into subfolder X.
   - **Drop to Blank Space in Parent/Child Column**: Drag a file from Column ② to blank space in Column ①. Verify the file is moved to Column ①'s folder directory.
   - **Right-Click Paste**: Copy a file using Ctrl+C, right-click blank space in any Column Pane. Verify that "粘贴" (Paste) action is active and usable.
   - **Secondary Parent Highlight**: Click a file in Column ③. Verify that the parent folder in Column ② and grandparent folder in Column ① maintain visual secondary background highlights (`#334455`).
