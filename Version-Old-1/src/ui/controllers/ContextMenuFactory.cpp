#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../../util/ShellHelper.h"
#include "../ToolTipOverlay.h"
#include "../StyleLibrary.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>

namespace QuarkMeta {

QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [path]() {
        ShellHelper::openInExplorer(path);
    });
    return action;
}

QAction* ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制完整路径");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        QStringList cleanPaths;
        for (const auto& p : paths) {
            cleanPaths << QDir::toNativeSeparators(p);
        }
        QApplication::clipboard()->setText(cleanPaths.join("\n"));
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径至剪贴板", 1500, Style::SuccessGreen);
    });
    return action;
}

QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        QStringList names;
        for (const auto& p : paths) {
            names << QFileInfo(p).fileName();
        }
        QApplication::clipboard()->setText(names.join("\n"));
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制名称至剪贴板", 1500, Style::SuccessGreen);
    });
    return action;
}

QAction* ContextMenuFactory::buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver) {
    if (!menu) return nullptr;

    QIcon icon = UiHelper::getIcon(isPinned ? "pin_tilted" : "pin_vertical", QColor("#EEEEEE"), 18);
    QString text = isPinned ? "取消置顶" : "置顶";

    QAction* action = menu->addAction(icon, text);
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [isPinned, onToggle]() {
        if (onToggle) onToggle(!isPinned);
    });
    return action;
}

} // namespace QuarkMeta
