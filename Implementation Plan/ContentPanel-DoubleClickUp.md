# Implementation Plan - ContentPanel Blank Area Double Click Go Up

## 1. Overview
This implementation plan adds the functionality to navigate to the parent directory (`NavigationService::instance().goUp()`) when double clicking on the blank area of the content panel view.

Specifically:
1. Intercept `QEvent::MouseButtonDblClick` inside `ContentPanel::eventFilter(QObject* obj, QEvent* event)`.
2. Check if the event source is the viewport of `m_gridView`, `m_treeView`, or `m_columnView`.
3. Verify whether the click position (`event->pos()`) resolves to a valid index using `view->indexAt(mouseEvent->pos())`.
4. If `!index.isValid()` (meaning double clicked on blank area), trigger `NavigationService::instance().goUp()` and consume the event by returning `true`.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentPanel.cpp`
Update `ContentPanel::eventFilter`:

<<<<<<< SEARCH
#include "../meta/MetadataManager.h"
=======
#include "../meta/MetadataManager.h"
#include "../core/NavigationService.h"
#include <QMouseEvent>
>>>>>>> REPLACE

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
            QAbstractItemView* view = nullptr;
            if (m_gridView && (obj == m_gridView || obj == m_gridView->viewport())) {
                view = m_gridView;
            } else if (m_treeView && (obj == m_treeView || obj == m_treeView->viewport())) {
                view = m_treeView;
            }
            if (view) {
                QModelIndex idx = view->indexAt(mouseEvent->pos());
                if (!idx.isValid()) {
                    NavigationService::instance().goUp();
                    return true;
                }
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
2. Launch QuarkMeta app and navigate to any subfolder (e.g. `g:\c++\quarkmeta`).
3. Double-click on the blank space in the Grid/List view.
4. Verify that the app navigates up to the parent directory (e.g. `g:\c++`).
5. Double-click directly on a file or folder item, verifying normal open behavior still works.
