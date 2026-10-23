# ViewDragDropHelper Event Filter Implementation Plan

## 1. Overview
This implementation plan refactors the drag-and-drop mechanism across all item views by replacing duplicate derived subclass implementations (`DropListView`, `DropTreeView`, `DropJustifiedView`) with a high-cohesion, unified `DragDropEventFilter` installed directly onto item views or their viewports.

This eliminates code duplication across item view subclasses while ensuring 100% feature parity for path drop handling, drag hover highlighting, and drag initiation.

## 2. Modified Files List
- `src/ui/ViewDragDropHelper.h`
- `src/ui/ViewDragDropHelper.cpp`
- `src/ui/SectionedScrollCanvas.cpp`
- `src/ui/ColumnViewWidget.cpp`

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
    QAbstractItemView* m_targetView = nullptr;
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

bool DragDropEventFilter::eventFilter(QObject* watched, QEvent* event) {
    if (!m_targetView) return QObject::eventFilter(watched, event);

    if (event->type() == QEvent::DragEnter) {
        auto* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (ViewDragDropHelper::handleDragEnter(m_targetView, dragEvent)) {
            return true;
        }
    } else if (event->type() == QEvent::DragMove) {
        auto* moveEvent = static_cast<QDragMoveEvent*>(event);
        if (ViewDragDropHelper::handleDragMove(m_targetView, moveEvent)) {
            return true;
        }
    } else if (event->type() == QEvent::DragLeave) {
        ViewDragDropHelper::clearHover(m_targetView);
        return true;
    } else if (event->type() == QEvent::Drop) {
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

## 4. Build & Verification Steps
1. Build via CMake:
   ```bash
   cmake --build --preset x64-Debug --target QuarkMeta
   ```
2. Verify drag and drop functionality on GridView, ListView, TreeView, and ColumnView.
3. Confirm drag target hover highlights operate identically without drop subclass duplication.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Event Filter Architecture**: Conforms to Section 4.2 (Event Filter Restraint & Object Scope). Filter is strictly targeted to `QAbstractItemView` and its viewport.
- **`ViewDragDropHelper` SSOT**: Single static helper handles drag drop parsing.

## 6. Header API Signature Verification
| Class | Function / Member | Header File | Signature Verification |
|---|---|---|---|
| `DragDropEventFilter` | `install(QAbstractItemView*)` | `ViewDragDropHelper.h` | `static void install(QAbstractItemView* view);` |
| `DragDropEventFilter` | `pathsDropped(...)` | `ViewDragDropHelper.h` | `void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);` |
| `ViewDragDropHelper` | `handleDrop(...)` | `ViewDragDropHelper.h` | `static bool handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx);` |
