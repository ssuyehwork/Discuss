#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include <QMouseEvent>
#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>

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
    
    m_window->setWindowFlags(m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    m_window->setAttribute(Qt::WA_Hover, true);
    m_window->installEventFilter(this);

    if (m_titleBar) {
        m_titleBar->setAttribute(Qt::WA_Hover, true);
        m_titleBar->installEventFilter(this);
    }
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

    bool left   = pos.x() <= margin;
    bool right  = pos.x() >= w - margin;
    bool top    = pos.y() <= margin;
    bool bottom = pos.y() >= h - margin;

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
    if (!m_window) return false;

    if (obj == m_window) {
        switch (event->type()) {
            case QEvent::MouseMove: {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                const QPoint globalPos = mouseEvent->globalPosition().toPoint();

                if (m_isResizing) {
                    const QPoint delta = globalPos - m_resizeStartGlobalPos;
                    QRect r = m_resizeStartGeometry;

                    if (m_resizeDir == Left || m_resizeDir == TopLeft || m_resizeDir == BottomLeft)
                        r.setLeft(r.left() + delta.x());
                    if (m_resizeDir == Right || m_resizeDir == TopRight || m_resizeDir == BottomRight)
                        r.setRight(r.right() + delta.x());
                    if (m_resizeDir == Top || m_resizeDir == TopLeft || m_resizeDir == TopRight)
                        r.setTop(r.top() + delta.y());
                    if (m_resizeDir == Bottom || m_resizeDir == BottomLeft || m_resizeDir == BottomRight)
                        r.setBottom(r.bottom() + delta.y());

                    if (r.width() >= m_window->minimumWidth() && r.height() >= m_window->minimumHeight()) {
                        m_window->setGeometry(r);
                    }
                    return true;
                }

                if (!m_isDragging && !m_window->isMaximized()) {
                    const QPoint localPos = mouseEvent->position().toPoint();
                    ResizeDirection dir = calculateResizeDirection(localPos);
                    updateCursorShape(dir);
                }
                break;
            }

            case QEvent::MouseButtonPress: {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton && !m_window->isMaximized()) {
                    const QPoint localPos = mouseEvent->position().toPoint();
                    ResizeDirection dir = calculateResizeDirection(localPos);
                    if (dir != None) {
                        m_isResizing = true;
                        m_isDragging = false;
                        m_resizeDir = dir;
                        m_resizeStartGlobalPos = mouseEvent->globalPosition().toPoint();
                        m_resizeStartGeometry  = m_window->geometry();
                        return true;
                    }
                }
                break;
            }

            case QEvent::MouseButtonRelease: {
                if (m_isResizing) {
                    m_isResizing = false;
                    m_resizeDir = None;
                    updateCursorShape(None);
                    return true;
                }
                break;
            }

            case QEvent::Leave: {
                if (!m_isResizing && !m_isDragging) {
                    updateCursorShape(None);
                }
                break;
            }

            default:
                break;
        }
    }

    if (obj == m_titleBar || (m_titleBar && m_titleBar->isAncestorOf(qobject_cast<QWidget*>(obj)))) {
        QWidget* targetWidget = qobject_cast<QWidget*>(obj);

        bool isInteractiveChild = targetWidget && (
            qobject_cast<QPushButton*>(targetWidget) ||
            qobject_cast<QToolButton*>(targetWidget) ||
            qobject_cast<QLineEdit*>(targetWidget) ||
            qobject_cast<QSlider*>(targetWidget)
        );

        if (!isInteractiveChild) {
            switch (event->type()) {
                case QEvent::MouseButtonDblClick: {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (mouseEvent->button() == Qt::LeftButton) {
                        if (m_window->isMaximized()) {
                            m_window->showNormal();
                        } else {
                            m_window->showMaximized();
                        }
                        return true;
                    }
                    break;
                }

                case QEvent::MouseButtonPress: {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (mouseEvent->button() == Qt::LeftButton && !m_isResizing) {
                        m_isDragging = true;
                        m_dragStartGlobalPos = mouseEvent->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
                        return true;
                    }
                    break;
                }

                case QEvent::MouseMove: {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (m_isDragging && (mouseEvent->buttons() & Qt::LeftButton)) {
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
                    break;
                }

                case QEvent::MouseButtonRelease: {
                    m_isDragging = false;
                    break;
                }

                default:
                    break;
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

} // namespace QuarkMeta
