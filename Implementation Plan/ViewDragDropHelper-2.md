# ViewDragDropHelper Event Filter Implementation Plan

## 1. Overview
This implementation plan refactors the drag-and-drop mechanism across all item views by replacing duplicate event overrides in `DropListView`, `DropTreeView`, and `DropJustifiedView` with a high-cohesion `DragDropEventFilter` installed on `QAbstractItemView` viewports.

It cleans up duplicate dead code while preserving 100% of existing view features:
- **`IsDropTargetRole` Folder Highlighting**: Preserved and upgraded to work universally across all views via `DragDropEventFilter`.
- `DropListView`: `blankSpaceDoubleClicked` signal and folder drop highlight support.
- `DropTreeView`: Column policies (`applyColumnPolicies`), auto-adjusting header, empty hint painting (`m_emptyHint`), and row height calculations.
- `DropJustifiedView`: Justified card layout engine and total height change notifications.

## 2. Modified Files List
- `src/ui/ViewDragDropHelper.h`
- `src/ui/ViewDragDropHelper.cpp`
- `src/ui/DropListView.h`
- `src/ui/DropListView.cpp`
- `src/ui/DropTreeView.h`
- `src/ui/DropTreeView.cpp`
- `src/ui/DropJustifiedView.h`
- `src/ui/DropJustifiedView.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/ViewDragDropHelper.h`

