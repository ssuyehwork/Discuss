# Implementation Plan - ProgressNormalization.md

> **Base Commit Hash**: `2ab1186a2ebc058e8ba6a5e0ae3f81d93d3cabcd`
> **Prerequisite Plans**: None
> **Supersedes**: None

---

## Chapter 1: Overview

### 问题根因分析 (Root Cause Analysis)
在 QuarkMeta 当前代码库中，存在两套独立的进度指示 UI 控件：
1. `TaskProgressToolBar`（由 `TaskProgressService` 单例驱动，主窗口底部全局栏，用于标签应用、分类归入等非阻塞批量任务）；
2. `BatchProgressDialog`（模态/无边框独立弹窗，用于永久删除与回收站清理操作）。

这种双轨并行设计造成了 UI 控件与进度表达的重复造轮子（Redundant UI Implementations），违背了《AGENTS.md》与《Memories.md》中“界面交互一致性与逻辑归一化”原则。

### 重构设计目标 (Refactoring Goals)
1. **彻底归一化进度指示器**：放弃并删除 `BatchProgressDialog` 弹窗模式，全软件所有批量异步任务（包含深层物理删除、回收站清空、标签应用与分类归入）统一路由使用 `TaskProgressToolBar` + `TaskProgressService` 系统。
2. **物理彻底清除死代码**：物理删除 `src/ui/BatchProgressDialog.h` 文件，并在 `CMakeLists.txt` 中同步注销，清理 `style.qss` 中废弃的弹窗进度条样式。
3. **视觉品牌色统一**：将 `resources/style.qss` 中 `#TaskProgressBar` 进度条与 `#TaskProgressCountLabel` 文本颜色统一调整为品牌新调色 `#ff551c`。

---

## Chapter 2: Modified Files List

1. `src/ui/BatchProgressDialog.h` (**DELETE FILE**)
2. `CMakeLists.txt` (**MODIFY FILE**)
3. `src/ui/TrayController.cpp` (**MODIFY FILE**)
4. `src/core/PermanentDeleteService.cpp` (**MODIFY FILE**)
5. `resources/style.qss` (**MODIFY FILE**)

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. 删除 `src/ui/BatchProgressDialog.h`
```
<<<<<<< SEARCH DELETE FILE: src/ui/BatchProgressDialog.h
=======
>>>>>>> REPLACE
```

### 2. 更新 `CMakeLists.txt`
```
<<<<<<< SEARCH
    src/ui/BatchRenameDialog.cpp
    src/ui/BatchRenameDialog.h
    src/ui/BatchProgressDialog.h
    src/ui/FileCollisionDialog.h
=======
    src/ui/BatchRenameDialog.cpp
    src/ui/BatchRenameDialog.h
    src/ui/FileCollisionDialog.h
>>>>>>> REPLACE
```

### 3. 更新 `src/ui/TrayController.cpp`
```
<<<<<<< SEARCH
#include <QIcon>
#include <QDebug>
#include <QCursor>
#include <QProgressDialog>
#include "../meta/DatabaseManager.h"
#include "BatchProgressDialog.h"
#include "UiHelper.h"
=======
#include <QIcon>
#include <QDebug>
#include <QCursor>
#include "../meta/DatabaseManager.h"
#include "UiHelper.h"
>>>>>>> REPLACE
```

