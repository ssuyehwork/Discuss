#include "ScreenshotShapes.h"
#include <QPainterPath>
#include <QPainterPathStroker>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int BaseShape::getHandleAt(const QPoint& pos) const {
    auto handles = getHandles();
    for (int i = 0; i < handles.size(); ++i) {
        if (handles[i].contains(pos)) return i;
    }
    return -1;
}

void BaseShape::moveBy(const QPoint& delta) {
    for (auto& p : data.points) p += delta;
}

static bool isNearLine(const QPointF& p, const QPointF& s, const QPointF& e, int threshold) {
    double l2 = QPointF::dotProduct(e - s, e - s);
    if (l2 == 0.0) return (p - s).manhattanLength() < threshold;
    double t = std::max(0.0, std::min(1.0, QPointF::dotProduct(p - s, e - s) / l2));
    QPointF projection = s + t * (e - s);
    return (p - projection).manhattanLength() < threshold;
}

void RectShape::draw(QPainter& p, const QPixmap&) const {
    if (data.points.size() < 2) return;
    p.setPen(QPen(data.color, data.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(data.isFilled ? QBrush(data.color) : Qt::NoBrush);
    p.drawRect(QRectF(data.points[0], data.points[1]).normalized());
}

bool RectShape::hitTest(const QPoint& pos) const {
    if (data.points.size() < 2) return false;
    QRect r = QRectF(data.points[0], data.points[1]).normalized().toRect();
    if (data.isFilled && r.contains(pos)) return true;
    int t = std::max(5, data.strokeWidth + 2);
    return std::abs(pos.x() - r.left()) < t || std::abs(pos.x() - r.right()) < t ||
           std::abs(pos.y() - r.top()) < t || std::abs(pos.y() - r.bottom()) < t;
}

QList<QRect> RectShape::getHandles() const {
    if (data.points.size() < 2) return {};
    QList<QRect> h; int s = 10;
    QRect r = QRectF(data.points[0], data.points[1]).normalized().toRect();
    h << QRect(r.left()-s/2, r.top()-s/2, s, s) << QRect(r.right()-s/2, r.top()-s/2, s, s)
      << QRect(r.right()-s/2, r.bottom()-s/2, s, s) << QRect(r.left()-s/2, r.bottom()-s/2, s, s);
    return h;
}

void RectShape::updatePoint(int index, const QPoint& pos) {
    if (data.points.size() < 2) return;
    QRect r = QRectF(data.points[0], data.points[1]).normalized().toRect();
    if (index == 0) { r.setTopLeft(pos); }
    else if (index == 1) { r.setTopRight(pos); }
    else if (index == 2) { r.setBottomRight(pos); }
    else if (index == 3) { r.setBottomLeft(pos); }
    data.points[0] = r.topLeft(); data.points[1] = r.bottomRight();
}

void EllipseShape::draw(QPainter& p, const QPixmap&) const {
    if (data.points.size() < 2) return;
    p.setPen(QPen(data.color, data.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(data.isFilled ? QBrush(data.color) : Qt::NoBrush);
    p.drawEllipse(QRectF(data.points[0], data.points[1]).normalized());
}

bool EllipseShape::hitTest(const QPoint& pos) const {
    if (data.points.size() < 2) return false;
    QRect r = QRectF(data.points[0], data.points[1]).normalized().toRect();
    QPainterPath path; path.addEllipse(r);
    if (data.isFilled && path.contains(pos)) return true;
    QPainterPathStroker stroker; stroker.setWidth(std::max(10, data.strokeWidth + 4));
    return stroker.createStroke(path).contains(pos);
}

QList<QRect> EllipseShape::getHandles() const {
    if (data.points.size() < 2) return {};
    QList<QRect> h; int s = 10;
    QRect r = QRectF(data.points[0], data.points[1]).normalized().toRect();
    h << QRect(r.left()-s/2, r.top()-s/2, s, s) << QRect(r.right()-s/2, r.top()-s/2, s, s)
      << QRect(r.right()-s/2, r.bottom()-s/2, s, s) << QRect(r.left()-s/2, r.bottom()-s/2, s, s);
    return h;
}

void EllipseShape::updatePoint(int index, const QPoint& pos) {
    if (data.points.size() < 2) return;
    QRect r = QRectF(data.points[0], data.points[1]).normalized().toRect();
    if (index == 0) { r.setTopLeft(pos); }
    else if (index == 1) { r.setTopRight(pos); }
    else if (index == 2) { r.setBottomRight(pos); }
    else if (index == 3) { r.setBottomLeft(pos); }
    data.points[0] = r.topLeft(); data.points[1] = r.bottomRight();
}

void LineShape::draw(QPainter& p, const QPixmap&) const {
    if (data.points.size() < 2) return;
    p.setPen(QPen(data.color, data.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(data.points[0], data.points[1]);
}

bool LineShape::hitTest(const QPoint& pos) const {
    if (data.points.size() < 2) return false;
    return isNearLine(pos, data.points[0], data.points[1], std::max(8, data.strokeWidth + 3));
}

QList<QRect> LineShape::getHandles() const {
    if (data.points.size() < 2) return {};
    QList<QRect> h; int s = 10;
    h << QRect(data.points[0].toPoint().x()-s/2, data.points[0].toPoint().y()-s/2, s, s)
      << QRect(data.points[1].toPoint().x()-s/2, data.points[1].toPoint().y()-s/2, s, s);
    return h;
}

void ArrowShape::draw(QPainter& p, const QPixmap&) const {
    if (data.points.size() < 2) return;
    QPointF start = data.points[0], end = data.points[1];
    QPointF dir = end - start;
    double len = std::sqrt(QPointF::dotProduct(dir, dir));
    if (len < 4) return;
    QPointF unit = dir / len;
    QPointF perp(-unit.y(), unit.x());
    double angle = std::atan2(dir.y(), dir.x());

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    // [高保真几何矢量建模] 严格对标专业截图标注规范 (PixPin/Snipaste 标准流线型设计)
    // 基础箭杆半宽: 根据 strokeWidth 动态平滑映射，杜绝随放大过度粗暴膨胀
    double shaftHalfWidth = std::max(1.5, data.strokeWidth * 1.0);
    // 箭头头部长度与翼展: 保证任何比例下均呈现黄金几何三角
    double headLen = std::max(16.0, std::min(len * 0.45, 18.0 + data.strokeWidth * 3.2));
    double headHalfWidth = headLen * 0.65;
    // 箭翼凹陷深度 (Waist Indent): 现代专业标注工具标志性的回凹比例 (约为头部长度的 25%)
    double waistBack = headLen * 0.28;

    bool isOutline = (data.arrowStyle == ArrowStyle::OutlineSingle || data.arrowStyle == ArrowStyle::OutlineDouble || data.arrowStyle == ArrowStyle::OutlineDot);

    if (data.arrowStyle == ArrowStyle::SolidSingle || data.arrowStyle == ArrowStyle::OutlineSingle) {
        QPointF ptTip = end;
        QPointF ptBarbLeft = end - unit * headLen + perp * headHalfWidth;
        QPointF ptBarbRight = end - unit * headLen - perp * headHalfWidth;
        QPointF ptWaistLeft = end - unit * (headLen - waistBack) + perp * shaftHalfWidth;
        QPointF ptWaistRight = end - unit * (headLen - waistBack) - perp * shaftHalfWidth;
        QPointF ptBaseLeft = start + perp * shaftHalfWidth;
        QPointF ptBaseRight = start - perp * shaftHalfWidth;

        QPolygonF arrowPoly;
        arrowPoly << ptTip
                  << ptBarbLeft
                  << ptWaistLeft
                  << ptBaseLeft
                  << ptBaseRight
                  << ptWaistRight
                  << ptBarbRight;

        if (isOutline) {
            p.setPen(QPen(data.color, std::max(1.5, data.strokeWidth * 0.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::transparent);
            p.drawPolygon(arrowPoly);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(data.color);
            p.drawPolygon(arrowPoly);
        }
    } else if (data.arrowStyle == ArrowStyle::Thin) {
        double penW = std::max(2.0, data.strokeWidth * 1.2);
        p.setPen(QPen(data.color, penW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(start, end);
        double thinAngle = 0.48;
        double thinLen = headLen * 0.9;
        p.drawLine(end, end - QPointF(thinLen * std::cos(angle - thinAngle), thinLen * std::sin(angle - thinAngle)));
        p.drawLine(end, end - QPointF(thinLen * std::cos(angle + thinAngle), thinLen * std::sin(angle + thinAngle)));
    } else if (data.arrowStyle == ArrowStyle::SolidDouble || data.arrowStyle == ArrowStyle::OutlineDouble) {
        p.setPen(QPen(data.color, shaftHalfWidth * 2.0, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        p.drawLine(start + unit * (headLen * 0.7), end - unit * (headLen * 0.7));

        auto drawHeadAt = [&](const QPointF& tip, double ang) {
            QPointF u(std::cos(ang), std::sin(ang));
            QPointF pr(-u.y(), u.x());
            QPolygonF poly;
            poly << tip
                 << tip - u * headLen + pr * headHalfWidth
                 << tip - u * (headLen - waistBack) + pr * shaftHalfWidth
                 << tip - u * (headLen - waistBack) - pr * shaftHalfWidth
                 << tip - u * headLen - pr * headHalfWidth;
            if (isOutline) {
                p.setPen(QPen(data.color, std::max(1.5, data.strokeWidth * 0.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                p.setBrush(Qt::transparent);
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(data.color);
            }
            p.drawPolygon(poly);
        };
        drawHeadAt(end, angle);
        drawHeadAt(start, angle + M_PI);
    } else if (data.arrowStyle == ArrowStyle::SolidDot || data.arrowStyle == ArrowStyle::OutlineDot) {
        double dotRadius = std::max(4.0, 3.5 + data.strokeWidth * 0.8);
        p.setPen(QPen(data.color, shaftHalfWidth * 2.0, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        p.drawLine(start, end - unit * (headLen * 0.7));

        if (isOutline) {
            p.setPen(QPen(data.color, std::max(1.5, data.strokeWidth * 0.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::transparent);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(data.color);
        }
        p.drawEllipse(start, dotRadius, dotRadius);

        QPolygonF headPoly;
        headPoly << end
                 << end - unit * headLen + perp * headHalfWidth
                 << end - unit * (headLen - waistBack) + perp * shaftHalfWidth
                 << end - unit * (headLen - waistBack) - perp * shaftHalfWidth
                 << end - unit * headLen - perp * headHalfWidth;
        p.drawPolygon(headPoly);
    } else if (data.arrowStyle == ArrowStyle::Dimension) {
        double penW = std::max(1.5, data.strokeWidth * 0.8);
        p.setPen(QPen(data.color, penW, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        p.drawLine(start, end);
        double capLen = 8.0 + data.strokeWidth * 1.0;
        p.drawLine(start + perp * capLen, start - perp * capLen);
        p.drawLine(end + perp * capLen, end - perp * capLen);
    }
    p.restore();
}

bool ArrowShape::hitTest(const QPoint& pos) const {
    if (data.points.size() < 2) return false;
    return isNearLine(pos, data.points[0], data.points[1], std::max(15, data.strokeWidth + 8));
}

QList<QRect> ArrowShape::getHandles() const {
    if (data.points.size() < 2) return {};
    QList<QRect> h; int s = 10;
    h << QRect(data.points[0].toPoint().x()-s/2, data.points[0].toPoint().y()-s/2, s, s)
      << QRect(data.points[1].toPoint().x()-s/2, data.points[1].toPoint().y()-s/2, s, s);
    return h;
}

void PenShape::draw(QPainter& p, const QPixmap&) const {
    if (data.points.size() < 2) return;
    p.setPen(QPen(data.color, data.strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPainterPath path; path.moveTo(data.points[0]);
    for(int i=1; i<data.points.size(); ++i) path.lineTo(data.points[i]);
    p.drawPath(path);
}

bool PenShape::hitTest(const QPoint& pos) const {
    for (int i = 0; i < data.points.size() - 1; ++i) {
        if (isNearLine(pos, data.points[i], data.points[i+1], std::max(10, data.strokeWidth + 4))) return true;
    }
    return false;
}

void MarkerShape::draw(QPainter& p, const QPixmap&) const {
    if (data.points.isEmpty()) return;
    p.setBrush(data.color); p.setPen(Qt::NoPen); int r = 12 + data.strokeWidth;
    p.drawEllipse(data.points[0], r, r); p.setPen(Qt::white); p.setFont(QFont("Arial", r, QFont::Bold));
    p.drawText(QRectF(data.points[0].x()-r, data.points[0].y()-r, r*2, r*2), Qt::AlignCenter, data.text);
}

bool MarkerShape::hitTest(const QPoint& pos) const {
    if (data.points.isEmpty()) return false;
    int r = 12 + data.strokeWidth;
    return (pos - data.points[0].toPoint()).manhattanLength() < r + 5;
}

void TextShape::draw(QPainter& p, const QPixmap&) const {
    if (data.points.isEmpty() || data.text.isEmpty()) return;
    p.setPen(data.color);
    QFont font(data.fontFamily, data.fontSize);
    font.setBold(data.isBold);
    font.setItalic(data.isItalic);
    p.setFont(font);
    p.drawText(data.points[0], data.text);
}

bool TextShape::hitTest(const QPoint& pos) const {
    if (data.points.isEmpty() || data.text.isEmpty()) return false;
    QFont font(data.fontFamily, data.fontSize);
    font.setBold(data.isBold); font.setItalic(data.isItalic);
    QRect r = QFontMetrics(font).boundingRect(data.text);
    r.moveTo(data.points[0].toPoint().x(), data.points[0].toPoint().y() - r.height());
    return r.adjusted(-10, -10, 10, 10).contains(pos);
}

QList<QRect> TextShape::getHandles() const {
    if (data.points.isEmpty() || data.text.isEmpty()) return {};
    QList<QRect> h; int s = 10;
    // 为文字左上角提供一个移动句柄
    h << QRect(data.points[0].toPoint().x()-s/2, data.points[0].toPoint().y()-s/2, s, s);
    return h;
}

void MosaicShape::draw(QPainter& p, const QPixmap& mosaicPixmap) const {
    if (data.points.size() < 2) return;
    p.save();
    QPainterPath path; path.moveTo(data.points[0]);
    for(int i=1; i<data.points.size(); ++i) path.lineTo(data.points[i]);
    QPainterPathStroker s; s.setWidth(data.strokeWidth * 6);
    p.setClipPath(s.createStroke(path));
    p.drawPixmap(0, 0, mosaicPixmap);
    p.restore();
}

bool MosaicShape::hitTest(const QPoint& pos) const {
    for (int i = 0; i < data.points.size() - 1; ++i) {
        if (isNearLine(pos, data.points[i], data.points[i+1], data.strokeWidth * 3)) return true;
    }
    return false;
}

void MosaicRectShape::draw(QPainter& p, const QPixmap& mosaicPixmap) const {
    if (data.points.size() < 2) return;
    p.save();
    p.setClipRect(QRectF(data.points[0], data.points[1]).normalized());
    p.drawPixmap(0, 0, mosaicPixmap);
    p.restore();
}

bool MosaicRectShape::hitTest(const QPoint& pos) const {
    if (data.points.size() < 2) return false;
    return QRectF(data.points[0], data.points[1]).normalized().contains(pos);
}

QList<QRect> MosaicRectShape::getHandles() const {
    if (data.points.size() < 2) return {};
    QList<QRect> h; int s = 10;
    QRect r = QRectF(data.points[0], data.points[1]).normalized().toRect();
    h << QRect(r.left()-s/2, r.top()-s/2, s, s) << QRect(r.right()-s/2, r.top()-s/2, s, s)
      << QRect(r.right()-s/2, r.bottom()-s/2, s, s) << QRect(r.left()-s/2, r.bottom()-s/2, s, s);
    return h;
}

void MosaicRectShape::updatePoint(int index, const QPoint& pos) {
    if (data.points.size() < 2) return;
    QRect r = QRectF(data.points[0], data.points[1]).normalized().toRect();
    if (index == 0) { r.setTopLeft(pos); }
    else if (index == 1) { r.setTopRight(pos); }
    else if (index == 2) { r.setBottomRight(pos); }
    else if (index == 3) { r.setBottomLeft(pos); }
    data.points[0] = r.topLeft(); data.points[1] = r.bottomRight();
}

BaseShape* createShape(const DrawingAnnotation& ann) {
    switch(ann.type) {
        case ScreenshotToolType::Rect: return new RectShape(ann);
        case ScreenshotToolType::Ellipse: return new EllipseShape(ann);
        case ScreenshotToolType::Line: return new LineShape(ann);
        case ScreenshotToolType::Arrow: return new ArrowShape(ann);
        case ScreenshotToolType::Pen: return new PenShape(ann);
        case ScreenshotToolType::Marker: return new MarkerShape(ann);
        case ScreenshotToolType::Text: return new TextShape(ann);
        case ScreenshotToolType::Mosaic: return new MosaicShape(ann);
        case ScreenshotToolType::MosaicRect: return new MosaicRectShape(ann);
        default: return nullptr;
    }
}
