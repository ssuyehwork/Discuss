#include "SmartFolderDao.h"
#include "DatabaseManager.h"
#include "../core/FilterStateJson.h"
#include <sqlite3.h>
#include <QJsonDocument>

namespace QuarkMeta {

bool SmartFolderDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    // 检查列是否存在
    const char* pragmaSql = "PRAGMA table_info(smart_folders);";
    sqlite3_stmt* stmt = nullptr;
    bool hasFilterJson = false;

    if (sqlite3_prepare_v2(db, pragmaSql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (colName && strcmp(colName, "filter_json") == 0) {
                hasFilterJson = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
    }

    if (!hasFilterJson) {
        sqlite3_exec(db, "DROP TABLE IF EXISTS smart_folders;", nullptr, nullptr, nullptr);
    }

    const char* createSql = "CREATE TABLE IF NOT EXISTS smart_folders ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "name TEXT NOT NULL, "
                            "icon_key TEXT, "
                            "color_hex TEXT, "
                            "sort_order INTEGER DEFAULT 0, "
                            "filter_json TEXT);";

    char* errMsgs = nullptr;
    int rc = sqlite3_exec(db, createSql, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        if (errMsgs) sqlite3_free(errMsgs);
        return false;
    }
    return true;
}

QList<SmartFolderRecord> SmartFolderDao::getAllSmartFolders() {
    QList<SmartFolderRecord> list;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return list;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT id, name, icon_key, color_hex, sort_order, filter_json FROM smart_folders ORDER BY sort_order ASC, id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SmartFolderRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);
        const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        rec.sortOrder = sqlite3_column_int(stmt, 4);
        const char* filterStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

        if (nameStr) rec.name = QString::fromUtf8(nameStr);
        if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
        if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);

        if (filterStr && strlen(filterStr) > 0) {
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray(filterStr));
            if (doc.isObject()) {
                rec.filter = FilterStateJson::fromJson(doc.object());
            }
        }
        list.append(rec);
    }
    sqlite3_finalize(stmt);
    return list;
}

SmartFolderRecord SmartFolderDao::getSmartFolderById(int id) {
    SmartFolderRecord rec;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return rec;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT id, name, icon_key, color_hex, sort_order, filter_json FROM smart_folders WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return rec;

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        rec.id = sqlite3_column_int(stmt, 0);
        const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        rec.sortOrder = sqlite3_column_int(stmt, 4);
        const char* filterStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));

        if (nameStr) rec.name = QString::fromUtf8(nameStr);
        if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
        if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);

        if (filterStr && strlen(filterStr) > 0) {
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray(filterStr));
            if (doc.isObject()) {
                rec.filter = FilterStateJson::fromJson(doc.object());
            }
        }
    }
    sqlite3_finalize(stmt);
    return rec;
}

int SmartFolderDao::addSmartFolder(const SmartFolderRecord& rec) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "INSERT INTO smart_folders (name, icon_key, color_hex, sort_order, filter_json) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    std::string nameStd = rec.name.toStdString();
    std::string iconStd = rec.iconKey.toStdString();
    std::string colorStd = rec.colorHex.toStdString();

    QJsonObject json = FilterStateJson::toJson(rec.filter);
    QJsonDocument doc(json);
    std::string filterStd = doc.toJson(QJsonDocument::Compact).toStdString();

    sqlite3_bind_text(stmt, 1, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, rec.sortOrder);
    sqlite3_bind_text(stmt, 5, filterStd.c_str(), -1, SQLITE_TRANSIENT);

    int newId = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    }
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return newId;
}

bool SmartFolderDao::updateSmartFolder(const SmartFolderRecord& rec) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || rec.id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE smart_folders SET name = ?, icon_key = ?, color_hex = ?, sort_order = ?, filter_json = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string nameStd = rec.name.toStdString();
    std::string iconStd = rec.iconKey.toStdString();
    std::string colorStd = rec.colorHex.toStdString();

    QJsonObject json = FilterStateJson::toJson(rec.filter);
    QJsonDocument doc(json);
    std::string filterStd = doc.toJson(QJsonDocument::Compact).toStdString();

    sqlite3_bind_text(stmt, 1, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, rec.sortOrder);
    sqlite3_bind_text(stmt, 5, filterStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, rec.id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool SmartFolderDao::removeSmartFolderById(int id) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM smart_folders WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

} // namespace QuarkMeta
