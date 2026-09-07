#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QObject>
#include <QWidget>
#include <QPointer>
#include <QPoint>
#include <QRect>
#include <QEvent>

namespace QuarkMeta {

/**
 * @brief 工业级无边框窗口助手类
 */
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;

    bool m_isDraggingMaximized = false;
    QPoint m_dragNormalOffset;

    static constexpr int kBaseResizeMargin = 8;
};

} // namespace QuarkMeta