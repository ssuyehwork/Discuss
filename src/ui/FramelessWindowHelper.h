#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QObject>
#include <QWidget>
#include <QPointer>
#include <QPoint>
#include <QRect>
#include <QVector>

namespace QuarkMeta {

class ResizeHandle;

/**
 * @brief 全项目统一的正统纯 Qt 无边框窗口/浮层助手类 (路线 A)
 * 采用 8 向透明物理手柄控件覆盖边缘，光标由 Qt 控件原生持有，零全局事件污染，
 * 绝无 MouseMove 轮询计算，彻底消除重复造轮子。
 */
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

    void updateHandleGeometries();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override;

    void initHandles();

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;
    QVector<ResizeHandle*> m_handles;

    bool m_isTitleDragging = false;
    QPoint m_dragStartGlobalPos;
    QRect m_dragStartGeometry;

    static constexpr int kHandleMargin = 6;
};

} // namespace QuarkMeta