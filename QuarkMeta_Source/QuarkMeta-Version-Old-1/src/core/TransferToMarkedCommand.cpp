#include "TransferToMarkedCommand.h"
#include "DiskTrashService.h"
#include "../ui/ToolTipOverlay.h"
#include "../util/ShellHelper.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>

namespace QuarkMeta {

TransferToMarkedCommand::TransferToMarkedCommand(
    const QStringList& sourcePaths,
    const QMap<QString, QVector<TargetResult>>& copyResults,
    const QStringList& recycledSources)
    : m_sourcePaths(sourcePaths),
      m_copyResults(copyResults),
      m_recycledSources(recycledSources) 
{
}

QString TransferToMarkedCommand::description() const {
    return QString("分发迁移至标记目的地 (%1 项)").arg(m_sourcePaths.size());
}

bool TransferToMarkedCommand::affectsPath(const QString& path) const {
    for (const QString& src : m_sourcePaths) {
        if (src.compare(path, Qt::CaseInsensitive) == 0) return true;
    }
    return false;
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
        for (const auto& res : it.value()) {
            if (!res.createdCopyPath.isEmpty()) {
                QDir().mkpath(QFileInfo(res.createdCopyPath).absolutePath());
                ShellHelper::copyOrMoveItems({srcPath}, res.targetDir, false, true, false);
            }
        }
    }

    if (!m_recycledSources.isEmpty()) {
        DiskTrashService::moveToDiskTrash(m_recycledSources);
    }
}

} // namespace QuarkMeta
