# Implementation Plan - Self Drag-And-Drop Collision Bypassing Fix (ContentFileOpsHandler.md)

## 1. Overview
This implementation plan resolves a bug where dragging and dropping items onto blank space within the same directory erroneously triggered the `FileCollisionDialog` ("An item named XYZ already exists...").

When items are dropped within their current directory (`src` file directory matches `destDir`), `ContentFileOpsHandler::onPathsDropped` should recognize this as a self drag-and-drop operation, ignore same-file existence false positives during conflict checking, and return early without triggering unnecessary Disk I/O or collision popups.

---

## 2. Modified Files List
- `src/ui/controllers/ContentFileOpsHandler.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/controllers/ContentFileOpsHandler.cpp`
Filter out paths whose source parent directory is identical to `destDir` before performing collision detection, and return early if all paths originated from `destDir`.

```
<<<<<<< SEARCH
    // 检测目标文件夹中的同名冲突文件
    QStringList conflictingSources;
    for (const QString& src : paths) {
        QString fileName = QFileInfo(src).fileName();
        QString destPath = QDir(destDir).filePath(fileName);
        if (QFile::exists(destPath)) {
            conflictingSources.append(src);
        }
    }
=======
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

    // 1. 仅对来自外部目录的项目检测目标文件夹中的同名冲突文件
    QStringList conflictingSources;
    for (const QString& src : externalPaths) {
        QString fileName = QFileInfo(src).fileName();
        QString destPath = QDir(destDir).filePath(fileName);
        if (QFile::exists(destPath)) {
            conflictingSources.append(src);
        }
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    DiskIoContext ioCtx;
    ioCtx.sources = paths;
    ioCtx.destination = destDir;
    ioCtx.isMove = isMove;

    if (!conflictingSources.isEmpty()) {
        QStringList activeSources = paths;
=======
    DiskIoContext ioCtx;
    ioCtx.sources = externalPaths;
    ioCtx.destination = destDir;
    ioCtx.isMove = isMove;

    if (!conflictingSources.isEmpty()) {
        QStringList activeSources = externalPaths;
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional Verification**:
   - Open any directory in `ContentPanel` (e.g. `测试-4`).
   - Select a file or folder and drag it onto the blank canvas area within the same directory.
   - Release the drag.
   - Verify that **no `FileCollisionDialog` ("一个名为 XYZ 的项目已在...中存在") pops up**, and the view remains responsive and clean.
   - Drag files from an external folder into `测试-4` where a file with the same name exists, and verify that `FileCollisionDialog` correctly pops up for actual external file collisions.
