#pragma once

#include <QMenu>
#include <QAction>
#include <QStringList>
#include <functional>

namespace QuarkMeta {

class ContextMenuFactory {
public:
    /**
     * @brief 复制路径列表至剪贴板 SSOT 入口 (统一原生路径风格与 \r\n 换行符)
     */
    static bool copyPathsToClipboard(const QStringList& paths, bool showOverlay = true);

    /**
     * @brief 复制文件名列表至剪贴板 SSOT 入口 (统一 \r\n 换行符)
     */
    static bool copyNamesToClipboard(const QStringList& paths, bool showOverlay = true);

    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);
};

} // namespace QuarkMeta
