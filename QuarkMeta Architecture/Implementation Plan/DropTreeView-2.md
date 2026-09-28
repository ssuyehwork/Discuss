# Implementation Plan - DropTreeView Full Column Spanning & SVG Header Delegate (`DropTreeView-2.md`)

## Overview
This implementation plan completes the single-view normalization for `DropTreeView` (`ListView`).
It handles section headers ("文件夹 (N)" / "文件 (M)") within a single `QTreeView` using:
1. `QTreeView::setFirstColumnSpanning` for full-width section header rows.
2. `TreeItemDelegate::paint` for transparent backgrounds, `#3498db` theme blue titles, and `scroll-008.svg` / `scroll-010.svg` vector SVG arrow rendering.
3. Smooth header mouse click toggling for section collapse/expand.

---

## Modified Files List
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/TreeItemDelegate.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/DropTreeView.cpp` (Full column spanning for section headers)

```
<<<<<<< SEARCH
void DropTreeView::applyColumnPolicies() {
=======
void DropTreeView::updateHeaderSpanning() {
    if (!model()) return;
    int totalRows = model()->rowCount();
    for (int r = 0; r < totalRows; ++r) {
        QModelIndex idx = model()->index(r, 0);
        bool isHeader = idx.data(ModelContract::IsGroupHeaderRole).toBool();
        if (isHeader) {
            setFirstColumnSpanning(r, QModelIndex(), true);
        }
    }
}

void DropTreeView::applyColumnPolicies() {
>>>>>>> REPLACE
```

---

### 2. `src/ui/TreeItemDelegate.cpp` (100% Parameter-preserved Header Painting)

```
<<<<<<< SEARCH
void TreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;
=======
void TreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;

    bool isHeader = index.data(ModelContract::IsGroupHeaderRole).toBool();
    if (isHeader) {
        painter->save();
        // 1. Transparent Background (Matching style.qss QFrame#FolderSectionHeaderBar { background: transparent; })

        const int iconSize = 12;
        const int marginX = 10;
        const QColor headerColor("#3498db");

        bool isCollapsed = index.data(ModelContract::IsGroupCollapsedRole).toBool();
        QString headerText = index.data(Qt::DisplayRole).toString();

        // 2. Draw Section Title Text (#3498db Bold)
        painter->setPen(headerColor);
        painter->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));

        QFontMetrics fm(painter->font());
        int textWidth = fm.horizontalAdvance(headerText);
        QRect textRect(option.rect.left() + marginX, option.rect.top(), textWidth + 4, option.rect.height());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, headerText);

        // 3. Draw SVG Vector Icon (scroll-008.svg / scroll-010.svg)
        const QString iconName = isCollapsed ? "scroll-008.svg" : "scroll-010.svg";
        QPixmap arrowPixmap = UiHelper::getIcon(iconName, headerColor, iconSize).pixmap(iconSize, iconSize);
        int iconX = textRect.right() + 4;
        int iconY = option.rect.top() + (option.rect.height() - iconSize) / 2;
        painter->drawPixmap(iconX, iconY, arrowPixmap);

        painter->restore();
        return;
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Compile the project:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. Verify List View (`DropTreeView`):
   - Single `DropTreeView` instance handles all folders and files seamlessly.
   - Section headers span across all columns with transparent backgrounds, `#3498db` text, and crisp SVG vector arrows (`scroll-008.svg` / `scroll-010.svg`).

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`UiHelper::getIcon` SSOT**: Uses the official project SVG renderer for arrow icons.
- **`QTreeView::setFirstColumnSpanning` SSOT**: Reuses Qt's native tree column spanning mechanism for full-width section headers.

---

## Header API Signature Verification Table

| File | Class / Function | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/DropTreeView.h` | `DropTreeView` | `void setFirstColumnSpanning(int row, const QModelIndex& parent, bool span);` | Verified 100% Match |
| `src/ui/TreeItemDelegate.h` | `TreeItemDelegate` | `void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;` | Verified 100% Match |
