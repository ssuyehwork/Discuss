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

namespace QuarkMeta {

bool ShellHelper::moveToTrash(const QStringList& paths) {
    return DiskTrashService::moveToDiskTrash(paths);
}

bool ShellHelper::copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove) {
#ifdef Q_OS_WIN
    if (sourcePaths.isEmpty() || destDir.isEmpty()) return false;
    
    std::wstring from;
    for (const QString& p : sourcePaths) {
        from += QDir::toNativeSeparators(p).toStdWString() + L'\0';
    }
    from += L'\0';

    std::wstring to = QDir::toNativeSeparators(destDir).toStdWString() + L'\0' + L'\0';

    SHFILEOPSTRUCTW fileOp = { 0 };
    fileOp.wFunc = isMove ? FO_MOVE : FO_COPY;
    fileOp.pFrom = from.c_str();
    fileOp.pTo = to.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
    bool ok = (SHFileOperationW(&fileOp) == 0);
    if (ok && isMove) {
        for (const QString& p : sourcePaths) {
            QFileInfo info(p);
            QString newPath = QDir(destDir).filePath(info.fileName());
            // 1. 物理漫游迁移 .QuarkMeta.json 元数据 
            QuarkMetaJson::migrateItemMetadata(p, newPath); 
            // 2. 同步内存/数据库缓存 
            MetadataManager::instance().renameItem(p.toStdWString(), newPath.toStdWString());
        }
    }
    return ok;
#else
    Q_UNUSED(sourcePaths);
    Q_UNUSED(destDir);
    Q_UNUSED(isMove);
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
