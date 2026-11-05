#pragma once
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

namespace QuarkMeta {

enum class SmartRuleType {
    Rating = 0, Color = 1, Tag = 2, Extension = 3, Name = 4,
    Folder = 5, FolderName = 6, AddedDate = 7, ModifiedDate = 8,
    CreatedDate = 9, Link = 10, Note = 11, Width = 12, Height = 13,
    FileSize = 14, AutoColor = 15
};

enum class SmartRuleOperator {
    Equals = 0, GreaterOrEqual = 1, LessOrEqual = 2, Contains = 3,
    NotContains = 4, StartsWith = 5, EndsWith = 6, IsFolder = 7,
    IsNotFolder = 8, Before = 9, After = 10, Between = 11,
    RecentDays = 12, IsEmpty = 13, IsNotEmpty = 14
};

struct SmartFolderRule {
    SmartRuleType type = SmartRuleType::Rating;
    SmartRuleOperator op = SmartRuleOperator::GreaterOrEqual;
    QString value;
    QStringList tags;
};

struct SmartFolderRecord {
    int id = 0;
    QString name = "新智能文件夹";
    QString matchMode = "ALL";
    QString iconKey = "smart-folder-1.svg";
    QString colorHex = "#888888";
    int sortOrder = 0;
    QList<SmartFolderRule> rules;
};

class SmartFolderDao {
public:
    static bool initTable() { return true; }
    static QList<SmartFolderRecord> getAllSmartFolders() { return {}; }
    static SmartFolderRecord getSmartFolderById(int) { return {}; }
    static int addSmartFolder(const SmartFolderRecord&) { return 0; }
    static bool updateSmartFolder(const SmartFolderRecord&) { return true; }
    static bool updateSmartFolderOrder(const QList<QPair<int, int>>&) { return true; }
    static bool removeSmartFolderById(int) { return true; }
    static int countMatchingItems(const SmartFolderRecord&) { return 0; }
    static QMap<int, int> countMatchingItemsBatchReadOnly(const QString&, const QList<SmartFolderRecord>&) { return {}; }
    static QStringList getMatchingPaths(const SmartFolderRecord&) { return {}; }
};

} // namespace QuarkMeta
