#include "ElasticEdit.h"
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QtMath>
#include <QLayout>

namespace QuarkMeta {

ElasticEdit::ElasticEdit(QWidget* parent) : QTextEdit(parent) {
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QTextEdit::WidgetWidth);
    QTextOption opt = document()->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document()->setDefaultTextOption(opt);
    document()->setDocumentMargin(0);
    connect(this, &QTextEdit::textChanged, this, &ElasticEdit::adjustHeight);
}

void ElasticEdit::adjustHeight() {
    QMargins margins = contentsMargins();
    int hMargin = margins.left() + margins.right() + 12; // 精准匹配 QSS 左右 padding (6px * 2) + 边框
    int vMargin = margins.top() + margins.bottom() + 8;  // 精准匹配 QSS 上下 padding (4px * 2) + 边框

    int w = width();
    if (w > 30) {
        int textW = qMax(10, w - hMargin);
        if (qAbs(document()->textWidth() - textW) > 1.0) {
            document()->setTextWidth(textW);
        }
    }
    qreal docHeight = document()->documentLayout()->documentSize().height();
    int newHeight = qMax(24, (int)qCeil(docHeight + vMargin));
    if (this->height() != newHeight) {
        setFixedHeight(newHeight);
        updateGeometry(); 
        QWidget* p = parentWidget();
        while (p) {
            if (p->layout()) p->layout()->activate();
            if (qobject_cast<QScrollArea*>(p)) break;
            p = p->parentWidget();
        }
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
