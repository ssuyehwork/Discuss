#include "ElasticEdit.h"
#include <QTextDocument>
#include <QKeyEvent>
#include <QtMath>
#include <QEvent>

namespace QuarkMeta {

ElasticEdit::ElasticEdit(QWidget* parent) : QTextEdit(parent) {
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Minimum);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QTextEdit::WidgetWidth);
    QTextOption opt = document()->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document()->setDefaultTextOption(opt);
    document()->setDocumentMargin(2); // 留出 2px 文档内边距，保证字不贴顶且不切字
    connect(this, &QTextEdit::textChanged, this, &QWidget::updateGeometry);
}

bool ElasticEdit::hasHeightForWidth() const {
    return true;
}

int ElasticEdit::heightForWidth(int w) const {
    if (w <= 0) return 28;

    QMargins m = contentsMargins();
    int fw = frameWidth();
    qreal docMargin = document()->documentMargin();

    int availableTextW = w - m.left() - m.right() - 2 * fw - static_cast<int>(docMargin * 2);
    if (availableTextW <= 10) availableTextW = 10;

    QTextDocument doc;
    doc.setDefaultFont(font());
    doc.setDefaultTextOption(document()->defaultTextOption());
    doc.setDocumentMargin(docMargin);

    QString htmlText = toHtml();
    if (!htmlText.isEmpty()) {
        doc.setHtml(htmlText);
    } else {
        doc.setPlainText(toPlainText());
    }
    doc.setTextWidth(availableTextW);

    int vertMargin = m.top() + m.bottom() + 2 * fw;
    int calculatedH = static_cast<int>(qCeil(doc.size().height() + vertMargin));

    return qMax(28, calculatedH);
}

QSize ElasticEdit::sizeHint() const {
    int w = width() > 0 ? width() : 200;
    return QSize(w, heightForWidth(w));
}

QSize ElasticEdit::minimumSizeHint() const {
    int w = width() > 0 ? width() : 200;
    return QSize(w, heightForWidth(w));
}

void ElasticEdit::changeEvent(QEvent* e) {
    QTextEdit::changeEvent(e);
    if (e && (e->type() == QEvent::StyleChange || e->type() == QEvent::FontChange)) {
        updateGeometry();
    }
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