### 4. 重构 `src/core/PermanentDeleteService.cpp`
```
<<<<<<< SEARCH
#include "PermanentDeleteService.h"
#include "../ui/dialogs/FramelessMessageBox.h"
#include "../ui/BatchProgressDialog.h"
#include "../ui/ToolTipOverlay.h"
=======
#include "PermanentDeleteService.h"
#include "../ui/dialogs/FramelessMessageBox.h"
#include "../core/TaskProgressService.h"
#include "../ui/ToolTipOverlay.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    BatchProgressDialog* progress = new BatchProgressDialog("正在执行永久删除（深层抹除）...", parentWidget);
    progress->show();

    QPointer<BatchProgressDialog> weakProgress(progress);
    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([paths, isSecureShred, weakProgress, weakThis]() {
        int total = paths.size();
        int count = 0;

        for (const QString& p : paths) {
            std::wstring wp = QDir::toNativeSeparators(p).toStdWString();

            int trashId = -1;
            QString trashPath;
            bool foundRecord = TrashRepository::instance().getDiskTrashRecordByPath(wp, trashId, trashPath);
            bool isTrashItem = foundRecord || p.contains(".QuarkMeta/disk_trash");

            if (isTrashItem) {
                if (isSecureShred) {
                    SecureFileEraser::shredFile(p);
                }
                DiskTrashService::permanentlyDeleteDiskTrash(trashId, p);
                UndoManager::instance().removeCommandsForPath(p);
            } else {
                bool physicalOk = false;
                if (isSecureShred) {
                    physicalOk = SecureFileEraser::shredFile(p);
                } else {
                    QFileInfo info(p);
                    if (info.isDir()) {
                        physicalOk = QDir(p).removeRecursively();
                    } else {
                        physicalOk = QFile::remove(p);
                    }
                }

                if (physicalOk) {
                    QuarkMetaJson::removeItemMetadata(p);
                    AppCommand cmd;
                    cmd.type = AppCommandType::DeletePermanently;
                    cmd.targetPaths << QString::fromStdWString(wp);
                    CoreEngine::instance().executeCommand(cmd);
                    UndoManager::instance().removeCommandsForPath(p);
                }
            }

            count++;
            if (weakProgress) {
                int percent = (int)((float)count / total * 100);
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, percent]() {
                    if (weakProgress) weakProgress->setValue(percent);
                });
            }
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, weakThis]() {
            if (weakProgress) {
                weakProgress->accept();
                weakProgress->deleteLater();
            }
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });
=======
    int jobId = TaskProgressService::instance().createJob("正在执行永久删除（深层抹除）...", paths.size());

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([paths, isSecureShred, jobId, weakThis]() {
        int total = paths.size();
        int count = 0;

        for (const QString& p : paths) {
            std::wstring wp = QDir::toNativeSeparators(p).toStdWString();

            int trashId = -1;
            QString trashPath;
            bool foundRecord = TrashRepository::instance().getDiskTrashRecordByPath(wp, trashId, trashPath);
            bool isTrashItem = foundRecord || p.contains(".QuarkMeta/disk_trash");

            if (isTrashItem) {
                if (isSecureShred) {
                    SecureFileEraser::shredFile(p);
                }
                DiskTrashService::permanentlyDeleteDiskTrash(trashId, p);
                UndoManager::instance().removeCommandsForPath(p);
            } else {
                bool physicalOk = false;
                if (isSecureShred) {
                    physicalOk = SecureFileEraser::shredFile(p);
                } else {
                    QFileInfo info(p);
                    if (info.isDir()) {
                        physicalOk = QDir(p).removeRecursively();
                    } else {
                        physicalOk = QFile::remove(p);
                    }
                }

                if (physicalOk) {
                    QuarkMetaJson::removeItemMetadata(p);
                    AppCommand cmd;
                    cmd.type = AppCommandType::DeletePermanently;
                    cmd.targetPaths << QString::fromStdWString(wp);
                    CoreEngine::instance().executeCommand(cmd);
                    UndoManager::instance().removeCommandsForPath(p);
                }
            }

            count++;
            TaskProgressService::instance().updateProgress(jobId, count, total, QString("已擦除 %1 / %2 项").arg(count).arg(total));
        }

        TaskProgressService::instance().finishJob(jobId);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    BatchProgressDialog* progress = new BatchProgressDialog("正在销毁回收站项目...", parentWidget);
    progress->show();

    QPointer<BatchProgressDialog> weakProgress(progress);
    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([trashItems, weakProgress, weakThis]() {
        int total = static_cast<int>(trashItems.size());
        int count = 0;

        for (const auto& item : trashItems) {
            SecureFileEraser::shredFile(item.second);
            DiskTrashService::permanentlyDeleteDiskTrash(item.first, item.second);
            UndoManager::instance().removeCommandsForPath(item.second);
            count++;
            if (weakProgress) {
                int percent = (int)((float)count / total * 100);
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, percent]() {
                    if (weakProgress) weakProgress->setValue(percent);
                });
            }
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, weakThis]() {
            if (weakProgress) {
                weakProgress->accept();
                weakProgress->deleteLater();
            }
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });
=======
    int jobId = TaskProgressService::instance().createJob("正在销毁回收站项目...", static_cast<int>(trashItems.size()));

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([trashItems, jobId, weakThis]() {
        int total = static_cast<int>(trashItems.size());
        int count = 0;

        for (const auto& item : trashItems) {
            SecureFileEraser::shredFile(item.second);
            DiskTrashService::permanentlyDeleteDiskTrash(item.first, item.second);
            UndoManager::instance().removeCommandsForPath(item.second);
            count++;
            TaskProgressService::instance().updateProgress(jobId, count, total, QString("已销毁 %1 / %2 项").arg(count).arg(total));
        }

        TaskProgressService::instance().finishJob(jobId);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    BatchProgressDialog* progress = new BatchProgressDialog("正在清空回收站（深层物理抹除）...", parentWidget);
    progress->show();

    QPointer<BatchProgressDialog> weakProgress(progress);
    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([allTrash, isSecureShred, weakProgress, weakThis]() {
        int total = static_cast<int>(allTrash.size());
        int count = 0;

        for (const auto& item : allTrash) {
            QString path = QString::fromStdWString(item.trashPath);
            if (isSecureShred) {
                SecureFileEraser::shredFile(path);
            }
            DiskTrashService::permanentlyDeleteDiskTrash(item.id, path);
            UndoManager::instance().removeCommandsForPath(QString::fromStdWString(item.originalPath));

            count++;
            if (weakProgress) {
                int percent = (int)((float)count / total * 100);
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, percent]() {
                    if (weakProgress) weakProgress->setValue(percent);
                });
            }
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakProgress, weakThis]() {
            if (weakProgress) {
                weakProgress->accept();
                weakProgress->deleteLater();
            }
            MetadataManager::instance().endInternalOperation();
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });
=======
    int jobId = TaskProgressService::instance().createJob("正在清空回收站（深层物理抹除）...", static_cast<int>(allTrash.size()));

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([allTrash, isSecureShred, jobId, weakThis]() {
        int total = static_cast<int>(allTrash.size());
        int count = 0;

        for (const auto& item : allTrash) {
            QString path = QString::fromStdWString(item.trashPath);
            if (isSecureShred) {
                SecureFileEraser::shredFile(path);
            }
            DiskTrashService::permanentlyDeleteDiskTrash(item.id, path);
            UndoManager::instance().removeCommandsForPath(QString::fromStdWString(item.originalPath));

            count++;
            TaskProgressService::instance().updateProgress(jobId, count, total, QString("已清空 %1 / %2 项").arg(count).arg(total));
        }

        TaskProgressService::instance().finishJob(jobId);

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });
>>>>>>> REPLACE
```

