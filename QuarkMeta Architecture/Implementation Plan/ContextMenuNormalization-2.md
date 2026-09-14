# Implementation Plan - Context Menu Architecture Normalization (ContextMenuNormalization-2.md)

## Overview
This implementation plan systematically normalizes all remaining unnormalized right-click context menus across QuarkMeta to guarantee 100% compliance with `Memories.md` (Chapter 1 & Chapter 2).

### Issues Addressed
1. **FavoritePanel Colored Icon Violation**: Remove colored icon styling in `FavoritePanel`'s "切换图标" submenu and enforce neutral monochrome (`#EEEEEE`).
2. **QTextEdit / QPlainTextEdit Multi-Line Context Menu Interception**: Extend `QuarkApplication::notify` and `UiHelper` to intercept context menu events for `QTextEdit` / `QPlainTextEdit` controls, replacing default OS context menus with QuarkMeta's exclusive dark context menu with SVG icons and 10px spacing.
3. **NavPanel Context Menu Unification**: Refactor `NavPanel` context menus to use `ContextMenuFactory` helpers (`buildShowInExplorerAction`, `buildCopyPathAction`, `buildAddToFavoritesAction`) instead of loose inline actions.
4. **TitleBarWidget & DriveBarWidget Menu Styling**: Replace inline QSS and raw `new QMenu` instantiations in `TitleBarWidget` and `DriveBarWidget` with `UiHelper::createMenu` / `UiHelper::applyMenuStyle`.
5. **TagManagerDialog & Submenu Styling**: Ensure all submenus created via `addMenu(...)` in `TagManagerDialog` and `ContentContextMenu` explicitly call `UiHelper::applyMenuStyle(menu)`.

---

## Modified Files List
- `src/ui/UiHelper.h`
- `src/ui/UiHelper.cpp`
- `src/ui/QuarkApplication.cpp`
- `src/ui/FavoritePanel.cpp`
- `src/ui/NavPanel.cpp`
- `src/ui/TitleBarWidget.cpp`
- `src/ui/DriveBarWidget.cpp`
- `src/ui/TagManagerDialog.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/UiHelper.h` & `src/ui/UiHelper.cpp`
Extend `UiHelper` to support `QTextEdit` and `QPlainTextEdit` context menus:

