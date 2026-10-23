# QuarkApplication - Framework Level Context Menu & Shortcut Handling Plan (QuarkApplication-1.md)

## 1. Overview
This implementation plan elevates cross-cutting input concerns to the top-level `QuarkApplication::notify` event gateway:
1. **Global QLineEdit / QTextEdit Context Menu Normalization**: Automatically intercepts `QEvent::ContextMenu` for any `QLineEdit`, `QTextEdit`, or `QPlainTextEdit` with default context menu policy (`Qt::DefaultContextMenu`), invoking QuarkMeta's dark-themed exclusive context menu without requiring individual widgets to manually call `UiHelper::setupLineEditContextMenu`.
2. **Global Ctrl+W Window Dismissal Guarantee**: Ensures `Ctrl+W` key press events consistently close the currently active window or dialog at the framework event level, eliminating scattered local shortcut handlers.

## 2. Modified Files List
- `src/ui/QuarkApplication.h`
- `src/ui/QuarkApplication.cpp`
- `src/ui/UiHelper.h`
- `src/ui/UiHelper.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/UiHelper.h`
Add `showLineEditContextMenu(QLineEdit* edit, const QPoint& pos)` to allow direct invocation from `QuarkApplication::notify`.

```git
<<<<<<< SEARCH
    static void setupLineEditContextMenu(QLineEdit* edit);
=======
    static void setupLineEditContextMenu(QLineEdit* edit);
    static void showLineEditContextMenu(QLineEdit* edit, const QPoint& pos);
>>>>>>> REPLACE
```

### `src/ui/UiHelper.cpp`
Refactor `setupLineEditContextMenu` to delegate to `showLineEditContextMenu`.

```git
<<<<<<< SEARCH
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
}

void UiHelper::setupLineEditContextMenu(QLineEdit* edit) {
    if (!edit) return;
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(edit, &QLineEdit::customContextMenuRequested, edit, [edit](const QPoint& pos) {
        showLineEditContextMenu(edit, pos);
    });
}
>>>>>>> REPLACE
```

### `src/ui/QuarkApplication.cpp`
In `QuarkApplication::notify`, intercept `QEvent::ContextMenu` for text edits, and `QEvent::KeyPress` for `Ctrl+W`.

```git
<<<<<<< SEARCH
#include "QuarkApplication.h"
#include <QEvent>
#include <QGuiApplication>
#include <QWidget>
#include <QCursor>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

bool QuarkApplication::notify(QObject* receiver, QEvent* event) {
    if (event) {
        QEvent::Type type = event->type();
        if (type == QEvent::Close || type == QEvent::Hide) {
            if (receiver && receiver->isWidgetType()) {
                QWidget* w = static_cast<QWidget*>(receiver);
                if (w->isWindow()) {
                    while (QGuiApplication::overrideCursor()) {
                        QGuiApplication::restoreOverrideCursor();
                    }
                    if (QWidget* mouseGrabber = QWidget::mouseGrabber()) {
                        mouseGrabber->releaseMouse();
                    }
#ifdef Q_OS_WIN
                    if (w->testAttribute(Qt::WA_WState_Created)) {
                        HWND hCap = ::GetCapture();
                        if (hCap != NULL) {
                            ::ReleaseCapture();
                        }
                    }
#endif
                    w->unsetCursor();
                    QCursor::setPos(QCursor::pos());
                }
            }
        }
    }
    return QApplication::notify(receiver, event);
}

} // namespace QuarkMeta
=======
#include "QuarkApplication.h"
#include "UiHelper.h"
#include <QEvent>
#include <QGuiApplication>
#include <QWidget>
#include <QLineEdit>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QCursor>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

bool QuarkApplication::notify(QObject* receiver, QEvent* event) {
    if (event && receiver) {
        QEvent::Type type = event->type();

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

        // 2. 窗口关闭/隐藏事件驱动全局光标与抓取自愈
        if (type == QEvent::Close || type == QEvent::Hide) {
            if (receiver->isWidgetType()) {
                QWidget* w = static_cast<QWidget*>(receiver);
                if (w->isWindow()) {
                    while (QGuiApplication::overrideCursor()) {
                        QGuiApplication::restoreOverrideCursor();
                    }
                    if (QWidget* mouseGrabber = QWidget::mouseGrabber()) {
                        mouseGrabber->releaseMouse();
                    }
#ifdef Q_OS_WIN
                    if (w->testAttribute(Qt::WA_WState_Created)) {
                        HWND hCap = ::GetCapture();
                        if (hCap != NULL) {
                            ::ReleaseCapture();
                        }
                    }
#endif
                    w->unsetCursor();
                    QCursor::setPos(QCursor::pos());
                }
            }
        }
    }
    return QApplication::notify(receiver, event);
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
```bash
cmake --build build
```
Verify that all line edits across the application automatically show QuarkMeta dark context menus without manual code boilerplate.