### 5. 更新 `resources/style.qss` 绝美色值
```
<<<<<<< SEARCH
QProgressBar#ProgressDialogBar {
    border: none;
    border-radius: 4px;
    background-color: #2D2D30;
    text-align: center;
    color: transparent;
}
QProgressBar#ProgressDialogBar::chunk {
    background-color: #378ADD;
    border-radius: 4px;
}
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QProgressBar#TaskProgressBar::chunk {
    background-color: #378ADD;
    border-radius: 2px;
}
QLabel#TaskProgressDetailLabel {
    color: #888888;
    font-size: 11px;
    background: transparent;
}
QLabel#TaskProgressCountLabel {
    color: #378ADD;
    font-size: 11px;
    font-weight: bold;
    background: transparent;
}
=======
QProgressBar#TaskProgressBar::chunk {
    background-color: #ff551c;
    border-radius: 2px;
}
QLabel#TaskProgressDetailLabel {
    color: #888888;
    font-size: 11px;
    background: transparent;
}
QLabel#TaskProgressCountLabel {
    color: #ff551c;
    font-size: 11px;
    font-weight: bold;
    background: transparent;
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps

### 1. 编译验证
在项目根目录下运行如下 CMake 命令行编译项目：
```bash
cmake -B build -G Ninja
cmake --build build
```
验证目标：无编译错误，链接期无未定义符号 `BatchProgressDialog`。

### 2. 运行时验收标准
1. **永久删除/擦除**：选中若干文件后按 `Shift+Delete`，弹出的确认框确认后，主窗口底部平滑展开 `TaskProgressToolBar`，进度条填充颜色为橙红色 `#ff551c`，更新进度文案如 “已擦除 1 / 5 项”。
2. **清空回收站**：右键清空回收站，底部无缝展开 `TaskProgressToolBar` 显示抹除进度。
3. **彻底物理隔离**：搜索全工程代码，已无任何 `BatchProgressDialog` 相关引用与遗留文件。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

