# Implementation Plan - Dedicated TitleBarEventFilter Decoupling (`titlebar-filter.md`)

## 1. Overview
This implementation plan decouples the frameless window TitleBar double-click and drag-to-move/restore interactions into a standalone, dedicated event filter `TitleBarEventFilter`:
1. **Single Responsibility**: `TitleBarEventFilter` handles left-button double-click to toggle maximize/restore and mouse drag to move or restore the window when dragged while maximized.
2. **Clean MainWindow**: Removes all title bar drag/double-click conditional branches from `MainWindow::eventFilter`.
3. **MOC Registration**: Registers `src/ui/TitleBarEventFilter.h` and `src/ui/TitleBarEventFilter.cpp` in `CMakeLists.txt`.

---

## 2. Modified Files List
- `CMakeLists.txt`
- `src/ui/TitleBarEventFilter.h` (New)
- `src/ui/TitleBarEventFilter.cpp` (New)
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
Add `src/ui/TitleBarEventFilter.h` and `src/ui/TitleBarEventFilter.cpp` under the `ui` sources section.

```
<<<<<<< SEARCH
    src/ui/ResizeEventFilter.cpp
    src/ui/ResizeEventFilter.h
=======
    src/ui/ResizeEventFilter.cpp
    src/ui/ResizeEventFilter.h
    src/ui/TitleBarEventFilter.cpp
    src/ui/TitleBarEventFilter.h
>>>>>>> REPLACE
```

### 3.2 `src/ui/TitleBarEventFilter.h` (New File)
```cpp
#pragma once

#include <QObject>
#include <QEvent>
#include <QMainWindow>
#include <QPoint>

namespace QuarkMeta {

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
```

### 3.3 `src/ui/TitleBarEventFilter.cpp` (New File)
```cpp
#include "TitleBarEventFilter.h"
#include <QMouseEvent>

namespace QuarkMeta {

TitleBarEventFilter::TitleBarEventFilter(QMainWindow* window, QObject* parent)
    : QObject(parent ? parent : window), m_window(window) {
}

bool TitleBarEventFilter::eventFilter(QObject* watched, QEvent* event) {
    Q_UNUSED(watched);
    if (!m_window) return false;

    if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
        if (mouseEv->button() == Qt::LeftButton) {
            if (m_window->isMaximized()) {
                m_window->showNormal();
            } else {
                m_window->showMaximized();
            }
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
        if (mouseEv->button() == Qt::LeftButton) {
            m_isDragging = true;
            m_dragPosition = mouseEv->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
        }
    } else if (event->type() == QEvent::MouseMove && m_isDragging) {
        QMouseEvent* mouseEv = static_cast<QMouseEvent*>(event);
        if (mouseEv->buttons() & Qt::LeftButton) {
            if (m_window->isMaximized()) {
                double widthRatio = (double)mouseEv->position().x() / m_window->width();
                m_window->showNormal();
                int normalW = m_window->width();
                int newX = mouseEv->globalPosition().toPoint().x() - static_cast<int>(normalW * widthRatio);
                int newY = mouseEv->globalPosition().toPoint().y() - 15;
                m_window->move(newX, newY);
                m_dragPosition = mouseEv->globalPosition().toPoint() - m_window->frameGeometry().topLeft();
            } else {
                m_window->move(mouseEv->globalPosition().toPoint() - m_dragPosition);
            }
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_isDragging = false;
    }

    return false;
}

} // namespace QuarkMeta
```

### 3.4 `src/ui/MainWindow.cpp`
```
<<<<<<< SEARCH
    if (watched == m_titleBarWidget || watched == m_logoLabel || watched == m_appNameLabel) {
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build project using CMake / Ninja.
2. Verify clean compilation without MOC or undefined symbol errors.
3. Verify double-clicking title bar toggles maximize/restore.
4. Verify dragging title bar while maximized smoothly restores window and follows cursor.
