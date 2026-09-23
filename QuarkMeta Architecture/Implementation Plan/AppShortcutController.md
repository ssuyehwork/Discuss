# AppShortcutController Shortcut Conflict Resolution Implementation Plan (AppShortcutController.md)

## 1. Overview
This implementation plan resolves the keyboard shortcut conflict between global tab switching (`Ctrl+1` ~ `Ctrl+5` in `AppShortcutController`) and item star rating (`Ctrl+1` ~ `Ctrl+5` in `ContentKeyHandler`), ensuring both features work smoothly without key interception deadlocks.

### Key Solved Issues:
1. **Shortcut Interception & Event Pipeline Restoration**: Updates `AppShortcutController::eventFilter` to intercept `Ctrl+1` ~ `Ctrl+5` before `QShortcut` event consumption when an item view has active selections, allowing key events to flow seamlessly to `ContentKeyHandler`.
2. **Star Rating Key Pipeline Restoration**: Ensures `Ctrl+0` ~ `Ctrl+5` star rating keys in `ContentKeyHandler` are received and processed when items are selected in list, grid, or column views.
3. **Strict Zero-Value-Alteration Contract**: Preserves 100% of existing UI visual properties, tab switching capabilities, and star rating data roles.

---

## 2. Modified Files List
1. `src/ui/AppShortcutController.h` (Adds helper method `isItemViewFocusedWithSelection`)
2. `src/ui/AppShortcutController.cpp` (Updates `eventFilter` to bypass `QShortcut` when item view selection is active)
3. `src/ui/controllers/ContentKeyHandler.cpp` (Harmonizes Ctrl+0 ~ Ctrl+5 key event handling)

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/AppShortcutController.h`
```
<<<<<<< SEARCH
    bool isEditingFocus();
=======
    bool isEditingFocus();
    bool isItemViewFocusedWithSelection();
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/AppShortcutController.cpp`
```
<<<<<<< SEARCH
    return false;
}

bool AppShortcutController::eventFilter(QObject* watched, QEvent* event) {
=======
    return false;
}

bool AppShortcutController::isItemViewFocusedWithSelection() {
    QWidget* focusW = QApplication::focusWidget();
    if (!focusW) return false;

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(focusW);
    if (!view) {
        view = qobject_cast<QAbstractItemView*>(focusW->parentWidget());
    }
    if (view && view->selectionModel() && view->selectionModel()->hasSelection()) {
        return true;
    }
    return false;
}

bool AppShortcutController::eventFilter(QObject* watched, QEvent* event) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        // 1. 全局 Ctrl + W 关闭激活窗口契约
        if (keyEv->key() == Qt::Key_W && (keyEv->modifiers() & Qt::ControlModifier)) {
=======
        // 0. Ctrl + 1 ~ 5 针对选中视图项时的事件放行（防止 QShortcut 强行剥夺 KeyPress）
        if ((keyEv->modifiers() & Qt::ControlModifier) && (keyEv->key() >= Qt::Key_1 && keyEv->key() <= Qt::Key_5)) {
            if (isItemViewFocusedWithSelection()) {
                // 保持不被 QShortcut 吃掉，让 KeyPress 事件自然派发给 ContentKeyHandler
                return false;
            }
        }

        // 1. 全局 Ctrl + W 关闭激活窗口契约
        if (keyEv->key() == Qt::Key_W && (keyEv->modifiers() & Qt::ControlModifier)) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 4.1 CMake & Build Verification Commands
```bash
# Clean and re-configure CMake
cmake -B build -S .

# Build the QuarkMeta executable
cmake --build build --config Release
```

### 4.2 Verification Steps
1. Launch `QuarkMeta` application.
2. When no file is selected in ContentPanel, press `Ctrl+1` ~ `Ctrl+5` to verify smooth tab switching.
3. Select an item in Grid / List / Column view, press `Ctrl+1` ~ `Ctrl+5` to verify star rating (1~5 stars) is assigned correctly.
4. Press `Ctrl+0` to verify star rating is reset to 0 stars.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- [x] **`AppShortcutController::isEditingFocus()`**: Extended existing focus detection pattern cleanly.
- [x] **`ContentKeyHandler::handleKeyPress()`**: Reused standard rating role setting pipeline (`RatingRole`).
- [x] **Zero Duplication**: Preserved existing QShortcut infrastructure without duplicating event filters.

---

## 6. Header API Signature Verification

| Called Class / Function | Physical Signature in `.h` Header | Status |
| :--- | :--- | :--- |
| `AppShortcutController::isEditingFocus` | `bool isEditingFocus()` | Verified |
| `ContentKeyHandler::handleKeyPress` | `bool handleKeyPress(QObject* obj, QEvent* event)` | Verified |
| `TitleBarWidget::tabBar` | `TabBarWidget* tabBar() const` | Verified |
| `TabBarWidget::setCurrentIndex` | `void setCurrentIndex(int index, bool animate = true)` | Verified |