- 完美复用 QuarkMeta 标准后台进度框架 `TaskProgressService` 单例（API：`createJob`、`updateProgress`、`finishJob`），消除了独立的模态弹窗类。

---

## Chapter 6: Data SSOT Declaration Table

| 数据/状态项 | 唯一 SSOT 数据源 | 派生 UI/副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 全局后台任务列表与进度 | `TaskProgressService::m_jobs` | `TaskProgressToolBar` | `TaskProgressService` 线程安全更新并向 GUI 发射 `progressUpdated` 信号 |

---

## Chapter 7: Interface & Visibility Consistency Check

- `TaskProgressService::createJob(const QString& title, int totalSteps)`: `src/core/TaskProgressService.h:32`
- `TaskProgressService::updateProgress(int jobId, int currentStep, int totalSteps, const QString& detail)`: `src/core/TaskProgressService.h:37`
- `TaskProgressService::finishJob(int jobId)`: `src/core/TaskProgressService.h:42`
- `PermanentDeleteService` 内部子线程调用均符合线程安全契约。

---

## Chapter 8: Header Chain & Closure Check

- 在 `PermanentDeleteService.cpp` 中包含了 `#include "../core/TaskProgressService.h"`，类型闭合，零缺失。
- `CMakeLists.txt` 已移除对 `BatchProgressDialog.h` 的编译引用。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 检查项 | 结论 | 位置/说明 |
| :--- | :--- | :--- |
| 1. SEARCH 块唯一存在性 | 满足 | 物理文件精确匹配 |
| 2. 头文件包含闭合 | 满足 | cpp 已全量闭合包含 |
| 3. CMakeLists.txt 同步注册/移除 | 满足 | 已移除 `BatchProgressDialog.h` |
| 4. 线程 IO 与主线程 UI 安全 | 满足 | `TaskProgressService` 内部具备互斥锁与 `invokeMethod` 发射信号安全 |
| 5. 样式与颜色零硬编码 | 满足 | 统一写入 `resources/style.qss` |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

| 交互/样式项 | 旧版表现 | 新版归一化表现 |
| :--- | :--- | :--- |
| 物理擦除/删除进度展示 | 弹出模态 `BatchProgressDialog` 窗口 | 统一底部 `TaskProgressToolBar` |
| 进度条颜色 | `#378ADD`（蓝色） | 用户指定 `#ff551c`（橙红色） |
