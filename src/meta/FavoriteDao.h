#pragma once
#include <QString>
#include <QColor>
#include <QList>
#include <QPair>

namespace QuarkMeta {

struct FavoriteRecord {
    int id = 0;
    QString path;
    QString name;
    QString iconKey = "folder";
    QString colorHex = "#FDB70A";
    int sortOrder = 0;
};

class FavoriteDao {
public:
    static bool initTable();
    static QList<FavoriteRecord> getAllFavorites();
    static bool addFavorite(const QString& path, const QString& iconKey = "folder", const QString& colorHex = "#FDB70A");
    static bool removeFavorite(const QString& path);
    static bool updateFavorite(const QString& path, const QString& iconKey, const QString& colorHex);
    static bool containsPath(const QString& path);
    static bool updateSortOrders(const QList<QPair<QString, int>>& orders);
};

} // namespace QuarkMeta
