#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include <QApplication>
#include <QCoreApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>
#include <QAbstractButton>
#include <QComboBox>
#include <QSpinBox>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QMouseEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

namespace QuarkMeta {

FramelessWindowHelper* FramelessWindowHelper::apply(QWidget* window, QWidget* titleBar) {
    if (!window) return nullptr;
    return new FramelessWindowHelper(window, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar) {
    
    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint;
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }

    if (m_window) {
        m_window->setMouseTracking(true);
        if (QCoreApplication::instance()) {
            QCoreApplication::instance()->installEventFilter(this);
        }
    }
}

FramelessWindowHelper::~FramelessWindowHelper() {
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeEventFilter(this);
    }
}

bool FramelessWindowHelper::isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window) {
    QWidget* wWidget = child;
    while (wWidget && wWidget != titleBar && wWidget != window) {
        if (qobject_cast<QAbstractButton*>(wWidget) ||
            qobject_cast<QLineEdit*>(wWidget) ||
            qobject_cast<QSlider*>(wWidget) ||
            qobject_cast<QComboBox*>(wWidget) ||
            qobject_cast<QSpinBox*>(wWidget) ||
            qobject_cast<QScrollBar*>(wWidget) ||
            qobject_cast<QAbstractItemView*>(wWidget)) {
            return true;
        }
        wWidget = wWidget->parentWidget();
    }
    return false;
}

bool FramelessWindowHelper::handleNativeEvent(void* message, qintptr* result) {
#ifdef Q_OS_WIN
    if (!m_window || !result) return false;

    MSG* msg = static_cast<MSG*>(message);
    if (!msg) return false;

    // 1. 最大化多显示器边缘工作区补偿
    if (msg->message == WM_NCCALCSIZE) {
        if (msg->wParam == TRUE && m_window->isMaximized()) {
            NCCALCSIZE_PARAMS* pnc = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
            HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
            if (monitor) {
                MONITORINFO monitorInfo = {};
                monitorInfo.cbSize = sizeof(MONITORINFO);
                if (GetMonitorInfo(monitor, &monitorInfo)) {
                    pnc->rgrc[0] = monitorInfo.rcWork;
                }
            }
        }
        *result = 0;
        return true;
    }

    if (msg->message == WM_GETMINMAXINFO) {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        if (mmi) {
            HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
            if (monitor) {
                MONITORINFO monitorInfo = {};
                monitorInfo.cbSize = sizeof(MONITORINFO);
                if (GetMonitorInfo(monitor, &monitorInfo)) {
                    RECT workArea = monitorInfo.rcWork;
                    RECT monitorArea = monitorInfo.rcMonitor;
                    mmi->ptMaxPosition.x = workArea.left - monitorArea.left;
                    mmi->ptMaxPosition.y = workArea.top - monitorArea.top;
                    mmi->ptMaxSize.x = workArea.right - workArea.left;
                    mmi->ptMaxSize.y = workArea.bottom - workArea.top;
                }
            }
            if (m_window) {
                QSize minSz = m_window->minimumSize();
                if (minSz.width() > 0) mmi->ptMinTrackSize.x = minSz.width();
                if (minSz.height() > 0) mmi->ptMinTrackSize.y = minSz.height();
            }
        }
        *result = 0;
        return true;
    }

    // 2. 原生标题栏拖动与最大化“还原+移动”处理
    if (msg->message == WM_NCLBUTTONDOWN && msg->wParam == HTCAPTION) {
        if (m_window->isMaximized()) {
            POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
            QPoint globalPos(screenPt.x, screenPt.y);

            // 计算当前光标在最大化宽度上的比例
            QRect maxGeom = m_window->geometry();
            double factorX = static_cast<double>(globalPos.x() - maxGeom.left()) / maxGeom.width();

            // 执行还原
            m_window->showNormal();

            // 精确计算还原后尺寸下的鼠标 TopLeft 偏移
            QRect normalGeom = m_window->geometry();
            int newX = globalPos.x() - static_cast<int>(normalGeom.width() * factorX);
            int newY = globalPos.y() - 15; // 居中挂载在 34px 标题栏中上部

            m_window->move(newX, newY);
            m_isDraggingMaximized = true;

            // 触发系统原生拖拽
            ReleaseCapture();
            SendMessageW(msg->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, msg->lParam);
            *result = 0;
            return true;
        }
    }

    if (msg->message == WM_NCHITTEST) {
        POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
        QPoint localPos = m_window->mapFromGlobal(QPoint(screenPt.x, screenPt.y));

        // 1. 在还原/正常尺寸下，优先处理四周 8px 原生非客户区缩放热区
        if (!m_window->isMaximized() && !m_window->isFullScreen()) {
            int x = localPos.x();
            int y = localPos.y();
            int w = m_window->width();
            int h = m_window->height();
            const int margin = kBaseResizeMargin; // 8px

            bool left   = (x <= margin);
            bool right  = (x >= w - margin);
            bool top    = (y <= margin);
            bool bottom = (y >= h - margin);

            if (top && left)     { *result = HTTOPLEFT;     return true; }
            if (top && right)    { *result = HTTOPRIGHT;    return true; }
            if (bottom && left)  { *result = HTBOTTOMLEFT;  return true; }
            if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
            if (left)            { *result = HTLEFT;        return true; }
            if (right)           { *result = HTRIGHT;       return true; }
            if (top)             { *result = HTTOP;         return true; }
            if (bottom)          { *result = HTBOTTOM;      return true; }
        }

        // 2. 允许全窗口任意非交互空白区域（标题栏及各面板底板空白处）响应拖拽与还原
        if (m_window && !m_window->isFullScreen()) {
            QWidget* childAtPt = m_window->childAt(localPos);
            if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                *result = HTCAPTION;
                return true;
            }
        }

        return false;
    }

    // 3. 原生双击标题栏最大化 / 还原
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (msg->wParam == HTCAPTION) {
            if (m_window->isMaximized()) {
                m_window->showNormal();
            } else {
                m_window->showMaximized();
            }
            *result = 0;
            return true;
        }
    }
#else
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return false;
}

void FramelessWindowHelper::setAlwaysOnTop(QWidget* window, bool onTop) {
    if (!window) return;

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    SetWindowPos(hwnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
#else
    Qt::WindowFlags flags = window->windowFlags();
    if (onTop) flags |= Qt::WindowStaysOnTopHint;
    else flags &= ~Qt::WindowStaysOnTopHint;
    window->setWindowFlags(flags);
    window->show();
#endif
}

bool FramelessWindowHelper::isAlwaysOnTop(QWidget* window) {
    if (!window) return false;

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    return (exStyle & WS_EX_TOPMOST) != 0;
#else
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
#endif
}

bool FramelessWindowHelper::eventFilter(QObject* obj, QEvent* event) {
    return QObject::eventFilter(obj, event);
}

} // namespace QuarkMeta