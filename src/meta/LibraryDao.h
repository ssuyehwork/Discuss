#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include "MetadataDefs.h"

namespace QuarkMeta {

#ifndef QUARKMETA_NODEORDERENTRY_DEFINED
#define QUARKMETA_NODEORDERENTRY_DEFINED
struct NodeOrderEntry {
    int id = 0;
    int parentId = 0;
    int sortOrder = 0;
};
#endif

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
    static bool updateNodesParentAndOrder(const QList<NodeOrderEntry>& entries);
    static bool removeCategoryById(int id);
    static bool updateCategoryPaths(int id, const QStringList& paths);
    static bool addPathsToCategory(int id, const QStringList& paths);
    static bool removePathsFromCategory(int id, const QStringList& paths);
    static QStringList getCategoryPaths(int id);
    static bool updatePresetTags(int id, const QStringList& tags);

    static bool initItemIndexTable();
    static bool indexItemMetadata(int categoryId, const QString& filePath, const ItemMeta& meta);
    static bool removeIndexedItem(int categoryId, const QString& filePath);
    static QList<ItemMeta> getCategoryIndexedItems(int categoryId);
};

} // namespace QuarkMeta
