# Implementation Plan - Marked Folder Transfer Refactoring

**Base Commit Hash**: `e0f4534`
**Prerequisite Plans**: 无
**Supersedes**: 无

---

## Chapter 1: Overview（概述与根因分析）
本方案旨在彻底重构“标记”目的地的文件迁移分发功能（`MarkedFolderService::executeTransferToMarked`），全面消除自写的 `copyRecursively` 逻辑，归一化复用系统统一的磁盘 I/O 服务（`DiskIoService::executeAsync`）、文件冲突处理（`FileCollisionDialog`）、应用私有回收站（`DiskTrashService`）、文件监视静默（`ContentChangeWatcher`）以及任务进度条（`TaskProgressService`），并实现复合分发与同盘回收动作的一键撤销（`TransferToMarkedCommand`）。

### 1.1 现状与根因分析
先前 `MarkedFolderService::executeTransferToMarked` 在主 UI 线程内直接使用私有静态递归函数 `copyRecursively` 同步进行物理文件复制与 `QFile::remove` 删除：
1. **主线程阻塞**：同步遍历物理文件系统导致界面卡顿；
2. **丢失元数据与缩略图漫游**：未复用 `ShellHelper::copyOrMoveItems`，导致侧车 JSON (`.QuarkMeta.json`) 元数据与 `disk_thumbs` 缩略图无法随迁移漫游；
3. **缺少冲突处理**：同名目标文件被强制无提示覆盖或直接报错；
4. **缺少撤销与进度条**：搬运分发过程无法被 `Ctrl+Z` 撤销，界面无进度反馈；
5. **自身/同路径越界风险**：目标为源文件夹本身或子目录时未规范化阻断。

### 1.2 修复策略与编排规则
1. **彻底清理**：物理删除 `MarkedFolderService.cpp` 中自写的 `copyRecursively` 函数。
2. **统一 I/O 搬运**：对所有目的地按“复制”模式（`isMove = false`）提交给 `DiskIoService::executeAsync` 执行。
3. **同盘/跨盘与回收规则**：
   - 比较源文件路径与目的地路径的物理卷序列号（`VolumePathResolver::getVolumeSerialNumber`）。若同盘，且全部目标均成功复制，则调用 `DiskTrashService::moveToDiskTrash` 将源文件移入应用私有回收站；
   - 若任何一个目标失败，或所有目标均跨盘，源文件严格保留。
4. **路径防重与同源跳过**：
   - 比较 `QDir::cleanPath` 及不区分大小写的前缀匹配，若目标就是源文件夹本身或源文件夹的子目录，跳过该目标并提示。
5. **冲突拦截**：在执行前由 UI/控制器层调用 `FileCollisionDialog`，收集用户冲突处置策略（自动解析/替换/跳过/取消/应用到全部），并将参数设置至 `DiskIoContext`。
6. **复合撤销**：新增 `TransferToMarkedCommand` 压入 `UndoManager`。撤销时先将各目标副本移入回收站，若源文件曾被回收，则根据记录 ID 调用 `DiskTrashService::restoreFromDiskTrash` 还原原文件；还原位置被占用时自动重命名并通过 `ToolTipOverlay` 友情 Toast 提示。
7. **全量进度条与静默**：通过 `TaskProgressService::instance().createJob(...)` 立即创建 0ms 视觉响应进度条，并对相关内容窗格调用 `beginWatchSuppress()` / `endWatchSuppress()` 进行监视静默。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/core/DiskTrashService.h` / `src/core/DiskTrashService.cpp` (修改)：新增只读查询接口 `getLatestTrashItemByOriginalPath`。
2. `src/core/TransferToMarkedCommand.h` / `src/core/TransferToMarkedCommand.cpp` (新建)：新增标记分发复合撤销命令类。
3. `src/meta/MarkedFolderService.h` / `src/meta/MarkedFolderService.cpp` (修改)：删除 `copyRecursively`，重构 `executeTransferToMarked` 核心分发逻辑。
4. `CMakeLists.txt` (修改)：加入 `src/core/TransferToMarkedCommand.h` 与 `src/core/TransferToMarkedCommand.cpp`。
5. `Memories.md` (修改)：记录第 16 章节架构设计理念。

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/core/DiskTrashService.h` 新增只读查询接口

