#include "ShellHelper.h"
#include <QFileInfo>
#include <QDateTime>
#include <atomic>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

#include "../meta/MetadataManager.h"
#include "../meta/StatisticsService.h"
#include "../meta/QuarkMetaJson.h"
#include "../core/DiskTrashService.h"
#include "DiskMediaExtractor.h"

namespace QuarkMeta {

bool ShellHelper::moveToTrash(const QStringList& paths) {
    return DiskTrashService::moveToDiskTrash(paths);
}

bool ShellHelper::copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove,
                                   bool overwriteAll, bool autoRenameAll,
                                   const QSet<QString>& overwriteFiles, const QSet<QString>& autoRenameFiles) {
#ifdef Q_OS_WIN
    if (sourcePaths.isEmpty() || destDir.isEmpty()) return false;
    
    bool overallOk = true;
    for (const QString& p : sourcePaths) {
        QFileInfo info(p);
        QString destPath = QDir(destDir).filePath(info.fileName());

        bool shouldAutoRename = autoRenameAll || autoRenameFiles.contains(p);
        bool shouldOverwrite = overwriteAll || overwriteFiles.contains(p);

        if (QFile::exists(destPath)) {
            if (shouldAutoRename) {
                QString baseName = info.completeBaseName();
                QString suffix = info.suffix();
                int counter = 1;
                while (QFile::exists(destPath)) {
                    QString newFileName = suffix.isEmpty() 
                        ? QString("%1 (%2)").arg(baseName).arg(counter++)
                        : QString("%1 (%2).%3").arg(baseName).arg(counter++).arg(suffix);
                    destPath = QDir(destDir).filePath(newFileName);
                }
            } else if (shouldOverwrite) {
                QFile::remove(destPath);
            }
        }

        std::wstring from = QDir::toNativeSeparators(p).toStdWString() + L'\0' + L'\0';
        std::wstring to = QDir::toNativeSeparators(destPath).toStdWString() + L'\0' + L'\0';

        SHFILEOPSTRUCTW fileOp = { 0 };
        fileOp.wFunc = isMove ? FO_MOVE : FO_COPY;
        fileOp.pFrom = from.c_str();
        fileOp.pTo = to.c_str();
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_SILENT | FOF_MULTIDESTFILES;
        
        bool ok = (SHFileOperationW(&fileOp) == 0 && !fileOp.fAnyOperationsAborted);
        if (ok) {
            QuarkMetaJson::roamItemMetadata(p, destPath, isMove);
            DiskMediaExtractor::roamThumbnailCache(p, destPath, isMove);
        } else {
            overallOk = false;
        }
    }
    return overallOk;
#else
    Q_UNUSED(sourcePaths);
    Q_UNUSED(destDir);
    Q_UNUSED(isMove);
    Q_UNUSED(overwrite);
    Q_UNUSED(autoRename);
    return false;
#endif
}

void ShellHelper::showProperties(const QString& path) {
#ifdef Q_OS_WIN
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_INVOKEIDLIST;
    sei.lpVerb = L"properties";
    std::wstring wpath = QDir::toNativeSeparators(path).toStdWString();
    sei.lpFile = wpath.c_str();
    sei.nShow = SW_SHOW;
    ShellExecuteExW(&sei);
#else
    Q_UNUSED(path);
#endif
}

void ShellHelper::openInExplorer(const QString& path) {
#ifdef Q_OS_WIN
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(path);
    QProcess::startDetached("explorer", args);
#else
    Q_UNUSED(path);
#endif
}

bool ShellHelper::renameItem(const QString& oldPath, const QString& newPath) {
    if (QFile::rename(oldPath, newPath)) {
        // 1. 物理漫游迁移 .QuarkMeta.json 元数据 
        QuarkMetaJson::migrateItemMetadata(oldPath, newPath);
        // 同步数据库
        MetadataManager::instance().renameItem(oldPath.toStdWString(), newPath.toStdWString());
        return true;
    }
    return false;
}

QString ShellHelper::formatSize(qint64 bytes) {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
    if (bytes < 1024LL * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

void ShellHelper::ensureHidden(const std::wstring& path) {
#ifdef Q_OS_WIN
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
    Q_UNUSED(path);
#endif
}

} // namespace QuarkMeta
