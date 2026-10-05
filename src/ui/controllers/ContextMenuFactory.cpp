#include "ContextMenuFactory.h"
#include "../UiHelper.h"
#include "../../util/ShellHelper.h"
#include "../ToolTipOverlay.h"
#include "../StyleLibrary.h"
#include "../../core/CoreEngine.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>

namespace QuarkMeta {

bool ContextMenuFactory::togglePinState(const QStringList& paths, bool pin) {
    if (paths.isEmpty()) return false;

    AppCommand cmd;
    cmd.type = AppCommandType::SetPinned;
    cmd.targetPaths = paths;
    cmd.params["pinned"] = pin;
    return CoreEngine::instance().executeCommand(cmd);
}

QAction* ContextMenuFactory::buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver) {
    if (!menu || path.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("folder_search", QColor("#EEEEEE"), 18), "在“资源管理器”中显示");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [path]() {
        ShellHelper::openInExplorer(path);
    });
    return action;
}

bool ContextMenuFactory::copyPathsToClipboard(const QStringList& paths, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList cleanPaths;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            cleanPaths << QDir::toNativeSeparators(p);
        }
    }
    if (cleanPaths.isEmpty()) return false;

    QApplication::clipboard()->setText(cleanPaths.join("\r\n"));
    if (showOverlay) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制路径至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

bool ContextMenuFactory::copyNamesToClipboard(const QStringList& paths, bool showOverlay) {
    if (paths.isEmpty()) return false;

    QStringList names;
    for (const auto& p : paths) {
        if (!p.isEmpty()) {
            names << QFileInfo(p).fileName();
        }
    }
    if (names.isEmpty()) return false;

    QApplication::clipboard()->setText(names.join("\r\n"));
    if (showOverlay) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "已复制名称至剪贴板", 1500, Style::SuccessGreen);
    }
    return true;
}

QAction* ContextMenuFactory::buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("link", QColor("#EEEEEE"), 18), "复制完整路径");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        copyPathsToClipboard(paths, true);
    });
    return action;
}

QAction* ContextMenuFactory::buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver) {
    if (!menu || paths.isEmpty()) return nullptr;

    QAction* action = menu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE"), 18), "复制名称");
    QObject::connect(action, &QAction::triggered, receiver ? receiver : menu, [paths]() {
        copyNamesToClipboard(paths, true);
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
