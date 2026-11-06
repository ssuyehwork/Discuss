#pragma once

#include <QString>
#include <QStringList>
#include <QList>

namespace RapidNotes {

struct NodeOrderEntry {
    int id = 0;
    int parentId = 0;
    int sortOrder = 0;
};

struct LibraryCategoryRecord {
    int id = 0;
    int parentId = 0;
    QString name;
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    bool isPinned = false;
    QStringList presetTags;
    bool isAutoRecursive = false;
    QStringList associatedPaths;
};

class LibraryDao {
public:
    static bool initTable() { return true; }
    static QList<LibraryCategoryRecord> getAllCategories() { return {}; }
    static LibraryCategoryRecord getCategoryById(int) { return {}; }
    static QStringList getCategoryPaths(int id) {
        Q_UNUSED(id);
        return {};
    }
    static bool setCategoryPinned(int id, bool pinned) {
        Q_UNUSED(id);
        Q_UNUSED(pinned);
        return true;
    }
    static bool updateCategoryNode(int id, const QString& name, const QString& iconKey, const QString& colorHex) {
        Q_UNUSED(id);
        Q_UNUSED(name);
        Q_UNUSED(iconKey);
        Q_UNUSED(colorHex);
        return true;
    }
    static bool updateNodesParentAndOrder(const QList<NodeOrderEntry>& entries) {
        Q_UNUSED(entries);
        return true;
    }
    static int addCategory(const QString& name, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888") {
        Q_UNUSED(name);
        Q_UNUSED(parentId);
        Q_UNUSED(iconKey);
        Q_UNUSED(colorHex);
        return 0;
    }
    static bool removeCategoryById(int id) {
        Q_UNUSED(id);
        return true;
    }
};

} // namespace RapidNotes
