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

    // 2. 原生标题栏拖动识别：坚决杜绝抢占顶部 8px 缩放热区
    if (msg->message == WM_NCHITTEST) {
        POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
        QPoint localPos = m_window->mapFromGlobal(QPoint(screenPt.x, screenPt.y));

        if (!m_window->isMaximized() && !m_window->isFullScreen() && localPos.y() <= kBaseResizeMargin) {
            return false; // 顶部 8px 放行给 Qt
        }

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
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

bool FramelessWindowHelper::eventFilter(QObject* obj, QEvent* event) {
    if (!m_window || !m_window->isVisible()) return false;

    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget || (widget != m_window && !m_window->isAncestorOf(widget))) {
        return false;
    }

    QEvent::Type type = event->type();

    // 1. 鼠标按下：四周 8px 边缘按压时开启鼠标锁定 (grabMouse)
    if (type == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton && !m_window->isMaximized() && !m_window->isFullScreen()) {
            QPoint windowLocalPos = m_window->mapFromGlobal(me->globalPosition().toPoint());
            m_resizeDir = getResizeDirection(windowLocalPos);
            if (m_resizeDir != 0) {
                m_isResizing = true;
                m_resizeStartGlobalPos = me->globalPosition().toPoint();
                m_resizeStartGeometry = m_window->geometry();
                m_window->grabMouse(); // 强制锁定鼠标
                return true;
            }
        }
    } 
    // 2. 鼠标移动：拉伸处理与双向箭头光标自适应
    else if (type == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (m_isResizing) {
            QPoint delta = me->globalPosition().toPoint() - m_resizeStartGlobalPos;
            QRect newGeom = m_resizeStartGeometry;

            if (m_resizeDir & 1) newGeom.setLeft(m_resizeStartGeometry.left() + delta.x());
            if (m_resizeDir & 2) newGeom.setRight(m_resizeStartGeometry.right() + delta.x());
            if (m_resizeDir & 4) newGeom.setTop(m_resizeStartGeometry.top() + delta.y());
            if (m_resizeDir & 8) newGeom.setBottom(m_resizeStartGeometry.bottom() + delta.y());

            int minW = m_window->minimumWidth();
            int minH = m_window->minimumHeight();
            if (newGeom.width() < minW) {
                if (m_resizeDir & 1) newGeom.setLeft(newGeom.right() - minW + 1);
                else newGeom.setRight(newGeom.left() + minW - 1);
            }
            if (newGeom.height() < minH) {
                if (m_resizeDir & 4) newGeom.setTop(newGeom.bottom() - minH + 1);
                else newGeom.setBottom(newGeom.top() + minH - 1);
            }

            m_window->setGeometry(newGeom);
            return true;
        } else if (!m_window->isMaximized() && !m_window->isFullScreen()) {
            QPoint windowLocalPos = m_window->mapFromGlobal(me->globalPosition().toPoint());
            int dir = getResizeDirection(windowLocalPos);
            if (dir != 0) {
                updateCursorShape(dir);
            } else if (m_window->cursor().shape() != Qt::ArrowCursor) {
                m_window->unsetCursor();
            }
        }
    } 
    // 3. 鼠标释放：安全释放鼠标锁定
    else if (type == QEvent::MouseButtonRelease) {
        if (m_isResizing) {
            m_isResizing = false;
            m_resizeDir = 0;
            m_window->releaseMouse(); // 安全释放
            m_window->unsetCursor();
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

int FramelessWindowHelper::getResizeDirection(const QPoint& pos) const {
    if (!m_window) return 0;
    const int margin = kBaseResizeMargin;
    int dir = 0;
    if (pos.x() <= margin) dir |= 1;
    if (pos.x() >= m_window->width() - margin) dir |= 2;
    if (pos.y() <= margin) dir |= 4;
    if (pos.y() >= m_window->height() - margin) dir |= 8;
    return dir;
}

void FramelessWindowHelper::updateCursorShape(int dir) {
    if (!m_window) return;
    if (dir == (1 | 4) || dir == (2 | 8)) m_window->setCursor(Qt::SizeFDiagCursor);
    else if (dir == (2 | 4) || dir == (1 | 8)) m_window->setCursor(Qt::SizeBDiagCursor);
    else if (dir == 1 || dir == 2) m_window->setCursor(Qt::SizeHorCursor);
    else if (dir == 4 || dir == 8) m_window->setCursor(Qt::SizeVerCursor);
    else m_window->setCursor(Qt::ArrowCursor);
}

} // namespace QuarkMeta