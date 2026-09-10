# FramelessDialog - Shortcut Deduplication & Architecture Cleaning Plan (FramelessDialog-1.md)

## 1. Overview
Removes redundant local `QShortcut(Ctrl+W)` and duplicate `keyPressEvent` checks inside `FramelessDialog` and `TagManagerDialog`. Because `AppShortcutController` and top-level event interception already handle global `Ctrl+W` active window closing, having multiple redundant shortcut listeners inside `FramelessDialog` and `TagManagerDialog` creates unnecessary handler overlap and code duplication.

## 2. Modified Files List
- `src/ui/FramelessDialog.cpp`
- `src/ui/TagManagerDialog.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FramelessDialog.cpp`
Remove duplicate `QShortcut` allocation in `FramelessDialog::FramelessDialog`.

```git
<<<<<<< SEARCH
    QShortcut* scClose = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    scClose->setContext(Qt::WindowShortcut);
    connect(scClose, &QShortcut::activated, this, &QDialog::reject);
}
=======
}
>>>>>>> REPLACE
```

### `src/ui/TagManagerDialog.cpp`
Remove redundant `Ctrl+W` check in `TagManagerDialog::keyPressEvent`, delegating clean window close handling to `FramelessDialog`.

```git
<<<<<<< SEARCH
    if (event->key() == Qt::Key_W && (event->modifiers() & Qt::ControlModifier)) {
        close();
        return;
    }
    FramelessDialog::keyPressEvent(event);
=======
    FramelessDialog::keyPressEvent(event);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
```bash
cmake --build build
```
Verify pressing `Ctrl+W` on `TagManagerDialog` or any `FramelessDialog` smoothly closes the dialog without regression.
