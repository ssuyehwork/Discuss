# Implementation Plan - FramelessWindowHelper WindowRole Normalization & Native Dragging Refactoring

## Architecture Gate 3-Question Answers (架构三问回答)
1. **SSOT Source (真理源溯源)**:
   The window frameless capabilities, OS style flags, and native `WM_NCHITTEST` dragging/resizing logic are solely owned and maintained by `FramelessWindowHelper` as the single source of truth (SSOT). `WindowRole` explicitly categorizes caller capabilities (`Primary`, `Tool`, `Dialog`).
2. **Black-box Integrity (黑盒完整性)**:
   Encapsulation is strictly preserved. Calling windows (`MainWindow`, `TagSelectorOverlay`, `FramelessDialog`) interact with `FramelessWindowHelper` via the clean `FramelessWindowHelper::apply(window, role, titleBar)` public contract. Internal Win32 message handling is hidden inside `FramelessWindowHelper::handleNativeEvent`.
3. **Root Cause Analysis (根因 vs 症状)**:
   Previously, `FramelessDialog` used manual `ReleaseCapture() + SendMessageW(WM_NCLBUTTONDOWN)` in `mousePressEvent`, creating duplicated, scattered, and conflicting window dragging implementations across the codebase. By introducing `WindowRole` and unifying window native style flags and `WM_NCHITTEST` handling inside `FramelessWindowHelper`, all frameless dragging/resizing behaviors are fully normalized without manual mouse-capturing hacks.

---

## 1. Overview
This implementation plan normalizes the frameless window handling across QuarkMeta by introducing explicit `WindowRole` parameters (`Primary`, `Tool`, `Dialog`) and integrating `FramelessDialog` into the unified `FramelessWindowHelper` architecture. It removes duplicate manual mouse-dragging hacks from `FramelessDialog` and provides a matrix of capabilities per role.

---

## 2. Modified Files List
1. `src/ui/FramelessWindowHelper.h`
2. `src/ui/FramelessWindowHelper.cpp`
3. `src/ui/FramelessDialogBase.h`
4. `src/ui/FramelessDialog.cpp`
5. `src/ui/MainWindow.cpp`
6. `src/ui/TagSelectorOverlay.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/FramelessWindowHelper.h`
```
<<<<<<< SEARCH
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override = default;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;

    static constexpr int kBaseResizeMargin = 8;
};
=======
enum class WindowRole {
    Primary,   // Main window: full title bar, minimize/maximize/restore, native resize/drag
    Tool,      // Floating tool window: no title bar, no maximize, edge resize only
    Dialog     // Dialog: custom title bar, native drag, no double-click maximize
};

class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, WindowRole role = WindowRole::Primary, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

    WindowRole role() const { return m_role; }

private:
    explicit FramelessWindowHelper(QWidget* window, WindowRole role, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override = default;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;
    WindowRole m_role = WindowRole::Primary;

    static constexpr int kBaseResizeMargin = 8;
};
>>>>>>> REPLACE
```

### File 2: `src/ui/FramelessWindowHelper.cpp`
```
<<<<<<< SEARCH
FramelessWindowHelper* FramelessWindowHelper::apply(QWidget* window, QWidget* titleBar) {
    if (!window) return nullptr;
    return new FramelessWindowHelper(window, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar) {

    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint;
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    // 只有真正带标题栏（传入了 titleBar）的窗口，才需要完整的系统窗口属性
    // 像 TagSelectorOverlay 这种 Qt::Tool 悬浮面板，不该被强行赋予标题栏/最大化/系统菜单语义
    if (m_titleBar) {
        SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU);
    } else {
        SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME);
    }
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#endif
}
=======
FramelessWindowHelper* FramelessWindowHelper::apply(QWidget* window, WindowRole role, QWidget* titleBar) {
    if (!window) return nullptr;
    return new FramelessWindowHelper(window, role, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, WindowRole role, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar), m_role(role) {

    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint;
    if (m_role == WindowRole::Primary) {
        requiredFlags |= Qt::WindowMinMaxButtonsHint;
    }
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);

    switch (m_role) {
        case WindowRole::Primary:
            SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU);
            break;
        case WindowRole::Tool:
            SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME);
            break;
        case WindowRole::Dialog:
            SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU);
            break;
    }
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#endif
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 5. 原生双击标题栏最大化 / 还原（通过 Win32 消息总线响应）
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (msg->wParam == HTCAPTION) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, isMax ? SC_RESTORE : SC_MAXIMIZE, 0);
            *result = 0;
            return true;
        }
    }
=======
    // 5. 原生双击标题栏最大化 / 还原（唯独 Primary 角色支持双击标题栏最大化/还原）
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (msg->wParam == HTCAPTION) {
            if (m_role == WindowRole::Primary) {
                ::SendMessage(hwnd, WM_SYSCOMMAND, isMax ? SC_RESTORE : SC_MAXIMIZE, 0);
                *result = 0;
                return true;
            }
        }
    }
>>>>>>> REPLACE
```

