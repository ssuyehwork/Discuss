# UiHelper Implementation Plan - LineEdit Escape Clear Key Filter Standard

This implementation plan details the custom event filter extension in `UiHelper` (`src/ui/UiHelper.h` / `src/ui/UiHelper.cpp`) to handle `Qt::Key_Escape` on `QLineEdit` widgets across the application.

---

## 1. Overview & Problem Statement
* **Issue**: Pressing `Esc` inside text input fields (`QLineEdit`) does not clear non-empty text; instead, the `Esc` key bubbles up to parent windows (such as `FramelessDialog` or `QuickLookWindow`), accidentally closing the window or popup.
* **Solution**: Implement `UiHelper::attachEscClearFilter(QLineEdit* edit)` using an event filter that intercepts `QEvent::KeyPress` with `Qt::Key_Escape`. If `!edit->text().isEmpty()`, it clears text (`edit->clear()`) and consumes the event (`return true;`). If `edit->text().isEmpty()`, it lets the event bubble up (`return false;`).

---

## 2. Modified Files List
- `src/ui/UiHelper.h`
- `src/ui/UiHelper.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/UiHelper.h`

```git
<<<<<<< SEARCH
    static void setupLineEditContextMenu(QLineEdit* edit);
=======
    static void setupLineEditContextMenu(QLineEdit* edit);
    static void attachEscClearFilter(QLineEdit* edit);
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/UiHelper.cpp`

```git
<<<<<<< SEARCH
        menu.exec(edit->mapToGlobal(pos));
    });
}

} // namespace QuarkMeta
=======
        menu.exec(edit->mapToGlobal(pos));
    });
}

class EscClearEventFilter : public QObject {
public:
    explicit EscClearEventFilter(QLineEdit* edit) : QObject(edit), m_edit(edit) {}
protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEv = static_cast<QKeyEvent*>(event);
            if (keyEv->key() == Qt::Key_Escape) {
                if (m_edit && !m_edit->text().isEmpty()) {
                    m_edit->clear();
                    return true;
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }
private:
    QLineEdit* m_edit;
};

void UiHelper::attachEscClearFilter(QLineEdit* edit) {
    if (!edit) return;
    edit->installEventFilter(new EscClearEventFilter(edit));
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Verify `UiHelper::attachEscClearFilter` intercepts `Qt::Key_Escape` and clears text when `!edit->text().isEmpty()`.
2. Confirm `UiHelper-2.md` is present in `QuarkMeta Architecture/Implementation Plan/`.
3. Confirm Chapter 4 in `QuarkMeta-Architecture-Planning.md` contains the LineEdit Escape Clear Contract.
