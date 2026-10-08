# CreateCopyActionRefinement.md Implementation Plan

## 1. Overview
Refines duplicate naming convention and adds drag-distance safety guard for `Ctrl + Drag` in QuarkMeta:
1. **Naming Convention Refinement (`ShellHelper.cpp`)**: Changes auto-increment naming pattern from `Filename (1).ext` to `Filename-1.ext`.
2. **`Ctrl + Drag` Distance Guard (`ViewDragDropHelper.h` / `ViewDragDropHelper.cpp` / `ContentFileOpsHandler.cpp`)**:
   - Records drag start position `m_dragStartPos` during `executeStartDrag`.
   - Records drop position during `handleDrop` and checks `manhattanLength() >= 100` pixels when `Ctrl` key is pressed for in-place copy operations.
   - If `Ctrl + Drag` distance is under 100px (e.g. accidental drag during Ctrl+click multi-selection), silently ignores/cancels the drop to prevent unwanted duplicate creation.

## 2. Modified Files List
- `src/util/ShellHelper.cpp`
- `src/ui/ViewDragDropHelper.h`
- `src/ui/ViewDragDropHelper.cpp`
- `src/ui/controllers/ContentFileOpsHandler.cpp`

## 3. Detailed Line-by-Line Changes

```path
src/util/ShellHelper.cpp
```

<<<<<<< SEARCH
                while (QFile::exists(destPath)) {
                    QString newFileName = suffix.isEmpty() 
                        ? QString("%1 (%2)").arg(baseName).arg(counter++)
                        : QString("%1 (%2).%3").arg(baseName).arg(counter++).arg(suffix);
                    destPath = QDir(destDir).filePath(newFileName);
                }
=======
                while (QFile::exists(destPath)) {
                    QString newFileName = suffix.isEmpty() 
                        ? QString("%1-%2").arg(baseName).arg(counter++)
                        : QString("%1-%2.%3").arg(baseName).arg(counter++).arg(suffix);
                    destPath = QDir(destDir).filePath(newFileName);
                }
>>>>>>> REPLACE

```path
src/ui/ViewDragDropHelper.h
```

<<<<<<< SEARCH
    static bool handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx);
    static void executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions);
=======
    static bool handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx, QPoint* outStartPos = nullptr);
    static void executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions);
    static QPoint lastDragStartPos() { return s_lastDragStartPos; }

private:
    static QPoint s_lastDragStartPos;
>>>>>>> REPLACE

```path
src/ui/ViewDragDropHelper.cpp
```

<<<<<<< SEARCH
QAbstractItemView* ViewDragDropHelper::s_hoverView = nullptr;
QPersistentModelIndex ViewDragDropHelper::s_hoverIndex;
=======
QAbstractItemView* ViewDragDropHelper::s_hoverView = nullptr;
QPersistentModelIndex ViewDragDropHelper::s_hoverIndex;
QPoint ViewDragDropHelper::s_lastDragStartPos;
>>>>>>> REPLACE

<<<<<<< SEARCH
bool ViewDragDropHelper::handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx) {
    outPaths.clear();
    outTargetIdx = QModelIndex();

    if (event->mimeData() && event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            QString localPath = url.toLocalFile();
            if (!localPath.isEmpty()) {
                outPaths.append(QDir::toNativeSeparators(localPath));
            }
        }
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

void ViewDragDropHelper::executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions) {
=======
bool ViewDragDropHelper::handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx, QPoint* outStartPos) {
    outPaths.clear();
    outTargetIdx = QModelIndex();
    if (outStartPos) *outStartPos = s_lastDragStartPos;

    if (event->mimeData() && event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            QString localPath = url.toLocalFile();
            if (!localPath.isEmpty()) {
                outPaths.append(QDir::toNativeSeparators(localPath));
            }
        }
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

void ViewDragDropHelper::executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions) {
    if (view && view->viewport()) {
        s_lastDragStartPos = view->viewport()->mapFromGlobal(QCursor::pos());
    } else {
        s_lastDragStartPos = QCursor::pos();
    }
>>>>>>> REPLACE

```path
src/ui/controllers/ContentFileOpsHandler.cpp
```

<<<<<<< SEARCH
    // 0. 原地/同目录拖放保护：剔除源目录与目标目录一模一样的项目
    QStringList externalPaths;
    for (const QString& src : paths) {
        QFileInfo srcInfo(src);
        if (QDir::cleanPath(srcInfo.absolutePath()) != QDir::cleanPath(QDir(destDir).absolutePath())) {
            externalPaths.append(src);
        }
    }

    if (externalPaths.isEmpty()) {
        // 全为同目录内自拖放，直接静默恢复/忽略，绝不误触同名冲突弹窗
        return;
    }
=======
    // 0. 原地/同目录拖放保护与 Ctrl+Drag 100px 距门禁
    bool isCtrlPressed = (QApplication::keyboardModifiers() & Qt::ControlModifier);
    QStringList externalPaths;

    for (const QString& src : paths) {
        QFileInfo srcInfo(src);
        bool isSameDirectory = (QDir::cleanPath(srcInfo.absolutePath()) == QDir::cleanPath(QDir(destDir).absolutePath()));

        if (isSameDirectory) {
            if (isCtrlPressed) {
                // 判断 Ctrl+Drag 拖拽物理距离，防止 Ctrl+Click 多选误触
                QPoint dragStartPos = ViewDragDropHelper::lastDragStartPos();
                QPoint dropPos = QCursor::pos();
                int dragDistance = (dropPos - dragStartPos).manhattanLength();

                if (dragDistance >= 100) {
                    externalPaths.append(src);
                }
            }
        } else {
            externalPaths.append(src);
        }
    }

    if (externalPaths.isEmpty()) {
        // 全为同目录内自拖放且未触发 Ctrl+Drag 100px 门禁，静默处理
        return;
    }
>>>>>>> REPLACE

## 4. Build & Verification Steps
1. Run CMake build / compile script to compile the project.
2. Launch QuarkMeta and test right-click "创建副本":
   - Confirm new files are named `Filename-1.ext`, `Filename-2.ext` instead of `Filename (1).ext`.
3. Test `Ctrl + Click` multi-selection:
   - Hold `Ctrl` and click multiple items without moving mouse >= 100px. Confirm no unwanted duplicate files are generated.
4. Test `Ctrl + Drag` duplicate creation:
   - Hold `Ctrl` and drag a file >= 100px across the viewport.
   - Confirm a copy named `Filename-1.ext` is created upon drop.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `ViewDragDropHelper` and `ShellHelper::copyOrMoveItems`.
- Reused `m_panel->refreshAll()` for in-place view update.

## 6. Header API Signature Verification
- `ShellHelper::copyOrMoveItems(...)`: Declared in `ShellHelper.h`.
- `ViewDragDropHelper::lastDragStartPos()`: Declared in `ViewDragDropHelper.h`.

## 7. Header Inclusion Chain & Type Completeness Check
- `ViewDragDropHelper.cpp` includes `<QCursor>`.
- Type completeness is 100% verified.
