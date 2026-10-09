#include "ColorBlock.h"
#include "../ToolTipOverlay.h"
#include <QPainter>
#include <QCursor>

namespace QuarkMeta {

ColorBlock::ColorBlock(const QColor& color, QWidget* parent) 
    : QWidget(parent), m_color(color) {
    setFixedSize(16, 16);
    setCursor(Qt::PointingHandCursor);
}

void ColorBlock::setChecked(bool checked) {
    m_checked = checked;
    update();
}

void ColorBlock::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(m_color);
    painter.drawRoundedRect(rect(), 3, 3);

    if (m_checked || m_hovered) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::white, 1.5));
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 3, 3);
    }
}

void ColorBlock::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_color);
    }
}

void ColorBlock::enterEvent(QEnterEvent*) {
    m_hovered = true;
    update();
    QString tip = QString("颜色: %1\n匹配项: %2").arg(m_color.name().toUpper()).arg(m_count);
    ToolTipOverlay::instance()->showText(QCursor::pos(), tip, 0);
}

void ColorBlock::leaveEvent(QEvent*) {
    m_hovered = false;
    update();
    ToolTipOverlay::hideTip();
}

} // namespace QuarkMeta
