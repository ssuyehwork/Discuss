#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

namespace QuarkMeta {

class TagRepository {
public:
    struct TagGroup {
        int id = 0;
        QString name;
        QString color;
        QStringList tags;
    };

    // --- 1. 分组管理 (Group CRUD) ---
    static QList<TagGroup> getAllGroups();
    static int createGroup(const QString& name, const QString& color = "#3498db");
    static bool renameGroup(int groupId, const QString& newName);
    static bool deleteGroup(int groupId);

    // --- 2. 标签与分组关联 ---
    static bool addTagToGroup(const QString& tagName, int groupId);
    static bool removeTagFromGroup(const QString& tagName, int groupId = -1);

    // --- 3. 标签词库本体管理 (Tag Master Dictionary) ---
    static bool createTag(const QString& tagName, const QString& color = "");
    static bool deleteTag(const QString& tagName);
    static bool renameTag(const QString& oldName, const QString& newName);
    static QStringList getAllMasterTags();
    static QStringList getRecentTags(int limit = 30);
    static void recordTagUsage(const QString& tagName);
};

} // namespace QuarkMeta