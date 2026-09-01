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

        /* 🚀【全局唯一样式真理源】：卡片 5px 实体物理切缝 + Dual-mode 深色风格 */
        QSplitter {
            background: transparent;
            border: none;
        }
        QSplitter::handle:horizontal {
            background-color: #1E1E1E;
            width: 1px;
        }
        QSplitter::handle:horizontal:hover {
            background-color: #378ADD;
        }

        /* 2. 五大实体栏区卡片底板（通过左右各 2px margin + 1px 分隔手柄构建 5px 物理缝隙） */
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: 1px solid #333333;
            border-radius: 0px;
            margin: 0px 2px;
            padding: 0px;
        }
        #ContainerHeader {
            background-color: #252526;
            border-radius: 0px;
            border-bottom: 1px solid #333333;
        }

        /* 3. 统一全软件 QMenu 菜单 (包括托盘菜单与右键菜单) */
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
            background-color: #3E3E42;
            color: #FFFFFF;
        }
        QMenu::item:disabled {
            color: #666666;
            background-color: transparent;
        }
        QMenu::separator {
            height: 1px;
            background-color: #333333;
            margin: 4px 6px;
        }

        /* 4. 统一全局滚动条 */
        QScrollBar:vertical { border: none; background: transparent; width: 10px; }
        QScrollBar::handle:vertical { background: #333333; min-height: 20px; border-radius: 3px; }
        QScrollBar::handle:vertical:hover { background: #4E4E52; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width: 0px; height: 0px; }
        QScrollBar:horizontal { border: none; background: transparent; height: 10px; }
        QScrollBar::handle:horizontal { background: #333333; min-width: 20px; border-radius: 3px; }
        QScrollBar::handle:horizontal:hover { background: #4E4E52; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; height: 0px; }

        /* 5. 统一全局输入框 */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background: #252526; border: 1px solid #333333; border-radius: 4px; color: #EEEEEE; padding-left: 6px;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus { border: 1px solid #378ADD; }

        /* 6. 统一 QTreeView 列表视图暗色斑马纹与高亮 */
        QTreeView {
            background-color: #1E1E1E;
            alternate-background-color: #252526;
            qproperty-alternateBase: #252526;
            color: #EEEEEE;
            border: none;
            outline: 0;
        }
        QTreeView::item {
            height: 28px;
            border: none;
        }
        QTreeView::item:alternate {
            background-color: #252526;
        }
        QTreeView::item:hover {
            background-color: #2A2D2E;
        }
        QTreeView::item:selected {
            background-color: rgba(55, 138, 221, 0.3);
            color: #FFFFFF;
        }
    )");
}

void ThemeManager::applyMenuStyle(QWidget* menu) const {
    if (!menu) return;
    menu->setAttribute(Qt::WA_TranslucentBackground, true);
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

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
        "   background-color: #3E3E42;"
        "   color: #FFFFFF;"
        "}"
        "QMenu::item:disabled {"
        "   color: #666666;"
        "   background-color: transparent;"
        "}"
        "QMenu::separator {"
        "   height: 1px;"
        "   background-color: #333333;"
        "   margin: 4px 6px;"
        "}"
    );
}

} // namespace QuarkMeta