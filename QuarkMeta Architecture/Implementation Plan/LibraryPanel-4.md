# Implementation Plan - LibraryPanel-4.md

## 1. Overview
This implementation plan fine-tunes the inline renaming text edit box (`QLineEdit`) geometry in `LibraryPanel` based on visual feedback (`image.png`).

As requested by the user:
- Shift the left boundary of the inline line edit box 3 pixels to the left (`option.rect.left() + 33` instead of `+ 36`).
- Increase the total width of the inline line edit box by 5 pixels (`textRect.setRight(option.rect.right() - 3)` instead of `- 6`).
- In a 230px column, the editor width becomes `(230 - 3) - 33 = 193px` (a 5px increase from the previous 188px).

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/LibraryPanel-4.md`.

## 2. Modified Files List
- `src/ui/LibraryPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/LibraryPanel.cpp`

```diff
<<<<<<< SEARCH
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
=======
void LibraryItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    if (!editor) return;

    int leftMargin = 10;
    int iconSize = 18;
    int spacing = 8;
    int leftShift = 3;

    QRect textRect = option.rect;
    textRect.setLeft(option.rect.left() + leftMargin + iconSize + spacing - leftShift);
    textRect.setRight(option.rect.right() - 3);

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
   - Right-click a category and select "重命名" (or press F2) to trigger inline editing.
   - Verify that the line editor box starts 3 pixels further left (`X = 33px`) and extends 3 pixels from the right border (`width = 193px` in a 230px column).
   - Verify that the visual alignment matches the user's expected visual design perfectly.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Standard `QStyledItemDelegate::updateEditorGeometry` Qt Delegate API.
- **Zero Redundancy**: Directly refines geometry bounds with zero extra widgets or hacks.

## 6. Header API Signature Verification
- `LibraryItemDelegate::updateEditorGeometry` in `src/ui/LibraryPanel.h` signature remains 100% unchanged and frozen.

## 7. Header Inclusion Chain & Type Completeness Check
- No headers modified or removed.
