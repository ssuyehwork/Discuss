#ifndef QUICKSHORTCUTHANDLER_H
#define QUICKSHORTCUTHANDLER_H

#include <QObject>
#include <QElapsedTimer>

class QEvent;
class QKeyEvent;
class QObject;
class QWheelEvent;

class QuickWindow;

namespace RapidNotes {

/**
 * @brief 集中处理 QuickWindow 的键盘导航、快捷键与列表滚轮翻页。
 *
 * QuickWindow::eventFilter 应将待处理事件转发给 handleEventFilter；
 * UI 事件过滤器的其他职责仍由 QuickWindow 自行处理。
 */
class QuickShortcutHandler : public QObject {
    Q_OBJECT

public:
    explicit QuickShortcutHandler(QuickWindow* window, QObject* parent = nullptr);
    ~QuickShortcutHandler() override = default;

    bool handleEventFilter(QObject* watched, QEvent* event);

private:
    bool handleKeyPress(QObject* watched, QKeyEvent* keyEvent);
    bool handleShortcutOverride(QKeyEvent* keyEvent);
    bool handleWheelScroll(QObject* watched, QWheelEvent* wheelEvent);
    bool handleEscClear();
    bool handleTreeKeyPress(QObject* watched, QKeyEvent* keyEvent);
    bool handleListKeyPress(QKeyEvent* keyEvent);
    bool handleListNumericNavigation(int key);
    bool switchToAllData();
    bool isSidebarVisible() const;

    QuickWindow* m_window = nullptr;
    QElapsedTimer m_lastWheelPageTimer;
};

} // namespace RapidNotes

#endif // QUICKSHORTCUTHANDLER_H
