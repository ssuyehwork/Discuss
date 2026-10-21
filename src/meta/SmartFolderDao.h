#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include "../core/FilterState.h"

namespace QuarkMeta {

struct SmartFolderRecord {
    int id = 0;
    QString name = "新智能文件夹";
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    FilterState filter;
};

class SmartFolderDao {
public:
    static bool initTable();
    static QList<SmartFolderRecord> getAllSmartFolders();
    static SmartFolderRecord getSmartFolderById(int id);
    static int addSmartFolder(const SmartFolderRecord& rec);
    static bool updateSmartFolder(const SmartFolderRecord& rec);
    static bool removeSmartFolderById(int id);
};

} // namespace QuarkMeta
