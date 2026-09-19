#pragma once

#include <QMenu>
#include <QAction>
#include <QStringList>
#include <functional>

namespace QuarkMeta {

class ContextMenuFactory {
public:
    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
};

} // namespace QuarkMeta
