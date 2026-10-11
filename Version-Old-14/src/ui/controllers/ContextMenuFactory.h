#pragma once

#include <QMenu>
#include <QAction>
#include <QStringList>
#include <functional>

namespace QuarkMeta {

class ContextMenuFactory {
public:
    /**
     * @brief 置顶/取消置顶 SSOT 统一执行入口 (驱动 CoreEngine 并且通过 CentralEventHub 发送全局状态变动)
     */
    static bool togglePinState(const QStringList& paths, bool pin);

    /**
     * @brief 复制路径列表至剪贴板 SSOT 入口 (统一原生路径风格与 \r\n 换行符)
     */
    static bool copyPathsToClipboard(const QStringList& paths, bool showOverlay = true);

    /**
     * @brief 复制文件名列表至剪贴板 SSOT 入口 (统一 \r\n 换行符)
     */
    static bool copyNamesToClipboard(const QStringList& paths, bool showOverlay = true);

    /**
     * @brief 提取文件文本内容至剪贴板 SSOT 入口 (快捷键 Ctrl+Shift+E 与 右键菜单 "支持提取内容" 共同调用)
     */
    static bool extractContentToClipboard(const QString& path);

    /**
     * @brief 构建“提取内容”右键菜单项 (自动处理可提取/不可提取状态与点击回调)
     */
    static QAction* buildExtractContentAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);

    static QAction* buildShowInExplorerAction(QMenu* menu, const QString& path, QObject* receiver = nullptr);
    static QAction* buildCopyPathAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildCopyNameAction(QMenu* menu, const QStringList& paths, QObject* receiver = nullptr);
    static QAction* buildPinToggleAction(QMenu* menu, bool isPinned, std::function<void(bool)> onToggle, QObject* receiver = nullptr);

    /**
     * @brief 构建“切换图标”二级菜单 SSOT 入口 (内置内置图标库与 ColorStripPicker 颜色选择)
     * @param parentMenu 父级右键菜单
     * @param curIconKey 当前图标 key
     * @param curColorHex 当前颜色 hex
     * @param onIconSelected 图标被选中时的回调 (iconKey)
     * @param onColorSelected 颜色被选中时的回调 (colorHex)
     */
    static QMenu* buildIconPickerMenu(QMenu* parentMenu,
                                      const QString& curIconKey,
                                      const QString& curColorHex,
                                      std::function<void(const QString& iconKey)> onIconSelected,
                                      std::function<void(const QString& colorHex)> onColorSelected);
};

} // namespace QuarkMeta
