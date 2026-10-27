#include "MarkedFolderService.h"
#include "MarkedFolderDao.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QAction>

namespace QuarkMeta {

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

    QString label = allMarked ? "取消标记" : "标记为目的地";
    QIcon icon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 18);

    QAction* action = parentMenu->addAction(icon, label);
    if (receiver) {
        connect(action, &QAction::triggered, receiver, [this, validDirs]() {
            toggleMarked(validDirs);
        });
    }
    return action;
}

} // namespace QuarkMeta
