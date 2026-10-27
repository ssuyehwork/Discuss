#pragma once

#include <QStringList>
#include <QDateTime>

namespace QuarkMeta {

struct MarkedFolderRecord {
    int id = -1;
    QString folderPath;
    bool isMarked = true;
    QDateTime updatedAt;
};

class MarkedFolderDao {
public:
    static bool initTable();
    static bool addMarkedFolder(const QString& folderPath);
    static bool unmarkFolder(const QString& folderPath);
    static bool unmarkAllFolders();
    static bool removeFolderRecord(const QString& folderPath);
    static bool clearAllFolderRecords();
    static bool isMarked(const QString& folderPath);
    static int getMarkedCount();
    static QStringList getAllMarkedFolders();
    static QStringList getActiveMarkedFolders();
};

} // namespace QuarkMeta
