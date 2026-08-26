#include "TitleBarEventFilter.h"
#include <QMouseEvent>

namespace QuarkMeta {

TitleBarEventFilter::TitleBarEventFilter(QMainWindow* window, QObject* parent)
    : QObject(parent ? parent : window), m_window(window) {
}

bool TitleBarEventFilter::eventFilter(QObject* watched, QEvent* event) {
    Q_UNUSED(watched);
    if (!m_window) return false;

    if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
        if (mouseEv->button() == Qt::LeftButton) {
            if (m_window->isMaximized()) {
                m_window->showNormal();
            } else {
                m_window->showMaximized();
            }
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
        if (mouseEv->button() == Qt::LeftButton) {
            m_isDragging = true;
            m_dragPosition = mouseEv->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
        }
    } else if (event->type() == QEvent::MouseMove && m_isDragging) {
        QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
        if (mouseEv->buttons() & Qt::LeftButton) {
            if (m_window->isMaximized()) {
                double widthRatio = (double)mouseEv->position().x() / m_window->width();
                m_window->showNormal();
                int normalW = m_window->width();
                int newX = mouseEv->globalPosition().toPoint().x() - static_cast<int>(normalW * widthRatio);
                int newY = mouseEv->globalPosition().toPoint().y() - 15;
                m_window->move(newX, newY);
                m_dragPosition = mouseEv->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
            } else {
                m_window->move(mouseEv->globalPosition().toPoint() - m_dragPosition);
            }
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_isDragging = false;
    }

    return false;
}

} // namespace QuarkMeta
