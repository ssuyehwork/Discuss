#pragma once
#include <QString>
#include <QStringList>
#include <QList>

namespace RapidNotes {

struct LibraryCategoryRecord {
    int id = 0;
    int parentId = 0;
    QString name;
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    QStringList presetTags;
    bool isAutoRecursive = false;
};

class LibraryDao {
public:
    static bool initTable() { return true; }
    static QList<LibraryCategoryRecord> getAllCategories() { return {}; }
    static LibraryCategoryRecord getCategoryById(int) { return {}; }
};

} // namespace RapidNotes
