#include "DiskFileManager.h"
#include "../meta/MetadataManager.h"
#include "../meta/CategoryRepo.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>

namespace ArcMeta {

int DiskFileManager::batchRenameDiskFiles(const std::vector<std::wstring>& originalPaths,
                                         const std::vector<std::wstring>& newNames,
                                         const QString& targetDir,
                                         bool isCopy,
                                         bool isMove) {
    int successCount = 0;

    // 🚨 开启防抖与内部操作锁定
    MetadataManager::instance().beginInternalOperation();

    for (int i = 0; i < (int)originalPaths.size(); ++i) {
        QString oldPath = QString::fromStdWString(originalPaths[i]);
        QFileInfo oldInfo(oldPath);
        QString finalTargetDir = (!isCopy && !isMove) ? oldInfo.absolutePath() : targetDir;
        QString newPathStr = QDir(finalTargetDir).filePath(QString::fromStdWString(newNames[i]));

        bool ok = false;
        if (isCopy) {
            ok = QFile::copy(oldPath, newPathStr);
        } else if (isMove) {
            if (QFile::copy(oldPath, newPathStr)) {
                ok = QFile::remove(oldPath);
            }
        } else {
            ok = QFile::rename(oldPath, newPathStr);
        }

        if (ok) {
            successCount++;
            if (!isCopy) {
                std::wstring oldW = oldInfo.absoluteFilePath().toStdWString();
                std::wstring newW = QDir(finalTargetDir).absoluteFilePath(QString::fromStdWString(newNames[i])).toStdWString();

                // 同步进行磁盘离散元数据、哈希 JSON 的重命名与平滑迁移
                MetadataManager::instance().renameItem(oldW, newW);
                CategoryRepo::renamePhysicalCategoryPath(oldW, newW);
            }
        }
    }

    // 🚨 关闭内部操作锁定并提交
    MetadataManager::instance().endInternalOperation();

    // 发射全量 UI 刷新信号
    MetadataManager::instance().notifyFullUIRebuild();

    return successCount;
}

} // namespace ArcMeta
