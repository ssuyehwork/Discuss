#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QObject>
#include <QWidget>
#include <QPointer>

namespace QuarkMeta {

/**
 * @brief 无边框窗口原生消息助手类
 * 集中接管 Win32 原生 WM_NCCALCSIZE, WM_GETMINMAXINFO, WM_NCHITTEST 与 WM_SETCURSOR 消息，
 * 提供硬件级平滑拖拽、边缘缩放、光标切换及双击最大化响应，可复用于任何 QWidget/QMainWindow 顶层窗口。
 */
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);

    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;

    static constexpr int kBaseResizeMargin = 6;
};

} // namespace QuarkMeta
