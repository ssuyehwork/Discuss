#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QObject>
#include <QWidget>
#include <QPoint>
#include <QRect>
#include <QPointer>

namespace QuarkMeta {

class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static void apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override = default;

    enum ResizeDirection {
        None = 0,
        Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };

    ResizeDirection calculateResizeDirection(const QPoint& localPos) const;
    void updateCursorShape(ResizeDirection dir);

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;

    bool m_isResizing = false;
    bool m_isDragging = false;
    ResizeDirection m_resizeDir = None;

    QPoint m_dragStartGlobalPos;
    QPoint m_resizeStartGlobalPos;
    QRect  m_resizeStartGeometry;

    static constexpr int kBaseResizeMargin = 6;
};

} // namespace QuarkMeta
