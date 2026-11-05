#pragma once
#include <QString>
#include <QColor>
#include <QList>
#include <QPair>

namespace QuarkMeta {

#ifndef QUARKMETA_NODEORDERENTRY_DEFINED
#define QUARKMETA_NODEORDERENTRY_DEFINED
struct NodeOrderEntry {
    int id = 0;
    int parentId = 0;
    int sortOrder = 0;
};
#endif

enum class FavoriteNodeType {
    VirtualCategory = 0,
    RealPath = 1
};

struct FavoriteRecord {
    int id = 0;
    int parentId = 0;
    FavoriteNodeType nodeType = FavoriteNodeType::RealPath;
    QString path;
    QString name;
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    QStringList presetTags;
    bool isAutoRecursive = false;
};

class FavoriteDao {
public:
    static bool initTable() { return true; }
    static QList<FavoriteRecord> getAllFavorites() { return {}; }
    static int addVirtualCategory(const QString&, int = 0, const QString& = "folder_filled", const QString& = "#888888") { return 0; }
    static bool addFavorite(const QString&, int = 0, const QString& = "folder_filled", const QString& = "#888888") { return true; }
    static bool removeFavoriteById(int) { return true; }
    static bool removeFavorite(const QString&) { return true; }
    static bool updateFavorite(const QString&, const QString&, const QString&) { return true; }
    static bool updateFavoriteNode(int, const QString&, const QString&, const QString&) { return true; }
    static bool updateNodeParentAndOrder(int, int, int) { return true; }
    static bool updateNodesParentAndOrder(const QList<NodeOrderEntry>&) { return true; }
    static bool updatePresetTags(int, const QStringList&) { return true; }
    static bool updateAutoRecursive(int, bool) { return true; }
    static bool containsPath(const QString&) { return false; }
    static bool updateSortOrders(const QList<QPair<int, int>>&) { return true; }
};

} // namespace QuarkMeta
