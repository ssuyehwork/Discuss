#include "DbFileSystemHelper.h"
#include <windows.h>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace ArcMeta {

void DbFileSystemHelper::ensureFileHidden(const std::wstring& path) {
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
}

QString DbFileSystemHelper::handleDriveDriftRename(const std::wstring& volumeSerial, const QString& driveLetter, const QString& currentPath, const QString& appDir) {
    QString cleanLetter = "";
    if (!driveLetter.isEmpty()) {
        cleanLetter = driveLetter.at(0).toUpper();
    }

    QString expectedFileName = QString("Arcmeta_%1_%2.db").arg(QString::fromStdWString(volumeSerial).toUpper()).arg(cleanLetter);
    if (!currentPath.endsWith(expectedFileName)) {
        qDebug() << "[DB] 检测到盘符漂移，执行动态迁移:" << currentPath << " -> " << expectedFileName;

        QString metaDir = appDir + "/.arcmeta";
        QString targetPath = metaDir + "/" + expectedFileName;
        qDebug() << "[DB] 准备执行重命名:" << currentPath << "->" << targetPath;

        // 如果目标已存在且不是自己，先将其移走（按用户规则重命名为无效）
        if (QFile::exists(targetPath) && targetPath != currentPath) {
            QString invalidBase = QString("%1/Arcmeta_%2_无效").arg(metaDir).arg(QString::fromStdWString(volumeSerial).toUpper());
            QString invalidPath = invalidBase + ".db";
            int counter = 1;
            while (QFile::exists(invalidPath)) {
                invalidPath = QString("%1_%2.db").arg(invalidBase).arg(counter++);
            }
            qDebug() << "[DB] 目标文件已存在，先将其重命名为无效:" << invalidPath;
            QFile::rename(targetPath, invalidPath);
        }

        if (QFile::rename(currentPath, targetPath)) {
            qDebug() << "[DB] 重命名成功";
            return targetPath;
        } else {
            qWarning() << "[DB] 重命名失败";
        }
    }
    return currentPath;
}

void DbFileSystemHelper::cleanupInvalidDatabases(const std::wstring& volumeSerial, const QString& appDir) {
    QString metaDir = appDir + "/.arcmeta";
    QString serialStr = QString::fromStdWString(volumeSerial).toUpper();
    QDir dir(metaDir);
    QStringList filters;
    filters << QString("Arcmeta_%1*.db").arg(serialStr);
    QFileInfoList list = dir.entryInfoList(filters, QDir::Files | QDir::Hidden | QDir::System, QDir::Time);

    if (list.size() > 1) {
        // 处理冲突的其他旧文件
        for (int i = 1; i < list.size(); ++i) {
            QString conflictPath = list.at(i).absoluteFilePath();
            QString invalidBase = QString("%1/Arcmeta_%2_无效").arg(metaDir).arg(serialStr);
            QString invalidPath = invalidBase + ".db";
            int counter = 1;
            while (QFile::exists(invalidPath)) {
                invalidPath = QString("%1_%2.db").arg(invalidBase).arg(counter++);
            }
            if (QFile::rename(conflictPath, invalidPath)) {
                qDebug() << "[DB] 冲突处理：将冗余数据库标记为无效" << list.at(i).fileName() << "->" << QFileInfo(invalidPath).fileName();
            }
        }
    }
}

} // namespace ArcMeta
