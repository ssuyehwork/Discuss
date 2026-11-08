#ifndef SCREENSHOTSHAPES_H
#define SCREENSHOTSHAPES_H

#include <QPointF>
#include <QColor>
#include <QString>
#include <QList>
#include <QRect>
#include <QPixmap>
#include <QPainter>
#include <memory>

enum class ScreenshotState { Selecting, Editing };
enum class ScreenshotToolType { None, Rect, Ellipse, Arrow, Line, Pen, Marker, Text, Mosaic, MosaicRect, Eraser, Picker };
enum class ArrowStyle { 
    SolidSingle, OutlineSingle, 
    SolidDouble, OutlineDouble, 
    SolidDot, OutlineDot,
    Thin,
    Dimension
};
enum class LineStyle { Solid, Dash, Dot };

struct DrawingAnnotation {
    ScreenshotToolType type = ScreenshotToolType::None;
    QList<QPointF> points;
    QColor color = Qt::red;
    QString text;
    int strokeWidth = 3;
    LineStyle lineStyle = LineStyle::Solid;
    ArrowStyle arrowStyle = ArrowStyle::SolidSingle;
    bool isFilled = false;
    // 文本相关属性
    QString fontFamily = "Microsoft YaHei";
    int fontSize = 12;
    bool isBold = false;
    bool isItalic = false;
};

class BaseShape {
public:
    explicit BaseShape(const DrawingAnnotation& ann) : data(ann) {}
    virtual ~BaseShape() = default;
    virtual void draw(QPainter& painter, const QPixmap& mosaicPixmap) const = 0;
    virtual bool hitTest(const QPoint& pos) const = 0;
    virtual QList<QRect> getHandles() const { return {}; }
    virtual int getHandleAt(const QPoint& pos) const;
    virtual void updatePoint(int index, const QPoint& pos) { if(index >= 0 && index < data.points.size()) data.points[index] = pos; }
    virtual void moveBy(const QPoint& delta);
    DrawingAnnotation data;
};

class RectShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
    QList<QRect> getHandles() const override;
    void updatePoint(int index, const QPoint& pos) override;
};

class EllipseShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
    QList<QRect> getHandles() const override;
    void updatePoint(int index, const QPoint& pos) override;
};

class LineShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
    QList<QRect> getHandles() const override;
};

class ArrowShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
    QList<QRect> getHandles() const override;
};

class PenShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
};

class MarkerShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
};

class TextShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
    QList<QRect> getHandles() const override;
};

class MosaicShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
};

class MosaicRectShape : public BaseShape {
public:
    using BaseShape::BaseShape;
    void draw(QPainter& painter, const QPixmap& mosaicPixmap) const override;
    bool hitTest(const QPoint& pos) const override;
    QList<QRect> getHandles() const override;
    void updatePoint(int index, const QPoint& pos) override;
};

BaseShape* createShape(const DrawingAnnotation& ann);

#endif // SCREENSHOTSHAPES_H
