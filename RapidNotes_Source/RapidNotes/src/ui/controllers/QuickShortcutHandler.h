#ifndef QUICKSHORTCUTHANDLER_H
#define QUICKSHORTCUTHANDLER_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QElapsedTimer>

class QuickWindow;

namespace RapidNotes {

class QuickShortcutHandler : public QObject {
    Q_OBJECT
public:
    explicit QuickShortcutHandler(QuickWindow* window, QObject* parent = nullptr);
    ~QuickShortcutHandler() override = default;

    // 核心统一事件过滤器拦截入口 (处理键盘按键、快捷键覆盖、滚轮翻页等)
    bool handleEventFilter(QObject* watched, QEvent* event);

private:
    bool handleKeyPress(QObject* watched, QKeyEvent* keyEvent);
    bool handleShortcutOverride(QKeyEvent* keyEvent);
    bool handleWheelScroll(QObject* watched, QWheelEvent* wheelEvent);
    bool isEditingFocus(QWidget* widget) const;

    // 阶梯式 Esc 清空逻辑
    bool handleEscClear();

    // 列表数字键 1/2/3/4 边界回环翻页与连续定位
    bool handleListNumericNavigation(int key);

    QuickWindow* m_window = nullptr;
    QElapsedTimer m_lastWheelPageTimer; // 滚轮换页防抖定时器
};

} // namespace RapidNotes

#endif // QUICKSHORTCUTHANDLER_H