```
<<<<<<< SEARCH
    static void showLineEditContextMenu(QLineEdit* edit, const QPoint& pos);
=======
    static void showLineEditContextMenu(QLineEdit* edit, const QPoint& pos);
    static void showTextEditContextMenu(QWidget* textWidget, const QPoint& pos);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void UiHelper::showLineEditContextMenu(QLineEdit* edit, const QPoint& pos) {
    if (!edit) return;
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

    QAction* actDelete = menu.addAction(getIcon("trash", QColor("#EEEEEE")), "删除");
    actDelete->setEnabled(!edit->isReadOnly() && edit->hasSelectedText());
    QObject::connect(actDelete, &QAction::triggered, edit, [edit]() {
        edit->del();
    });

    menu.addSeparator();

    QAction* actSelectAll = menu.addAction(getIcon("select", QColor("#EEEEEE")), "全选");
    actSelectAll->setShortcut(QKeySequence::SelectAll);
    actSelectAll->setEnabled(!edit->text().isEmpty());
    QObject::connect(actSelectAll, &QAction::triggered, edit, &QLineEdit::selectAll);

    menu.exec(edit->mapToGlobal(pos));
}
=======
void UiHelper::showLineEditContextMenu(QLineEdit* edit, const QPoint& pos) {
    if (!edit) return;
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

    QAction* actDelete = menu.addAction(getIcon("trash", QColor("#EEEEEE")), "删除");
    actDelete->setEnabled(!edit->isReadOnly() && edit->hasSelectedText());
    QObject::connect(actDelete, &QAction::triggered, edit, [edit]() {
        edit->del();
    });

    menu.addSeparator();

    QAction* actSelectAll = menu.addAction(getIcon("select", QColor("#EEEEEE")), "全选");
    actSelectAll->setShortcut(QKeySequence::SelectAll);
    actSelectAll->setEnabled(!edit->text().isEmpty());
    QObject::connect(actSelectAll, &QAction::triggered, edit, &QLineEdit::selectAll);

    menu.exec(edit->mapToGlobal(pos));
}

void UiHelper::showTextEditContextMenu(QWidget* textWidget, const QPoint& pos) {
    if (!textWidget) return;
    QMenu menu(textWidget);
    applyMenuStyle(&menu);

    QTextEdit* textEdit = qobject_cast<QTextEdit*>(textWidget);
    QPlainTextEdit* plainEdit = qobject_cast<QPlainTextEdit*>(textWidget);
    if (!textEdit && !plainEdit) return;

    bool canUndo = textEdit ? textEdit->document()->isUndoAvailable() : plainEdit->document()->isUndoAvailable();
    bool canRedo = textEdit ? textEdit->document()->isRedoAvailable() : plainEdit->document()->isRedoAvailable();
    bool isReadOnly = textEdit ? textEdit->isReadOnly() : plainEdit->isReadOnly();
    bool hasSelection = textEdit ? textEdit->textCursor().hasSelection() : plainEdit->textCursor().hasSelection();

    QAction* actUndo = menu.addAction(getIcon("undo", QColor("#EEEEEE")), "撤销");
    actUndo->setShortcut(QKeySequence::Undo);
    actUndo->setEnabled(canUndo);
    if (textEdit) QObject::connect(actUndo, &QAction::triggered, textEdit, &QTextEdit::undo);
    else QObject::connect(actUndo, &QAction::triggered, plainEdit, &QPlainTextEdit::undo);

    QAction* actRedo = menu.addAction(getIcon("redo", QColor("#EEEEEE")), "重做");
    actRedo->setShortcut(QKeySequence::Redo);
    actRedo->setEnabled(canRedo);
    if (textEdit) QObject::connect(actRedo, &QAction::triggered, textEdit, &QTextEdit::redo);
    else QObject::connect(actRedo, &QAction::triggered, plainEdit, &QPlainTextEdit::redo);

    menu.addSeparator();

    QAction* actCut = menu.addAction(getIcon("cut", QColor("#EEEEEE")), "剪切");
    actCut->setShortcut(QKeySequence::Cut);
    actCut->setEnabled(!isReadOnly && hasSelection);
    if (textEdit) QObject::connect(actCut, &QAction::triggered, textEdit, &QTextEdit::cut);
    else QObject::connect(actCut, &QAction::triggered, plainEdit, &QPlainTextEdit::cut);

    QAction* actCopy = menu.addAction(getIcon("copy", QColor("#EEEEEE")), "复制");
    actCopy->setShortcut(QKeySequence::Copy);
    actCopy->setEnabled(hasSelection);
    if (textEdit) QObject::connect(actCopy, &QAction::triggered, textEdit, &QTextEdit::copy);
    else QObject::connect(actCopy, &QAction::triggered, plainEdit, &QPlainTextEdit::copy);

    QAction* actPaste = menu.addAction(getIcon("paste", QColor("#EEEEEE")), "粘贴");
    actPaste->setShortcut(QKeySequence::Paste);
    actPaste->setEnabled(!isReadOnly && !QApplication::clipboard()->text().isEmpty());
    if (textEdit) QObject::connect(actPaste, &QAction::triggered, textEdit, &QTextEdit::paste);
    else QObject::connect(actPaste, &QAction::triggered, plainEdit, &QPlainTextEdit::paste);

    menu.addSeparator();

    QAction* actSelectAll = menu.addAction(getIcon("select", QColor("#EEEEEE")), "全选");
    actSelectAll->setShortcut(QKeySequence::SelectAll);
    if (textEdit) QObject::connect(actSelectAll, &QAction::triggered, textEdit, &QTextEdit::selectAll);
    else QObject::connect(actSelectAll, &QAction::triggered, plainEdit, &QPlainTextEdit::selectAll);

    menu.exec(textWidget->mapToGlobal(pos));
}
>>>>>>> REPLACE
```

---

### 2. `src/ui/QuarkApplication.cpp`
Intercept `QTextEdit` and `QPlainTextEdit` context menu events:

