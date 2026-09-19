# ViewDragDropHelper Event Filter Signal Routing & ShellHelper Fix Plan

## 1. Overview
This implementation plan fixes drag-and-drop item migration failure when dragging items (e.g., images) onto folder targets in `ContentPanel` / `SectionedScrollCanvas`.

It addresses two root causes:
1. **Signal Disconnect in `DragDropEventFilter`**: When `DragDropEventFilter` handled `QEvent::Drop` and emitted `pathsDropped(paths, targetIdx)`, the signal was emitted on `DragDropEventFilter` rather than `m_targetView`. Because `DropJustifiedView` / `DropTreeView` / `DropListView` relied on their own QObject `pathsDropped` signals, `SectionedScrollCanvas` subscriber lambdas were never triggered.
2. **Win32 `SHFILEOPSTRUCTW.fFlags` Flag Issue**: In `ShellHelper::copyOrMoveItems`, `FOF_MULTIDESTFILES` was passed while processing individual files sequentially, causing Windows `SHFileOperationW` to return error codes and abort file moves.

## 2. Modified Files List
- `src/ui/ViewDragDropHelper.cpp`
- `src/util/ShellHelper.cpp`
- `QuarkMeta Architecture/Implementation Plan/ViewDragDropHelper-3.md`

## 3. Detailed Line-by-Line Changes

### `src/ui/ViewDragDropHelper.cpp`

```
<<<<<<< SEARCH
void DragDropEventFilter::install(QAbstractItemView* view) {
    if (!view) return;
    view->setAcceptDrops(true);
    auto* filter = new DragDropEventFilter(view, view);
    view->installEventFilter(filter);
    if (view->viewport()) {
        view->viewport()->installEventFilter(filter);
    }
}
=======
void DragDropEventFilter::install(QAbstractItemView* view) {
    if (!view) return;
    view->setAcceptDrops(true);
    auto* filter = new DragDropEventFilter(view, view);
    view->installEventFilter(filter);
    if (view->viewport()) {
        view->viewport()->installEventFilter(filter);
    }

    // 🚀【核心修复】：将事件过滤器的 pathsDropped 动态信号直接桥接至宿主视图的 pathsDropped 信号
    QObject::connect(filter, SIGNAL(pathsDropped(QStringList,QModelIndex)),
                     view, SIGNAL(pathsDropped(QStringList,QModelIndex)));
}
>>>>>>> REPLACE
```

### `src/util/ShellHelper.cpp`

```
<<<<<<< SEARCH
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_SILENT | FOF_MULTIDESTFILES;
=======
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_SILENT;
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build via CMake build target `QuarkMeta`:
   ```bash
   cmake --build --preset x64-Debug --target QuarkMeta
   ```
2. Drag files (e.g. PNG/AI images) onto a folder card in Grid or List view mode.
3. Confirm files are moved seamlessly into the target folder and the current view is refreshed automatically.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Signal Forwarding**: Directly connects `DragDropEventFilter::pathsDropped` to `view`'s existing signal interface without adding new duplicate events or modifying public `.h` API contracts.

## 6. Header API Signature Verification
| Class | Function / Member | Header File | Signature Verification |
|---|---|---|---|
| `DragDropEventFilter` | `pathsDropped(const QStringList&, const QModelIndex&)` | `ViewDragDropHelper.h` | `void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);` |
| `DropJustifiedView` | `pathsDropped(const QStringList&, const QModelIndex&)` | `DropJustifiedView.h` | `void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);` |
| `DropTreeView` | `pathsDropped(const QStringList&, const QModelIndex&)` | `DropTreeView.h` | `void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);` |
| `DropListView` | `pathsDropped(const QStringList&, const QModelIndex&)` | `DropListView.h` | `void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);` |
