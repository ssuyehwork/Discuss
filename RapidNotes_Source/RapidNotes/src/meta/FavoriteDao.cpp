#include "FavoriteDao.h"

namespace QuarkMeta {

bool FavoriteDao::initTable() { return true; }
QList<FavoriteRecord> FavoriteDao::getAllFavorites() { return {}; }
int FavoriteDao::addVirtualCategory(const QString&, int, const QString&, const QString&) { return 0; }
bool FavoriteDao::addFavorite(const QString&, int, const QString&, const QString&) { return true; }
bool FavoriteDao::removeFavoriteById(int) { return true; }
bool FavoriteDao::removeFavorite(const QString&) { return true; }
bool FavoriteDao::updateFavorite(const QString&, const QString&, const QString&) { return true; }
bool FavoriteDao::updateFavoriteNode(int, const QString&, const QString&, const QString&) { return true; }
bool FavoriteDao::updateNodeParentAndOrder(int, int, int) { return true; }
bool FavoriteDao::updateNodesParentAndOrder(const QList<NodeOrderEntry>&) { return true; }
bool FavoriteDao::updatePresetTags(int, const QStringList&) { return true; }
bool FavoriteDao::updateAutoRecursive(int, bool) { return true; }
bool FavoriteDao::containsPath(const QString&) { return false; }
bool FavoriteDao::updateSortOrders(const QList<QPair<int, int>>&) { return true; }

} // namespace QuarkMeta
