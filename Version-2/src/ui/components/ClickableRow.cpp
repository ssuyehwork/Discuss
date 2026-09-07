#include "components/ClickableRow.h"
#include "components/StyledCheckBox.h"
#include <QMouseEvent>

namespace QuarkMeta {

ClickableRow::ClickableRow(StyledCheckBox* cb, QWidget* parent)
    : QWidget(parent), m_cb(cb) {
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_StyledBackground);
}

void ClickableRow::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        QPoint local = m_cb->mapFromGlobal(e->globalPosition().toPoint());
        if (!m_cb->rect().contains(local)) {
            m_cb->setChecked(!m_cb->isChecked());
        }
    }
    QWidget::mousePressEvent(e);
}

void ClickableRow::enterEvent(QEnterEvent* e) {
    setStyleSheet("QWidget { background: #2A2A2A; border-radius: 4px; }");
    QWidget::enterEvent(e);
}

void ClickableRow::leaveEvent(QEvent* e) {
    setStyleSheet("");
    QWidget::leaveEvent(e);
}

} // namespace QuarkMeta
