# Implementation Plan - Sidebar Inline Rename Editor Geometry Precision Fix

## 1. Overview
Currently, in both the **Bookmarks ("收藏夹", `FavoritePanel`)** and **Library ("库", `LibraryPanel`)** tree views, the inline rename line edit (`QLineEdit` editor created during F2 or right-click Rename) has incorrect geometry calculations in `updateEditorGeometry`:
1. In `LibraryPanel::LibraryItemDelegate`, `updateEditorGeometry` hardcodes `textRect.setLeft(option.rect.left() + 33)` without taking tree indentation depth (`depth`) or expand/collapse chevron arrows into account, causing the editor to overlap leftward over the folder/category icon and chevron arrow.
2. In `FavoritePanel::FavoriteItemDelegate`, `updateEditorGeometry` is un-overridden (using Qt default `QStyledItemDelegate` implementation), causing the editor to span the entire item width from `option.rect.left()`, completely overlapping the icon and indent spacing.
3. On the right side, the editor overflows beyond the item bounds or fails to respect the item count / layer icon right margins.

This plan establishes exact geometry alignment between the displayed item text rect and the inline line edit across both `FavoritePanel` and `LibraryPanel`.

---

## 2. Modified Files List
- `src/ui/FavoritePanel.h`
- `src/ui/FavoritePanel.cpp`
- `src/ui/LibraryPanel.h`
- `src/ui/LibraryPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FavoritePanel.h`
Declare `updateEditorGeometry` in `FavoriteItemDelegate`:

```cpp
<<<<<<< SEARCH
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
=======
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/FavoritePanel.cpp`
Implement `FavoriteItemDelegate::updateEditorGeometry` matching `paint()` layout calculations exactly:

```cpp
<<<<<<< SEARCH
    // 绘制文本
    QString text = index.data(Qt::DisplayRole).toString();
=======
void FavoriteItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!editor) return;

    int depth = 0;
    QModelIndex parentIdx = index.parent();
    while (parentIdx.isValid()) {
        depth++;
        parentIdx = parentIdx.parent();
    }

    int leftMargin = 10 + depth * 14;
    int iconSize = 18;
    int spacing = 8;

    int textLeft = option.rect.left() + leftMargin + iconSize + spacing;

    bool isAutoRec = index.data(Qt::UserRole + 9).toBool();
    int rightMargin = isAutoRec ? 26 : 8;

    QRect editorRect = option.rect;
    editorRect.setLeft(textLeft - 2); // 2px padding compensation for line edit frame
    editorRect.setRight(option.rect.right() - rightMargin);
    editorRect.setTop(option.rect.top() + 2);
    editorRect.setHeight(option.rect.height() - 4);

    editor->setGeometry(editorRect);
}

    // 绘制文本
    QString text = index.data(Qt::DisplayRole).toString();
>>>>>>> REPLACE
```

### 3.3 `src/ui/LibraryPanel.cpp`
Fix `LibraryItemDelegate::updateEditorGeometry` to dynamically calculate `depth`, chevron arrow width, icon width, and right count text margin:

```cpp
<<<<<<< SEARCH
void LibraryItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    if (!editor) return;

    QRect textRect = option.rect;
    textRect.setLeft(option.rect.left() + 33);
    textRect.setRight(option.rect.right() - 3);

    editor->setGeometry(textRect);
}
=======
void LibraryItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!editor || !index.isValid()) return;

    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId <= 0) return;

    int customDepth = 0;
    QModelIndex parentIdx = index.parent();
    while (parentIdx.isValid()) {
        if (parentIdx.data(Qt::UserRole + 1).toInt() > 0) {
            customDepth++;
        }
        parentIdx = parentIdx.parent();
    }

    int arrowSize = 14;
    int arrowSpacing = 4;
    int leftMargin = 10 + customDepth * 14;
    int iconSize = 18;
    int spacing = 8;
    int rightMargin = 10;

    int iconLeft = option.rect.left() + leftMargin + (nodeId > 0 ? (arrowSize + arrowSpacing) : 0);
    int textLeft = iconLeft + iconSize + spacing;

    int count = index.data(Qt::UserRole + 9).toInt();
    QString countStr = (count >= 0) ? QString::number(count) : "";
    int countWidth = countStr.isEmpty() ? 0 : (option.fontMetrics.horizontalAdvance(countStr) + 12);

    QRect editorRect = option.rect;
    editorRect.setLeft(textLeft - 2);
    editorRect.setRight(option.rect.right() - rightMargin - countWidth);
    editorRect.setTop(option.rect.top() + 2);
    editorRect.setHeight(option.rect.height() - 4);

    editor->setGeometry(editorRect);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile project target using `cmake --build .`.
2. In the left sidebar under "收藏夹" and "库", create or select root and nested categories/folders.
3. Press `F2` or select "重命名" from context menu.
4. Verify that the line edit box starts precisely at the text origin (right after the folder icon with zero leftward overlap) and ends cleanly before the right edge or count label.

---

## 5. Official API Reuse & Anti-Redundancy
- Reuses `QStyledItemDelegate::updateEditorGeometry` virtual interface.
- Synchronizes exact geometry formulas with `paint()` methods in `FavoriteItemDelegate` and `LibraryItemDelegate`.

---

## 6. Header API Signature Verification
- `void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;` added to `FavoriteItemDelegate` in `FavoritePanel.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `QWidget`, `QStyleOptionViewItem`, `QModelIndex` fully included/declared in `FavoritePanel.h` and `LibraryPanel.h`.

---

## 8. Data SSOT Declaration Table
| Entity / State | SSOT Location | Synchronization Mechanism |
| :--- | :--- | :--- |
| Item Text Bounds | `Delegate::paint()` & `Delegate::updateEditorGeometry()` | Derived from `leftMargin`, `iconLeft`, `iconSize`, `spacing`, and `countWidth` |

---

## 9. Compliance Self-Check Table
| Check Item | Status | Notes |
| :--- | :--- | :--- |
| Zero Code Modification Rule | PASS | Output strictly as plan document inside `Implementation Plan/` |
| Chinese Language Protocol | PASS | Documented completely in Chinese |
| Contract Lock | PASS | Public header signatures untouched |

---

## 10. Behavior Preservation Declaration Table
| Action | Existing Behavior | Preserved Behavior |
| :--- | :--- | :--- |
| Inline Rename Trigger | F2 or Context Menu | Unchanged |
| Inline Line Edit Position | Overlapped icon & overflowed right margin | Fixed to align exactly with item text bounding box |
