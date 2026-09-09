# Implementation Plan - Win32 System Capture Break & Edge Resize Restoration (TagSelectorOverlay & TagManagerDialog)

## 1. Overview
This implementation plan implements the ultimate fix for Win32 mouse capture leaks and edge resizing on `TagSelectorOverlay` and `TagManagerDialog`:
1. **Win32 System Capture Break (`::ReleaseCapture()`)**: Executing `::ReleaseCapture()` upon closing `TagSelectorOverlay` and `TagManagerDialog::showDialog` breaks any orphaned Win32 mouse capture locks, immediately unblocking DWM `WM_NCHITTEST` and `WM_SETCURSOR` message dispatching to `MainWindow`.
2. **Restore Frameless Window Edge Resizing**: Re-attaches `FramelessWindowHelper::apply(this, nullptr)` on `TagSelectorOverlay` so edge resizing and dragging are fully functional, while ensuring `::ReleaseCapture()` and `activateWindow()` prevent cursor sticking or focus loss.
3. **Clean Window Activation**: Invokes `topWin->activateWindow()` upon overlay closure to return activation to `MainWindow`.

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/TagManagerDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Re-attach FramelessWindowHelper & nativeEvent in TagSelectorOverlay (`src/ui/TagSelectorOverlay.h`)

```
<<<<<<< SEARCH
#include "components/FlowLayout.h"
#include "../core/TagLexiconService.h"

namespace QuarkMeta {

class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;

signals:
    void selectionChanged(const QStringList& selectedTags);
    void overlayClosed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
=======
#include "components/FlowLayout.h"
#include "../core/TagLexiconService.h"
#include "FramelessWindowHelper.h"

namespace QuarkMeta {

class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;

signals:
    void selectionChanged(const QStringList& selectedTags);
    void overlayClosed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private:
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    bool m_isClosing = false;
    bool m_isDragging = false;
    QPoint m_dragPos;
};
=======
    bool m_isClosing = false;
    bool m_isDragging = false;
    QPoint m_dragPos;

    FramelessWindowHelper* m_framelessHelper = nullptr;
};
>>>>>>> REPLACE
```

---

### 3.2 Apply FramelessWindowHelper & Release Capture in TagSelectorOverlay (`src/ui/TagSelectorOverlay.cpp`)

```
<<<<<<< SEARCH
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
    setObjectName("TagSelectorOverlay");
    setFrameShape(QFrame::StyledPanel);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    initUi();
=======
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
    setObjectName("TagSelectorOverlay");
    setFrameShape(QFrame::StyledPanel);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);

    initUi();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;

    // 1. 立即拔除全局事件过滤器，绝不等析构
    if (qApp) {
        qApp->removeEventFilter(this);
    }

    // 2. 强制解除全局所有子控件的鼠标抓取（解决 QPushButton 隐式抓取悬空）
    if (QWidget::mouseGrabber()) {
        QWidget::mouseGrabber()->releaseMouse();
    }

    emit overlayClosed();
    close();

    // 3. 解决 Qt::Tool (WS_EX_TOOLWINDOW) 关闭时不向 Owner 归还激活的 Win32 缺陷：显式唤醒主窗口
    QWidget* topWin = parentWidget() ? parentWidget()->window() : nullptr;
    if (topWin) {
        topWin->activateWindow();
    }

    deleteLater();
}
=======
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;

#ifdef Q_OS_WIN
    ::ReleaseCapture();
#endif

    if (QWidget::mouseGrabber()) {
        QWidget::mouseGrabber()->releaseMouse();
    }

    if (qApp) {
        qApp->removeEventFilter(this);
    }

    emit overlayClosed();
    close();

    QWidget* topWin = parentWidget() ? parentWidget()->window() : nullptr;
    if (topWin) {
        topWin->activateWindow();
    }

    deleteLater();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool TagSelectorOverlay::eventFilter(QObject* obj, QEvent* event) {
=======
bool TagSelectorOverlay::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (m_framelessHelper && m_framelessHelper->handleNativeEvent(message, result)) {
        return true;
    }
    return QFrame::nativeEvent(eventType, message, result);
}

bool TagSelectorOverlay::eventFilter(QObject* obj, QEvent* event) {
>>>>>>> REPLACE
```

---

### 3.3 Win32 ReleaseCapture in TagManagerDialog (`src/ui/TagManagerDialog.cpp`)

```
<<<<<<< SEARCH
void TagManagerDialog::showDialog(QWidget* parent, const QString& currentPath, bool isMirrorSource) {
    QWidget* topParent = parent ? parent->window() : nullptr;

    // 栈分配保证退栈时完全析构
    TagManagerDialog dlg(currentPath, isMirrorSource, topParent);
    dlg.exec();

    // 模态退出后，正规交还激活与焦点
    if (topParent) {
        topParent->activateWindow();
        topParent->setFocus();
    }
}
=======
void TagManagerDialog::showDialog(QWidget* parent, const QString& currentPath, bool isMirrorSource) {
    QWidget* topParent = parent ? parent->window() : nullptr;

    TagManagerDialog dlg(currentPath, isMirrorSource, topParent);
    dlg.exec();

#ifdef Q_OS_WIN
    ::ReleaseCapture();
#endif

    if (topParent) {
        topParent->activateWindow();
        topParent->setFocus();
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional Verification**:
   - Verify `TagSelectorOverlay` can be edge-resized smoothly.
   - Open and close `TagSelectorOverlay` and `TagManagerDialog`.
   - Confirm `WM_NCHITTEST` and `WM_SETCURSOR` are immediately restored to `MainWindow`, edge resizing works 100%, and cursor does not stick in `PointingHandCursor`.
