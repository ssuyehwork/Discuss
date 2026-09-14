# Implementation Plan Supplement - ContentPanel-DoubleClickUp-1 (Column View Support)

## 1. Overview
This implementation plan supplement extends the blank area double-click "Go Up" feature to full coverage including Column View mode (`ColumnViewWidget` and `ColumnViewPane`).

Specifically:
1. Refactor `ContentPanel::eventFilter` to dynamically cast `obj` or `obj->parent()` to `QAbstractItemView*`. This automatically handles `m_gridView`, `m_treeView`, and all `QListView` instances inside `ColumnViewPane`s without hardcoding specific view pointers.
2. Install event filter on `m_columnView` and `m_columnView->viewport()` in `ContentPanel.cpp`.
3. If double-clicked on the blank area of any column pane or on `m_columnView` background, trigger `NavigationService::instance().goUp()`.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.cpp`

Install event filter on `m_columnView`:

<<<<<<< SEARCH
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
=======
    m_columnView = new ColumnViewWidget(this, this);
    m_columnView->installEventFilter(this);
    if (m_columnView->viewport()) {
        m_columnView->viewport()->installEventFilter(this);
    }
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
>>>>>>> REPLACE

Update `ContentPanel::eventFilter` to support Column View and generic `QAbstractItemView` instances:

<<<<<<< SEARCH
bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    if (m_keyHandler && m_keyHandler->handleEvent(obj, event)) return true;
    return QFrame::eventFilter(obj, event);
}
=======
bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    if (event && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent && mouseEvent->button() == Qt::LeftButton) {
            QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
            if (!view && obj) {
                view = qobject_cast<QAbstractItemView*>(obj->parent());
            }

            if (view) {
                QModelIndex idx = view->indexAt(mouseEvent->pos());
                if (!idx.isValid()) {
                    NavigationService::instance().goUp();
                    return true;
                }
            } else if (m_columnView && (obj == m_columnView || obj == m_columnView->viewport())) {
                NavigationService::instance().goUp();
                return true;
            }
        }
    }

    if (m_keyHandler && m_keyHandler->handleEvent(obj, event)) return true;
    return QFrame::eventFilter(obj, event);
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`
2. Launch QuarkMeta app and switch to Column View mode.
3. Double-click on the blank space inside any column pane, or on the scroll area background of Column View.
4. Verify that the app navigates up to the parent directory (`NavigationService::instance().goUp()`).
5. Double-click on an item in Column View to verify normal selection/navigation still works.
