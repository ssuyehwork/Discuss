#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include <QMouseEvent>
#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <QCoreApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>
#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

void FramelessWindowHelper::apply(QWidget* window, QWidget* titleBar) {
    if (!window) return;
    new FramelessWindowHelper(window, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar) {
    
    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint;
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }
    
    // 安装至全局应用事件总线，穿透所有子控件的物理遮蔽
    QCoreApplication::instance()->installEventFilter(this);
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

FramelessWindowHelper::ResizeDirection FramelessWindowHelper::calculateResizeDirection(const QPoint& pos) const {
    if (!m_window || m_window->isMaximized() || m_window->isFullScreen()) return None;

    int margin = kBaseResizeMargin;
    if (m_window->windowHandle() && m_window->windowHandle()->screen()) {
        margin = qRound(m_window->windowHandle()->screen()->logicalDotsPerInch() / 96.0 * kBaseResizeMargin);
    }

    const int w = m_window->width();
    const int h = m_window->height();

    bool left   = pos.x() >= 0 && pos.x() <= margin;
    bool right  = pos.x() >= (w - margin) && pos.x() <= w;
    bool top    = pos.y() >= 0 && pos.y() <= margin;
    bool bottom = pos.y() >= (h - margin) && pos.y() <= h;

    if (top && left)     return TopLeft;
    if (top && right)    return TopRight;
    if (bottom && left)  return BottomLeft;
    if (bottom && right) return BottomRight;
    if (left)            return Left;
    if (right)           return Right;
    if (top)             return Top;
    if (bottom)          return Bottom;

    return None;
}

void FramelessWindowHelper::updateCursorShape(ResizeDirection dir) {
    if (!m_window) return;

    switch (dir) {
        case Left:        case Right:       m_window->setCursor(Qt::SizeHorCursor);  break;
        case Top:         case Bottom:      m_window->setCursor(Qt::SizeVerCursor);  break;
        case TopLeft:     case BottomRight: m_window->setCursor(Qt::SizeFDiagCursor); break;
        case TopRight:    case BottomLeft:  m_window->setCursor(Qt::SizeBDiagCursor); break;
        default:                            m_window->setCursor(Qt::ArrowCursor);    break;
    }
}

bool FramelessWindowHelper::eventFilter(QObject* obj, QEvent* event) {
    if (!m_window || !m_window->isVisible()) return false;

    // 仅拦截属于当前窗口及其子控件树的事件
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget || (widget != m_window && !m_window->isAncestorOf(widget))) {
        return false;
    }

    QEvent::Type type = event->type();

    // 1. 边缘检测与光标切换（监听全窗口范围内所有子控件的 MouseMove / HoverMove）
    if (type == QEvent::MouseMove || type == QEvent::HoverMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalPos = mouseEvent->globalPosition().toPoint();
        QPoint localPos = m_window->mapFromGlobal(globalPos);

        // 如果正在拉伸中，锁定几何形状计算，防止依赖动态 mapFromGlobal 导致原点跳变闪烁
        if (m_isResizing) {
            const QPoint delta = globalPos - m_resizeStartGlobalPos;
            QRect r = m_resizeStartGeometry;

            int minW = m_window->minimumWidth();
            int minH = m_window->minimumHeight();

            if (m_resizeDir == Left || m_resizeDir == TopLeft || m_resizeDir == BottomLeft) {
                int newLeft = r.left() + delta.x();
                if (r.right() - newLeft + 1 < minW) newLeft = r.right() - minW + 1;
                r.setLeft(newLeft);
            }
            if (m_resizeDir == Right || m_resizeDir == TopRight || m_resizeDir == BottomRight) {
                int newRight = r.right() + delta.x();
                if (newRight - r.left() + 1 < minW) newRight = r.left() + minW - 1;
                r.setRight(newRight);
            }
            if (m_resizeDir == Top || m_resizeDir == TopLeft || m_resizeDir == TopRight) {
                int newTop = r.top() + delta.y();
                if (r.bottom() - newTop + 1 < minH) newTop = r.bottom() - minH + 1;
                r.setTop(newTop);
            }
            if (m_resizeDir == Bottom || m_resizeDir == BottomLeft || m_resizeDir == BottomRight) {
                int newBottom = r.bottom() + delta.y();
                if (newBottom - r.top() + 1 < minH) newBottom = r.top() + minH - 1;
                r.setBottom(newBottom);
            }

            m_window->setGeometry(r);
            return true;
        }

        // 仅在非拉伸非拖拽非最大化时动态更新光标形状
        if (!m_isDragging && !m_window->isMaximized()) {
            ResizeDirection dir = calculateResizeDirection(localPos);
            updateCursorShape(dir);
            
            // 如果处于边缘且按住了左键，直接启动拉伸
            if (dir != None && (mouseEvent->buttons() & Qt::LeftButton)) {
                m_isResizing = true;
                m_resizeDir = dir;
                m_resizeStartGlobalPos = globalPos;
                m_resizeStartGeometry  = m_window->geometry();
                return true;
            }
        }
    }

    // 2. 鼠标按下：判定边缘拉伸启动
    if (type == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && !m_window->isMaximized()) {
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            QPoint localPos = m_window->mapFromGlobal(globalPos);
            ResizeDirection dir = calculateResizeDirection(localPos);

            if (dir != None) {
                m_isResizing = true;
                m_isDragging = false;
                m_resizeDir = dir;
                m_resizeStartGlobalPos = globalPos;
                m_resizeStartGeometry  = m_window->geometry();
                return true; // 消费事件，防止子控件响应点击
            }
        }
    }

    // 3. 鼠标释放：重置状态
    if (type == QEvent::MouseButtonRelease) {
        if (m_isResizing) {
            m_isResizing = false;
            m_resizeDir = None;
            updateCursorShape(None);
            return true;
        }
        if (m_isDragging) {
            m_isDragging = false;
            return true;
        }
    }

    // 4. 标题栏交互（双击最大化与按住拖动窗口）
    if (m_titleBar && (widget == m_titleBar || m_titleBar->isAncestorOf(widget))) {
        bool isInteractive = (
            qobject_cast<QPushButton*>(widget) ||
            qobject_cast<QToolButton*>(widget) ||
            qobject_cast<QLineEdit*>(widget) ||
            qobject_cast<QSlider*>(widget)
        );

        if (!isInteractive) {
            if (type == QEvent::MouseButtonDblClick) {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    if (m_window->isMaximized()) m_window->showNormal();
                    else m_window->showMaximized();
                    return true;
                }
            } else if (type == QEvent::MouseButtonPress) {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton && !m_isResizing) {
                    m_isDragging = true;
                    m_dragStartGlobalPos = mouseEvent->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
                    return true;
                }
            } else if (type == QEvent::MouseMove && m_isDragging) {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->buttons() & Qt::LeftButton) {
                    if (m_window->isMaximized()) {
                        const double ratio = static_cast<double>(mouseEvent->globalPosition().toPoint().x()) / m_window->width();
                        m_window->showNormal();
                        const int newX = mouseEvent->globalPosition().toPoint().x() - static_cast<int>(m_window->width() * ratio);
                        m_window->move(newX, mouseEvent->globalPosition().toPoint().y() - 10);
                        m_dragStartGlobalPos = mouseEvent->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
                    } else {
                        m_window->move(mouseEvent->globalPosition().toPoint() - m_dragStartGlobalPos);
                    }
                    return true;
                }
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

} // namespace QuarkMeta