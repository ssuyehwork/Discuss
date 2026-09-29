# Implementation Plan - Fixing Inline Rename Focus on Item Creation (`ContentFileOpsHandler-InlineRenameFix.md`)

## 1. Overview
When a user presses `Ctrl+Shift+N` (or creates a folder/file via context menu), `ContentFileOpsHandler::createNewItem` creates the physical item and calls `m_panel->setPendingSelectName(finalName, true)`. However, because `m_panel->loadDirectory` initiates an asynchronous scan via `QtConcurrent` in `ContentDataLoader`, the model reset and proxy filtering complete after a delay.

When `restoreSelections()` was called immediately during the async transition, the view viewport lacked active focus and proxy mapping was not yet synchronized, causing Qt's `QAbstractItemView::edit(index)` to fail silently.

This implementation plan fixes this timing race condition by:
1. Guaranteeing that the active item view receives keyboard/UI focus (`view->setFocus()`) before triggering `edit(index)`.
2. Postponing `edit(lastIdx)` via a 0ms single-shot timer microtask (`QTimer::singleShot(0, ...)`) inside `ContentPanel::restoreSelections()` and `ContentViewCoordinator::restoreSelections()`, ensuring Qt's event loop completes view layout and proxy model index mapping before opening the inline rename line edit widget.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp` (Ensure view focus and defer `edit(index)` execution)
- `src/ui/controllers/ContentViewCoordinator.cpp` (Defer `edit(index)` execution in coordinator)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
            view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (m_isPendingEdit) view->edit(lastIdx); }
=======
            view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) {
                view->scrollTo(lastIdx);
                if (m_isPendingEdit) {
                    m_isPendingEdit = false;
                    QPointer<QAbstractItemView> weakView(view);
                    QTimer::singleShot(0, this, [weakView, lastIdx]() {
                        if (weakView && lastIdx.isValid()) {
                            weakView->setFocus();
                            weakView->setCurrentIndex(lastIdx);
                            weakView->edit(lastIdx);
                        }
                    });
                }
            }
>>>>>>> REPLACE
```

### 3.2 `src/ui/controllers/ContentViewCoordinator.cpp`
```
<<<<<<< SEARCH
            view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) { view->scrollTo(lastIdx); if (isPendingEdit) view->edit(lastIdx); }
=======
            view->selectionModel()->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastIdx.isValid()) {
                view->scrollTo(lastIdx);
                if (isPendingEdit) {
                    QPointer<QAbstractItemView> weakView(view);
                    QTimer::singleShot(0, m_panel, [weakView, lastIdx]() {
                        if (weakView && lastIdx.isValid()) {
                            weakView->setFocus();
                            weakView->setCurrentIndex(lastIdx);
                            weakView->edit(lastIdx);
                        }
                    });
                }
            }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure CMake build environment:
   ```bash
   cmake -B build -S .
   ```
2. Build executable:
   ```bash
   cmake --build build --config Debug
   ```
3. Run QuarkMeta application.
4. Press `Ctrl+Shift+N` inside any directory.
5. Verify that:
   - A new folder `"新建文件夹"` is created.
   - The view automatically selects and scrolls to the new folder.
   - The inline rename text box opens immediately with all text pre-selected for immediate typing.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Reuse**: Reuses `m_panel->restoreSelections()` and `QAbstractItemView::edit(lastIdx)` without adding second-source state variables.
- **Timing Guarantee**: Utilizes Qt's `QTimer::singleShot(0, ...)` microtask queue to align with layout engine completion.

---

## 6. Header API Signature Verification
- `ContentPanel::setPendingSelectName(const QString& name, bool edit)` -> `src/ui/ContentPanel.h` (Verified)
- `QAbstractItemView::edit(const QModelIndex&)` -> `<QAbstractItemView>` (Verified Qt API)
- `QAbstractItemView::setFocus()` -> `<QWidget>` (Verified Qt API)
