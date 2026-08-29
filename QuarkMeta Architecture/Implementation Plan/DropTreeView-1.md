# Implementation Plan - DropTreeView-1

This implementation plan fixes a critical crash in `DropTreeView` during internal drag-and-drop item reordering (e.g. inside `FavoritePanel`).

## 1. Overview
- **Root Cause Analysis**: `DropTreeView::startDrag` injected physical path URLs into `QMimeData`. When dragging items internally to reorder, `dropEvent` misidentified internal drags as external path drops (`hasUrls() == true`), triggered `pathsDropped`, deleted source rows, and corrupted model row indexes, resulting in NULL pointer dereferences and crashes during downward moves.
- **Isolate Internal Drag-and-Drop**: Add `event->source() != this` check to `dragEnterEvent`, `dragMoveEvent`, and `dropEvent`. Internal drags will bypass custom external file import handling and fall back to `QTreeView::dropEvent(event)` for safe `QStandardItemModel` row moving.

## 2. Modified Files List
- `src/ui/DropTreeView.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/DropTreeView.cpp`
```diff
<<<<<<< SEARCH
void DropTreeView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

void DropTreeView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        // 物理同步：显式调用基类逻辑以激活放置指示器 (Drop Indicator)
        QTreeView::dragMoveEvent(event);
        event->acceptProposedAction();
    } else {
        QTreeView::dragMoveEvent(event);
    }
}

void DropTreeView::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QStringList paths;
        for (const QUrl& u : event->mimeData()->urls()) {
            if (u.isLocalFile()) {
                paths << QDir::toNativeSeparators(u.toLocalFile());
            }
        }
        QModelIndex idx = indexAt(event->position().toPoint());
        if (!paths.isEmpty()) {
            emit pathsDropped(paths, idx);
        }
        event->acceptProposedAction();
    } else {
        QTreeView::dropEvent(event);
    }
}
=======
void DropTreeView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->source() != this && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

void DropTreeView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->source() != this && event->mimeData()->hasUrls()) {
        // 物理同步：显式调用基类逻辑以激活放置指示器 (Drop Indicator)
        QTreeView::dragMoveEvent(event);
        event->acceptProposedAction();
    } else {
        QTreeView::dragMoveEvent(event);
    }
}

void DropTreeView::dropEvent(QDropEvent* event) {
    if (event->source() != this && event->mimeData()->hasUrls()) {
        QStringList paths;
        for (const QUrl& u : event->mimeData()->urls()) {
            if (u.isLocalFile()) {
                paths << QDir::toNativeSeparators(u.toLocalFile());
            }
        }
        QModelIndex idx = indexAt(event->position().toPoint());
        if (!paths.isEmpty()) {
            emit pathsDropped(paths, idx);
        }
        event->acceptProposedAction();
    } else {
        QTreeView::dropEvent(event);
    }
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the project using CMake:
   ```bash
   cmake -B build
   cmake --build build
   ```
2. Test internal reordering in `FavoritePanel`:
   - Drag a favorite item downwards: Verify row position moves smoothly and sort orders save to SQLite `global.db` without crash.