```
<<<<<<< SEARCH
        // 1. 全局 ContextMenu 拦截并应用 QuarkMeta 专属暗色右键菜单
        if (type == QEvent::ContextMenu && receiver->isWidgetType()) {
            if (QLineEdit* edit = qobject_cast<QLineEdit*>(receiver)) {
                if (edit->contextMenuPolicy() == Qt::DefaultContextMenu) {
                    QContextMenuEvent* cme = static_cast<QContextMenuEvent*>(event);
                    UiHelper::showLineEditContextMenu(edit, cme->pos());
                    event->accept();
                    return true;
                }
            }
        }
=======
        // 1. 全局 ContextMenu 拦截并应用 QuarkMeta 专属暗色右键菜单
        if (type == QEvent::ContextMenu && receiver->isWidgetType()) {
            if (QLineEdit* edit = qobject_cast<QLineEdit*>(receiver)) {
                if (edit->contextMenuPolicy() == Qt::DefaultContextMenu) {
                    QContextMenuEvent* cme = static_cast<QContextMenuEvent*>(event);
                    UiHelper::showLineEditContextMenu(edit, cme->pos());
                    event->accept();
                    return true;
                }
            } else if (qobject_cast<QTextEdit*>(receiver) || qobject_cast<QPlainTextEdit*>(receiver)) {
                QWidget* textWidget = static_cast<QWidget*>(receiver);
                if (textWidget->contextMenuPolicy() == Qt::DefaultContextMenu) {
                    QContextMenuEvent* cme = static_cast<QContextMenuEvent*>(event);
                    UiHelper::showTextEditContextMenu(textWidget, cme->pos());
                    event->accept();
                    return true;
                }
            }
        }
>>>>>>> REPLACE
```

---

### 3. `src/ui/FavoritePanel.cpp`
Enforce neutral monochrome icon in `FavoritePanel`'s submenu:

```
<<<<<<< SEARCH
        QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor(curColorHex)), "切换图标");
=======
        QMenu* iconMenu = menu.addMenu(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "切换图标");
        ThemeManager::applyMenuStyle(iconMenu);
>>>>>>> REPLACE
```

---

### 4. `src/ui/NavPanel.cpp`
Reuse `ContextMenuFactory` for `NavPanel` context menus:

```
<<<<<<< SEARCH
    QMenu menu(this);
    ThemeManager::applyMenuStyle(&menu);

    menu.addAction(UiHelper::getIcon("open_external", QColor("#EEEEEE")), "在文件资源管理器中打开", [cleanPath]() {
        ShellHelper::showInExplorer(cleanPath);
    });

    menu.addAction(UiHelper::getIcon("copy", QColor("#EEEEEE")), "复制路径", [cleanPath]() {
        QApplication::clipboard()->setText(cleanPath);
    });
=======
    QMenu menu(this);
    ThemeManager::applyMenuStyle(&menu);

    ContextMenuFactory::buildShowInExplorerAction(&menu, cleanPath, this);
    ContextMenuFactory::buildCopyPathAction(&menu, {cleanPath}, this);
>>>>>>> REPLACE
```

---

### 5. `src/ui/TitleBarWidget.cpp` & `src/ui/DriveBarWidget.cpp`
Use `UiHelper::createMenu` and remove inline QSS:

```
<<<<<<< SEARCH
    QMenu* createMenu = new QMenu(m_btnCreate);
=======
    QMenu* createMenu = UiHelper::createMenu(m_btnCreate);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QMenu* extMenu = new QMenu(m_btnExtensionManager);
=======
    QMenu* extMenu = UiHelper::createMenu(m_btnExtensionManager);
>>>>>>> REPLACE
```

---

### 6. `src/ui/controllers/ContentContextMenu.cpp` & `src/ui/TagManagerDialog.cpp`
Ensure submenus explicitly apply `ThemeManager::applyMenuStyle`:

```
<<<<<<< SEARCH
            QMenu* moreMenu = menu.addMenu(UiHelper::getIcon("more_horizontal", QColor("#EEEEEE"), 18), "更多");
=======
            QMenu* moreMenu = menu.addMenu(UiHelper::getIcon("more_horizontal", QColor("#EEEEEE"), 18), "更多");
            ThemeManager::applyMenuStyle(moreMenu);
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. Build Instructions
Run CMake build to compile:
```bash
cmake --build build --config Release
```

### 2. Verification Steps
1. **FavoritePanel Test**: Right-click on a favorite item in `FavoritePanel`. Verify "切换图标" submenu icon is neutral monochrome (`#EEEEEE`), not colored.
2. **QTextEdit / QPlainTextEdit Test**: Right-click on the note editor in `MetaPanel` or multi-line text boxes. Verify the QuarkMeta dark context menu pops up with SVG icons (`#EEEEEE`) and 10px spacing instead of the OS default English menu.
3. **NavPanel Test**: Right-click on a directory node in `NavPanel`. Verify actions match standard QuarkMeta context menu style and behavior.
4. **Submenu Test**: Open context menus with submenus (e.g. "更多", "移动到", "新建..."). Verify submenus inherit standard dark background (`#252526`), `#3E3E42` selection highlights, and rounded corners.
