# Implementation Plan - QuarkApplication Framework-Level Self-Healing (QuarkApplication.md)

## 1. Overview
This implementation plan establishes a framework-level self-healing architecture by subclassing `QApplication` as `QuarkApplication` and overriding its virtual `notify(QObject* receiver, QEvent* event)` method.

Whenever any `QWindow`, `QDialog`, or `QWidget` receives a `QEvent::Close` or `QEvent::Hide` event, `QuarkApplication::notify` intercepts the event at the earliest possible stage and automatically performs global cursor and capture self-healing:
1. Restores/unstacks all global `QGuiApplication::overrideCursor()` overrides.
2. Releases active Qt mouse grabbers (`QWidget::mouseGrabber()->releaseMouse()`).
3. Releases Win32 native mouse capture locks (`::ReleaseCapture()`).
4. Resets the target widget's cursor (`unsetCursor()`).
5. Forces immediate WM_SETCURSOR Hit-Test re-evaluation using `QCursor::setPos(QCursor::pos())`.

This framework-level architecture eliminates the need for any child dialog, popup, or `MainWindow` to include scattered cleanup code.

---

## 2. Modified Files List
- `src/ui/QuarkApplication.h` (New File)
- `src/ui/QuarkApplication.cpp` (New File)
- `CMakeLists.txt`
- `src/main.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Create `src/ui/QuarkApplication.h`

```cpp
#pragma once

#include <QApplication>

namespace QuarkMeta {

class QuarkApplication : public QApplication {
    Q_OBJECT
public:
    QuarkApplication(int& argc, char** argv);
    ~QuarkApplication() override = default;

    bool notify(QObject* receiver, QEvent* event) override;

private:
    void performGlobalSelfHealing(QWidget* widget);
};

} // namespace QuarkMeta
```

### 3.2 Create `src/ui/QuarkApplication.cpp`

```cpp
#include "QuarkApplication.h"
#include <QEvent>
#include <QWidget>
#include <QWindow>
#include <QGuiApplication>
#include <QCursor>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

QuarkApplication::QuarkApplication(int& argc, char** argv)
    : QApplication(argc, argv) {}

void QuarkApplication::performGlobalSelfHealing(QWidget* widget) {
    while (QGuiApplication::overrideCursor()) {
        QGuiApplication::restoreOverrideCursor();
    }

    if (QWidget* grabber = QWidget::mouseGrabber()) {
        if (!widget || grabber == widget || widget->isAncestorOf(grabber)) {
            grabber->releaseMouse();
        }
    }

#ifdef Q_OS_WIN
    if (widget && widget->testAttribute(Qt::WA_WState_Created)) {
        HWND hwnd = reinterpret_cast<HWND>(widget->winId());
        if (GetCapture() == hwnd) {
            ::ReleaseCapture();
        }
    }
#endif

    if (widget) {
        widget->unsetCursor();
    }

    QPoint currentPos = QCursor::pos();
    QCursor::setPos(currentPos);
}

bool QuarkApplication::notify(QObject* receiver, QEvent* event) {
    if (event) {
        QEvent::Type type = event->type();
        if (type == QEvent::Close || type == QEvent::Hide) {
            QWidget* w = qobject_cast<QWidget*>(receiver);
            if (w && (w->isWindow() || w->inherits("QDialog") || w->inherits("QFrame"))) {
                performGlobalSelfHealing(w);
            }
        }
    }
    return QApplication::notify(receiver, event);
}

} // namespace QuarkMeta
```

### 3.3 Register in `CMakeLists.txt`

```
<<<<<<< SEARCH
    src/ui/UiHelper.cpp
=======
    src/ui/QuarkApplication.cpp
    src/ui/UiHelper.cpp
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/ui/UiHelper.h
=======
    src/ui/QuarkApplication.h
    src/ui/UiHelper.h
>>>>>>> REPLACE
```

### 3.4 Instantiate `QuarkApplication` in `src/main.cpp`

```
<<<<<<< SEARCH
#include <QApplication>
=======
#include "ui/QuarkApplication.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
=======
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QuarkMeta::QuarkApplication a(argc, argv);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Framework-Level Self-Healing Verification**:
   - Open any window or popup (e.g. `BatchRenameDialog`, `TagManagerDialog`, `FileCollisionDialog`).
   - Dismiss or close the popup.
   - Verify that `QuarkApplication::notify` intercepts the `QEvent::Close` / `QEvent::Hide` event at the top level and immediately restores normal arrow and resize handle cursors across all windows without requiring any child-level cleanup code.
