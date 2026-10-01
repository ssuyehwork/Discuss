#pragma once

#include <QString>
#include <QStringList>
#include <QList>

namespace QuarkMeta {

struct LibraryCategoryRecord {
    int id = 0;
    int parentId = 0;
    QString name;
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    QStringList presetTags;
    QStringList associatedPaths;
};

class LibraryDao {
public:
    static bool initTable();
    static QList<LibraryCategoryRecord> getAllCategories();
    static int addCategory(const QString& name, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888");
    static bool updateCategoryNode(int id, const QString& name, const QString& iconKey, const QString& colorHex);
    static bool updateNodeParentAndOrder(int id, int newParentId, int sortOrder);
    static bool removeCategoryById(int id);
    static bool updateCategoryPaths(int id, const QStringList& paths);
    static bool addPathsToCategory(int id, const QStringList& paths);
    static bool removePathsFromCategory(int id, const QStringList& paths);
    static QStringList getCategoryPaths(int id);
};

} // namespace QuarkMeta
