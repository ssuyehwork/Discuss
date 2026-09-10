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
