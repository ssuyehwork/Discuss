# Implementation Plan - CreateCopyActionRefinement-2.md

## 1. Overview
Fixes conflicts in `Ctrl + Drag` duplicate creation by addressing 4 core causes:
1. **Dynamic `defaultAction` in `QDrag::exec` (`ViewDragDropHelper.cpp`)**: Sets default action to `Qt::CopyAction` when `Ctrl` key is pressed at drag start (`drag->exec(..., Qt::CopyAction)`), allowing Qt's native OLE drag-and-drop mechanism to send `Qt::CopyAction` in `QDropEvent`.
2. **Post-Drag Coordinate Reset (`ViewDragDropHelper.cpp`)**: Clears `s_lastDragStartPos = QPoint()` immediately after `drag->exec(...)` finishes, preventing stale coordinates from affecting subsequent operations.
3. **`DropAction` Propagation (`ViewDragDropHelper.h`, `ViewDragDropHelper.cpp`, `DropListView.h`, `DropTreeView.h`, `DropJustifiedView.h`, `ContentPanel.h`, `ContentPanel.cpp`, `ContentFileOpsHandler.h`, `ContentFileOpsHandler.cpp`)**: Passes `Qt::DropAction` explicitly through `pathsDropped` signals down to `ContentFileOpsHandler::onPathsDropped`.
4. **Asynchronous Key-Release Resilience (`ContentFileOpsHandler.cpp`)**: Determines copy operation via `(dropAction == Qt::CopyAction) || (keyboardModifiers() & Qt::ControlModifier)`, ensuring robustness even if the user releases `Ctrl` a millisecond before releasing the mouse button.

---

