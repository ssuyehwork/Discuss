#include "FavoriteDao.h"
#include "DatabaseManager.h"
#include <sqlite3.h>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

namespace QuarkMeta {

bool FavoriteDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "CREATE TABLE IF NOT EXISTS favorites ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "path TEXT UNIQUE NOT NULL, "
                      "name TEXT, "
                      "icon_key TEXT DEFAULT 'folder', "
                      "color_hex TEXT DEFAULT '#FDB70A', "
                      "sort_order INTEGER DEFAULT 0, "
                      "created_at INTEGER);";

    char* errMsgs = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        if (errMsgs) sqlite3_free(errMsgs);
        return false;
    }
    return true;
}

QList<FavoriteRecord> FavoriteDao::getAllFavorites() {
    QList<FavoriteRecord> list;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return list;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT id, path, name, icon_key, color_hex, sort_order FROM favorites ORDER BY sort_order ASC, id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FavoriteRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);
        const char* pathStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        rec.sortOrder = sqlite3_column_int(stmt, 5);

        if (pathStr) rec.path = QString::fromUtf8(pathStr);
        if (nameStr) rec.name = QString::fromUtf8(nameStr);
        if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
        if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);

        list.append(rec);
    }
    sqlite3_finalize(stmt);
    return list;
}

bool FavoriteDao::addFavorite(const QString& path, const QString& iconKey, const QString& colorHex) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    QFileInfo fi(cleanPath);
    QString name = fi.fileName().isEmpty() ? cleanPath : fi.fileName();

    const char* sql = "INSERT INTO favorites (path, name, icon_key, color_hex, sort_order, created_at) "
                      "VALUES (?, ?, ?, ?, (SELECT COALESCE(MAX(sort_order), 0) + 1 FROM favorites), ?)"
                      "ON CONFLICT(path) DO NOTHING;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string pathStd = cleanPath.toStdString();
    std::string nameStd = name.toStdString();
    std::string iconStd = iconKey.toStdString();
    std::string colorStd = colorHex.toStdString();

    sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, QDateTime::currentSecsSinceEpoch());

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool FavoriteDao::removeFavorite(const QString& path) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    const char* sql = "DELETE FROM favorites WHERE path = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string pathStd = cleanPath.toStdString();
    sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool FavoriteDao::updateFavorite(const QString& path, const QString& iconKey, const QString& colorHex) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    const char* sql = "UPDATE favorites SET icon_key = ?, color_hex = ? WHERE path = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string pathStd = cleanPath.toStdString();
    std::string iconStd = iconKey.toStdString();
    std::string colorStd = colorHex.toStdString();

    sqlite3_bind_text(stmt, 1, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pathStd.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool FavoriteDao::containsPath(const QString& path) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    const char* sql = "SELECT COUNT(*) FROM favorites WHERE path = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string pathStd = cleanPath.toStdString();
    sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }
    sqlite3_finalize(stmt);
    return exists;
}

bool FavoriteDao::updateSortOrders(const QList<QPair<QString, int>>& orders) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE favorites SET sort_order = ? WHERE path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const auto& pair : orders) {
        std::string pathStd = pair.first.toStdString();
        sqlite3_bind_int(stmt, 1, pair.second);
        sqlite3_bind_text(stmt, 2, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}

} // namespace QuarkMeta
