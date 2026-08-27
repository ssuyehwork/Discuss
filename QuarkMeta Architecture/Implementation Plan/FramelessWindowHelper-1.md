# QuarkMeta 无边框窗口壳体全局事件总线穿透改造实施方案 (FramelessWindowHelper-1.md)

## 1. Overview
本方案旨在根治由于 `centralWidget` 与各侧边栏子控件 100% 物理覆盖 `MainWindow` 几何区域（0 边距），导致边缘 `MouseMove` / `MouseButtonPress` 事件被子控件抢占消费、宿主窗口收不到事件引发无双向箭头光标且无法拉伸的问题。通过将 `FramelessWindowHelper` 事件过滤器安装至 `QCoreApplication::instance()` 全局应用事件管道，实现了无视子控件遮蔽的边缘感应与拉伸。

---

## 2. Modified Files List
1. **修改** `src/ui/FramelessWindowHelper.cpp`
2. **修改** `src/ui/MainWindow.h`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FramelessWindowHelper.cpp` 全局管道挂载与坐标映射重构

```cpp
FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar) {
    
    m_window->setWindowFlags(m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    
    // 🚀【核心根治点】：安装至全局应用事件总线，穿透所有子控件的物理遮蔽！
    QCoreApplication::instance()->installEventFilter(this);
}
```

```cpp
bool FramelessWindowHelper::eventFilter(QObject* obj, QEvent* event) {
    if (!m_window || !m_window->isVisible()) return false;

    // 仅拦截属于当前窗口及其子控件树的事件
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget || (widget != m_window && !m_window->isAncestorOf(widget))) {
        return false;
    }

    QEvent::Type type = event->type();

    // 1. 边缘检测与光标切换（监听全窗口范围内所有子控件的 MouseMove / HoverMove）
    if (type == QEvent::MouseMove || type == QEvent::HoverMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalPos = mouseEvent->globalPosition().toPoint();
        QPoint localPos = m_window->mapFromGlobal(globalPos);

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
            ResizeDirection dir = calculateResizeDirection(localPos);
            updateCursorShape(dir);
            
            if (dir != None && (mouseEvent->buttons() & Qt::LeftButton)) {
                m_isResizing = true;
                m_resizeDir = dir;
                m_resizeStartGlobalPos = globalPos;
                m_resizeStartGeometry  = m_window->geometry();
                return true;
            }
        }
    }
    // ... [标题栏与按键响应逻辑保持高内聚] ...
}
```

---

### 3.2 `src/ui/MainWindow.h` 彻底清理废弃变量

```cpp
<<<<<<< SEARCH
    enum ResizeDirection {
        None = 0,
        Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };

    ResizeDirection m_resizeDir = None;
    bool m_isResizing = false;
    QPoint m_resizeStartGlobal;
    QRect  m_resizeStartGeometry;

    static constexpr int kResizeMargin = 6; // 边缘热区宽度（像素）

    ResizeDirection getResizeDirection(const QPoint& localPos) const;
    void updateCursorShape(ResizeDirection dir);
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **全局事件管道穿透验证**：
   - 鼠标悬停在窗口边缘（包含位于最上层的各侧栏与中央内容区边缘），验证指针 0 毫秒变为双向箭头。
2. **边缘拖拽拉伸测试**：
   - 拖拽 8 个方向的边缘，验证窗口拉伸缩放顺畅无误。
