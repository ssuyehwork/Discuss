#include "ElasticEdit.h"
#include <QTextDocument>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QtMath>
#include <QLayout>
#include <QTimer>

namespace QuarkMeta {

ElasticEdit::ElasticEdit(QWidget* parent) : QTextEdit(parent) {
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QTextEdit::WidgetWidth);
    QTextOption opt = document()->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document()->setDefaultTextOption(opt);
    document()->setDocumentMargin(2); // 留出 2px 文档内边距，保证字不贴顶且不切字
    connect(this, &QTextEdit::textChanged, this, &ElasticEdit::adjustHeight);
}

void ElasticEdit::adjustHeight() {
    if (m_isAdjusting) return;

    int vpWidth = viewport() ? viewport()->width() : 0;
    qreal docMargin = document()->documentMargin();
    int textW = 0;

    if (vpWidth > 0) {
        textW = qMax(10, vpWidth - static_cast<int>(docMargin * 2));
    } else {
        QMargins m = contentsMargins();
        int fw = frameWidth();
        textW = qMax(10, width() - m.left() - m.right() - 2 * fw - static_cast<int>(docMargin * 2));
    }

    if (document()->textWidth() != textW) {
        document()->setTextWidth(textW);
    }

    int verticalDiff = height() - (viewport() ? viewport()->height() : 0);
    if (verticalDiff <= 0) {
        QMargins m = contentsMargins();
        int fw = frameWidth();
        verticalDiff = m.top() + m.bottom() + 2 * fw;
    }

    qreal docHeight = document()->size().height();
    int newHeight = qMax(28, static_cast<int>(qCeil(docHeight + verticalDiff)));

    if (this->height() != newHeight) {
        m_isAdjusting = true;
        setFixedHeight(newHeight);
        updateGeometry(); 
        QWidget* p = parentWidget();
        while (p) {
            if (p->layout()) p->layout()->activate();
            if (qobject_cast<QScrollArea*>(p)) break;
            p = p->parentWidget();
        }
        m_isAdjusting = false;

        QTimer::singleShot(0, this, &ElasticEdit::adjustHeight);
    }
}

void ElasticEdit::resizeEvent(QResizeEvent* e) {
    QTextEdit::resizeEvent(e);
    adjustHeight();
}

void ElasticEdit::keyPressEvent(QKeyEvent* e) {
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !(e->modifiers() & Qt::ShiftModifier)) {
        emit returnPressed();
        clearFocus();
        return;
    }
    QTextEdit::keyPressEvent(e);
}

} // namespace QuarkMeta