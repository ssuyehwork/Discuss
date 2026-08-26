#include "ResizeEventFilter.h"
#include <QMouseEvent>
#include <QScreen>
#include <QWindow>
#include <QApplication>

namespace QuarkMeta {

ResizeEventFilter::ResizeEventFilter(QMainWindow* window) 
    : QObject(window), m_window(window) {}

bool ResizeEventFilter::eventFilter(QObject* watched, QEvent* event) {
    if (!m_window || m_window->isMaximized()) return QObject::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            QPoint localPos = m_window->mapFromGlobal(me->globalPosition().toPoint());
            ResizeDirection dir = getResizeDirection(localPos);
            if (dir != None) {
                m_isResizing = true;
                m_resizeDir = dir;
                m_resizeStartGlobal = me->globalPosition().toPoint();
                m_resizeStartGeometry = m_window->geometry();
                return true; // 拦截按键，执行真实拉伸
            }
        }
    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (m_isResizing && (me->buttons() & Qt::LeftButton)) {
            const QPoint delta = me->globalPosition().toPoint() - m_resizeStartGlobal;
            QRect r = m_resizeStartGeometry;

            if (m_resizeDir == Left || m_resizeDir == TopLeft || m_resizeDir == BottomLeft)
                r.setLeft(r.left() + delta.x());
            if (m_resizeDir == Right || m_resizeDir == TopRight || m_resizeDir == BottomRight)
                r.setRight(r.right() + delta.x());
            if (m_resizeDir == Top || m_resizeDir == TopLeft || m_resizeDir == TopRight)
                r.setTop(r.top() + delta.y());
            if (m_resizeDir == Bottom || m_resizeDir == BottomLeft || m_resizeDir == BottomRight)
                r.setBottom(r.bottom() + delta.y());

            if (r.width() >= m_window->minimumWidth() && r.height() >= m_window->minimumHeight())
                m_window->setGeometry(r);

            return true; // 拦截 Move，推进拉伸
        } else if (!m_isResizing) {
            QPoint localPos = m_window->mapFromGlobal(me->globalPosition().toPoint());
            ResizeDirection dir = getResizeDirection(localPos);
            updateCursorShape(dir);
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        if (m_isResizing) {
            m_isResizing = false;
            m_resizeDir = None;
            m_window->setCursor(Qt::ArrowCursor);
            return true;
        }
    } else if (event->type() == QEvent::Leave && watched == m_window && !m_isResizing) {
        m_window->setCursor(Qt::ArrowCursor);
    }

    return QObject::eventFilter(watched, event);
}

ResizeEventFilter::ResizeDirection ResizeEventFilter::getResizeDirection(const QPoint& pos) const {
    // 2026-05-08 按照用户要求：根据 DPI 动态计算感应宽度
    int margin = 6;
    if (m_window->windowHandle()) {
        margin = qRound(m_window->screen()->logicalDotsPerInch() / 96.0 * 6.0);
    }
    
    const int w = m_window->width(), h = m_window->height();
    bool left   = pos.x() < margin;
    bool right  = pos.x() > w - margin;
    bool top    = pos.y() < margin;
    bool bottom = pos.y() > h - margin;

    if (top    && left)  return TopLeft;
    if (top    && right) return TopRight;
    if (bottom && left)  return BottomLeft;
    if (bottom && right) return BottomRight;
    if (left)   return Left;
    if (right)  return Right;
    if (top)    return Top;
    if (bottom) return Bottom;
    return None;
}

void ResizeEventFilter::updateCursorShape(ResizeDirection dir) {
    switch (dir) {
        case Left:        case Right:       m_window->setCursor(Qt::SizeHorCursor);  break;
        case Top:         case Bottom:      m_window->setCursor(Qt::SizeVerCursor);  break;
        case TopLeft:     case BottomRight: m_window->setCursor(Qt::SizeFDiagCursor); break;
        case TopRight:    case BottomLeft:  m_window->setCursor(Qt::SizeBDiagCursor); break;
        default:                            m_window->setCursor(Qt::ArrowCursor);    break;
    }
}

} // namespace QuarkMeta