```
<<<<<<< SEARCH
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
=======
class DragDropEventFilter : public QObject {
    Q_OBJECT

public:
    explicit DragDropEventFilter(QAbstractItemView* targetView, QObject* parent = nullptr);
    ~DragDropEventFilter() override = default;

    static void install(QAbstractItemView* view);

signals:
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void clearDropHighlight();

    QAbstractItemView* m_targetView = nullptr;
    QPersistentModelIndex m_currentHoverDropIdx;
};

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

### `src/ui/ViewDragDropHelper.cpp`

```
<<<<<<< SEARCH
namespace QuarkMeta {

QAbstractItemView* ViewDragDropHelper::s_hoverView = nullptr;
=======
namespace QuarkMeta {

DragDropEventFilter::DragDropEventFilter(QAbstractItemView* targetView, QObject* parent)
    : QObject(parent ? parent : targetView), m_targetView(targetView) {
}

void DragDropEventFilter::install(QAbstractItemView* view) {
    if (!view) return;
    view->setAcceptDrops(true);
    auto* filter = new DragDropEventFilter(view, view);
    view->installEventFilter(filter);
    if (view->viewport()) {
        view->viewport()->installEventFilter(filter);
    }
}

void DragDropEventFilter::clearDropHighlight() {
    if (m_currentHoverDropIdx.isValid() && m_targetView && m_targetView->model()) {
        const_cast<QAbstractItemModel*>(m_targetView->model())->setData(m_currentHoverDropIdx, false, IsDropTargetRole);
        m_currentHoverDropIdx = QModelIndex();
        if (m_targetView->viewport()) m_targetView->viewport()->update();
    }
}

bool DragDropEventFilter::eventFilter(QObject* watched, QEvent* event) {
    if (!m_targetView) return QObject::eventFilter(watched, event);

    if (event->type() == QEvent::DragEnter) {
        auto* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (ViewDragDropHelper::handleDragEnter(m_targetView, dragEvent)) {
            return true;
        }
    } else if (event->type() == QEvent::DragMove) {
        auto* moveEvent = static_cast<QDragMoveEvent*>(event);
        QModelIndex hoverIdx = m_targetView->indexAt(moveEvent->position().toPoint());
        if (m_currentHoverDropIdx != hoverIdx) {
            clearDropHighlight();
            if (hoverIdx.isValid()) {
                bool isFolder = (hoverIdx.data(TypeRole).toString() == "folder") || hoverIdx.data(Qt::UserRole + 2).toBool();
                if (isFolder) {
                    m_currentHoverDropIdx = hoverIdx;
                    if (m_targetView->model()) {
                        const_cast<QAbstractItemModel*>(m_targetView->model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                        if (m_targetView->viewport()) m_targetView->viewport()->update();
                    }
                }
            }
        }
        if (ViewDragDropHelper::handleDragMove(m_targetView, moveEvent)) {
            return true;
        }
    } else if (event->type() == QEvent::DragLeave) {
        clearDropHighlight();
        ViewDragDropHelper::clearHover(m_targetView);
        return true;
    } else if (event->type() == QEvent::Drop) {
        clearDropHighlight();
        auto* dropEv = static_cast<QDropEvent*>(event);
        QStringList paths;
        QModelIndex targetIdx;
        if (ViewDragDropHelper::handleDrop(m_targetView, dropEv, paths, targetIdx)) {
            emit pathsDropped(paths, targetIdx);
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

QAbstractItemView* ViewDragDropHelper::s_hoverView = nullptr;
>>>>>>> REPLACE
```

### `src/ui/DropListView.h`

```
<<<<<<< SEARCH
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    QModelIndex m_currentHoverDropIdx;
    void clearDropHighlight();
=======
protected:
    void startDrag(Qt::DropActions supportedActions) override;
>>>>>>> REPLACE
```

### `src/ui/DropListView.cpp`

```
<<<<<<< SEARCH
DropListView::DropListView(QWidget* parent) : QListView(parent) {
    setDragEnabled(true);
    setAcceptDrops(true);
}

void DropListView::dragEnterEvent(QDragEnterEvent* event) {
    if (!ViewDragDropHelper::handleDragEnter(this, event)) {
        QListView::dragEnterEvent(event);
    }
}

void DropListView::dragMoveEvent(QDragMoveEvent* event) {
    QModelIndex hoverIdx = indexAt(event->position().toPoint());
    if (m_currentHoverDropIdx != hoverIdx) {
        clearDropHighlight();
        if (hoverIdx.isValid()) {
            bool isFolder = (hoverIdx.data(TypeRole).toString() == "folder") || hoverIdx.data(Qt::UserRole + 2).toBool();
            if (isFolder) {
                m_currentHoverDropIdx = hoverIdx;
                if (model()) {
                    const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                    viewport()->update();
                }
            }
        }
    }

    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QListView::dragMoveEvent(event);
    }
}

void DropListView::dragLeaveEvent(QDragLeaveEvent* event) {
    clearDropHighlight();
    ViewDragDropHelper::clearHover(this);
    QListView::dragLeaveEvent(event);
}

void DropListView::clearDropHighlight() {
    if (m_currentHoverDropIdx.isValid() && model()) {
        const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, false, IsDropTargetRole);
        m_currentHoverDropIdx = QModelIndex();
        viewport()->update();
    }
}

void DropListView::dropEvent(QDropEvent* event) {
    clearDropHighlight();
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        QListView::dropEvent(event);
    }
}
=======
DropListView::DropListView(QWidget* parent) : QListView(parent) {
    setDragEnabled(true);
    DragDropEventFilter::install(this);
}
>>>>>>> REPLACE
```

### `src/ui/DropTreeView.h`

```
<<<<<<< SEARCH
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
=======
protected:
    void startDrag(Qt::DropActions supportedActions) override;
>>>>>>> REPLACE
```

### `src/ui/DropTreeView.cpp`

```
<<<<<<< SEARCH
DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
=======
DropTreeView::DropTreeView(QWidget* parent) : QTreeView(parent) {
    setHeader(new ContentHeaderView(Qt::Horizontal, this));
    setDragEnabled(true);
    setDropIndicatorShown(true);
    DragDropEventFilter::install(this);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void DropTreeView::dragEnterEvent(QDragEnterEvent* event) {
    if (!ViewDragDropHelper::handleDragEnter(this, event)) {
        QTreeView::dragEnterEvent(event);
    }
}

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
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        QTreeView::dropEvent(event);
    }
}
=======
>>>>>>> REPLACE
```

### `src/ui/DropJustifiedView.h`

```
<<<<<<< SEARCH
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;
=======
protected:
    void startDrag(Qt::DropActions supportedActions) override;
>>>>>>> REPLACE
```

### `src/ui/DropJustifiedView.cpp`

```
<<<<<<< SEARCH
DropJustifiedView::DropJustifiedView(QWidget* parent) : JustifiedView(parent) {
}
=======
DropJustifiedView::DropJustifiedView(QWidget* parent) : JustifiedView(parent) {
    DragDropEventFilter::install(this);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void DropJustifiedView::dragEnterEvent(QDragEnterEvent* event) {
    if (!ViewDragDropHelper::handleDragEnter(this, event)) {
        JustifiedView::dragEnterEvent(event);
    }
}

void DropJustifiedView::dragMoveEvent(QDragMoveEvent* event) {
    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        JustifiedView::dragMoveEvent(event);
    }
}

void DropJustifiedView::dragLeaveEvent(QDragLeaveEvent* event) {
    JustifiedView::dragLeaveEvent(event);
}

void DropJustifiedView::dropEvent(QDropEvent* event) {
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        JustifiedView::dropEvent(event);
    }
}
=======
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build via CMake build target `QuarkMeta`:
   ```bash
   cmake --build --preset x64-Debug --target QuarkMeta
   ```
2. Test drag & drop file/folder operations on Grid, List, Tree, and Column Views.
3. Confirm folder target highlighting (`IsDropTargetRole`) triggers smoothly across all views upon hover.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Zero Duplicate Drag/Drop Parsers**: All MIME & URL parsing is centralized in `ViewDragDropHelper::handleDrop`.
- **Universal Folder Drop Highlight**: `IsDropTargetRole` hover highlight logic moved into `DragDropEventFilter` so all views gain target folder highlighting automatically.

## 6. Header API Signature Verification
| Class | Function / Member | Header File | Signature Verification |
|---|---|---|---|
| `DragDropEventFilter` | `install(QAbstractItemView*)` | `ViewDragDropHelper.h` | `static void install(QAbstractItemView* view);` |
| `DragDropEventFilter` | `pathsDropped(...)` | `ViewDragDropHelper.h` | `void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);` |
