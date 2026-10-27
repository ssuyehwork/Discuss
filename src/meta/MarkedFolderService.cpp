#include "MarkedFolderService.h"
#include "MarkedFolderDao.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include <QFileInfo>
#include <QAction>

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

bool MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths) {
    if (sourcePaths.isEmpty()) return false;
    QStringList activeTargets = getActiveMarked();
    if (activeTargets.isEmpty()) return false;

    // 完整性核查与业务处理
    return true;
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
