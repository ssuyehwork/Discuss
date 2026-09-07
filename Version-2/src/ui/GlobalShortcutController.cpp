#include "GlobalShortcutController.h"
#include "MainWindow.h"
#include "ContentPanel.h"
#include "SearchController.h"
#include "../core/UndoManager.h"
#include <QKeyEvent>

namespace QuarkMeta {

GlobalShortcutController::GlobalShortcutController(MainWindow* mainWindow, QObject* parent)
    : QObject(parent), m_mainWindow(mainWindow) {
    if (m_mainWindow) {
        m_mainWindow->installEventFilter(this);
    }
}

bool GlobalShortcutController::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_mainWindow && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (handleKeyPress(keyEvent)) {
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

bool GlobalShortcutController::handleKeyPress(QKeyEvent* event) {
    if (!m_mainWindow) return false;

    // 1. F5: 刷新当前目录
    if (event->key() == Qt::Key_F5) {
        if (m_mainWindow->contentPanel()) {
            m_mainWindow->contentPanel()->refreshAll();
        }
        event->accept();
        return true;
    }

    // 2. Ctrl+Z / Ctrl+Shift+Z: 撤销与重做
    if (event->key() == Qt::Key_Z && (event->modifiers() & Qt::ControlModifier)) {
        if (event->modifiers() & Qt::ShiftModifier) {
            UndoManager::instance().redo();
        } else {
            UndoManager::instance().undo();
        }
        event->accept();
        return true;
    }

    // 3. Alt+Q: 切换窗口置顶状态
    if (event->key() == Qt::Key_Q && (event->modifiers() & Qt::AltModifier)) {
        if (m_mainWindow->btnPinTop()) {
            m_mainWindow->btnPinTop()->setChecked(!m_mainWindow->btnPinTop()->isChecked());
        }
        event->accept();
        return true;
    }

    // 4. Ctrl+F: 聚焦搜索过滤框
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        if (m_mainWindow->searchController() && m_mainWindow->searchController()->searchEdit()) {
            m_mainWindow->searchController()->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_mainWindow->searchController()->searchEdit()->selectAll();
        }
        event->accept();
        return true;
    }

    return false;
}

} // namespace QuarkMeta
