#include "InlineHueSlider.h"
#include <QPainter>
#include <QLinearGradient>
#include <cmath>

namespace QuarkMeta {

InlineHueSlider::InlineHueSlider(QWidget* parent) : QWidget(parent) {
    setFixedHeight(28); 
    setCursor(Qt::PointingHandCursor);
}

void InlineHueSlider::setHue(int h) {
    m_h = h;
    update();
}

void InlineHueSlider::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int margin = 10; 
    int bwgWidth = 42;
    int gap = 6;
    int barHeight = 12;
    int barY = (height() - barHeight) / 2;

    QRectF blackRect(margin, barY, 14, barHeight);
    QRectF grayRect(margin + 14, barY, 14, barHeight);
    QRectF whiteRect(margin + 28, barY, 14, barHeight);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRect(blackRect);
    painter.setBrush(QColor("#808080"));
    painter.drawRect(grayRect);
    painter.setBrush(Qt::white);
    painter.drawRect(whiteRect);

    int hueStartX = margin + bwgWidth + gap;
    int hueWidth = width() - hueStartX - margin;

    if (hueWidth > 0) {
        QRectF hueRect(hueStartX, barY, hueWidth, barHeight);
        QLinearGradient grad(hueRect.topLeft(), hueRect.topRight());
        for (int i = 0; i <= 360; i += 60) {
            grad.setColorAt(i / 360.0, QColor::fromHsv(i, 220, 220));
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(grad);
        painter.drawRoundedRect(hueRect, 2, 2);
    }

    int tx = 0;
    if (m_h == 1000) tx = static_cast<int>(blackRect.center().x());
    else if (m_h == 1001) tx = static_cast<int>(grayRect.center().x());
    else if (m_h == 1002) tx = static_cast<int>(whiteRect.center().x());
    else {
        double ratio = qBound(0, m_h, 359) / 359.0;
        tx = hueStartX + static_cast<int>(ratio * hueWidth);
    }
    
    painter.setBrush(Qt::white);
    painter.setPen(QPen(QColor(50, 50, 50), 1));
    painter.drawEllipse(QPoint(tx, height() / 2), 8, 8);
}

void InlineHueSlider::updateFromPos(int x) {
    int margin = 10;
    int bwgWidth = 42;
    int gap = 6;
    int hueStartX = margin + bwgWidth + gap;

    if (x < margin + 14) {
        m_h = 1000;
    } else if (x < margin + 28) {
        m_h = 1001;
    } else if (x < margin + 42) {
        m_h = 1002;
    } else {
        int hueWidth = width() - hueStartX - margin;
        if (hueWidth <= 0) return;
        int lx = qBound(0, x - hueStartX, hueWidth);
        m_h = (lx * 359) / hueWidth;
    }
    update();
    emit hueChanged(m_h);
}

void InlineHueSlider::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) updateFromPos(event->pos().x());
}

void InlineHueSlider::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) updateFromPos(event->pos().x());
}

void InlineHueSlider::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) emit sliderReleased();
}

} // namespace QuarkMeta
