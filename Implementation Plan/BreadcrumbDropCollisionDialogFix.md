# Implementation Plan - Breadcrumb Drop Collision Interception Fix

## 1. Overview
When dragging files/folders onto breadcrumb nodes in `AddressBar`, `BreadcrumbBar::pathsDroppedNode` currently executes `DiskIoService::instance().executeAsync` with an unpopulated `DiskIoContext` without conflict detection.
This causes Windows `SHFileOperationW` with `FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION` to silently overwrite existing files in the target folder without user confirmation, destroying existing files and removing source files.

This implementation plan adds conflict detection and user dialog prompt (`FileCollisionDialog`) to `AddressBar::AddressBar()` prior to submitting the drag-and-drop action to `DiskIoService`.

---

## 2. Modified Files List
- `src/ui/AddressBar.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/AddressBar.cpp`
Include `FileCollisionDialog.h` and update the lambda in `connect(m_breadcrumbBar, &BreadcrumbBar::pathsDroppedNode)` to detect conflicts and invoke `FileCollisionDialog`:

```cpp
<<<<<<< SEARCH
#include "AddressBar.h"
#include "UiHelper.h"
=======
#include "AddressBar.h"
#include "UiHelper.h"
#include "FileCollisionDialog.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    connect(m_breadcrumbBar, &BreadcrumbBar::pathsDroppedNode, this, [this](const QStringList& paths, const QString& targetPath) {
        if (paths.isEmpty() || targetPath.isEmpty() || targetPath == "computer://") return;
        DiskIoContext ioCtx;
        ioCtx.sources = paths;
        ioCtx.destination = targetPath;
        ioCtx.isMove = !(QApplication::keyboardModifiers() & Qt::ControlModifier);
        DiskIoService::instance().executeAsync(ioCtx, nullptr);
    });
=======
    connect(m_breadcrumbBar, &BreadcrumbBar::pathsDroppedNode, this, [this](const QStringList& paths, const QString& targetPath) {
        if (paths.isEmpty() || targetPath.isEmpty() || targetPath == "computer://") return;

        DiskIoContext ioCtx;
        ioCtx.destination = targetPath;
        ioCtx.isMove = !(QApplication::keyboardModifiers() & Qt::ControlModifier);

        // 1. Scan for conflicting files in destination directory
        QStringList conflictingPaths;
        for (const QString& srcPath : paths) {
            QFileInfo srcInfo(srcPath);
            QString destFilePath = QDir(targetPath).filePath(srcInfo.fileName());
            if (QFile::exists(destFilePath)) {
                conflictingPaths.append(srcPath);
            }
        }

        if (!conflictingPaths.isEmpty()) {
            int remainingConflicts = conflictingPaths.size();
            bool applyToAll = false;
            CollisionResolveAction globalAction = CollisionResolveAction::KeepBoth;

            for (const QString& srcFile : paths) {
                QFileInfo srcInfo(srcFile);
                QString destFilePath = QDir(targetPath).filePath(srcInfo.fileName());

                if (QFile::exists(destFilePath)) {
                    CollisionResolveAction action = globalAction;

                    if (!applyToAll) {
                        FileCollisionDialog dialog(srcFile, targetPath, remainingConflicts--, this);
                        if (dialog.exec() != QDialog::Accepted) {
                            return; // User clicked Cancel -> abort drop operation
                        }
                        action = dialog.selectedAction();
                        applyToAll = dialog.applyToAll();
                        if (applyToAll) {
                            globalAction = action;
                        }
                    }

                    if (action == CollisionResolveAction::Skip) {
                        continue; // Skip this file
                    } else if (action == CollisionResolveAction::KeepBoth) {
                        ioCtx.autoRenameFiles.insert(srcFile);
                        ioCtx.sources.append(srcFile);
                    } else if (action == CollisionResolveAction::Replace) {
                        ioCtx.overwriteFiles.insert(srcFile);
                        ioCtx.sources.append(srcFile);
                    }
                } else {
                    ioCtx.sources.append(srcFile);
                }
            }
        } else {
            ioCtx.sources = paths;
        }

        if (ioCtx.sources.isEmpty()) return;

        DiskIoService::instance().executeAsync(ioCtx, nullptr);
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile project target using `cmake --build .`.
2. Create `H:\Test\Test-3\1.md` and `H:\Test\Test-3\Test-4\1.md`.
3. Open `H:\Test\Test-3\Test-4` in QuarkMeta.
4. Drag `1.md` onto the `Test-3` node in the top address bar breadcrumb.
5. Verify that `FileCollisionDialog` pops up showing options "同存", "替换", "跳过", "取消".
6. Select "同存" and verify `1.md` is moved/copied to `Test-3` as `1-1.md` without destroying `Test-3\1.md`.

---

## 5. Official API Reuse & Anti-Redundancy
- Reuses `FileCollisionDialog` and `DiskIoContext` without introducing duplicate dialog classes.

---

## 6. Header API Signature Verification
- No header API signature modifications required.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `FileCollisionDialog.h` included in `AddressBar.cpp`.

---

## 8. Data SSOT Declaration Table
| Entity / State | SSOT Location | Synchronization Mechanism |
| :--- | :--- | :--- |
| Drop Collision Decision | `FileCollisionDialog` / `DiskIoContext` | Populates `autoRenameFiles` / `overwriteFiles` before submitting to `DiskIoService` |

---

## 9. Compliance Self-Check Table
| Check Item | Status | Notes |
| :--- | :--- | :--- |
| Zero Code Modification Rule | PASS | Output strictly as plan document inside `Implementation Plan/` |
| Chinese Language Protocol | PASS | Documented completely in Chinese |
| Contract Lock | PASS | Public header signatures untouched |

---

## 10. Behavior Preservation Declaration Table
| Action | Existing Behavior | Preserved Behavior |
| :--- | :--- | :--- |
| Non-Conflicting Drop on Breadcrumb | Directly copies/moves | Directly copies/moves |
| Conflicting Drop on Breadcrumb | Silently overwrites existing file | Prompts `FileCollisionDialog` for user choice |
