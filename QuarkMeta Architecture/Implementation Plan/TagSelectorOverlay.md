# Implementation Plan - TagSelectorOverlay (Failsafe Cursor & Mouse Grab Release)

## 1. Overview
This implementation plan fixes the mouse cursor sticking in `Qt::PointingHandCursor` when `TagSelectorOverlay` closes or loses focus:
1. **Mouse Grab Release**: Ensures `releaseMouse()` is explicitly called if the overlay held mouse grab.
2. **Cursor Restoration Failsafe**: Calls `QGuiApplication::restoreOverrideCursor()` and forces a Win32 cursor re-evaluation (`QCursor::setPos(QCursor::pos())`) in `closeOverlay()` and `hideEvent()`.

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Override `hideEvent` in TagSelectorOverlay (`src/ui/TagSelectorOverlay.h`)

```
<<<<<<< SEARCH
protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
=======
protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
>>>>>>> REPLACE
```

---

### 3.2 Failsafe Cursor Restoration in TagSelectorOverlay (`src/ui/TagSelectorOverlay.cpp`)

```
<<<<<<< SEARCH
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
    emit overlayClosed();
    close();
    deleteLater();
}
=======
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
    if (this->mouseGrabber() == this) {
        this->releaseMouse();
    }
    emit overlayClosed();
    close();
    QGuiApplication::restoreOverrideCursor();
    QCursor::setPos(QCursor::pos());
    deleteLater();
}

void TagSelectorOverlay::hideEvent(QHideEvent* event) {
    QFrame::hideEvent(event);
    if (this->mouseGrabber() == this) {
        this->releaseMouse();
    }
    QGuiApplication::restoreOverrideCursor();
    QCursor::setPos(QCursor::pos());
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Cursor Verification**:
   - Open `TagSelectorOverlay`, hover over tag pills (cursor becomes hand).
   - Close overlay by clicking outside or pressing Esc.
   - Verify that cursor immediately restores to normal arrow without sticking.
