#include "QuarkApplication.h"
#include <QEvent>
#include <QWidget>
#include <QWindow>
#include <QGuiApplication>
#include <QCursor>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

QuarkApplication::QuarkApplication(int& argc, char** argv)
    : QApplication(argc, argv) {}

void QuarkApplication::performGlobalSelfHealing(QWidget* widget) {
    while (QGuiApplication::overrideCursor()) {
        QGuiApplication::restoreOverrideCursor();
    }

    if (QWidget* grabber = QWidget::mouseGrabber()) {
        if (!widget || grabber == widget || widget->isAncestorOf(grabber)) {
            grabber->releaseMouse();
        }
    }

#ifdef Q_OS_WIN
    if (widget && widget->testAttribute(Qt::WA_WState_Created)) {
        HWND hwnd = reinterpret_cast<HWND>(widget->winId());
        if (GetCapture() == hwnd) {
            ::ReleaseCapture();
        }
    }
#endif

    if (widget) {
        widget->unsetCursor();
    }

    QPoint currentPos = QCursor::pos();
    QCursor::setPos(currentPos);
}

bool QuarkApplication::notify(QObject* receiver, QEvent* event) {
    if (event) {
        QEvent::Type type = event->type();
        if (type == QEvent::Close || type == QEvent::Hide) {
            QWidget* w = qobject_cast<QWidget*>(receiver);
            if (w && (w->isWindow() || w->inherits("QDialog"))) {
                performGlobalSelfHealing(w);
            }
        }
    }
    return QApplication::notify(receiver, event);
}

} // namespace QuarkMeta
