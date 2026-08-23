# List View F2 Rename Text Selection Fix Implementation Plan (f2_selection_fix.md)

## Overview
When pressing F2 to trigger inline file renaming in List View (`DropTreeView` / `TreeItemDelegate`), the standard `QLineEdit` created in `TreeItemDelegate::createEditor` highlights the entire text including the file extension. In contrast, Grid View (`ThumbnailDelegate`) uses a custom `FileNameLineEdit` class which overrides `focusInEvent` to select only the file name (excluding the extension).

This implementation plan refactors `TreeItemDelegate` to utilize `FileNameLineEdit` instead of a standard `QLineEdit`, ensuring consistent text selection behavior (excluding file extension for files, selecting all for folders/categories) across all view modes when inline editing is activated.

## Modified Files List
- `src/ui/TreeItemDelegate.h`

## Detailed Line-by-Line Changes

### 1. `src/ui/TreeItemDelegate.h`
Include `ThumbnailDelegate.h` for `FileNameLineEdit`, use `FileNameLineEdit` in `createEditor`, and delegate text selection to `FileNameLineEdit::focusInEvent` in `setEditorData`.

```
<<<<<<< SEARCH
#include "ContentPanel.h"
#include "../meta/MetadataManager.h"
#include "../core/ModelContract.h"
#include "UiHelper.h"
#include "CardPainterHelper.h"
#include "StyleLibrary.h"
=======
#include "ContentPanel.h"
#include "ThumbnailDelegate.h"
#include "../meta/MetadataManager.h"
#include "../core/ModelContract.h"
#include "UiHelper.h"
#include "CardPainterHelper.h"
#include "StyleLibrary.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        Q_UNUSED(index);
        QLineEdit* editor = new QLineEdit(parent);
        // 2026-07-26 极致重构：应用精致的暗黑带蓝边框样式（背景 `#2D2D2D`，外框 `#3498db`，圆角 `4px`），消除默认白色粗糙样式
        editor->setStyleSheet(
            "QLineEdit {"
            "  background-color: #2D2D2D;"
            "  color: white;"
            "  selection-background-color: #3498db;"
            "  border: 1px solid #3498db;"
            "  border-radius: 4px;"
            "  padding: 0px 4px;"
            "  margin: 0px;"
            "  font-size: 8pt;"
            "}"
        );
        editor->installEventFilter(const_cast<TreeItemDelegate*>(this));
        return editor;
    }
=======
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        FileNameLineEdit* editor = new FileNameLineEdit(parent);
        // 2026-07-26 极致重构：应用精致的暗黑带蓝边框样式（背景 `#2D2D2D`，外框 `#3498db`，圆角 `4px`），消除默认白色粗糙样式
        editor->setStyleSheet(
            "QLineEdit {"
            "  background-color: #2D2D2D;"
            "  color: white;"
            "  selection-background-color: #3498db;"
            "  border: 1px solid #3498db;"
            "  border-radius: 4px;"
            "  padding: 0px 4px;"
            "  margin: 0px;"
            "  font-size: 8pt;"
            "}"
        );
        bool isFolder = (index.data(TypeRole).toString() == "folder" || index.data(TypeRole).toString() == "category");
        editor->setIsFolder(isFolder);
        editor->installEventFilter(const_cast<TreeItemDelegate*>(this));
        return editor;
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        QString value = index.model()->data(index, Qt::EditRole).toString();
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
        if (!lineEdit) return;

        lineEdit->setText(value);

        // 🚀 【拔除 0ms 补丁】：同步精准设定选区，无需使用 QTimer 在下一个事件循环中强行覆盖 
        bool isFolder = (index.data(TypeRole).toString() == "folder" || index.data(TypeRole).toString() == "category"); 
        if (isFolder) { 
            lineEdit->selectAll(); 
        } else { 
            int lastDot = value.lastIndexOf('.'); 
            if (lastDot > 0) { 
                lineEdit->setSelection(0, lastDot); 
            } else { 
                lineEdit->selectAll(); 
            } 
        } 
    }
=======
    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        QString value = index.model()->data(index, Qt::EditRole).toString();
        FileNameLineEdit* lineEdit = qobject_cast<FileNameLineEdit*>(editor);
        if (lineEdit) {
            lineEdit->setText(value);
        }
    }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile project:
   ```bash
   cmake --build build --config Debug
   ```
2. Switch to List View mode (`m_treeView`).
3. Select a file with an extension (e.g. `sample.png`) and press F2.
4. Verify that only `sample` is highlighted and `.png` remains unselected, matching Grid View behavior.
