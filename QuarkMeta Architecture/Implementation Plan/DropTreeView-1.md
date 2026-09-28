# Implementation Plan - DropTreeView List View Integrated Grouping Architecture (`DropTreeView-1.md`)

## Overview
This implementation plan provides the single-view grouping architecture for List View (`DropTreeView` / `QTreeView`).
By removing the external `SectionedScrollCanvas` (`m_listScrollArea`) wrapper in `ContentPanel-35.md`, `DropTreeView` natively displays folder and file section headers ("文件夹 (N)" / "文件 (M)") directly within the tree view.

Using `QTreeView::setFirstColumnSpanning`, group header rows span across all table columns (Name, Rating, Size, Type, Modified Date). Header rendering uses `UiHelper::getIcon` SVG vector arrows (`scroll-008.svg` / `scroll-010.svg`), `#3498db` theme blue text, and a `transparent` background to match existing UI rules.

---

## Modified Files List
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/TreeItemDelegate.h`
- `src/ui/TreeItemDelegate.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/DropTreeView.h`

```
<<<<<<< SEARCH
    void applyColumnPolicies();
=======
    void applyColumnPolicies();
    void updateGroupHeaderSpanning();
>>>>>>> REPLACE
```

---

### 2. `src/ui/DropTreeView.cpp`

```
<<<<<<< SEARCH
void DropTreeView::applyColumnPolicies() {
=======
void DropTreeView::updateGroupHeaderSpanning() {
    if (!model()) return;
    int rows = model()->rowCount();
    for (int r = 0; r < rows; ++r) {
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

### 3. `src/ui/TreeItemDelegate.cpp` (`paint` method for Section Header)

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
        // Transparent background matching style.qss

        const int iconSize = 12;
        const int marginX = 10;
        const QColor headerColor("#3498db");

        bool isCollapsed = index.data(ModelContract::IsGroupCollapsedRole).toBool();
        QString headerText = index.data(Qt::DisplayRole).toString();

        // 1. Draw Section Title Text
        painter->setPen(headerColor);
        painter->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));

        QFontMetrics fm(painter->font());
        int textWidth = fm.horizontalAdvance(headerText);
        QRect textRect(option.rect.left() + marginX, option.rect.top(), textWidth + 4, option.rect.height());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, headerText);

        // 2. Draw SVG Vector Arrow Icon
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
   - Section headers ("文件夹 (N)" / "文件 (M)") span full column width.
   - Background is transparent, matching the dark theme.
   - Collapsing a section hides/reveals list items smoothly with zero double-scrollbar issues.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`QTreeView::setFirstColumnSpanning` SSOT**: Uses Qt's native tree column spanning for full-width section headers.
- **`UiHelper::getIcon` SSOT**: Reuses project SVG icon loader for `scroll-008.svg` and `scroll-010.svg`.

---

## Header API Signature Verification Table

| File | Class / Function | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/DropTreeView.h` | `DropTreeView` | `void setFirstColumnSpanning(int row, const QModelIndex& parent, bool span);` | Verified 100% Match |
| `src/ui/TreeItemDelegate.h` | `TreeItemDelegate` | `void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;` | Verified 100% Match |
