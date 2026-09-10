#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QObject>
#include <QWidget>
#include <QPointer>

namespace QuarkMeta {

/**
 * @brief 无边框窗口的角色类型——决定该窗口应具备哪些原生能力
 * 新增窗口角色时，只需要在这里加一个枚举值，并在 .cpp 的角色能力表里补一行，
 * 不需要改动任何现有调用点。
 */
enum class WindowRole {
    Primary,   // 主窗口：完整标题栏拖拽、最大化/还原、系统菜单、边缘缩放
    Tool,      // 悬浮工具窗：无标题栏、不可最大化，仅边缘缩放（整体拖动由窗口自己实现）
    Dialog     // 对话框：有自定义标题栏、可拖拽/最大化，但不需要标题栏最大化图标原生同步
};

/**
 * @brief 工业级无边框窗口助手类
 * 完全基于 Windows 原生 WM_NCHITTEST 进行硬件级缩放与拖拽，不使用暴力 grabMouse
 * 全项目统一的无边框窗口机制——MainWindow / 悬浮工具窗 / 对话框均通过 WindowRole 接入，
 * 不再各自维护一套拖拽/缩放实现。
 */
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, WindowRole role, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

private:
    explicit FramelessWindowHelper(QWidget* window, WindowRole role, QWidget* titleBar);
    ~FramelessWindowHelper() override = default;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;
    WindowRole m_role;

    static constexpr int kBaseResizeMargin = 8;
};

} // namespace QuarkMeta