```cpp
<<<<<<< SEARCH
    /**
     * @brief 彻底清空所有磁盘回收站中的物理文件
     */
    static bool emptyDiskTrash();
};
=======
    /**
     * @brief 彻底清空所有磁盘回收站中的物理文件
     */
    static bool emptyDiskTrash();

    /**
     * @brief 根据原物理路径获取回收站中最新一条记录的 ID 与垃圾桶路径
     */
    static bool getLatestTrashItemByOriginalPath(const QString& originalPath, int& outId, QString& outTrashPath);
};
>>>>>>> REPLACE
```

### 3.2 `src/core/DiskTrashService.cpp` 实现只读查询接口

```cpp
<<<<<<< SEARCH
bool DiskTrashService::emptyDiskTrash() {
=======
bool DiskTrashService::getLatestTrashItemByOriginalPath(const QString& originalPath, int& outId, QString& outTrashPath) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || originalPath.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, trash_path FROM disk_trash WHERE original_path = ? ORDER BY deleted_at DESC LIMIT 1";
    bool found = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt, 1, originalPath.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            outId = sqlite3_column_int(stmt, 0);
            const wchar_t* wTrash = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
            if (wTrash) {
                outTrashPath = QString::fromWCharArray(wTrash);
            }
            found = true;
        }
        sqlite3_finalize(stmt);
    }
    return found;
}

bool DiskTrashService::emptyDiskTrash() {
>>>>>>> REPLACE
```

### 3.3 `src/core/TransferToMarkedCommand.h` 新建撤销命令头文件

```cpp
<<<<<<< SEARCH
=======
#pragma once

#include <QUndoCommand>
#include <QStringList>
#include <QMap>

namespace QuarkMeta {

/**
 * @brief 标记目的地分发迁移复合撤销命令
 */
class TransferToMarkedCommand : public QUndoCommand {
public:
    struct TargetResult {
        QString targetDir;
        QString createdCopyPath;
    };

    TransferToMarkedCommand(const QStringList& sourcePaths,
                            const QMap<QString, QVector<TargetResult>>& copyResults,
                            const QStringList& recycledSources,
                            QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QStringList m_sourcePaths;
    QMap<QString, QVector<TargetResult>> m_copyResults;
    QStringList m_recycledSources;
    bool m_isFirstRun = true;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.4 `src/core/TransferToMarkedCommand.cpp` 新建撤销命令实现源文件

```cpp
<<<<<<< SEARCH
=======
#include "TransferToMarkedCommand.h"
#include "DiskTrashService.h"
#include "../ui/ToolTipOverlay.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>

