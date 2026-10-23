# UiHelper Implementation Plan - LineEdit Custom Context Menu Standard

This implementation plan details the custom context menu builder helper in `UiHelper` (`src/ui/UiHelper.h` / `src/ui/UiHelper.cpp`) to replace all default Qt / OS native context menus on `QLineEdit` and `QTextEdit` widgets with QuarkMeta-styled SVG-icon context menus.

---

## 1. Overview & Problem Statement
* **Issue**: Right-clicking inside text inputs (`QLineEdit` / `QTextEdit`) brings up OS native context menus with default English texts ("Undo", "Redo", "Cut", "Copy", "Paste", "Delete", "Select All") lacking SVG icons and QuarkMeta dark styling.
* **Solution**: Implement a reusable `UiHelper::createLineEditContextMenu(QLineEdit* edit)` / `UiHelper::attachCustomContextMenu(QLineEdit* edit)` helper that creates a stylized `QMenu` configured with `UiHelper::applyMenuStyle`, QuarkMeta localized labels, and semantic monochrome SVG icons (`undo`, `redo`, `cut`, `copy`, `paste`, `delete_forever`, `select`).

---

## 2. Modified Files List
- `src/ui/UiHelper.h`
- `src/ui/UiHelper.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/UiHelper.h`

```git
<<<<<<< SEARCH
    static inline void applyMenuStyle(QWidget* menu) {
        ThemeManager::instance().applyMenuStyle(menu);
    }
=======
    static inline void applyMenuStyle(QWidget* menu) {
        ThemeManager::instance().applyMenuStyle(menu);
    }

    static void setupLineEditContextMenu(QLineEdit* edit);
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/UiHelper.cpp`

```git
<<<<<<< SEARCH
#include "UiHelper.h"
#include "ThemeManager.h"
#include "SvgIcons.h"
#include <QApplication>
=======
#include "UiHelper.h"
#include "ThemeManager.h"
#include "SvgIcons.h"
#include <QApplication>
#include <QLineEdit>
#include <QMenu>
#include <QClipboard>

namespace QuarkMeta {

void UiHelper::setupLineEditContextMenu(QLineEdit* edit) {
    if (!edit) return;
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QLineEdit::customContextMenuRequested, edit, [edit](const QPoint& pos) {
        QMenu menu(edit);
        applyMenuStyle(&menu);

        QAction* actUndo = menu.addAction(getIcon("undo", QColor("#EEEEEE")), "撤销");
        actUndo->setShortcut(QKeySequence::Undo);
        actUndo->setEnabled(edit->isUndoAvailable());
        QObject::connect(actUndo, &QAction::triggered, edit, &QLineEdit::undo);

        QAction* actRedo = menu.addAction(getIcon("redo", QColor("#EEEEEE")), "重做");
        actRedo->setShortcut(QKeySequence::Redo);
        actRedo->setEnabled(edit->isRedoAvailable());
        QObject::connect(actRedo, &QAction::triggered, edit, &QLineEdit::redo);

        menu.addSeparator();

        QAction* actCut = menu.addAction(getIcon("cut", QColor("#EEEEEE")), "剪切");
        actCut->setShortcut(QKeySequence::Cut);
        actCut->setEnabled(!edit->isReadOnly() && edit->hasSelectedText());
        QObject::connect(actCut, &QAction::triggered, edit, &QLineEdit::cut);

        QAction* actCopy = menu.addAction(getIcon("copy", QColor("#EEEEEE")), "复制");
        actCopy->setShortcut(QKeySequence::Copy);
        actCopy->setEnabled(edit->hasSelectedText());
        QObject::connect(actCopy, &QAction::triggered, edit, &QLineEdit::copy);

        QAction* actPaste = menu.addAction(getIcon("paste", QColor("#EEEEEE")), "粘贴");
        actPaste->setShortcut(QKeySequence::Paste);
        actPaste->setEnabled(!edit->isReadOnly() && !QApplication::clipboard()->text().isEmpty());
        QObject::connect(actPaste, &QAction::triggered, edit, &QLineEdit::paste);

        QAction* actDelete = menu.addAction(getIcon("delete_forever", QColor("#EEEEEE")), "删除");
        actDelete->setEnabled(!edit->isReadOnly() && edit->hasSelectedText());
        QObject::connect(actDelete, &QAction::triggered, edit, [edit]() {
            edit->insert("");
        });

        menu.addSeparator();

        QAction* actSelectAll = menu.addAction(getIcon("select", QColor("#EEEEEE")), "全选");
        actSelectAll->setShortcut(QKeySequence::SelectAll);
        actSelectAll->setEnabled(!edit->text().isEmpty());
        QObject::connect(actSelectAll, &QAction::triggered, edit, &QLineEdit::selectAll);

        menu.exec(edit->mapToGlobal(pos));
    });
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `UiHelper::setupLineEditContextMenu` creates the custom QMenu with `applyMenuStyle` and localized QuarkMeta SVG actions.
2. Confirm `UiHelper-1.md` is present in `QuarkMeta Architecture/Implementation Plan/`.
3. Confirm Chapter 1 in `QuarkMeta-Architecture-Planning.md` contains the App-Exclusive LineEdit Context Menu Contract.
