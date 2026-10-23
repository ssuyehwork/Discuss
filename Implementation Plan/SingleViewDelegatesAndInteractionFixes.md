# Implementation Plan - Step 3: View Delegates & Interaction Fixes

This implementation plan details adding section header rendering in `ColumnItemDelegate`, fixing mouse event handling for section headers across all view controls, and fixing drag-and-drop target path extraction in `ContentFileOpsHandler`.

## 1. Overview
Currently:
1. `ColumnItemDelegate` does not check for `SectionHeaderRole`, causing section headers in Column View to render as empty file rows.
2. Clicking section headers in `DropListView`, `JustifiedView`, and `DropTreeView` triggers default view selection logic, clearing user selection.
3. `ContentFileOpsHandler::onPathsDropped` attempts proxy conversions using `mapToSource`, which fails when indexes belong to `SectionProxyModel`.

This step fixes all three issues:
- `ColumnItemDelegate::paint` handles `SectionHeaderRole` / `SectionKindRole` to render header rows with `#1E1E1E` background, `#3498db` text, and collapse arrows.
- Views accept header mouse events without clearing selection or starting rubber-band box selection.
- `ContentFileOpsHandler::onPathsDropped` directly queries `targetIndex.data(PathRole)` without invalid `mapToSource` proxy model conversions.

---

## 2. Modified Files List
1. `src/ui/ColumnItemDelegate.cpp`
2. `src/ui/DropListView.cpp` & `src/ui/DropListView.h`
3. `src/ui/JustifiedView.cpp`
4. `src/ui/DropTreeView.cpp`
5. `src/ui/TreeItemDelegate.cpp`
6. `src/ui/controllers/ContentFileOpsHandler.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ColumnItemDelegate.cpp`
<<<<<<< SEARCH
void ColumnItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;
=======
void ColumnItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;

    if (index.data(SectionHeaderRole).toBool()) {
        painter->save();
        painter->fillRect(option.rect, QColor("#1E1E1E"));

        QFont font = option.font;
        font.setBold(true);
        font.setPixelSize(12);
        painter->setFont(font);
        painter->setPen(QColor("#3498db"));

        QString text = index.data(SectionHeaderTextRole).toString();
        QRect textRect = option.rect.adjusted(10, 0, -30, 0);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        int kind = index.data(SectionKindRole).toInt();
        if (kind == 1) { // Folder Header
            bool collapsed = index.data(SectionCollapsedRole).toBool();
            QIcon arrowIcon = UiHelper::getIcon(collapsed ? "scroll-008" : "scroll-010", QColor("#3498db"), 12);
            QRect iconRect(option.rect.right() - 25, option.rect.top() + (option.rect.height() - 12) / 2, 12, 12);
            arrowIcon.paint(painter, iconRect);
        }

        painter->restore();
        return;
    }
>>>>>>> REPLACE

---

### 3.2 `src/ui/DropListView.cpp`
<<<<<<< SEARCH
void DropListView::mousePressEvent(QMouseEvent* event) {
    QModelIndex index = indexAt(event->pos());
=======
void DropListView::mousePressEvent(QMouseEvent* event) {
    QModelIndex index = indexAt(event->pos());
    if (index.isValid() && index.data(SectionHeaderRole).toBool()) {
        emit sectionHeaderClicked(index);
        event->accept();
        return;
    }
>>>>>>> REPLACE

---

### 3.3 `src/ui/controllers/ContentFileOpsHandler.cpp`
<<<<<<< SEARCH
    // Legacy sourceModelOverride mapToSource conversion
=======
void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& currentDir) {
    QString targetDir = currentDir;
    if (targetIndex.isValid() && !targetIndex.data(SectionHeaderRole).toBool()) {
        QString itemPath = targetIndex.data(PathRole).toString();
        if (!itemPath.isEmpty() && QFileInfo(itemPath).isDir()) {
            targetDir = itemPath;
        }
    }
    // Proceed with file operations targeting targetDir...
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Apply changes to `ColumnItemDelegate.cpp`, `DropListView.cpp/.h`, `JustifiedView.cpp`, `DropTreeView.cpp`, `ContentFileOpsHandler.cpp`.
2. Compile project using CMake.
3. Verify section headers render properly in Column View and clicking section headers toggles collapse without wiping selections.
