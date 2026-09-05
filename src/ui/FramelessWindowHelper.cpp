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
#include <QScrollBar>
#include <QAbstractItemView>
#include <QMouseEvent>
#include <QResizeEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

enum class EdgeDirection {
    Left = 1,
    Right = 2,
    Top = 4,
    Bottom = 8,
    TopLeft = 1 | 4,
    TopRight = 2 | 4,
    BottomLeft = 1 | 8,
    BottomRight = 2 | 8
};

/**
 * @brief 边缘物理手柄子控件：透明、原生物理捕获、自带对应光标
 */
class ResizeHandle : public QWidget {
public:
    ResizeHandle(EdgeDirection dir, QWidget* targetWindow, QWidget* parent)
        : QWidget(parent), m_dir(dir), m_targetWindow(targetWindow) {
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAutoFillBackground(false);

        switch (m_dir) {
            case EdgeDirection::Left:
            case EdgeDirection::Right:
                setCursor(Qt::SizeHorCursor);
                break;
            case EdgeDirection::Top:
            case EdgeDirection::Bottom:
                setCursor(Qt::SizeVerCursor);
                break;
            case EdgeDirection::TopLeft:
            case EdgeDirection::BottomRight:
                setCursor(Qt::SizeFDiagCursor);
                break;
            case EdgeDirection::TopRight:
            case EdgeDirection::BottomLeft:
                setCursor(Qt::SizeBDiagCursor);
                break;
        }
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_targetWindow && !m_targetWindow->isMaximized() && !m_targetWindow->isFullScreen()) {
            m_dragStartGlobalPos = event->globalPosition().toPoint();
            m_dragStartGeometry = m_targetWindow->geometry();
            grabMouse();
            event->accept();
        } else {
            QWidget::mousePressEvent(event);
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_targetWindow && (event->buttons() & Qt::LeftButton)) {
            QPoint delta = event->globalPosition().toPoint() - m_dragStartGlobalPos;
            QRect newGeom = m_dragStartGeometry;
            int dirMask = static_cast<int>(m_dir);

            if (dirMask & 1) newGeom.setLeft(m_dragStartGeometry.left() + delta.x());
            if (dirMask & 2) newGeom.setRight(m_dragStartGeometry.right() + delta.x());
            if (dirMask & 4) newGeom.setTop(m_dragStartGeometry.top() + delta.y());
            if (dirMask & 8) newGeom.setBottom(m_dragStartGeometry.bottom() + delta.y());

            int minW = m_targetWindow->minimumWidth();
            int minH = m_targetWindow->minimumHeight();
            if (newGeom.width() < minW) {
                if (dirMask & 1) newGeom.setLeft(newGeom.right() - minW + 1);
                else newGeom.setRight(newGeom.left() + minW - 1);
            }
            if (newGeom.height() < minH) {
                if (dirMask & 4) newGeom.setTop(newGeom.bottom() - minH + 1);
                else newGeom.setBottom(newGeom.top() + minH - 1);
            }

            m_targetWindow->setGeometry(newGeom);
            event->accept();
        } else {
            QWidget::mouseMoveEvent(event);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            releaseMouse();
            event->accept();
        } else {
            QWidget::mouseReleaseEvent(event);
        }
    }

private:
    EdgeDirection m_dir;
    QWidget* m_targetWindow = nullptr;
    QPoint m_dragStartGlobalPos;
    QRect m_dragStartGeometry;
};

FramelessWindowHelper* FramelessWindowHelper::apply(QWidget* window, QWidget* titleBar) {
    if (!window) return nullptr;
    return new FramelessWindowHelper(window, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar) {
    
    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint;
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }

    if (m_window) {
        // 彻底绝缘全局事件拦截，仅在宿主自身安装尺寸跟随监听
        m_window->installEventFilter(this);
        initHandles();
    }

    if (m_titleBar) {
        // 仅在标题栏自身安装局部拖动监听
        m_titleBar->installEventFilter(this);
    }
}

FramelessWindowHelper::~FramelessWindowHelper() = default;

