#pragma once
#include <QString>
#include <QColor>
#include <QList>
#include <QPair>

namespace QuarkMeta {

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
};

class FavoriteDao {
public:
    static bool initTable();
    static QList<FavoriteRecord> getAllFavorites();
    static int addVirtualCategory(const QString& name, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888");
    static bool addFavorite(const QString& path, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888");
    static bool removeFavoriteById(int id);
    static bool removeFavorite(const QString& path);
    static bool updateFavorite(const QString& path, const QString& iconKey, const QString& colorHex);
    static bool updateFavoriteNode(int id, const QString& name, const QString& iconKey, const QString& colorHex);
    static bool updateNodeParentAndOrder(int id, int newParentId, int sortOrder);
    static bool containsPath(const QString& path);
    static bool updateSortOrders(const QList<QPair<int, int>>& orders);
};

} // namespace QuarkMeta
