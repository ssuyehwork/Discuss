#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>
#include <QAbstractButton>
#include <QComboBox>
#include <QSpinBox>
#include <QScrollBar>
#include <QAbstractItemView>

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

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    // 关键修正：必须具备 WS_CAPTION、WS_THICKFRAME、系统菜单和最大最小化盒子，Windows 才会为其维护合法的恢复尺寸 (WINDOWPLACEMENT)
    SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#endif
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

    HWND hwnd = msg->hwnd;

    // 1. 无边框客户区撑满，消除 WS_CAPTION 带来的原生标题栏
    if (msg->message == WM_NCCALCSIZE) {
        if (msg->wParam == TRUE) {
            // 关键修正：必须使用原生 ::IsZoomed(hwnd)，坚决不能用 Qt 滞后的 m_window->isMaximized()！
            if (::IsZoomed(hwnd)) {
                NCCALCSIZE_PARAMS* pnc = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                if (monitor) {
                    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
                    if (GetMonitorInfo(monitor, &monitorInfo)) {
                        // 最大化时，内容区等于屏幕工作区，吃掉不可见拉伸边框
                        pnc->rgrc[0] = monitorInfo.rcWork;
                    }
                }
            }
            // 返回 0 表示客户区占据整个窗口范围，彻底消除原生标题栏与边框
            *result = 0;
            return true;
        }
        return false;
    }

    if (msg->message == WM_GETMINMAXINFO) {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        if (mmi) {
            HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            if (monitor) {
                MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
                if (GetMonitorInfo(monitor, &monitorInfo)) {
                    RECT rcWork = monitorInfo.rcWork;
                    RECT rcMonitor = monitorInfo.rcMonitor;
                    mmi->ptMaxPosition.x = rcWork.left - rcMonitor.left;
                    mmi->ptMaxPosition.y = rcWork.top - rcMonitor.top;
                    mmi->ptMaxSize.x = rcWork.right - rcWork.left;
                    mmi->ptMaxSize.y = rcWork.bottom - rcWork.top;
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

    // 2. 原生 WM_NCHITTEST 精确命中检测
    if (msg->message == WM_NCHITTEST) {
        POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
        QPoint localPos = m_window->mapFromGlobal(QPoint(screenPt.x, screenPt.y));

        int width = m_window->width();
        int height = m_window->height();

        // 关键修正：必须使用原生 IsZoomed 判断
        bool isMax = ::IsZoomed(hwnd) || m_window->isFullScreen();

        if (!isMax) {
            const int m = kBaseResizeMargin;
            bool left = localPos.x() >= 0 && localPos.x() < m;
            bool right = localPos.x() >= width - m && localPos.x() < width;
            bool top = localPos.y() >= 0 && localPos.y() < m;
            bool bottom = localPos.y() >= height - m && localPos.y() < height;

            if (top && left)     { *result = HTTOPLEFT;     return true; }
            if (top && right)    { *result = HTTOPRIGHT;    return true; }
            if (bottom && left)  { *result = HTBOTTOMLEFT;  return true; }
            if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
            if (left)            { *result = HTLEFT;        return true; }
            if (right)           { *result = HTRIGHT;       return true; }
            if (top)             { *result = HTTOP;         return true; }
            if (bottom)          { *result = HTBOTTOM;      return true; }
        }

        // 标题栏原生拖拽与双击识别（排除交互控件）
        if (m_titleBar && !m_window->isFullScreen()) {
            QRect titleRect = QRect(m_titleBar->mapTo(m_window, QPoint(0, 0)), m_titleBar->size());
            if (titleRect.contains(localPos)) {
                QWidget* childAtPt = m_window->childAt(localPos);
                if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
        }

        *result = HTCLIENT;
        return true;
    }

    // 3. 原生双击标题栏最大化 / 还原
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (msg->wParam == HTCAPTION) {
            // 关键修正：通过 WM_SYSCOMMAND 派发，走 Windows 原生状态机！
            ::SendMessage(hwnd, WM_SYSCOMMAND, ::IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
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
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

} // namespace QuarkMeta
