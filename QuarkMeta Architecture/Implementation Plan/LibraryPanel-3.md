# Implementation Plan - LibraryPanel-3.md

## 1. Overview
This implementation plan fixes the inline renaming text edit box (`QLineEdit`) geometry issue in `LibraryPanel`.

As shown in user screenshots:
- When triggering inline renaming for a category in `LibraryPanel`, the text editor box overlays the icon on the left and expands with an un-adapted, fixed width.

By overriding `updateEditorGeometry` in `LibraryItemDelegate`, the line editor box (`editor`) is precisely placed inside the `textRect` area (skipping the left 10px margin, 18px icon, and 8px spacing). Its width automatically adapts dynamically to the tree view column/panel width (`option.rect.right() - 6`), creating a clean, responsive inline editing experience.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/LibraryPanel-3.md`.

## 2. Modified Files List
- `src/ui/LibraryPanel.h`
- `src/ui/LibraryPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/LibraryPanel.h`

Add `updateEditorGeometry` override to `LibraryItemDelegate`:
```diff
<<<<<<< SEARCH
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
=======
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
>>>>>>> REPLACE
```

### `src/ui/LibraryPanel.cpp`

Implement `updateEditorGeometry`:
```diff
<<<<<<< SEARCH
    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}
=======
    QString elidedText = opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width() - 6);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    painter->restore();
}

void LibraryItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    if (!editor) return;

    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 8;

    QRect textRect = option.rect;
    textRect.setLeft(option.rect.left() + leftMargin + iconSize + spacing);
    textRect.setRight(option.rect.right() - 6);

    editor->setGeometry(textRect);
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`, open the "库" tab in the left sidebar:
   - Right-click a category and choose "重命名" (Rename) or press F2 to enter inline editing.
   - Verify that the inline editing text box (`QLineEdit`) is cleanly aligned to the right of the folder icon (no longer covering the icon).
   - Resize the left sidebar panel width and verify that the inline editing text box dynamically adjusts its width to match the panel.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Standard `QStyledItemDelegate::updateEditorGeometry` Qt Delegate API.
- **Zero Redundancy**: Cleanly calculates geometry based on the exact same layout metrics used in `LibraryItemDelegate::paint`.

## 6. Header API Signature Verification
- `LibraryItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override` in `src/ui/LibraryPanel.h` matches standard Qt Delegate虚函数签名.

## 7. Header Inclusion Chain & Type Completeness Check
- `QWidget` is fully declared or accessible via Qt headers in `LibraryPanel.h`/`LibraryPanel.cpp`.
