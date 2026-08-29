#include "ThemeManager.h"
#include <QFile>

namespace QuarkMeta {

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager(QObject* parent) : QObject(parent) {}

void ThemeManager::initialize(QApplication* app) {
    if (!app) return;
    app->setStyleSheet(getGlobalStyleSheet());
}

QString ThemeManager::getGlobalStyleSheet() const {
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        return QLatin1String(file.readAll());
    }

    return QString(R"(
        /* 1. 顶层窗口与基础容器 */
        QMainWindow, QDialog { background-color: #1E1E1E; color: #EEEEEE; }
        QWidget#CentralWidget { background-color: #1E1E1E; }
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E; border: none; border-radius: 0px; margin: 0px; padding: 0px;
        }
        #ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; }

        /* 2. 统一全软件 QMenu 菜单 (包括托盘菜单与右键菜单) */
        QMenu {
            background-color: #252526;
            color: #EEEEEE;
            border: 1px solid #333333;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item {
            background-color: transparent;
            color: #EEEEEE;
            padding: 6px 24px 6px 12px;
            border-radius: 4px;
            font-size: 12px;
        }
        QMenu::item:selected {
            background-color: #378ADD;
            color: #FFFFFF;
        }
        QMenu::separator {
            height: 1px;
            background-color: #333333;
            margin: 4px 6px;
        }

        /* 3. 统一全局滚动条 */
        QScrollBar:vertical { border: none; background: transparent; width: 10px; }
        QScrollBar::handle:vertical { background: #333333; min-height: 20px; border-radius: 3px; }
        QScrollBar::handle:vertical:hover { background: #4E4E52; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width: 0px; height: 0px; }
        QScrollBar:horizontal { border: none; background: transparent; height: 10px; }
        QScrollBar::handle:horizontal { background: #333333; min-width: 20px; border-radius: 3px; }
        QScrollBar::handle:horizontal:hover { background: #4E4E52; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; height: 0px; }

        /* 4. 统一全局输入框 */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background: #252526; border: 1px solid #333333; border-radius: 4px; color: #EEEEEE; padding-left: 6px;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus { border: 1px solid #378ADD; }
    )");
}

void ThemeManager::applyMenuStyle(QWidget* menu) const {
    if (!menu) return;
    menu->setAttribute(Qt::WA_TranslucentBackground, true);
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    // 🚀【托盘与右键菜单深色自适应】：显式注入 setStyleSheet，强制 Qt 绕过 Win32 原生 HMENU，走 Qt 自绘 QMenu
    menu->setStyleSheet(
        "QMenu {"
        "   background-color: #252526;"
        "   color: #EEEEEE;"
        "   border: 1px solid #333333;"
        "   border-radius: 6px;"
        "   padding: 4px;"
        "}"
        "QMenu::item {"
        "   background-color: transparent;"
        "   color: #EEEEEE;"
        "   padding: 6px 24px 6px 12px;"
        "   border-radius: 4px;"
        "   font-size: 12px;"
        "}"
        "QMenu::item:selected {"
        "   background-color: #378ADD;"
        "   color: #FFFFFF;"
        "}"
        "QMenu::separator {"
        "   height: 1px;"
        "   background-color: #333333;"
        "   margin: 4px 6px;"
        "}"
    );
}

} // namespace QuarkMeta
