#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

namespace QuarkMeta {

enum class SmartRuleType {
    Rating,
    Color,
    Tag,
    Extension,
    Keyword
};

enum class SmartRuleOperator {
    Equals,
    GreaterOrEqual,
    LessOrEqual,
    Contains,
    NotContains
};

struct SmartFolderRule {
    SmartRuleType type = SmartRuleType::Rating;
    SmartRuleOperator op = SmartRuleOperator::GreaterOrEqual;
    QString value;

    QJsonObject toJson() const;
    static SmartFolderRule fromJson(const QJsonObject& obj);
};

struct SmartFolderRecord {
    int id = 0;
    QString name = "新智能文件夹";
    QString matchMode = "ALL"; // "ALL" (AND) 或 "ANY" (OR)
    QString iconKey = "smart-folder-1.svg";
    QString colorHex = "#888888";
    int sortOrder = 0;
    QList<SmartFolderRule> rules;
};

class SmartFolderDao {
public:
    static bool initTable();
    static QList<SmartFolderRecord> getAllSmartFolders();
    static SmartFolderRecord getSmartFolderById(int id);
    static int addSmartFolder(const SmartFolderRecord& rec);
    static bool updateSmartFolder(const SmartFolderRecord& rec);
    static bool removeSmartFolderById(int id);
    static int countMatchingItems(const SmartFolderRecord& rec);
    static QStringList getMatchingPaths(const SmartFolderRecord& rec);

private:
    static QString buildWhereClause(const SmartFolderRecord& rec);
};

} // namespace QuarkMeta