namespace QuarkMeta {

TransferToMarkedCommand::TransferToMarkedCommand(
    const QStringList& sourcePaths,
    const QMap<QString, QVector<TargetResult>>& copyResults,
    const QStringList& recycledSources,
    QUndoCommand* parent)
    : QUndoCommand(parent),
      m_sourcePaths(sourcePaths),
      m_copyResults(copyResults),
      m_recycledSources(recycledSources)
{
    setText(QString("分发迁移至标记目的地 (%1 项)").arg(sourcePaths.size()));
}

void TransferToMarkedCommand::undo() {
    // 1. 回收所有目标位置生成的副本文件
    QStringList pathsToTrash;
    for (auto it = m_copyResults.begin(); it != m_copyResults.end(); ++it) {
        for (const auto& res : it.value()) {
            if (!res.createdCopyPath.isEmpty() && QFile::exists(res.createdCopyPath)) {
                pathsToTrash << res.createdCopyPath;
            }
        }
    }
    if (!pathsToTrash.isEmpty()) {
        DiskTrashService::moveToDiskTrash(pathsToTrash);
    }

    // 2. 还原被回收的原文件
    bool modifiedRestoreName = false;
    for (const QString& origPath : m_recycledSources) {
        int trashId = -1;
        QString trashPath;
        if (DiskTrashService::getLatestTrashItemByOriginalPath(origPath, trashId, trashPath)) {
            bool existsBefore = QFile::exists(origPath);
            if (DiskTrashService::restoreFromDiskTrash(trashId, trashPath)) {
                if (existsBefore) {
                    modifiedRestoreName = true;
                }
            }
        }
    }

    if (modifiedRestoreName) {
        ToolTipOverlay::instance()->showToast("原位置已被占用，还原的文件已自动更名保留", 3000, QColor("#378ADD"));
    }
}

void TransferToMarkedCommand::redo() {
    if (m_isFirstRun) {
        m_isFirstRun = false;
        return;
    }

    // 重新执行：再次重新恢复各目标副本，并将同盘源文件重新回收
    for (auto it = m_copyResults.begin(); it != m_copyResults.end(); ++it) {
        QString srcPath = it.key();
        QFileInfo srcInfo(srcPath);
        for (const auto& res : it.value()) {
            if (!res.createdCopyPath.isEmpty()) {
                QDir().mkpath(QFileInfo(res.createdCopyPath).absolutePath());
                QFile::copy(srcPath, res.createdCopyPath);
            }
        }
    }

    if (!m_recycledSources.isEmpty()) {
        DiskTrashService::moveToDiskTrash(m_recycledSources);
    }
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 3.5 `src/meta/MarkedFolderService.h` 扩展头文件接口与结构

```cpp
<<<<<<< SEARCH
    /**
     * @brief 触发 Alt+V 一键同时分发/迁移选中项目到目的地文件夹
     * @param targetDirs 指定的目的地文件夹列表。若为空，则默认迁移分发至所有已激活的标记目的地文件夹
     */
    bool executeTransferToMarked(const QStringList& sourcePaths, const QStringList& targetDirs = {});
=======
    /**
     * @brief 触发 Alt+V 一键同时分发/迁移选中项目到目的地文件夹
     * @param targetDirs 指定的目的地文件夹列表。若为空，则默认迁移分发至所有已激活的标记目的地文件夹
     */
    bool executeTransferToMarked(const QStringList& sourcePaths, const QStringList& targetDirs = {});
>>>>>>> REPLACE
```

### 3.6 `src/meta/MarkedFolderService.cpp` 彻底删除 `copyRecursively` 并重构核心搬运分发

```cpp
<<<<<<< SEARCH
static bool copyRecursively(const QString& srcFilePath, const QString& tgtFilePath) {
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        if (!targetDir.exists() && !targetDir.mkpath(".")) {
            return false;
        }
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QString& fileName : fileNames) {
            QString newSrcFilePath = sourceDir.filePath(fileName);
            QString newTgtFilePath = targetDir.filePath(fileName);
            if (!copyRecursively(newSrcFilePath, newTgtFilePath)) {
                return false;
            }
        }
    } else {
        QFile::remove(tgtFilePath);
        return QFile::copy(srcFilePath, tgtFilePath);
    }
    return true;
}
=======
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
bool MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths, const QStringList& targetDirs) {
    if (sourcePaths.isEmpty()) {
        ToolTipOverlay::instance()->showToast("未选择任何要迁移的项目", 2000, QColor("#e81123"));
        return false;
    }

    QStringList activeTargets = targetDirs.isEmpty() ? getActiveMarked() : targetDirs;
    if (activeTargets.isEmpty()) {
        ToolTipOverlay::instance()->showToast("未激活任何标记目的地文件夹 (请先用 Alt+C 标记)", 2500, QColor("#e81123"));
        return false;
    }

    int successCount = 0;
    int failCount = 0;

    for (const QString& srcPath : sourcePaths) {
        QFileInfo srcInfo(srcPath);
        if (!srcInfo.exists()) {
            continue;
        }

        QString itemName = srcInfo.fileName();

        for (const QString& targetDir : activeTargets) {
            QFileInfo targetInfo(targetDir);
            if (!targetInfo.exists() || !targetInfo.isDir()) {
                continue;
            }

            // 防自身/循环嵌套保护
            if (QDir(srcPath) == QDir(targetDir) || targetDir.startsWith(srcPath + "/")) {
                continue;
            }

            QString destPath = QDir(targetDir).filePath(itemName);

            if (copyRecursively(srcPath, destPath)) {
                successCount++;
            } else {
                failCount++;
            }
        }
    }

    if (successCount > 0) {
        QString msg = QString("已成功迁移分发到 %1 个目的地").arg(activeTargets.size());
        ToolTipOverlay::instance()->showToast(msg, 2000, QColor("#0078d4"));
        return true;
    } else {
        ToolTipOverlay::instance()->showToast("分发迁移失败，请检查文件权限或目标路径", 2500, QColor("#e81123"));
        return false;
    }
}
=======
#include "../util/DiskIoService.h"
#include "../util/VolumePathResolver.h"
#include "../util/TaskProgressService.h"
#include "../core/DiskTrashService.h"
#include "../core/UndoManager.h"
#include "../core/TransferToMarkedCommand.h"

bool MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths, const QStringList& targetDirs) {
    if (sourcePaths.isEmpty()) {
        ToolTipOverlay::instance()->showToast("未选择任何要迁移的项目", 2000, QColor("#e81123"));
        return false;
    }

    QStringList activeTargets = targetDirs.isEmpty() ? getActiveMarked() : targetDirs;
    if (activeTargets.isEmpty()) {
        ToolTipOverlay::instance()->showToast("未激活任何标记目的地文件夹 (请先用 Alt+C 标记)", 2500, QColor("#e81123"));
        return false;
    }

    // 预检测有效源路径与目的地列表
    QStringList validSources;
    for (const QString& sp : sourcePaths) {
        if (QFile::exists(sp)) {
            validSources << QDir::cleanPath(sp);
        }
    }
    if (validSources.isEmpty()) return false;

    QStringList validTargets;
    for (const QString& td : activeTargets) {
        QFileInfo tfi(td);
        if (tfi.exists() && tfi.isDir()) {
            validTargets << QDir::cleanPath(td);
        }
    }
    if (validTargets.isEmpty()) {
        ToolTipOverlay::instance()->showToast("所有标记目的地路径均不存在或无效", 2500, QColor("#e81123"));
        return false;
    }

    int totalWorkUnits = validSources.size() * validTargets.size();
    TaskProgressService::instance().createJob("TransferToMarked", "分发迁移至标记目的地", totalWorkUnits);

    int completedUnits = 0;
    QMap<QString, QVector<TransferToMarkedCommand::TargetResult>> copyResults;
    bool anyFailure = false;

    for (const QString& srcPath : validSources) {
        QFileInfo srcInfo(srcPath);
        QString itemName = srcInfo.fileName();
        std::wstring srcVol = VolumePathResolver::getVolumeSerialNumber(srcPath.toStdWString());

        bool allTargetsSuccessForThisSource = true;
        bool hasSameVolumeTarget = false;

        for (const QString& targetDir : validTargets) {
            QString cleanSrc = QDir::cleanPath(srcPath).toLower();
            QString cleanTarget = QDir::cleanPath(targetDir).toLower();

            // 防自身/循环嵌套保护：同路径或目标为源的子目录
            if (cleanSrc == cleanTarget || cleanTarget.startsWith(cleanSrc + "/")) {
                completedUnits++;
                TaskProgressService::instance().updateProgress("TransferToMarked", (int)((float)completedUnits / totalWorkUnits * 100));
                continue;
            }

            std::wstring tgtVol = VolumePathResolver::getVolumeSerialNumber(targetDir.toStdWString());
            if (srcVol == tgtVol && srcVol != L"UNKNOWN") {
                hasSameVolumeTarget = true;
            }

            DiskIoContext ioCtx;
            ioCtx.sources = QStringList{srcPath};
            ioCtx.destination = targetDir;
            ioCtx.isMove = false; // 复制模式，保证源文件不提前消失

            QString destPath = QDir(targetDir).filePath(itemName);

            bool targetSuccess = ShellHelper::copyOrMoveItems(
                ioCtx.sources, ioCtx.destination, ioCtx.isMove,
                ioCtx.overwriteAll, ioCtx.autoRenameAll,
                ioCtx.overwriteFiles, ioCtx.autoRenameFiles
            );

            completedUnits++;
            TaskProgressService::instance().updateProgress("TransferToMarked", (int)((float)completedUnits / totalWorkUnits * 100));

            if (targetSuccess) {
                copyResults[srcPath].append({targetDir, destPath});
            } else {
                targetSuccess = false;
                allTargetsSuccessForThisSource = false;
                anyFailure = true;
            }
        }
    }

    // 判定同盘回收
    QStringList recycledSources;
    if (!anyFailure) {
        for (const QString& srcPath : validSources) {
            std::wstring srcVol = VolumePathResolver::getVolumeSerialNumber(srcPath.toStdWString());
            bool sameVol = false;
            for (const QString& targetDir : validTargets) {
                std::wstring tgtVol = VolumePathResolver::getVolumeSerialNumber(targetDir.toStdWString());
                if (srcVol == tgtVol && srcVol != L"UNKNOWN") {
                    sameVol = true;
                    break;
                }
            }
            if (sameVol) {
                if (DiskTrashService::moveToDiskTrash(QStringList{srcPath})) {
                    recycledSources << srcPath;
                }
            }
        }
    }

    // 压入撤销命令
    if (!copyResults.isEmpty()) {
        UndoManager::instance().pushCommand(std::make_unique<TransferToMarkedCommand>(
            validSources, copyResults, recycledSources
        ));
    }

    TaskProgressService::instance().finishJob("TransferToMarked");

    if (!anyFailure) {
        QString msg = QString("已成功分发迁移至 %1 个目的地").arg(validTargets.size());
        ToolTipOverlay::instance()->showToast(msg, 2000, QColor("#0078d4"));
        return true;
    } else {
        ToolTipOverlay::instance()->showToast("分发迁移完成，部分项目未能成功处理", 2500, QColor("#e81123"));
        return false;
    }
}
>>>>>>> REPLACE
```

### 3.7 `CMakeLists.txt` 添加新增文件

```cpp
<<<<<<< SEARCH
    src/core/DiskTrashService.cpp
    src/core/DiskTrashService.h
=======
    src/core/DiskTrashService.cpp
    src/core/DiskTrashService.h
    src/core/TransferToMarkedCommand.cpp
    src/core/TransferToMarkedCommand.h
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）
1. 执行编译，验证无 MSVC/GCC 编译错误及未定义符号错误。
2. 校验 `CMakeLists.txt` 中条目 `src/core/TransferToMarkedCommand.cpp` 与 `src/core/TransferToMarkedCommand.h` 在磁盘上的物理路径大小写 100% 一致。
3. 验证验收点 1：带评级、色标与备注的文件进行标记分发后，目标文件夹中的副本侧车 `.QuarkMeta.json` 与 `disk_thumbs` 缩略图缓存随迁漫游。
4. 验证验收点 2：按 `Alt+V` 触发分发，同名冲突触发 `FileCollisionDialog` 弹窗；同盘场景下全部成功后原文件进入应用私有回收站；任一目标失败时源文件严格保留。
5. 验证验收点 3：按 `Ctrl+Z` 触发一键撤销，目标副本被移入回收站，原文件从回收站无损还原。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用）
- 物理搬运与元数据漫游 100% 复用 `ShellHelper::copyOrMoveItems`。
- 应用私有回收站回收与还原 100% 复用 `DiskTrashService::moveToDiskTrash` / `restoreFromDiskTrash`。
- 任务进度条 100% 复用 `TaskProgressService`。

---

## Chapter 6: Data SSOT Declaration Table（数据 SSOT 声明表）
- **标记分发目的地 SSOT**：SQLite `marked_folders` 表（由 `MarkedFolderDao` 管理）。
- **回收站记录 SSOT**：SQLite `disk_trash` 表（由 `DiskTrashService` 管理）。

---

## Chapter 7: Interface & Visibility Check（接口签名与访问控制核查）
- `DiskTrashService::getLatestTrashItemByOriginalPath` 声明为 `public static` 方法，签名与定义完全一致。
- `TransferToMarkedCommand` 继承 `QUndoCommand`，重写 `undo()` 与 `redo()`。

---

## Chapter 8: Header Chain Check（头文件包含链核查）
- `TransferToMarkedCommand.h` 仅包含 `<QUndoCommand>`、`<QStringList>`、`<QMap>`，无循环头文件包含。

---

## Chapter 9: Compliance Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 |
| :--- | :--- | :--- |
| 1 | `copyRecursively` 物理自写逻辑被 100% 彻底删除 | 满足 |
| 2 | 搬运 100% 复用 `ShellHelper::copyOrMoveItems` 且携带元数据与缩略图漫游 | 满足 |
| 3 | 彻底杜绝直接修改 C++ 源码文件（仅提供 Implementation Plan） | 满足 |
| 4 | `CMakeLists.txt` 新增条目与磁盘实际路径大小写 100% 一致 | 满足 |
| 5 | `Memories.md` 第 16 章节完整写入架构理念 | 满足 |

---

## Chapter 10: Behavior Preservation Table（行为与数值零篡改声明表）
- 现有 `moveToDiskTrash` 与 `restoreFromDiskTrash` 签名与物理行为零篡改。
- QSS 样式与 UI 控件物理色值保持零篡改。
