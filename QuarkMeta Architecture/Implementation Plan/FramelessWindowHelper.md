# QuarkMeta 无边框窗口壳体归一化实施方案 (frameless.md)

## 1. Overview
本方案旨在解决无边框窗口交互逻辑分散在 `MainWindow.cpp`、`ResizeEventFilter` 与 `TitleBarEventFilter` 中的问题。通过引入统一归一的助手类 `FramelessWindowHelper`，收敛 8 方向边缘感应、DPI 动态热区计算、光标切换、边缘拉伸、标题栏拖拽移动、双击最大化/还原以及 Win32/跨平台置顶抽象，彻底净化 `MainWindow` 并物理废除删除旧有的碎片事件过滤器。

---

## 2. Modified Files List
1. **新建** `src/ui/FramelessWindowHelper.h`
2. **新建** `src/ui/FramelessWindowHelper.cpp`
3. **物理删除** `src/ui/ResizeEventFilter.h`
4. **物理删除** `src/ui/ResizeEventFilter.cpp`
5. **物理删除** `src/ui/TitleBarEventFilter.h`
6. **物理删除** `src/ui/TitleBarEventFilter.cpp`
7. **修改** `src/ui/MainWindow.h`
8. **修改** `src/ui/MainWindow.cpp`
9. **修改** `CMakeLists.txt`

---

## 3. Detailed Line-by-Line Changes

### 3.1 新建 `src/ui/FramelessWindowHelper.h`
```cpp
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
```

