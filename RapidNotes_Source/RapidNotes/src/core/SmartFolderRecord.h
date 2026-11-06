#pragma once
#include <QString>
#include <QList>
#include <QMap>

namespace RapidNotes {

struct SmartFolderRecord {
    int id = 0;
    QString name;
    QString rules;
    QString iconKey = "smart-folder-1.svg";
    QString colorHex = "#888888";
};

class SmartFolderDao {
public:
    static QList<SmartFolderRecord> getAllSmartFolders() { return {}; }
    static SmartFolderRecord getSmartFolderById(int id) {
        Q_UNUSED(id);
        return {};
    }
    static int addSmartFolder(const SmartFolderRecord& rec) {
        Q_UNUSED(rec);
        return 0;
    }
    static bool updateSmartFolder(const SmartFolderRecord& rec) {
        Q_UNUSED(rec);
        return true;
    }
    static bool updateSmartFolderOrder(const QList<int>& ids) {
        Q_UNUSED(ids);
        return true;
    }
    static bool removeSmartFolderById(int id) {
        Q_UNUSED(id);
        return true;
    }
    static QMap<int, int> countMatchingItemsBatchReadOnly(const QString& dbPath, const QList<SmartFolderRecord>& smartFolders) {
        Q_UNUSED(dbPath);
        Q_UNUSED(smartFolders);
        return {};
    }
};

} // namespace RapidNotes
