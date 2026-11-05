#include "NavigationHistoryService.h"
#include "AppConfig.h"
#include <QDir>

namespace QuarkMeta {

NavigationHistoryService& NavigationHistoryService::instance() {
    static NavigationHistoryService inst;
    return inst;
}

NavigationHistoryService::NavigationHistoryService(QObject* parent) : QObject(parent) {}

QStringList NavigationHistoryService::getHistory() const {
    return AppConfig::instance().getValue("AddressBar/History").toStringList();
}

void NavigationHistoryService::appendPath(const QString& rawPath) {
    if (rawPath.isEmpty() || rawPath == "computer://" || rawPath.startsWith("分类: ")) return;
    
    QString cleanP = QDir::cleanPath(rawPath);
    if (cleanP.endsWith('/') || cleanP.endsWith('\\')) {
        if (cleanP.length() > 3) {
            cleanP.chop(1);
        }
    }

    QStringList history = getHistory();
    
    for (int i = history.size() - 1; i >= 0; --i) {
        if (QDir::cleanPath(history[i]).compare(cleanP, Qt::CaseInsensitive) == 0) {
            history.removeAt(i);
        }
    }
    
    history.prepend(cleanP);
    while (history.size() > m_maxLimit) {
        history.removeLast();
    }
    AppConfig::instance().setValue("AddressBar/History", history);
    AppConfig::instance().sync();
    emit historyChanged(history);
}

void NavigationHistoryService::removePath(const QString& path) {
    QStringList history = getHistory();
    history.removeAll(path);
    AppConfig::instance().setValue("AddressBar/History", history);
    AppConfig::instance().sync();
    emit historyChanged(history);
}

void NavigationHistoryService::clearAll() {
    AppConfig::instance().setValue("AddressBar/History", QStringList());
    AppConfig::instance().sync();
    emit historyChanged(QStringList());
}

void NavigationHistoryService::recordRecentVisitedFolder(const std::wstring& path) {
    if (path.empty()) return;
    QString qPath = QString::fromStdWString(path);
    QString key = "RecentVisited/Folders";
    QStringList list = AppConfig::instance().getValue(key, QStringList()).toStringList();
    list.removeAll(qPath);
    list.prepend(qPath);
    while (list.size() > 15) list.removeLast();
    AppConfig::instance().setValue(key, list);
    AppConfig::instance().sync();
}

QStringList NavigationHistoryService::getRecentVisitedFolders(const std::wstring& volSerial) {
    Q_UNUSED(volSerial);
    QString key = "RecentVisited/Folders";
    return AppConfig::instance().getValue(key, QStringList()).toStringList();
}

} // namespace QuarkMeta