### 3.2 新建 `src/ui/FramelessWindowHelper.cpp`
```cpp
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include <QMouseEvent>
#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

void FramelessWindowHelper::apply(QWidget* window, QWidget* titleBar) {
    if (!window) return;
    new FramelessWindowHelper(window, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar) {
    
    m_window->setWindowFlags(m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    m_window->setAttribute(Qt::WA_Hover, true);
    m_window->installEventFilter(this);

    if (m_titleBar) {
        m_titleBar->setAttribute(Qt::WA_Hover, true);
        m_titleBar->installEventFilter(this);
    }
}

void FramelessWindowHelper::setAlwaysOnTop(QWidget* window, bool onTop) {
    if (!window) return;

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    SetWindowPos(hwnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
#else
    Qt::WindowFlags flags = window->windowFlags();
    if (onTop) flags |= Qt::WindowStaysOnTopHint;
    else flags &= ~Qt::WindowStaysOnTopHint;
    window->setWindowFlags(flags);
    window->show();
#endif
}

bool FramelessWindowHelper::isAlwaysOnTop(QWidget* window) {
    if (!window) return false;
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

FramelessWindowHelper::ResizeDirection FramelessWindowHelper::calculateResizeDirection(const QPoint& pos) const {
    if (!m_window || m_window->isMaximized() || m_window->isFullScreen()) return None;

    int margin = kBaseResizeMargin;
    if (m_window->windowHandle() && m_window->windowHandle()->screen()) {
        margin = qRound(m_window->windowHandle()->screen()->logicalDotsPerInch() / 96.0 * kBaseResizeMargin);
    }

    const int w = m_window->width();
    const int h = m_window->height();

    bool left   = pos.x() <= margin;
    bool right  = pos.x() >= w - margin;
    bool top    = pos.y() <= margin;
    bool bottom = pos.y() >= h - margin;

    if (top && left)     return TopLeft;
    if (top && right)    return TopRight;
    if (bottom && left)  return BottomLeft;
    if (bottom && right) return BottomRight;
    if (left)            return Left;
    if (right)           return Right;
    if (top)             return Top;
    if (bottom)          return Bottom;

    return None;
}

void FramelessWindowHelper::updateCursorShape(ResizeDirection dir) {
    if (!m_window) return;

    switch (dir) {
        case Left:        case Right:       m_window->setCursor(Qt::SizeHorCursor);  break;
        case Top:         case Bottom:      m_window->setCursor(Qt::SizeVerCursor);  break;
        case TopLeft:     case BottomRight: m_window->setCursor(Qt::SizeFDiagCursor); break;
        case TopRight:    case BottomLeft:  m_window->setCursor(Qt::SizeBDiagCursor); break;
        default:                            m_window->setCursor(Qt::ArrowCursor);    break;
    }
}

bool FramelessWindowHelper::eventFilter(QObject* obj, QEvent* event) {
    if (!m_window) return false;

    if (obj == m_window) {
        switch (event->type()) {
            case QEvent::MouseMove: {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                const QPoint globalPos = mouseEvent->globalPosition().toPoint();

                if (m_isResizing) {
                    const QPoint delta = globalPos - m_resizeStartGlobalPos;
                    QRect r = m_resizeStartGeometry;

                    if (m_resizeDir == Left || m_resizeDir == TopLeft || m_resizeDir == BottomLeft)
                        r.setLeft(r.left() + delta.x());
                    if (m_resizeDir == Right || m_resizeDir == TopRight || m_resizeDir == BottomRight)
                        r.setRight(r.right() + delta.x());
                    if (m_resizeDir == Top || m_resizeDir == TopLeft || m_resizeDir == TopRight)
                        r.setTop(r.top() + delta.y());
                    if (m_resizeDir == Bottom || m_resizeDir == BottomLeft || m_resizeDir == BottomRight)
                        r.setBottom(r.bottom() + delta.y());

                    if (r.width() >= m_window->minimumWidth() && r.height() >= m_window->minimumHeight()) {
                        m_window->setGeometry(r);
                    }
                    return true;
                }

                if (!m_isDragging && !m_window->isMaximized()) {
                    const QPoint localPos = mouseEvent->position().toPoint();
                    ResizeDirection dir = calculateResizeDirection(localPos);
                    updateCursorShape(dir);
                }
                break;
            }

            case QEvent::MouseButtonPress: {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton && !m_window->isMaximized()) {
                    const QPoint localPos = mouseEvent->position().toPoint();
                    ResizeDirection dir = calculateResizeDirection(localPos);
                    if (dir != None) {
                        m_isResizing = true;
                        m_isDragging = false;
                        m_resizeDir = dir;
                        m_resizeStartGlobalPos = mouseEvent->globalPosition().toPoint();
                        m_resizeStartGeometry  = m_window->geometry();
                        return true;
                    }
                }
                break;
            }

            case QEvent::MouseButtonRelease: {
                if (m_isResizing) {
                    m_isResizing = false;
                    m_resizeDir = None;
                    updateCursorShape(None);
                    return true;
                }
                break;
            }

            case QEvent::Leave: {
                if (!m_isResizing && !m_isDragging) {
                    updateCursorShape(None);
                }
                break;
            }

            default:
                break;
        }
    }

    if (obj == m_titleBar || (m_titleBar && m_titleBar->isAncestorOf(qobject_cast<QWidget*>(obj)))) {
        QWidget* targetWidget = qobject_cast<QWidget*>(obj);

        bool isInteractiveChild = targetWidget && (
            qobject_cast<QPushButton*>(targetWidget) ||
            qobject_cast<QToolButton*>(targetWidget) ||
            qobject_cast<QLineEdit*>(targetWidget) ||
            qobject_cast<QSlider*>(targetWidget)
        );

        if (!isInteractiveChild) {
            switch (event->type()) {
                case QEvent::MouseButtonDblClick: {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (mouseEvent->button() == Qt::LeftButton) {
                        if (m_window->isMaximized()) {
                            m_window->showNormal();
                        } else {
                            m_window->showMaximized();
                        }
                        return true;
                    }
                    break;
                }

                case QEvent::MouseButtonPress: {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (mouseEvent->button() == Qt::LeftButton && !m_isResizing) {
                        m_isDragging = true;
                        m_dragStartGlobalPos = mouseEvent->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
                        return true;
                    }
                    break;
                }

                case QEvent::MouseMove: {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (m_isDragging && (mouseEvent->buttons() & Qt::LeftButton)) {
                        if (m_window->isMaximized()) {
                            const double ratio = static_cast<double>(mouseEvent->globalPosition().toPoint().x()) / m_window->width();
                            m_window->showNormal();
                            const int newX = mouseEvent->globalPosition().toPoint().x() - static_cast<int>(m_window->width() * ratio);
                            m_window->move(newX, mouseEvent->globalPosition().toPoint().y() - 10);
                            m_dragStartGlobalPos = mouseEvent->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
                        } else {
                            m_window->move(mouseEvent->globalPosition().toPoint() - m_dragStartGlobalPos);
                        }
                        return true;
                    }
                    break;
                }

                case QEvent::MouseButtonRelease: {
                    m_isDragging = false;
                    break;
                }

                default:
                    break;
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

} // namespace QuarkMeta
```

