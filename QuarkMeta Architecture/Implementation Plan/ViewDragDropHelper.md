# Implementation Plan - Drag and Drop Target Highlight Alignment Across All Item Views

## 1. Overview
This implementation plan adds unified drag-and-drop target item highlighting across all 4 item views (Grid View, List View, Tree View, Column View) without modifying the model layer (`setData` / `IsDropTargetRole`).

The state is maintained strictly in `ViewDragDropHelper` as a single static hover reference (`s_hoverView` and `s_hoverIndex`). During `dragMoveEvent`, the hover target is updated and corresponding view viewports are repainted. During `dragLeaveEvent` or `dropEvent`, the hover state is cleared. All Item Delegates (`TreeItemDelegate`, `ThumbnailDelegate`, `ColumnItemDelegate`) check `ViewDragDropHelper::isDropTarget(view, index)` at the very beginning of background painting to render the unified drag target highlight background (`#3498db` with `0.35f` opacity).

---

## 2. Modified Files List
- `src/ui/ViewDragDropHelper.h`
- `src/ui/ViewDragDropHelper.cpp`
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/DropListView.h`
- `src/ui/DropListView.cpp`
- `src/ui/DropJustifiedView.h`
- `src/ui/DropJustifiedView.cpp`
- `src/ui/TreeItemDelegate.h`
- `src/ui/ThumbnailDelegate.cpp`
- `src/ui/ColumnItemDelegate.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ViewDragDropHelper.h`
Add `isDropTarget` and `clearHover` public static methods:

```cpp
<<<<<<< SEARCH
class ViewDragDropHelper {
public:
    static bool handleDragEnter(QAbstractItemView* view, QDragEnterEvent* event);
    static bool handleDragMove(QAbstractItemView* view, QDragMoveEvent* event);
    static bool handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx);
    static void executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions);
};
=======
class ViewDragDropHelper {
public:
    static bool handleDragEnter(QAbstractItemView* view, QDragEnterEvent* event);
    static bool handleDragMove(QAbstractItemView* view, QDragMoveEvent* event);
    static bool handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx);
    static void executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions);

    static bool isDropTarget(const QAbstractItemView* view, const QModelIndex& index);
    static void clearHover(QAbstractItemView* view = nullptr);

private:
    static QAbstractItemView* s_hoverView;
    static QPersistentModelIndex s_hoverIndex;
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/ViewDragDropHelper.cpp`
Implement hover tracking in `handleDragMove`, state clearing in `handleDrop` and `clearHover`, and status query in `isDropTarget`:

```cpp
<<<<<<< SEARCH
namespace QuarkMeta {

bool ViewDragDropHelper::handleDragEnter(QAbstractItemView* /*view*/, QDragEnterEvent* event) {
=======
namespace QuarkMeta {

QAbstractItemView* ViewDragDropHelper::s_hoverView = nullptr;
QPersistentModelIndex ViewDragDropHelper::s_hoverIndex;

bool ViewDragDropHelper::isDropTarget(const QAbstractItemView* view, const QModelIndex& index) {
    return view && s_hoverView == view && s_hoverIndex.isValid() && s_hoverIndex == index;
}

void ViewDragDropHelper::clearHover(QAbstractItemView* view) {
    if (view && s_hoverView != view) return;
    QAbstractItemView* oldView = s_hoverView;
    s_hoverView = nullptr;
    s_hoverIndex = QPersistentModelIndex();
    if (oldView && oldView->viewport()) {
        oldView->viewport()->update();
    }
}

bool ViewDragDropHelper::handleDragEnter(QAbstractItemView* /*view*/, QDragEnterEvent* event) {
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
bool ViewDragDropHelper::handleDragMove(QAbstractItemView* /*view*/, QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return true;
    }
    return false;
}
=======
bool ViewDragDropHelper::handleDragMove(QAbstractItemView* view, QDragMoveEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();

        if (view) {
            QModelIndex newHover = view->indexAt(event->position().toPoint());
            if (s_hoverView != view || s_hoverIndex != newHover) {
                QAbstractItemView* oldView = s_hoverView;
                s_hoverView = view;
                s_hoverIndex = newHover;

                if (oldView && oldView->viewport()) oldView->viewport()->update();
                if (view->viewport()) view->viewport()->update();
            }
        }
        return true;
    }
    return false;
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
        outTargetIdx = view->indexAt(event->position().toPoint());
        if (!outPaths.isEmpty()) {
            event->acceptProposedAction();
            return true;
        }
    }
    return false;
}
=======
        outTargetIdx = view->indexAt(event->position().toPoint());
        if (!outPaths.isEmpty()) {
            event->acceptProposedAction();
            clearHover(view);
            return true;
        }
    }
    clearHover(view);
    return false;
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/DropTreeView.h` & `src/ui/DropTreeView.cpp`
Add `dragLeaveEvent` to `DropTreeView`:

In `DropTreeView.h`:
```cpp
<<<<<<< SEARCH
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
=======
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
>>>>>>> REPLACE
```

In `DropTreeView.cpp`:
```cpp
<<<<<<< SEARCH
void DropTreeView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QTreeView::dragMoveEvent(event);
    }
}

void DropTreeView::dropEvent(QDropEvent* event) {
=======
void DropTreeView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QTreeView::dragMoveEvent(event);
    }
}

