# DropTreeView-3 Implementation Plan: Native Single-View Group Headers for List View

## 1. Overview
This implementation plan restores and enforces native section headers ("文件夹 (N)" and "文件 (M)") inside the single unified `DropTreeView` control for List View mode (`ContentPanel::ListView`).
It ensures:
- A single `DropTreeView` instance renders group header rows spanned across all columns via `setFirstColumnSpanned()`.
- Group header rows render section titles in blue (`#3498db`) bold font with toggle SVG arrow icons (`scroll-008.svg` / `scroll-010.svg`).
- Clicking on a group header row toggles the collapsed state (`IsGroupCollapsedRole`) for that group without splitting into multiple tree views.

## 2. Modified Files List
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/TreeItemDelegate.h`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/DropTreeView.h`
Add `setModel` override and group header click event handler to automatically update column spanning and handle header collapse toggles.

```diff
<<<<<<< SEARCH
    void applyColumnPolicies();
    void updateGroupHeaderSpanning();
=======
    void applyColumnPolicies();
    void updateGroupHeaderSpanning();
    void setModel(QAbstractItemModel* model) override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
>>>>>>> REPLACE
```

### 3.2 `src/ui/DropTreeView.cpp`
Implement `setModel` hook and `mousePressEvent` header toggle logic.

```diff
<<<<<<< SEARCH
void DropTreeView::updateGroupHeaderSpanning() {
    if (!model()) return;
    int rows = model()->rowCount();
    for (int r = 0; r < rows; ++r) {
        QModelIndex idx = model()->index(r, 0);
        bool isHeader = idx.data(IsGroupHeaderRole).toBool();
        if (isHeader) {
            setFirstColumnSpanned(r, QModelIndex(), true);
        }
    }
}
=======
void DropTreeView::setModel(QAbstractItemModel* newModel) {
    QTreeView::setModel(newModel);
    if (newModel) {
        connect(newModel, &QAbstractItemModel::modelReset, this, &DropTreeView::updateGroupHeaderSpanning, Qt::UniqueConnection);
        connect(newModel, &QAbstractItemModel::rowsInserted, this, &DropTreeView::updateGroupHeaderSpanning, Qt::UniqueConnection);
        connect(newModel, &QAbstractItemModel::layoutChanged, this, &DropTreeView::updateGroupHeaderSpanning, Qt::UniqueConnection);
        updateGroupHeaderSpanning();
    }
}

void DropTreeView::updateGroupHeaderSpanning() {
    if (!model()) return;
    int rows = model()->rowCount();
    for (int r = 0; r < rows; ++r) {
        QModelIndex idx = model()->index(r, 0);
        bool isHeader = idx.data(IsGroupHeaderRole).toBool();
        if (isHeader) {
            setFirstColumnSpanned(r, QModelIndex(), true);
        }
    }
}

void DropTreeView::mousePressEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (idx.isValid() && idx.data(IsGroupHeaderRole).toBool()) {
        if (event->button() == Qt::LeftButton) {
            bool currentCollapsed = idx.data(IsGroupCollapsedRole).toBool();
            if (model()) {
                model()->setData(idx, !currentCollapsed, IsGroupCollapsedRole);
            }
            event->accept();
            return;
        }
    }
    QTreeView::mousePressEvent(event);
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/TreeItemDelegate.h`
Ensure `TreeItemDelegate::paint()` draws group headers with full column background spanning, theme blue (`#3498db`) title text, and SVG vector arrow toggle icons.

```diff
<<<<<<< SEARCH
        bool isHeader = index.data(IsGroupHeaderRole).toBool();
        if (isHeader) {
            painter->save();
            
            const int iconSize = 12;
            const int marginX = 10;
            const QColor headerColor("#3498db");

            bool isCollapsed = index.data(IsGroupCollapsedRole).toBool();
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
=======
        bool isHeader = index.data(IsGroupHeaderRole).toBool();
        if (isHeader) {
            painter->save();
            
            // Fill background for group header row across spanned width
            painter->fillRect(option.rect, QColor("#1E1E1E"));

            const int iconSize = 12;
            const int marginX = 12;
            const QColor headerColor("#3498db");

            bool isCollapsed = index.data(IsGroupCollapsedRole).toBool();
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
            int iconX = textRect.right() + 6;
            int iconY = option.rect.top() + (option.rect.height() - iconSize) / 2;
            painter->drawPixmap(iconX, iconY, arrowPixmap);

            painter->restore();
            return;
        }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure and build project with CMake:
   ```bash
   cmake -B build -S .
   cmake --build build --config Debug
   ```
2. Launch `QuarkMeta` application and switch to List View (`ListView`).
3. Confirm that section headers "文件夹 (N)" and "文件 (M)" are rendered with theme blue font (`#3498db`), arrow icons, and spanned column 0 across the single `DropTreeView`.
4. Click on section headers to verify that items collapse and expand smoothly inside the single `DropTreeView`.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **`IsGroupHeaderRole` / `IsGroupCollapsedRole`**: Reuses standard enum constants in `src/core/ModelContract.h`.
- **`UiHelper::getIcon()`**: Reuses central SVG icon renderer for `#3498db` colored vector icons.
- **Single View Constraint**: Operates strictly within a single `DropTreeView` control without creating dual split views.

## 6. Header API Signature Verification
- `QTreeView::setModel(QAbstractItemModel* model)`: Standard Qt header signature verified.
- `QTreeView::setFirstColumnSpanned(int row, const QModelIndex& parent, bool span)`: Standard Qt header signature verified.
- `QModelIndex::data(int role) const`: Standard Qt header signature verified.
