#include "MarkedFolderService.h"
#include "MarkedFolderDao.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include "../core/CentralEventHub.h"
#include <QFileInfo>
#include <QDir>
#include <QAction>

namespace QuarkMeta {

MarkedFolderService& MarkedFolderService::instance() {
    static MarkedFolderService s_instance;
    return s_instance;
}

MarkedFolderService::MarkedFolderService(QObject* parent) : QObject(parent) {
    MarkedFolderDao::initTable();
    refreshCache();

    // 监听全局事件：当文件夹或其父级目录被删除/重命名时自动清除标记状态 (is_marked = 0)
    connect(&CentralEventHub::instance(), &CentralEventHub::eventOccurred, this, [this](const QuarkMeta::AppEvent& event) {
        if (event.type == AppEventType::ItemsDeleted || event.type == AppEventType::ItemsRenamed) {
            bool changed = false;
            for (const QString& evPath : event.paths) {
                QString cleanEvPath = QDir::cleanPath(evPath);
                for (const QString& markedPath : m_activeMarkedCache) {
                    if (markedPath.compare(cleanEvPath, Qt::CaseInsensitive) == 0 ||
                        markedPath.startsWith(cleanEvPath + "/", Qt::CaseInsensitive)) {
                        MarkedFolderDao::unmarkFolder(markedPath);
                        changed = true;
                    }
                }
            }
            if (changed) {
                refreshCache();
                emit markedFoldersChanged();
            }
        }
    });
}

void MarkedFolderService::refreshCache() {
    m_activeMarkedCache.clear();
    QStringList activeList = MarkedFolderDao::getActiveMarkedFolders();
    for (const QString& p : activeList) {
        m_activeMarkedCache.insert(QDir::cleanPath(p));
    }
}

bool MarkedFolderService::isMarked(const QString& path) const {
    if (path.isEmpty()) return false;
    return m_activeMarkedCache.contains(QDir::cleanPath(path));
}

int MarkedFolderService::markedCount() const {
    return MarkedFolderDao::getMarkedCount();
}

bool MarkedFolderService::addMarked(const QString& path) {
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return false;

    if (MarkedFolderDao::addMarkedFolder(path)) {
        refreshCache();
        emit markedFoldersChanged();
        ToolTipOverlay::instance()->showToast("已标记为目的地", 1500, QColor("#378ADD"));
        return true;
    }
    return false;
}

bool MarkedFolderService::unmarkFolder(const QString& path) {
    if (MarkedFolderDao::unmarkFolder(path)) {
        refreshCache();
        emit markedFoldersChanged();
        ToolTipOverlay::instance()->showToast("已取消标记", 1500, QColor("#888888"));
        return true;
    }
    return false;
}

bool MarkedFolderService::unmarkAllFolders() {
    if (MarkedFolderDao::unmarkAllFolders()) {
        refreshCache();
        emit markedFoldersChanged();
        ToolTipOverlay::instance()->showToast("已取消所有标记", 1500, QColor("#888888"));
        return true;
    }
    return false;
}

bool MarkedFolderService::removeFolderRecord(const QString& path) {
    if (MarkedFolderDao::removeFolderRecord(path)) {
        refreshCache();
        emit markedFoldersChanged();
        ToolTipOverlay::instance()->showToast("已从标记历史中移除", 1500, QColor("#888888"));
        return true;
    }
    return false;
}

bool MarkedFolderService::clearAllFolderRecords() {
    if (MarkedFolderDao::clearAllFolderRecords()) {
        refreshCache();
        emit markedFoldersChanged();
        ToolTipOverlay::instance()->showToast("已清空标记历史", 1500, QColor("#888888"));
        return true;
    }
    return false;
}

bool MarkedFolderService::toggleMarked(const QString& path) {
    if (isMarked(path)) {
        return unmarkFolder(path);
    } else {
        return addMarked(path);
    }
}

QStringList MarkedFolderService::getAllMarked() const {
    return MarkedFolderDao::getAllMarkedFolders();
}

QStringList MarkedFolderService::getActiveMarkedFolders() const {
    return MarkedFolderDao::getActiveMarkedFolders();
}

QAction* MarkedFolderService::buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver) {
    if (!parentMenu) return nullptr;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return nullptr;

    bool marked = isMarked(path);
    QString label = marked ? "取消标记" : "标记为目的地";
    QIcon icon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 18);

    QAction* action = parentMenu->addAction(icon, label);
    if (receiver) {
        connect(action, &QAction::triggered, receiver, [this, path]() {
            toggleMarked(path);
        });
    }
    return action;
}

} // namespace QuarkMeta
