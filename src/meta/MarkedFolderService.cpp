#include "MarkedFolderService.h"
#include "MarkedFolderDao.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QAction>
#include "../util/ShellHelper.h"
#include "../util/DiskIoService.h"
#include "../util/VolumePathResolver.h"
#include "../core/TaskProgressService.h"
#include "../core/DiskTrashService.h"
#include "../core/UndoManager.h"
#include "../core/TransferToMarkedCommand.h"

namespace QuarkMeta {

MarkedFolderService& MarkedFolderService::instance() {
    static MarkedFolderService s_instance;
    return s_instance;
}

MarkedFolderService::MarkedFolderService(QObject* parent) : QObject(parent) {
    MarkedFolderDao::initTable();
}

bool MarkedFolderService::isMarked(const QString& path) const {
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return false;
    return MarkedFolderDao::isMarked(path);
}

int MarkedFolderService::markedCount() const {
    return MarkedFolderDao::getMarkedCount();
}

bool MarkedFolderService::addMarked(const QString& path) {
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return false;

    if (markedCount() >= 50 && !isMarked(path)) {
        ToolTipOverlay::instance()->showToast("标记数量已达 50 个上限，请先移除部分标记", 2000, QColor("#e81123"));
        return false;
    }

    if (MarkedFolderDao::addMarkedFolder(path)) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::keepOnlyMarked(const QString& targetPath) {
    if (targetPath.isEmpty()) return false;
    if (MarkedFolderDao::unmarkAllFolders()) {
        bool ok = addMarked(targetPath);
        emit markedFoldersChanged();
        return ok;
    }
    return false;
}

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
    int jobId = TaskProgressService::instance().createJob("分发迁移至标记目的地", totalWorkUnits);

    int completedUnits = 0;
    QMap<QString, QVector<TransferToMarkedCommand::TargetResult>> copyResults;
    bool anyFailure = false;

    for (const QString& srcPath : validSources) {
        QFileInfo srcInfo(srcPath);
        QString itemName = srcInfo.fileName();
        std::wstring srcVol = VolumePathResolver::getVolumeSerialNumber(srcPath.toStdWString());

        for (const QString& targetDir : validTargets) {
            QString cleanSrc = QDir::cleanPath(srcPath).toLower();
            QString cleanTarget = QDir::cleanPath(targetDir).toLower();

            // 防自身/循环嵌套保护：同路径或目标为源的子目录
            if (cleanSrc == cleanTarget || cleanTarget.startsWith(cleanSrc + "/")) {
                completedUnits++;
                TaskProgressService::instance().updateProgress(jobId, completedUnits, totalWorkUnits);
                continue;
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
            TaskProgressService::instance().updateProgress(jobId, completedUnits, totalWorkUnits);

            if (targetSuccess) {
                copyResults[srcPath].append({targetDir, destPath});
            } else {
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

    TaskProgressService::instance().finishJob(jobId);

    if (!anyFailure) {
        QString msg = QString("已成功分发迁移至 %1 个目的地").arg(validTargets.size());
        ToolTipOverlay::instance()->showToast(msg, 2000, QColor("#0078d4"));
        return true;
    } else {
        ToolTipOverlay::instance()->showToast("分发迁移完成，部分项目未能成功处理", 2500, QColor("#e81123"));
        return false;
    }
}

bool MarkedFolderService::removeMarked(const QString& path) {
    if (MarkedFolderDao::unmarkFolder(path)) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::toggleMarked(const QString& path) {
    if (isMarked(path)) {
        return removeMarked(path);
    } else {
        return addMarked(path);
    }
}

bool MarkedFolderService::toggleMarked(const QStringList& paths) {
    if (paths.isEmpty()) return false;
    bool allMarked = true;
    for (const QString& p : paths) {
        if (!isMarked(p)) {
            allMarked = false;
            break;
        }
    }

    bool changed = false;
    if (allMarked) {
        for (const QString& p : paths) {
            if (removeMarked(p)) changed = true;
        }
    } else {
        for (const QString& p : paths) {
            if (!isMarked(p)) {
                if (addMarked(p)) changed = true;
            }
        }
    }
    return changed;
}

QStringList MarkedFolderService::getAllMarked() const {
    return MarkedFolderDao::getAllMarkedFolders();
}

QStringList MarkedFolderService::getActiveMarked() const {
    return MarkedFolderDao::getActiveMarkedFolders();
}

bool MarkedFolderService::removeFolderRecord(const QString& path) {
    if (MarkedFolderDao::removeFolderRecord(path)) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::clearAllFolderRecords() {
    if (MarkedFolderDao::clearAllFolderRecords()) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::clearAll() {
    if (MarkedFolderDao::unmarkAllFolders()) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

QAction* MarkedFolderService::buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver) {
    return buildMarkAction(parentMenu, QStringList{path}, receiver);
}

QAction* MarkedFolderService::buildMarkAction(QMenu* parentMenu, const QStringList& paths, QObject* receiver) {
    if (!parentMenu || paths.isEmpty()) return nullptr;

    QStringList validDirs;
    for (const QString& p : paths) {
        QFileInfo fi(p);
        if (fi.exists() && fi.isDir()) {
            validDirs << p;
        }
    }
    if (validDirs.isEmpty()) return nullptr;

    bool allMarked = true;
    for (const QString& p : validDirs) {
        if (!isMarked(p)) {
            allMarked = false;
            break;
        }
    }

    QString label = allMarked ? "取消标记" : "标记为目标文件夹";
    QIcon icon = UiHelper::getIcon("Destination.svg", QColor("#EEEEEE"), 18);

    QAction* action = parentMenu->addAction(icon, label);
    if (receiver) {
        connect(action, &QAction::triggered, receiver, [this, validDirs]() {
            toggleMarked(validDirs);
        });
    }
    return action;
}

} // namespace QuarkMeta