void FramelessWindowHelper::initHandles() {
    if (!m_window) return;

    m_handles.append(new ResizeHandle(EdgeDirection::TopLeft, m_window, m_window));
    m_handles.append(new ResizeHandle(EdgeDirection::Top, m_window, m_window));
    m_handles.append(new ResizeHandle(EdgeDirection::TopRight, m_window, m_window));
    m_handles.append(new ResizeHandle(EdgeDirection::Left, m_window, m_window));
    m_handles.append(new ResizeHandle(EdgeDirection::Right, m_window, m_window));
    m_handles.append(new ResizeHandle(EdgeDirection::BottomLeft, m_window, m_window));
    m_handles.append(new ResizeHandle(EdgeDirection::Bottom, m_window, m_window));
    m_handles.append(new ResizeHandle(EdgeDirection::BottomRight, m_window, m_window));

    updateHandleGeometries();
}

void FramelessWindowHelper::updateHandleGeometries() {
    if (!m_window || m_handles.size() < 8) return;

    if (m_window->isMaximized() || m_window->isFullScreen()) {
        for (auto* h : m_handles) h->hide();
        return;
    }

    for (auto* h : m_handles) h->show();

    int w = m_window->width();
    int h = m_window->height();
    const int m = kHandleMargin;

    // 0: TopLeft, 1: Top, 2: TopRight
    m_handles[0]->setGeometry(0, 0, m, m);
    m_handles[1]->setGeometry(m, 0, w - 2 * m, m);
    m_handles[2]->setGeometry(w - m, 0, m, m);

    // 3: Left, 4: Right
    m_handles[3]->setGeometry(0, m, m, h - 2 * m);
    m_handles[4]->setGeometry(w - m, m, m, h - 2 * m);

    // 5: BottomLeft, 6: Bottom, 7: BottomRight
    m_handles[5]->setGeometry(0, h - m, m, m);
    m_handles[6]->setGeometry(m, h - m, w - 2 * m, m);
    m_handles[7]->setGeometry(w - m, h - m, m, m);

    for (auto* handle : m_handles) {
        handle->raise();
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

    // 仅保留 Windows 多显示器最大化时不遮挡系统任务栏的几何边界补偿
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
    if (!m_window) return false;

    // 1. 宿主窗口自身尺寸改变或状态变化：同步更新 8 向手柄物理几何贴合
    if (obj == m_window) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show || event->type() == QEvent::WindowStateChange) {
            updateHandleGeometries();
        } else if (!m_titleBar && event->type() == QEvent::MouseButtonPress) {
            // 无标题栏的浮层：在非交互区空白处点击支持拖动
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                QWidget* childAtPt = m_window->childAt(me->pos());
                if (!isInteractiveWidget(childAtPt, nullptr, m_window)) {
                    m_isTitleDragging = true;
                    m_dragStartGlobalPos = me->globalPosition().toPoint();
                    m_dragStartGeometry = m_window->geometry();
                    m_window->grabMouse();
                    return true;
                }
            }
        } else if (!m_titleBar && event->type() == QEvent::MouseMove) {
            if (m_isTitleDragging) {
                auto* me = static_cast<QMouseEvent*>(event);
                QPoint delta = me->globalPosition().toPoint() - m_dragStartGlobalPos;
                m_window->move(m_dragStartGeometry.topLeft() + delta);
                return true;
            }
        } else if (!m_titleBar && event->type() == QEvent::MouseButtonRelease) {
            if (m_isTitleDragging) {
                m_isTitleDragging = false;
                m_window->releaseMouse();
                return true;
            }
        }
    }

    // 2. 标题栏局部拖动与双击最大化
    if (obj == m_titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && !m_window->isMaximized() && !m_window->isFullScreen()) {
                QWidget* childAtPt = m_titleBar->childAt(me->pos());
                if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                    m_isTitleDragging = true;
                    m_dragStartGlobalPos = me->globalPosition().toPoint();
                    m_dragStartGeometry = m_window->geometry();
                    m_titleBar->grabMouse();
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (m_isTitleDragging) {
                auto* me = static_cast<QMouseEvent*>(event);
                QPoint delta = me->globalPosition().toPoint() - m_dragStartGlobalPos;
                m_window->move(m_dragStartGeometry.topLeft() + delta);
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_isTitleDragging) {
                m_isTitleDragging = false;
                m_titleBar->releaseMouse();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                QWidget* childAtPt = m_titleBar->childAt(me->pos());
                if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                    if (m_window->isMaximized()) m_window->showNormal();
                    else m_window->showMaximized();
                    return true;
                }
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

} // namespace QuarkMeta