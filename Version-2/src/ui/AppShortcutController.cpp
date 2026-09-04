#include "AppShortcutController.h"
#include "SearchController.h"
#include "../core/NavigationService.h"
#include "../core/UndoManager.h"
#include <QLineEdit>

namespace QuarkMeta {

AppShortcutController::AppShortcutController(QWidget* targetWindow,
                                             SearchController* searchController,
                                             QObject* parent)
    : QObject(parent),
      m_window(targetWindow),
      m_searchController(searchController) {
    initShortcuts();
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
