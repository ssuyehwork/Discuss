#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>
#include <QAbstractButton>
#include <QComboBox>
#include <QSpinBox>

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
}

FramelessWindowHelper::~FramelessWindowHelper() = default;

bool FramelessWindowHelper::isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window) {
    QWidget* wWidget = child;
    while (wWidget && wWidget != titleBar && wWidget != window) {
        if (qobject_cast<QAbstractButton*>(wWidget) ||
            qobject_cast<QLineEdit*>(wWidget) ||
            qobject_cast<QSlider*>(wWidget) ||
            qobject_cast<QComboBox*>(wWidget) ||
            qobject_cast<QSpinBox*>(wWidget)) {
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

    if (msg->message == WM_NCHITTEST) {
        POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };

        if (!m_window->isMaximized() && !m_window->isFullScreen()) {
            LONG dpi = 96;
            HMODULE user32 = GetModuleHandleW(L"user32.dll");
            if (user32) {
                using GetDpiForWindowFunc = UINT(WINAPI*)(HWND);
                auto pGetDpiForWindow = reinterpret_cast<GetDpiForWindowFunc>(GetProcAddress(user32, "GetDpiForWindow"));
                if (pGetDpiForWindow) {
                    dpi = static_cast<LONG>(pGetDpiForWindow(msg->hwnd));
                }
            }
            const int margin = MulDiv(kBaseResizeMargin, dpi, 96);

            RECT wr;
            if (GetWindowRect(msg->hwnd, &wr)) {
                int x = screenPt.x - wr.left;
                int y = screenPt.y - wr.top;
                int w = wr.right - wr.left;
                int h = wr.bottom - wr.top;

                bool left   = x >= 0 && x < margin;
                bool right  = x >= w - margin && x < w;
                bool top    = y >= 0 && y < margin;
                bool bottom = y >= h - margin && y < h;

                if (top && left)     { *result = HTTOPLEFT;     return true; }
                if (top && right)    { *result = HTTOPRIGHT;    return true; }
                if (bottom && left)  { *result = HTBOTTOMLEFT;  return true; }
                if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
                if (left)            { *result = HTLEFT;        return true; }
                if (right)           { *result = HTRIGHT;       return true; }
                if (top)             { *result = HTTOP;         return true; }
                if (bottom)          { *result = HTBOTTOM;      return true; }
            }
        }

        if (m_titleBar) {
            QPoint localPt = m_window->mapFromGlobal(QPoint(screenPt.x, screenPt.y));
            QWidget* childAtPt = m_window->childAt(localPt);
            bool inTitleBar = m_titleBar->rect().contains(m_titleBar->mapFromGlobal(QPoint(screenPt.x, screenPt.y)));

            bool isInteractive = isInteractiveWidget(childAtPt, m_titleBar, m_window);

            if (inTitleBar && !isInteractive) {
                *result = HTCAPTION;
                return true;
            }
        }
    }

    if (msg->message == WM_SETCURSOR) {
        WORD hitTest = LOWORD(msg->lParam);
        HCURSOR hCursor = nullptr;

        switch (hitTest) {
        case HTLEFT:
        case HTRIGHT:
            hCursor = ::LoadCursor(NULL, IDC_SIZEWE);
            break;
        case HTTOP:
        case HTBOTTOM:
            hCursor = ::LoadCursor(NULL, IDC_SIZENS);
            break;
        case HTTOPLEFT:
        case HTBOTTOMRIGHT:
            hCursor = ::LoadCursor(NULL, IDC_SIZENWSE);
            break;
        case HTTOPRIGHT:
        case HTBOTTOMLEFT:
            hCursor = ::LoadCursor(NULL, IDC_SIZENESW);
            break;
        }

        if (hCursor) {
            ::SetCursor(hCursor);
            *result = TRUE;
            return true;
        }
    }

    if (msg->message == WM_NCLBUTTONDOWN) {
        WPARAM hitTest = msg->wParam;
        if (hitTest >= HTLEFT && hitTest <= HTBOTTOMRIGHT) {
            int dir = 0;
            switch (hitTest) {
            case HTLEFT:        dir = 1; break; // WMSZ_LEFT
            case HTRIGHT:       dir = 2; break; // WMSZ_RIGHT
            case HTTOP:         dir = 3; break; // WMSZ_TOP
            case HTTOPLEFT:     dir = 4; break; // WMSZ_TOPLEFT
            case HTTOPRIGHT:    dir = 5; break; // WMSZ_TOPRIGHT
            case HTBOTTOM:      dir = 6; break; // WMSZ_BOTTOM
            case HTBOTTOMLEFT:  dir = 7; break; // WMSZ_BOTTOMLEFT
            case HTBOTTOMRIGHT: dir = 8; break; // WMSZ_BOTTOMRIGHT
            }
            if (dir > 0) {
                ::ReleaseCapture();
                ::SendMessageW(msg->hwnd, WM_SYSCOMMAND, 0xF000 | dir, msg->lParam);
                *result = 0;
                return true;
            }
        }
    }

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
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

} // namespace QuarkMeta