### File 3: `src/ui/FramelessDialogBase.h`
```
<<<<<<< SEARCH
namespace QuarkMeta {

class FramelessDialog : public QDialog {
    Q_OBJECT
public:
    enum DialogButton { Pin = 1, Min = 2, Max = 4, Close = 8, All = 15 };
    explicit FramelessDialog(const QString& title, QWidget* parent = nullptr);
    ~FramelessDialog() override;

    QWidget* getContentArea() const { return m_contentArea; }
    void setVisibleButtons(int flags);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    QWidget* m_contentArea;
    QVBoxLayout* m_mainLayout;
    QVBoxLayout* m_outerLayout;
    QHBoxLayout* m_titleLayout;
    QWidget* m_container;
    QLabel* m_titleLabel;
    QPushButton* m_pinBtn;
    QPushButton* m_minBtn;
    QPushButton* m_maxBtn;
    QPushButton* m_closeBtn;

private:
    QPoint m_dragPos;
    bool m_isDragging = false;
};

} // namespace QuarkMeta
=======
namespace QuarkMeta {

class FramelessWindowHelper;

class FramelessDialog : public QDialog {
    Q_OBJECT
public:
    enum DialogButton { Pin = 1, Min = 2, Max = 4, Close = 8, All = 15 };
    explicit FramelessDialog(const QString& title, QWidget* parent = nullptr);
    ~FramelessDialog() override;

    QWidget* getContentArea() const { return m_contentArea; }
    void setVisibleButtons(int flags);

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    QWidget* m_contentArea;
    QWidget* m_titleBar;
    QVBoxLayout* m_mainLayout;
    QVBoxLayout* m_outerLayout;
    QHBoxLayout* m_titleLayout;
    QWidget* m_container;
    QLabel* m_titleLabel;
    QPushButton* m_pinBtn;
    QPushButton* m_minBtn;
    QPushButton* m_maxBtn;
    QPushButton* m_closeBtn;

private:
    FramelessWindowHelper* m_framelessHelper = nullptr;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### File 4: `src/ui/FramelessDialog.cpp`
```
<<<<<<< SEARCH
#include "FramelessDialog.h"
#include "UiHelper.h"
#include <QMouseEvent>
=======
#include "FramelessDialog.h"
#include "FramelessWindowHelper.h"
#include "UiHelper.h"
#include <QMouseEvent>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_mainLayout = new QVBoxLayout(m_container);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    auto* titleBar = new QWidget();
    titleBar->setObjectName("TitleBar");
    titleBar->setFixedHeight(34);
    titleBar->setObjectName("FramelessTitleBar");
    m_titleLayout = new QHBoxLayout(titleBar);
    m_titleLayout->setContentsMargins(12, 0, 5, 0);
    m_titleLayout->setSpacing(4);
=======
    m_mainLayout = new QVBoxLayout(m_container);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_titleBar = new QWidget();
    m_titleBar->setObjectName("TitleBar");
    m_titleBar->setFixedHeight(34);
    m_titleBar->setObjectName("FramelessTitleBar");
    m_titleLayout = new QHBoxLayout(m_titleBar);
    m_titleLayout->setContentsMargins(12, 0, 5, 0);
    m_titleLayout->setSpacing(4);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_titleLayout->addWidget(m_pinBtn);
    m_titleLayout->addWidget(m_minBtn);
    m_titleLayout->addWidget(m_maxBtn);
    m_titleLayout->addWidget(m_closeBtn);

    m_mainLayout->addWidget(titleBar);
    m_mainLayout->addSpacing(4);