## 2. Modified Files List
- `src/ui/ViewDragDropHelper.h`
- `src/ui/ViewDragDropHelper.cpp`
- `src/ui/DropListView.h`
- `src/ui/DropTreeView.h`
- `src/ui/DropJustifiedView.h`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentFileOpsHandler.h`
- `src/ui/controllers/ContentFileOpsHandler.cpp`

---

## 3. Detailed Line-by-Line Changes

```path
src/ui/ViewDragDropHelper.h
```

<<<<<<< SEARCH
signals:
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
=======
signals:
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, Qt::DropAction action);
>>>>>>> REPLACE

<<<<<<< SEARCH
    static bool handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx, QPoint* outStartPos = nullptr);
=======
    static bool handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx, Qt::DropAction* outAction = nullptr, QPoint* outStartPos = nullptr);
>>>>>>> REPLACE

```path
src/ui/ViewDragDropHelper.cpp
```

<<<<<<< SEARCH
    // 🚀【核心修复】：将事件过滤器的 pathsDropped 动态信号直接桥接至宿主视图的 pathsDropped 信号
    QObject::connect(filter, SIGNAL(pathsDropped(QStringList,QModelIndex)),
                     view, SIGNAL(pathsDropped(QStringList,QModelIndex)));
=======
    // 🚀【核心修复】：将事件过滤器的 pathsDropped 动态信号直接桥接至宿主视图的 pathsDropped 信号
    QObject::connect(filter, SIGNAL(pathsDropped(QStringList,QModelIndex,Qt::DropAction)),
                     view, SIGNAL(pathsDropped(QStringList,QModelIndex,Qt::DropAction)));
>>>>>>> REPLACE

<<<<<<< SEARCH
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
=======
    } else if (event->type() == QEvent::Drop) {
        clearDropHighlight();
        auto* dropEv = static_cast<QDropEvent*>(event);
        QStringList paths;
        QModelIndex targetIdx;
        Qt::DropAction action = Qt::CopyAction;
        if (ViewDragDropHelper::handleDrop(m_targetView, dropEv, paths, targetIdx, &action)) {
            emit pathsDropped(paths, targetIdx, action);
            return true;
        }
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
bool ViewDragDropHelper::handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx, QPoint* outStartPos) {
    outPaths.clear();
    outTargetIdx = QModelIndex();
    if (outStartPos) *outStartPos = s_lastDragStartPos;

    if (event->mimeData() && event->mimeData()->hasUrls()) {
=======
bool ViewDragDropHelper::handleDrop(QAbstractItemView* view, QDropEvent* event, QStringList& outPaths, QModelIndex& outTargetIdx, Qt::DropAction* outAction, QPoint* outStartPos) {
    outPaths.clear();
    outTargetIdx = QModelIndex();
    if (outStartPos) *outStartPos = s_lastDragStartPos;
    if (outAction) *outAction = event->dropAction();

    if (event->mimeData() && event->mimeData()->hasUrls()) {
>>>>>>> REPLACE

<<<<<<< SEARCH
void ViewDragDropHelper::executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions) {
    if (view && view->viewport()) {
        s_lastDragStartPos = view->viewport()->mapFromGlobal(QCursor::pos());
    } else {
        s_lastDragStartPos = QCursor::pos();
    }
=======
void ViewDragDropHelper::executeStartDrag(QAbstractItemView* view, Qt::DropActions supportedActions) {
    s_lastDragStartPos = QCursor::pos();
>>>>>>> REPLACE

<<<<<<< SEARCH
    drag->exec(supportedActions | Qt::CopyAction, Qt::MoveAction);
}
=======
    bool isCtrl = (QApplication::keyboardModifiers() & Qt::ControlModifier);
    Qt::DropAction defaultAction = isCtrl ? Qt::CopyAction : Qt::MoveAction;

    drag->exec(supportedActions | Qt::CopyAction | Qt::MoveAction, defaultAction);
    s_lastDragStartPos = QPoint();
}
>>>>>>> REPLACE

```path
src/ui/DropListView.h
```

<<<<<<< SEARCH
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
=======
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, Qt::DropAction action);
>>>>>>> REPLACE

```path
src/ui/DropTreeView.h
```

<<<<<<< SEARCH
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
=======
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, Qt::DropAction action);
>>>>>>> REPLACE

```path
src/ui/DropJustifiedView.h
```

<<<<<<< SEARCH
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
=======
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, Qt::DropAction action);
>>>>>>> REPLACE

```path
src/ui/ContentPanel.h
```

<<<<<<< SEARCH
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString());
=======
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString(), Qt::DropAction action = Qt::CopyAction);
>>>>>>> REPLACE

```path
src/ui/ContentPanel.cpp
```

<<<<<<< SEARCH
    connect(jv, &JustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath());
    });
=======
    connect(jv, &JustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex, Qt::DropAction action) {
        onPathsDropped(paths, targetIndex, currentPath(), action);
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(tree, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath());
    });
=======
    connect(tree, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex, Qt::DropAction action) {
        onPathsDropped(paths, targetIndex, currentPath(), action);
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride) {
    if (m_fileOpsHandler) m_fileOpsHandler->onPathsDropped(paths, targetIndex, targetDirOverride);
}
=======
void ContentPanel::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, Qt::DropAction action) {
    if (m_fileOpsHandler) m_fileOpsHandler->onPathsDropped(paths, targetIndex, targetDirOverride, action);
}
>>>>>>> REPLACE

```path
src/ui/controllers/ContentFileOpsHandler.h
```

<<<<<<< SEARCH
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString());
=======
    void onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride = QString(), Qt::DropAction action = Qt::CopyAction);
>>>>>>> REPLACE

```path
src/ui/controllers/ContentFileOpsHandler.cpp
```

<<<<<<< SEARCH
void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride) {
    if (!m_panel || paths.isEmpty()) return;

    QString baseDir = !targetDirOverride.isEmpty() ? targetDirOverride : m_panel->currentPath();
    if (baseDir.isEmpty() || baseDir == "computer://") return;

    QString destDir = baseDir;

    if (targetIndex.isValid()) {
        QString targetPath = targetIndex.data(PathRole).toString();
        if (!targetPath.isEmpty() && QFileInfo(targetPath).isDir()) {
            destDir = targetPath;
        }
    }

    qDebug() << "[ColumnView DragDrop Debug] Sources:" << paths 
             << "| TargetDirOverride:" << targetDirOverride 
             << "| Final DestDir:" << destDir;

    bool isMove = !(QApplication::keyboardModifiers() & Qt::ControlModifier);

    if (!destDir.isEmpty() && destDir != "computer://") {
        NavigationHistoryService::recordRecentVisitedFolder(QDir::toNativeSeparators(destDir).toStdWString());
        AppConfig::instance().setValue("RecentVisited/LastDragDropDestination", destDir);
        AppConfig::instance().sync();
        if (isMove) {
            LastOperationManager::instance().recordMoveToFolder(destDir);
        }
    }

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

    // 1. 仅对来自外部目录的项目检测目标文件夹中的同名冲突文件
    QStringList conflictingSources;
    for (const QString& src : externalPaths) {
        QString fileName = QFileInfo(src).fileName();
        QString destPath = QDir(destDir).filePath(fileName);
        if (QFile::exists(destPath)) {
            conflictingSources.append(src);
        }
    }

    DiskIoContext ioCtx;
    ioCtx.sources = externalPaths;
    ioCtx.destination = destDir;
    ioCtx.isMove = isMove;
=======
void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex, const QString& targetDirOverride, Qt::DropAction action) {
    if (!m_panel || paths.isEmpty()) return;

    QString baseDir = !targetDirOverride.isEmpty() ? targetDirOverride : m_panel->currentPath();
    if (baseDir.isEmpty() || baseDir == "computer://") return;

    QString destDir = baseDir;

    if (targetIndex.isValid()) {
        QString targetPath = targetIndex.data(PathRole).toString();
        if (!targetPath.isEmpty() && QFileInfo(targetPath).isDir()) {
            destDir = targetPath;
        }
    }

    qDebug() << "[ColumnView DragDrop Debug] Sources:" << paths 
             << "| TargetDirOverride:" << targetDirOverride 
             << "| Final DestDir:" << destDir
             << "| DropAction:" << action;

    bool isCopyOperation = (action == Qt::CopyAction) || (QApplication::keyboardModifiers() & Qt::ControlModifier);
    bool isMove = !isCopyOperation;

    if (!destDir.isEmpty() && destDir != "computer://") {
        NavigationHistoryService::recordRecentVisitedFolder(QDir::toNativeSeparators(destDir).toStdWString());
        AppConfig::instance().setValue("RecentVisited/LastDragDropDestination", destDir);
        AppConfig::instance().sync();
        if (isMove) {
            LastOperationManager::instance().recordMoveToFolder(destDir);
        }
    }

    // 0. 原地/同目录拖放保护与 Ctrl+Drag 副本创建门禁
    QStringList externalPaths;
    bool isDuplicateCopy = false;

    for (const QString& src : paths) {
        QFileInfo srcInfo(src);
        bool isSameDirectory = (QDir::cleanPath(srcInfo.absolutePath()) == QDir::cleanPath(QDir(destDir).absolutePath()));

        if (isSameDirectory) {
            if (isCopyOperation) {
                // 判断 Ctrl+Drag 拖拽物理距离（全物理坐标对比），防止 Ctrl+Click 多选误触（门禁设为 50px）
                QPoint dragStartPos = ViewDragDropHelper::lastDragStartPos();
                QPoint dropPos = QCursor::pos();
                int dragDistance = (dragStartPos.isNull()) ? 100 : (dropPos - dragStartPos).manhattanLength();

                if (dragDistance >= 50) {
                    externalPaths.append(src);
                    isDuplicateCopy = true;
                }
            }
        } else {
            externalPaths.append(src);
        }
    }

    if (externalPaths.isEmpty()) {
        // 全为同目录内自拖放且未触发 Ctrl+Drag 副本创建门禁，静默处理
        return;
    }

    // 1. 仅对非同目录副本创建的项目检测目标文件夹中的同名冲突文件
    QStringList conflictingSources;
    if (!isDuplicateCopy) {
        for (const QString& src : externalPaths) {
            QString fileName = QFileInfo(src).fileName();
            QString destPath = QDir(destDir).filePath(fileName);
            if (QFile::exists(destPath)) {
                conflictingSources.append(src);
            }
        }
    }

    DiskIoContext ioCtx;
    ioCtx.sources = externalPaths;
    ioCtx.destination = destDir;
    ioCtx.isMove = isMove;

    if (isDuplicateCopy) {
        // 同目录 Ctrl+Drag 专属意图：强制自动追加序号重命名（如 filename-1.ext），绕过冲突弹窗
        ioCtx.autoRenameAll = true;
        ioCtx.isMove = false;
    }
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Run CMake compilation to verify zero syntax/signature errors.
2. Launch QuarkMeta and test `Ctrl + Drag` duplicate creation in same folder:
   - Hold `Ctrl`, click and drag a file over $50\text{px}$, then release.
   - Verify that `filename-1.ext` copy is immediately created with no collision dialog.
3. Test `Ctrl + Drag` with fast key release:
   - Hold `Ctrl`, start dragging, and release `Ctrl` right before releasing mouse left button.
   - Verify that `dropAction` (passed via `Qt::DropAction`) is still recognized as copy, producing a duplicate cleanly instead of triggering a move or silent ignore.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `ShellHelper::copyOrMoveItems(...)`.
- Reused `ViewDragDropHelper::lastDragStartPos()`.
- Reused `m_panel->refreshAll()`.

---

## 6. Header API Signature Verification
- `ViewDragDropHelper::handleDrop(...)` signature updated.
- `pathsDropped` signals aligned across view headers.

---

## 7. Header Inclusion Chain & Type Completeness Check
- Include `<QApplication>`, `<QCursor>` present.
- All types complete.
