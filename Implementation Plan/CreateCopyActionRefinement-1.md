# Implementation Plan - CreateCopyActionRefinement-1.md

## 1. Overview
Refines and fixes 3 severe logic issues in the in-place `Ctrl + Drag` duplicate creation mechanism:
1. **Coordinate System Fix (`ViewDragDropHelper.cpp`)**:
   Fixes coordinate domain mismatch by recording `s_lastDragStartPos` in **global screen coordinates (`QCursor::pos()`)**, matching `QCursor::pos()` during drop.
2. **Dedicated Duplicate Creation Logic & Collision Bypass (`ContentFileOpsHandler.cpp`)**:
   Identifies in-place `Ctrl + Drag` operations with distance $\ge 50\text{px}$ as dedicated duplicate creation requests (`isDuplicateCopy`).
   Bypasses `FileCollisionDialog` for these requests and forces `autoRenameAll = true` and `isMove = false` in `DiskIoContext`, generating `-1`, `-2` auto-increment suffixes seamlessly without interrupting user flow.
3. **Data Loss Guard**:
   By routing in-place duplicate creation directly to `autoRenameAll = true`, prevents accidental file replacement (`QFile::remove`) of source files during in-place copy.

---

## 2. Modified Files List
- `src/ui/ViewDragDropHelper.cpp`
- `src/ui/controllers/ContentFileOpsHandler.cpp`

---

## 3. Detailed Line-by-Line Changes

```path
src/ui/ViewDragDropHelper.cpp
```

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

```path
src/ui/controllers/ContentFileOpsHandler.cpp
```

<<<<<<< SEARCH
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
    // 0. 原地/同目录拖放保护与 Ctrl+Drag 副本创建门禁
    bool isCtrlPressed = (QApplication::keyboardModifiers() & Qt::ControlModifier);
    QStringList externalPaths;
    bool isDuplicateCopy = false;

    for (const QString& src : paths) {
        QFileInfo srcInfo(src);
        bool isSameDirectory = (QDir::cleanPath(srcInfo.absolutePath()) == QDir::cleanPath(QDir(destDir).absolutePath()));

        if (isSameDirectory) {
            if (isCtrlPressed) {
                // 判断 Ctrl+Drag 拖拽物理距离（使用全局坐标），防止 Ctrl+Click 多选误触（门禁设为 50px）
                QPoint dragStartPos = ViewDragDropHelper::lastDragStartPos();
                QPoint dropPos = QCursor::pos();
                int dragDistance = (dropPos - dragStartPos).manhattanLength();

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
1. Perform CMake compilation to ensure clean build without syntax/link errors.
2. Launch QuarkMeta and perform `Ctrl + Drag` test in the same folder:
   - Hold `Ctrl`, click and drag an item horizontally/vertically over $50\text{px}$.
   - Drop the item in the same view.
   - Verify that no `FileCollisionDialog` is shown, and a new copy (e.g., `filename-1.ext`) is created seamlessly.
3. Test `Ctrl + Click` multi-selection safety:
   - Hold `Ctrl` and select items with slight jitter ($< 50\text{px}$).
   - Verify that no duplicate files are generated during multi-selection.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `ShellHelper::copyOrMoveItems(...)` for file copy execution and auto-increment naming.
- Reused `ViewDragDropHelper::lastDragStartPos()` for global drag start coordinate retrieval.
- Reused `m_panel->refreshAll()` via standard callback for in-place view update.

---

## 6. Header API Signature Verification
- `ViewDragDropHelper::lastDragStartPos()` declared in `ViewDragDropHelper.h`:
  ```cpp
  static QPoint lastDragStartPos();
  ```
- `ShellHelper::copyOrMoveItems(...)` declared in `ShellHelper.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `ViewDragDropHelper.cpp` includes `<QCursor>`.
- `ContentFileOpsHandler.cpp` includes `<QCursor>`, `<QApplication>`, `ViewDragDropHelper.h`.
- Header dependencies and types are 100% complete.
