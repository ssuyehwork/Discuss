# Implementation Plan - TagSelectorOverlay (Clean Non-Hack Overlay Architecture & Native Event Unhooking)

## 1. Overview
This implementation plan addresses the 3 physical-level root causes causing mouse cursor sticking in `PointingHandCursor` and window inactivation:
1. **Immediate Event Filter Unregistration**: Unregister `qApp->removeEventFilter(this)` immediately in `closeOverlay()` instead of waiting for deferred `deleteLater()` destructor call.
2. **Global Child Mouse Grab Release**: Call `QWidget::mouseGrabber()->releaseMouse()` to release implicit mouse grabs on child `QPushButton` controls.
3. **Explicit Top-Window Activation**: Call `topWin->activateWindow()` on closing to overcome Win32 `WS_EX_TOOLWINDOW` activation non-return behavior.
4. **Unhook Native Event Filtering**: Remove `FramelessWindowHelper::apply(this, nullptr)` from `TagSelectorOverlay` to eliminate native event hook conflicts on small overlay windows.
5. **Clean Dialog Return in TagManagerDialog**: Clean up hacky `QCursor::setPos` / `EnableWindow` calls in `TagManagerDialog::showDialog` and rely on standard `activateWindow()` + `setFocus()`.

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/TagManagerDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Unhook FramelessWindowHelper & Update Constructor in TagSelectorOverlay (`src/ui/TagSelectorOverlay.cpp`)

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

    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);
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
>>>>>>> REPLACE
```

---

### 3.2 Remove nativeEvent & FramelessWindowHelper member (`src/ui/TagSelectorOverlay.h`)

```
<<<<<<< SEARCH
    FramelessWindowHelper* m_framelessHelper = nullptr;
};
=======
};
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
=======
>>>>>>> REPLACE
```

---

### 3.3 Clean Overlay Closure in TagSelectorOverlay (`src/ui/TagSelectorOverlay.cpp`)

```
<<<<<<< SEARCH
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;

    if (qApp) {
        qApp->removeEventFilter(this);
    }

    if (QWidget::mouseGrabber()) {
        QWidget::mouseGrabber()->releaseMouse();
    }

    emit overlayClosed();
    close();

    QWidget* topWin = parentWidget() ? parentWidget()->window() : nullptr;
    if (topWin) {
        topWin->activateWindow();
    }

    QGuiApplication::restoreOverrideCursor();
    QCursor::setPos(QCursor::pos());
    deleteLater();
}
=======
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;

    if (qApp) {
        qApp->removeEventFilter(this);
    }

    if (QWidget::mouseGrabber()) {
        QWidget::mouseGrabber()->releaseMouse();
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

---

### 3.4 Clean Dialog Activation in TagManagerDialog (`src/ui/TagManagerDialog.cpp`)

```
<<<<<<< SEARCH
void TagManagerDialog::showDialog(QWidget* parent, const QString& currentPath, bool isMirrorSource) {
    QWidget* topParent = parent ? parent->window() : nullptr;
    TagManagerDialog dlg(currentPath, isMirrorSource, topParent);
    dlg.exec();

    if (topParent) {
#ifdef Q_OS_WIN
        ::EnableWindow(reinterpret_cast<HWND>(topParent->winId()), TRUE);
#endif
        topParent->activateWindow();
        topParent->setFocus();
        QGuiApplication::restoreOverrideCursor();
        QCursor::setPos(QCursor::pos());
    }
}
=======
void TagManagerDialog::showDialog(QWidget* parent, const QString& currentPath, bool isMirrorSource) {
    QWidget* topParent = parent ? parent->window() : nullptr;
    TagManagerDialog dlg(currentPath, isMirrorSource, topParent);
    dlg.exec();

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
   - Open and close `TagSelectorOverlay` and `TagManagerDialog`.
   - Confirm that mouse grab is released, `qApp` event filter is uninstalled immediately, `MainWindow` regains active state, and edge resizing + normal cursor restoration work 100% cleanly without native event hooks interfering.
