#ifndef PINNEDSCREENSHOTWIDGET_H
#define PINNEDSCREENSHOTWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>

class PinnedScreenshotWidget : public QWidget {
    Q_OBJECT
public:
    explicit PinnedScreenshotWidget(const QPixmap& pixmap, const QRect& screenRect, QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
private:
    QPixmap m_pixmap;
    QPoint m_dragPos;
};

#endif // PINNEDSCREENSHOTWIDGET_H