void DropTreeView::dragLeaveEvent(QDragLeaveEvent* event) {
    ViewDragDropHelper::clearHover(this);
    QTreeView::dragLeaveEvent(event);
}

void DropTreeView::dropEvent(QDropEvent* event) {
>>>>>>> REPLACE
```

### 3.4 `src/ui/DropListView.h` & `src/ui/DropListView.cpp`
Add `dragLeaveEvent` to `DropListView`:

In `DropListView.h`:
```cpp
<<<<<<< SEARCH
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
=======
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
>>>>>>> REPLACE
```

In `DropListView.cpp`:
```cpp
<<<<<<< SEARCH
void DropListView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QListView::dragMoveEvent(event);
    }
}

void DropListView::dropEvent(QDropEvent* event) {
=======
void DropListView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QListView::dragMoveEvent(event);
    }
}

void DropListView::dragLeaveEvent(QDragLeaveEvent* event) {
    ViewDragDropHelper::clearHover(this);
    QListView::dragLeaveEvent(event);
}

void DropListView::dropEvent(QDropEvent* event) {
>>>>>>> REPLACE
```

### 3.5 `src/ui/DropJustifiedView.h` & `src/ui/DropJustifiedView.cpp`
Add `dragLeaveEvent` to `DropJustifiedView`:

In `DropJustifiedView.h`:
```cpp
<<<<<<< SEARCH
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
=======
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
>>>>>>> REPLACE
```

In `DropJustifiedView.cpp`:
```cpp
<<<<<<< SEARCH
void DropJustifiedView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        JustifiedView::dragMoveEvent(event);
    }
}

void DropJustifiedView::dropEvent(QDropEvent* event) {
=======
void DropJustifiedView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        JustifiedView::dragMoveEvent(event);
    }
}

void DropJustifiedView::dragLeaveEvent(QDragLeaveEvent* event) {
    ViewDragDropHelper::clearHover(this);
    JustifiedView::dragLeaveEvent(event);
}

void DropJustifiedView::dropEvent(QDropEvent* event) {
>>>>>>> REPLACE
```

### 3.6 `src/ui/TreeItemDelegate.h`
Insert `ViewDragDropHelper::isDropTarget` check before selection/hover background painting:

```cpp
<<<<<<< SEARCH
        if (selected) {
            bg = QColor("#378ADD");
            bg.setAlphaF(0.15f);
        } else if (hover) {
            bg = QColor("#2A2D2E");
        } else {
            // 根据控件是否开启斑马纹与行号奇偶精准赋值底色
            bg = (useAlternate && index.row() % 2 == 1) ? QColor("#252526") : QColor("#1E1E1E");
        }
=======
        bool isDropTarget = ViewDragDropHelper::isDropTarget(
            qobject_cast<const QAbstractItemView*>(option.widget), index);

        if (isDropTarget) {
            bg = QColor("#3498db");
            bg.setAlphaF(0.35f);
        } else if (selected) {
            bg = QColor("#378ADD");
            bg.setAlphaF(0.15f);
        } else if (hover) {
            bg = QColor("#2A2D2E");
        } else {
            // 根据控件是否开启斑马纹与行号奇偶精准赋值底色
            bg = (useAlternate && index.row() % 2 == 1) ? QColor("#252526") : QColor("#1E1E1E");
        }
>>>>>>> REPLACE
```

### 3.7 `src/ui/ThumbnailDelegate.cpp`
Insert `ViewDragDropHelper::isDropTarget` check in `paint()`:

```cpp
<<<<<<< SEARCH
    // ② 绘制卡片外边框
    CardPainterHelper::drawCardBorder(painter, l.coverRect, isSelected);
=======
    bool isDropTarget = ViewDragDropHelper::isDropTarget(
        qobject_cast<const QAbstractItemView*>(option.widget), index);

    // ② 绘制卡片外边框
    CardPainterHelper::drawCardBorder(painter, l.coverRect, isDropTarget || isSelected);
>>>>>>> REPLACE
```

### 3.8 `src/ui/ColumnItemDelegate.cpp`
Insert `ViewDragDropHelper::isDropTarget` check in `paint()`:

```cpp
<<<<<<< SEARCH
        if (isSelected) {
            bg = QColor("#378ADD");
            bg.setAlphaF(0.15f);
        } else if (isHovered) {
            bg = QColor("#2A2D2E");
        } else {
            bg = QColor("#1E1E1E");
        }
=======
        bool isDropTarget = ViewDragDropHelper::isDropTarget(
            qobject_cast<const QAbstractItemView*>(option.widget), index);

        if (isDropTarget) {
            bg = QColor("#3498db");
            bg.setAlphaF(0.35f);
        } else if (isSelected) {
            bg = QColor("#378ADD");
            bg.setAlphaF(0.15f);
        } else if (isHovered) {
            bg = QColor("#2A2D2E");
        } else {
            bg = QColor("#1E1E1E");
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile QuarkMeta.
2. Drag a file or folder across Tree View, List View, Grid View, and Column View.
3. Verify that the item directly underneath the mouse cursor highlights with `#3498db` (alpha `0.35f` / `0x59`).
4. Drop or drag outside the view and verify that the highlight clears immediately.