=======
    m_titleLayout->addWidget(m_pinBtn);
    m_titleLayout->addWidget(m_minBtn);
    m_titleLayout->addWidget(m_maxBtn);
    m_titleLayout->addWidget(m_closeBtn);

    m_mainLayout->addWidget(m_titleBar);
    m_mainLayout->addSpacing(4);

    m_framelessHelper = FramelessWindowHelper::apply(this, WindowRole::Dialog, m_titleBar);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool FramelessDialog::eventFilter(QObject* watched, QEvent* event) {
    return QDialog::eventFilter(watched, event);
}

} // namespace QuarkMeta
=======
bool FramelessDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (m_framelessHelper && m_framelessHelper->handleNativeEvent(message, result)) {
        return true;
    }
    return QDialog::nativeEvent(eventType, message, result);
}

bool FramelessDialog::eventFilter(QObject* watched, QEvent* event) {
    return QDialog::eventFilter(watched, event);
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

And remove `mousePressEvent`, `mouseMoveEvent`, and `mouseReleaseEvent` implementations from `src/ui/FramelessDialog.cpp`.

```
<<<<<<< SEARCH
namespace {
bool isInteractiveWidget(QWidget* widget) {
    while (widget) {
        if (qobject_cast<QAbstractButton*>(widget) ||
            qobject_cast<QLineEdit*>(widget) ||
            qobject_cast<QCheckBox*>(widget)) {
            return true;
        }
        widget = widget->parentWidget();
    }
    return false;
}
} // namespace

void FramelessDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (!child || !isInteractiveWidget(child)) {
#ifdef Q_OS_WIN
            ReleaseCapture();
            ::SendMessageW(reinterpret_cast<HWND>(winId()), WM_NCLBUTTONDOWN, HTCAPTION, 0);
            event->accept();
            return;
#else
            m_isDragging = true;
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
#endif
        }
    }
    QDialog::mousePressEvent(event);
}

void FramelessDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void FramelessDialog::mouseReleaseEvent(QMouseEvent* event) {
    m_isDragging = false;
    QDialog::mouseReleaseEvent(event);
}
=======
>>>>>>> REPLACE
```

### File 5: `src/ui/MainWindow.cpp`
```
<<<<<<< SEARCH
    m_framelessHelper = FramelessWindowHelper::apply(this, m_titleBarWidget);
=======
    m_framelessHelper = FramelessWindowHelper::apply(this, WindowRole::Primary, m_titleBarWidget);
>>>>>>> REPLACE
```

### File 6: `src/ui/TagSelectorOverlay.cpp`
```
<<<<<<< SEARCH
    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);
=======
    m_framelessHelper = FramelessWindowHelper::apply(this, WindowRole::Tool, nullptr);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute `mkdir build && cd build && cmake .. && cmake --build .` (or `ninja`).
2. Verify compilation succeeds with zero errors or C2039/C2027 warnings.
3. Test window behaviors:
   - `MainWindow`: Verify `WindowRole::Primary` native resizing, native title bar drag, and double-click to maximize/restore.
   - `TagSelectorOverlay`: Verify `WindowRole::Tool` edge resizing without caption style overhead.
   - `FramelessDialog` (e.g. `TagManagerDialog`, `PresetTagsDialog`): Verify `WindowRole::Dialog` native title bar dragging via `WM_NCHITTEST` without double-click maximize behavior.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Channel Reused**: Unified `FramelessWindowHelper::apply` entry point.
- **Dead Code Removed**: Removed scattered manual dragging implementations (`ReleaseCapture()`, `WM_NCLBUTTONDOWN`) from `FramelessDialog.cpp`.

---

## 6. Header API Signature Verification
| Class / Function | Declared Header | Verified Exact Signature |
| :--- | :--- | :--- |
| `FramelessWindowHelper::apply` | `src/ui/FramelessWindowHelper.h` | `static FramelessWindowHelper* apply(QWidget* window, WindowRole role = WindowRole::Primary, QWidget* titleBar = nullptr);` |
| `FramelessWindowHelper::handleNativeEvent` | `src/ui/FramelessWindowHelper.h` | `bool handleNativeEvent(void* message, qintptr* result);` |
| `FramelessDialog::nativeEvent` | `src/ui/FramelessDialogBase.h` | `bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;` |

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/FramelessDialog.cpp`: Added `#include "FramelessWindowHelper.h"` for `FramelessWindowHelper` and `WindowRole` definitions.
- `src/ui/FramelessDialogBase.h`: Added forward declaration `class FramelessWindowHelper;`.
