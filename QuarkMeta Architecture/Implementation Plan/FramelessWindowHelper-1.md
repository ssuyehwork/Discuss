# QuarkMeta 无边框窗口壳体归一化实施方案 (FramelessWindowHelper)

## 1. 目标与范围
- 新建 `FramelessWindowHelper`：将 8 方向边缘感应、DPI 动态热区、光标切换、边缘拉伸、标题栏拖拽移动、双击最大化/还原、任务栏防遮挡保护及跨平台置顶（AlwaysOnTop）**100% 归一化收敛至单一中枢**。
- 彻底物理废除并删除碎片文件：`src/ui/ResizeEventFilter.h/cpp` 与 `src/ui/TitleBarEventFilter.h/cpp`。
- 彻底净化 `MainWindow.h/cpp`：清除所有重写的鼠标事件虚函数、150+ 行底层几何数学算式及裸 Win32 API 杂质，构造函数仅保留 1 行标准装配代码。

---

## 2. 归一化核心模块实现

### 2.1 `src/ui/FramelessWindowHelper.h`
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
    /**
     * @brief 为目标窗口一键装配无边框拖拽、拉伸与标题栏交互
     * @param window 目标顶级窗口 (QMainWindow 或 QDialog)
     * @param titleBar 可选的自定义标题栏控件 (用于拖拽与双击最大化)
     */
    static void apply(QWidget* window, QWidget* titleBar = nullptr);

    /**
     * @brief 跨平台安全置顶/取消置顶抽象
     */
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

### 2.2 `src/ui/FramelessWindowHelper.cpp`
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
#include <cmath>

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

    // -------------------------------------------------------------
    // 1. 宿主窗口（m_window）事件拦截：负责 8 方向边缘拉伸与光标切换
    // -------------------------------------------------------------
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

    // -------------------------------------------------------------
    // 2. 标题栏（m_titleBar）事件拦截：负责按住拖拽与双击最大化/还原
    // -------------------------------------------------------------
    if (obj == m_titleBar || (m_titleBar && m_titleBar->isAncestorOf(qobject_cast<QWidget*>(obj)))) {
        QWidget* targetWidget = qobject_cast<QWidget*>(obj);

        // 过滤子控件交互，防止抢占按钮/输入框的点击
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
                            // 最大化拖拽还原吸附计算
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

## 3. `MainWindow.h` 与 `MainWindow.cpp` 彻底净化改造

### 3.1 `MainWindow.h` 净化
- 删除 `enum ResizeDirection` 及所有几何数学计算成员。
- 删除重写的鼠标虚函数：`mousePressEvent`、`mouseMoveEvent`、`mouseReleaseEvent`。
- 删除废弃的过滤器成员：`m_resizeFilter`、`m_titleBarFilter`、`m_hoverFilter`。

```cpp
// MainWindow.h 净化后关键区段：
#pragma once

#include <QMainWindow>
#include <QPointer>
// ... 业务子面板头文件包含 ...

namespace QuarkMeta {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

    // 🚨 彻底移除 mousePressEvent, mouseMoveEvent, mouseReleaseEvent!

private slots:
    void onPinToggled(bool checked);
    // ... 其他业务槽函数保持不变 ...

private:
    void initUi();
    // ... 其他纯 UI 装配函数 ...

    QWidget* m_titleBarWidget = nullptr;
    // 🚨 彻底移除 m_resizeFilter, m_titleBarFilter, m_hoverFilter, m_resizeDir 等字段!
};

} // namespace QuarkMeta
```

### 3.2 `MainWindow.cpp` 净化
- 在构造函数中**仅保留 1 行 `FramelessWindowHelper::apply(this, m_titleBarWidget)`**。
- 置顶槽函数统一收敛至 `FramelessWindowHelper::setAlwaysOnTop`。
- 彻底删除 `getResizeDirection` 与 `updateCursorShape` 约 150 行底层数学代码。

```cpp
// MainWindow.cpp 改造关键点：

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_currentDataSource("nav") {
    m_panelsInitialized = false;

    ToolTipOverlay::instance();
    setMinimumSize(465, 400); 
    setWindowTitle("QuarkMeta");

    m_shortcutController = new GlobalShortcutController(this, this);
    m_panelMediator = new PanelMediator(this, this);

    m_isPinned = AppConfig::instance().getValue("MainWindow/AlwaysOnTop", false).toBool();
    
    // 初始化 QSS
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        setStyleSheet(QLatin1String(file.readAll()));
    }

    initUi();

    // 🚀【核心归一化】：1 行代码接管全套无边框拉伸、标题栏拖拽与双击最大化！
    FramelessWindowHelper::apply(this, m_titleBarWidget);

    if (m_isPinned) {
        FramelessWindowHelper::setAlwaysOnTop(this, true);
    }

    m_trayController = new TrayController(this);
    m_trayController->show();

    // ... 导航恢复定时器保持不变 ...
}

void MainWindow::onPinToggled(bool checked) {
    if (m_isPinned == checked) return;
    m_isPinned = checked;

    // 🚀 统一调用无侵入接口，彻底消灭裸 Win32 SetWindowPos 代码
    FramelessWindowHelper::setAlwaysOnTop(this, checked);

    if (m_btnPinTop) {
        m_btnPinTop->setIcon(UiHelper::getIcon(m_isPinned ? "pin_vertical" : "pin_tilted", 
                                               m_isPinned ? Style::ActiveOrange : TextMain));
    }

    AppConfig::instance().setValue("MainWindow/AlwaysOnTop", m_isPinned);
}

// 🚨 彻底删除 MainWindow::mousePressEvent、mouseMoveEvent、mouseReleaseEvent、getResizeDirection、updateCursorShape!
```

---

## 4. 物理清理与构建配置更新

### 4.1 物理删除废弃文件
- 删除 `src/ui/ResizeEventFilter.h`
- 删除 `src/ui/ResizeEventFilter.cpp`
- 删除 `src/ui/TitleBarEventFilter.h`
- 删除 `src/ui/TitleBarEventFilter.cpp`

### 4.2 `CMakeLists.txt` 构建配置注册
```cmake
set(UI_SOURCES
    # ... 现有 UI 源文件 ...
    src/ui/FramelessWindowHelper.h
    src/ui/FramelessWindowHelper.cpp
    # 🚨 移除 ResizeEventFilter 与 TitleBarEventFilter
)
```