---

### 3.3 `src/ui/MainWindow.h` 彻底净化

```cpp
<<<<<<< SEARCH
#include "ResizeEventFilter.h"
#include "TitleBarEventFilter.h"
=======
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    enum ResizeDirection {
        None = 0,
        Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };

    ResizeDirection getResizeDirection(const QPoint& pos) const;
    void updateCursorShape(ResizeDirection dir);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
=======
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    ResizeEventFilter* m_resizeFilter = nullptr;
    TitleBarEventFilter* m_titleBarFilter = nullptr;
=======
>>>>>>> REPLACE
```

---

### 3.4 `src/ui/MainWindow.cpp` 彻底净化与 1 行装配

```cpp
<<<<<<< SEARCH
#include "ResizeEventFilter.h"
#include "TitleBarEventFilter.h"
=======
#include "FramelessWindowHelper.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_resizeFilter = new ResizeEventFilter(this);
    installEventFilter(m_resizeFilter);

    if (m_titleBarWidget) {
        m_titleBarFilter = new TitleBarEventFilter(this, m_titleBarWidget);
        m_titleBarWidget->installEventFilter(m_titleBarFilter);
    }
=======
    FramelessWindowHelper::apply(this, m_titleBarWidget);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void MainWindow::onPinToggled(bool checked) {
    if (m_isPinned == checked) return;
    m_isPinned = checked;

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    SetWindowPos(hwnd, m_isPinned ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
#else
    Qt::WindowFlags flags = windowFlags();
    if (m_isPinned) flags |= Qt::WindowStaysOnTopHint;
    else flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    show();
#endif

    if (m_btnPinTop) {
        m_btnPinTop->setIcon(UiHelper::getIcon(m_isPinned ? "pin_vertical" : "pin_tilted", 
                                               m_isPinned ? Style::ActiveOrange : TextMain));
    }

    AppConfig::instance().setValue("MainWindow/AlwaysOnTop", m_isPinned);
}
=======
void MainWindow::onPinToggled(bool checked) {
    if (m_isPinned == checked) return;
    m_isPinned = checked;

    FramelessWindowHelper::setAlwaysOnTop(this, checked);

    if (m_btnPinTop) {
        m_btnPinTop->setIcon(UiHelper::getIcon(m_isPinned ? "pin_vertical" : "pin_tilted", 
                                               m_isPinned ? Style::ActiveOrange : TextMain));
    }

    AppConfig::instance().setValue("MainWindow/AlwaysOnTop", m_isPinned);
}
>>>>>>> REPLACE
```

---

### 3.5 `CMakeLists.txt` 构建注册与清理

```cmake
<<<<<<< SEARCH
    src/ui/ResizeEventFilter.h
    src/ui/ResizeEventFilter.cpp
    src/ui/TitleBarEventFilter.h
    src/ui/TitleBarEventFilter.cpp
=======
    src/ui/FramelessWindowHelper.h
    src/ui/FramelessWindowHelper.cpp
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **构建与 MOC 验证**：
   - 检查新增的 `FramelessWindowHelper.h/cpp` 在 CMake 中正常注册。
   - 验证无废弃 `ResizeEventFilter` 或 `TitleBarEventFilter` 的未解析符号错误。

2. **壳体功能验证**：
   - **8 方向拉伸验证**：拖拽四周边缘及 4 个角，验证窗口正常尺寸调整与光标图形切换。
   - **高 DPI 验证**：在 DPI 缩放环境下测试边缘热区触达感。
   - **标题栏拖拽与还原**：测试拖拽标题栏移动窗口，双击标题栏进行最大化/还原，以及最大化按住拖拽吸附还原。
