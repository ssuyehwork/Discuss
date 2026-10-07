# Implementation Plan - MetaPanel Crash Fix (`MetaPanel.md`)

## 1. Overview
When a user selects items in the `ContentPanel` and then clicks a star button (rating) or color button (color label) in `MetaPanel`, or edits the `m_linkEdit` / `m_noteEdit` fields, a crash occurs.

### Crash Root Causes Analysis
1. **FocusOut Re-entrancy Crash**:
   When `m_linkEdit` or `m_noteEdit` loses focus (`FocusOut`), `MetaPanel::eventFilter` synchronously emitted `linkEdited` / `noteEdited` signals. This triggered `CoreEngine::executeCommand` -> `MetadataManager` -> `CentralEventHub` -> `ContentPanel::updateItemMetadata`, while Qt's `FocusOut` event loop was still active on the widget stack. Re-entering UI updates during `FocusOut` caused Access Violation crashes.
2. **Synchronous Signal Self-Reentrancy**:
   Clicking a star or color button invoked `MetaPanel::setRating` / `MetaPanel::setColor` with `fromUser = true`. This emitted `ratingChanged` / `colorChanged` synchronously, which traveled through `PanelMediator` -> `CoreEngine` -> `CentralEventHub` -> `PanelMediator` subscriber -> back into `MetaPanel::setRating(..., false)` / `setColor(..., false)` synchronously while the button click / stylesheet unpolish/polish loop was still running.

### Proposed Solution
1. **Asynchronous Microtask Dispatching**:
   Wrap `linkEdited`, `noteEdited`, `colorChanged`, and `ratingChanged` signal emissions in `QTimer::singleShot(0, ...)` microtasks inside `MetaPanel.cpp`. This ensures Qt completes its active event loop / button click handler before dispatching metadata change signals to the rest of the application.
2. **Editing State Atomicity Protection**:
   In `MetaPanel::setNote` and `MetaPanel::setURL`, check `if (m_isUserEditing) return;` to prevent background updates from overwriting UI state during active user editing.

---

## 2. Modified Files List
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: Asynchronous Dispatch for `noteEdited` and `linkEdited` in `eventFilter`
In `src/ui/MetaPanel.cpp`, wrap `emit noteEdited` and `emit linkEdited` in `QTimer::singleShot(0, ...)`:

```
<<<<<<< SEARCH
    if (watched == m_noteEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            emit noteEdited(m_editingPathsSnapshot, m_noteEdit->toPlainText());
        }
    } else if (watched == m_linkEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            emit linkEdited(m_editingPathsSnapshot, m_linkEdit->text().trimmed());
        }
    }
=======
    if (watched == m_noteEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            QStringList pathsCopy = m_editingPathsSnapshot;
            QString textCopy = m_noteEdit->toPlainText();
            QTimer::singleShot(0, this, [this, pathsCopy, textCopy]() {
                emit noteEdited(pathsCopy, textCopy);
            });
        }
    } else if (watched == m_linkEdit && event->type() == QEvent::FocusOut) {
        if (!m_editingPathsSnapshot.isEmpty()) {
            QStringList pathsCopy = m_editingPathsSnapshot;
            QString linkCopy = m_linkEdit->text().trimmed();
            QTimer::singleShot(0, this, [this, pathsCopy, linkCopy]() {
                emit linkEdited(pathsCopy, linkCopy);
            });
        }
    }
>>>>>>> REPLACE
```

### Change 2: Asynchronous Dispatch for `ratingChanged` and `colorChanged`
In `src/ui/MetaPanel.cpp`, wrap `emit ratingChanged` and `emit colorChanged` in `QTimer::singleShot(0, ...)` to break synchronous re-entrancy loops:

