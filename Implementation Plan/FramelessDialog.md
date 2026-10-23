# FramelessDialog Mouse Capture Release Implementation Plan

## Overview
When frameless dialogs (such as `TagManagerDialog`) close, active Win32 mouse capture or Qt mouse grabs can remain held on the closing dialog. This plan overrides `hideEvent` in `FramelessDialog` to explicitly release Win32 mouse capture (`ReleaseCapture()`) and Qt mouse grab (`QWidget::mouseGrabber()`), ensuring `MainWindow` immediately regains non-client event routing (`WM_NCHITTEST`), titlebar dragging, edge resizing, and cursor updates upon dialog closure.

## Modified Files List
- `src/ui/FramelessDialogBase.h`
- `src/ui/FramelessDialog.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FramelessDialogBase.h`
```diff
<<<<<<< SEARCH
#include <QShowEvent>
#include <QMouseEvent>
=======
#include <QShowEvent>
#include <QHideEvent>
#include <QMouseEvent>
>>>>>>> REPLACE
<<<<<<< SEARCH
    void setVisibleButtons(int flags);

protected:
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
=======
    void setVisibleButtons(int flags);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
>>>>>>> REPLACE
```

### `src/ui/FramelessDialog.cpp`
```diff
<<<<<<< SEARCH
void FramelessDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
}
=======
void FramelessDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
}

void FramelessDialog::hideEvent(QHideEvent* event) {
#ifdef Q_OS_WIN
    ::ReleaseCapture();
#endif
    if (QWidget* grabber = QWidget::mouseGrabber()) {
        grabber->releaseMouse();
    }
    QDialog::hideEvent(event);
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Apply diffs to `src/ui/FramelessDialogBase.h` and `src/ui/FramelessDialog.cpp`.
2. Open and close `TagManagerDialog`, and verify `MainWindow` immediately regains titlebar dragging, edge resizing, and cursor updates.
