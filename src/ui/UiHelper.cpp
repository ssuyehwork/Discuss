#include "UiHelper.h"
#include "ThemeManager.h"
#include "SvgIcons.h"

#include <QApplication>
#include <QLineEdit>
#include <QMenu>
#include <QClipboard>
#include <QKeyEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

void UiHelper::cleanupWidgetCursorState(QWidget* widget) {
    // 1. 彻底出栈所有可能的 QApplication 全局 overrideCursor
    while (QGuiApplication::overrideCursor()) {
        QGuiApplication::restoreOverrideCursor();
    }

    // 2. 释放 Qt 级别的孤儿 mouseGrabber
    if (QWidget* grabber = QWidget::mouseGrabber()) {
        if (grabber == widget || (widget && widget->isAncestorOf(grabber))) {
            grabber->releaseMouse();
        }
    }

    // 3. 解开 Win32 原生捕获锁
#ifdef Q_OS_WIN
    if (widget && widget->testAttribute(Qt::WA_WState_Created)) {
        HWND hwnd = reinterpret_cast<HWND>(widget->winId());
        if (GetCapture() == hwnd) {
            ::ReleaseCapture();
        }
    }
#endif

    // 4. 清空 Widget 及其父窗口身上的显式光标
    if (widget) {
        widget->unsetCursor();
        if (QWidget* parent = widget->parentWidget()) {
            parent->unsetCursor();
            if (QWidget* topWin = parent->window()) {
                topWin->unsetCursor();
            }
        }
    }

    // 5. 🚀【核心突破】：强行移动 0 像素鼠标位置，逼迫 Windows DWM 与 Qt 立即对当前静止鼠标重新触发 WM_SETCURSOR Hit-Test！
    QPoint currentPos = QCursor::pos();
    QCursor::setPos(currentPos);
}

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
