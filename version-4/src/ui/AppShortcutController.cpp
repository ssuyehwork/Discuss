#include "AppShortcutController.h"
#include "SearchController.h"
#include "../core/NavigationService.h"
#include "../core/UndoManager.h"
#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QAbstractItemView>
#include <QKeyEvent>

namespace QuarkMeta {

AppShortcutController::AppShortcutController(QWidget* targetWindow,
                                             SearchController* searchController,
                                             QObject* parent)
    : QObject(parent),
      m_window(targetWindow),
      m_searchController(searchController) {
    if (qApp) {
        qApp->installEventFilter(this);
    }
    initShortcuts();
}

AppShortcutController::~AppShortcutController() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
}

bool AppShortcutController::isEditingFocus() {
    QWidget* focusW = QApplication::focusWidget();
    if (!focusW) return false;

    // 1. 标准单行/多行/数值输入框
    if (qobject_cast<QLineEdit*>(focusW) ||
        qobject_cast<QTextEdit*>(focusW) ||
        qobject_cast<QPlainTextEdit*>(focusW) ||
        qobject_cast<QAbstractSpinBox*>(focusW)) {
        return true;
    }

    // 2. 可编辑下拉框
    if (auto cb = qobject_cast<QComboBox*>(focusW)) {
        if (cb->isEditable()) return true;
    }

    // 3. 元对象继承检查 (涵盖继承 QLineEdit/QTextEdit 等的自定义控件)
    if (focusW->inherits("QLineEdit") ||
        focusW->inherits("QTextEdit") ||
        focusW->inherits("QPlainTextEdit") ||
        focusW->inherits("QAbstractSpinBox")) {
        return true;
    }

    // 4. ItemView 行内编辑器检查: 焦点控件属于 QAbstractItemView 内部的子控件（且既不是 view 本身也不是 viewport）
    QWidget* p = focusW->parentWidget();
    while (p) {
        if (auto view = qobject_cast<QAbstractItemView*>(p)) {
            if (focusW != view && focusW != view->viewport()) {
                return true;
            }
        }
        p = p->parentWidget();
    }

    return false;
}

bool AppShortcutController::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress && m_window) {
        QKeyEvent* keyEv = static_cast<QKeyEvent*>(event);
        if (keyEv->key() == Qt::Key_Tab && keyEv->modifiers() == Qt::NoModifier) {
            QWidget* watchedW = qobject_cast<QWidget*>(watched);
            if (watchedW && (watchedW == m_window || m_window->isAncestorOf(watchedW))) {
                if (!isEditingFocus()) {
                    emit toggleImmersiveRequested();
                    event->accept();
                    return true;
                }
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

void AppShortcutController::initShortcuts() {
    if (!m_window) return;

    // 1. F5: 局内刷新当前目录
    QShortcut* scRefresh = new QShortcut(QKeySequence(Qt::Key_F5), m_window);
    scRefresh->setContext(Qt::WindowShortcut);
    connect(scRefresh, &QShortcut::activated, this, []() {
        NavigationService::instance().refresh();
    });

    // 2. Ctrl+Z: 局内文件撤销 (输入框获焦时 Qt 底层优先响应文字撤销)
    QShortcut* scUndo = new QShortcut(QKeySequence::Undo, m_window);
    scUndo->setContext(Qt::WindowShortcut);
    connect(scUndo, &QShortcut::activated, this, []() {
        UndoManager::instance().undo();
    });

    // 3. Ctrl+Shift+Z / Ctrl+Y: 局内文件重做
    QShortcut* scRedo = new QShortcut(QKeySequence::Redo, m_window);
    scRedo->setContext(Qt::WindowShortcut);
    connect(scRedo, &QShortcut::activated, this, []() {
        UndoManager::instance().redo();
    });

    // 4. Alt+Q: 局内切换窗口置顶
    QShortcut* scPin = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Q), m_window);
    scPin->setContext(Qt::WindowShortcut);
    connect(scPin, &QShortcut::activated, this, &AppShortcutController::togglePinRequested);

    // 5. Ctrl+F: 局内聚焦搜索栏
    QShortcut* scFind = new QShortcut(QKeySequence::Find, m_window);
    scFind->setContext(Qt::WindowShortcut);
    connect(scFind, &QShortcut::activated, this, [this]() {
        if (m_searchController && m_searchController->searchEdit()) {
            m_searchController->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_searchController->searchEdit()->selectAll();
        }
    });
}

} // namespace QuarkMeta
