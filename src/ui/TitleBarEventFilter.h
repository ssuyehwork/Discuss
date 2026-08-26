#pragma once

#include <QObject>
#include <QEvent>
#include <QMainWindow>
#include <QPoint>

namespace QuarkMeta {

/**
 * @brief 标题栏专属解耦事件过滤器
 * 专门承载标题栏（TitleBar）的双击最大化/还原与拖拽跟随逻辑
 */
class TitleBarEventFilter : public QObject {
    Q_OBJECT

public:
    explicit TitleBarEventFilter(QMainWindow* window, QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QMainWindow* m_window = nullptr;
    bool m_isDragging = false;
    QPoint m_dragPosition;
};

} // namespace QuarkMeta