```
<<<<<<< SEARCH
    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        emit ratingChanged(m_selectedPaths, rating);
    }
}

void MetaPanel::setColor(const QString& hexColor, bool fromUser) {
    m_currentColorHex = hexColor;

    for (QPushButton* btn : m_colorBtns) {
        QString hex = btn->property("hexColor").toString();
        bool active = (!hexColor.isEmpty() && hex.compare(hexColor, Qt::CaseInsensitive) == 0);

        btn->setProperty("active", active);
        btn->setStyleSheet(QString("background-color: %1;").arg(hex));
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }

    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        emit colorChanged(m_selectedPaths, hexColor);
    }
}
=======
    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        QStringList pathsCopy = m_selectedPaths;
        QTimer::singleShot(0, this, [this, pathsCopy, rating]() {
            emit ratingChanged(pathsCopy, rating);
        });
    }
}

void MetaPanel::setColor(const QString& hexColor, bool fromUser) {
    m_currentColorHex = hexColor;

    for (QPushButton* btn : m_colorBtns) {
        QString hex = btn->property("hexColor").toString();
        bool active = (!hexColor.isEmpty() && hex.compare(hexColor, Qt::CaseInsensitive) == 0);

        btn->setProperty("active", active);
        btn->setStyleSheet(QString("background-color: %1;").arg(hex));
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }

    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        QStringList pathsCopy = m_selectedPaths;
        QTimer::singleShot(0, this, [this, pathsCopy, hexColor]() {
            emit colorChanged(pathsCopy, hexColor);
        });
    }
}
>>>>>>> REPLACE
```

### Change 3: Atomic User-Editing Guard in `setNote` and `setURL`
In `src/ui/MetaPanel.cpp`, prevent setting text while `m_isUserEditing` is true:

```
<<<<<<< SEARCH
void MetaPanel::setNote(const QString& note) {
    m_isInternalUpdating = true;
    m_noteEdit->setPlainText(note);
=======
void MetaPanel::setNote(const QString& note) {
    if (m_isUserEditing) return;
    m_isInternalUpdating = true;
    m_noteEdit->setPlainText(note);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void MetaPanel::setURL(const QString& url) {
    m_isInternalUpdating = true;
    m_linkEdit->setText(url);
=======
void MetaPanel::setURL(const QString& url) {
    if (m_isUserEditing) return;
    m_isInternalUpdating = true;
    m_linkEdit->setText(url);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute CMake build in project root:
   `cmake -B build && cmake --build build --config Release`
2. Run `QuarkMeta` application.
3. Test Star/Color Rating in `MetaPanel`:
   - Select an item in `ContentPanel`.
   - Click star ratings 1 to 5 and color buttons in `MetaPanel`.
   - Verify smooth update on both `MetaPanel` and `ContentPanel` cards with zero crashes.
4. Test URL & Note FocusOut:
   - Click in `m_linkEdit`, type `https://example.com`, then click a star rating or click another item.
   - Verify focus out handles saving without crash or event filter re-entrancy.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Core Engine Command Reuse**: All metadata changes (`ratingChanged`, `colorChanged`, `noteEdited`, `linkEdited`) continue to route via `PanelMediator` to `CoreEngine::instance().executeCommand(cmd)` with `AppCommandType::SetColor`, `SetRating`, `SetNote`, and `SetURL`.
- **Anti-Redundancy**: No duplicate metadata modifying logic was introduced. Microtask decoupling preserves 100% SSOT compliance.

---

## 6. Header API Signature Verification
- `MetaPanel::ratingChanged(const QStringList& paths, int rating)`: Confirmed in `src/ui/MetaPanel.h:53`.
- `MetaPanel::colorChanged(const QStringList& paths, const QString& hexColor)`: Confirmed in `src/ui/MetaPanel.h:54`.
- `MetaPanel::noteEdited(const QStringList& paths, const QString& newNote)`: Confirmed in `src/ui/MetaPanel.h:57`.
- `MetaPanel::linkEdited(const QStringList& paths, const QString& newLink)`: Confirmed in `src/ui/MetaPanel.h:58`.
- `MetaPanel::setNote(const QString& note)`: Confirmed in `src/ui/MetaPanel.h:46`.
- `MetaPanel::setURL(const QString& url)`: Confirmed in `src/ui/MetaPanel.h:48`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include <QTimer>`: Present in `src/ui/MetaPanel.h:9`.
- `#include <QStringList>`: Included implicitly via `<QFrame>`/`<QString>` and present in Qt header chain.
- All modified signals, classes, and types (`QTimer::singleShot`, `QStringList`, `m_isUserEditing`) have complete header definitions in `MetaPanel.h` and `MetaPanel.cpp`.
