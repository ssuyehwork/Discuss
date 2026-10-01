#include "LibraryDao.h"
#include "DatabaseManager.h"
#include <sqlite3.h>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QRandomGenerator>

namespace QuarkMeta {

bool LibraryDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sqlCat = "CREATE TABLE IF NOT EXISTS library_categories ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "parent_id INTEGER DEFAULT 0, "
                         "name TEXT NOT NULL, "
                         "icon_key TEXT DEFAULT 'folder_filled', "
                         "color_hex TEXT DEFAULT '#888888', "
                         "sort_order INTEGER DEFAULT 0, "
                         "created_at INTEGER);";

    const char* sqlPaths = "CREATE TABLE IF NOT EXISTS library_category_paths ("
                           "category_id INTEGER, "
                           "path TEXT NOT NULL, "
                           "PRIMARY KEY(category_id, path));";

    char* errMsgs = nullptr;
    sqlite3_exec(db, sqlCat, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPaths, nullptr, nullptr, &errMsgs);
    return true;
}

QList<LibraryCategoryRecord> LibraryDao::getAllCategories() {
    QList<LibraryCategoryRecord> list;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return list;

    {
        std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

        const char* sql = "SELECT id, parent_id, name, icon_key, color_hex, sort_order FROM library_categories ORDER BY sort_order ASC, id ASC;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            LibraryCategoryRecord rec;
            rec.id = sqlite3_column_int(stmt, 0);
            rec.parentId = sqlite3_column_int(stmt, 1);
            const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            rec.sortOrder = sqlite3_column_int(stmt, 5);

            if (nameStr) rec.name = QString::fromUtf8(nameStr);
            if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
            if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);

            list.append(rec);
        }
        sqlite3_finalize(stmt);
    }

    for (auto& rec : list) {
        rec.associatedPaths = getCategoryPaths(rec.id);
    }

    return list;
}

int LibraryDao::addCategory(const QString& name, int parentId, const QString& iconKey, const QString& colorHex) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "INSERT INTO library_categories (parent_id, name, icon_key, color_hex, sort_order, created_at) "
                      "VALUES (?, ?, ?, ?, (SELECT COALESCE(MAX(sort_order), 0) + 1 FROM library_categories), ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    std::string nameStd = name.toStdString();
    std::string iconStd = iconKey.toStdString();
    std::string colorStd = colorHex.toStdString();

    sqlite3_bind_int(stmt, 1, parentId);
    sqlite3_bind_text(stmt, 2, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, QDateTime::currentSecsSinceEpoch());

    int newId = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    }
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return newId;
}

bool LibraryDao::updateCategoryNode(int id, const QString& name, const QString& iconKey, const QString& colorHex) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE library_categories SET name = ?, icon_key = ?, color_hex = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string nameStd = name.toStdString();
    std::string iconStd = iconKey.toStdString();
    std::string colorStd = colorHex.toStdString();

    sqlite3_bind_text(stmt, 1, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool LibraryDao::updateNodeParentAndOrder(int id, int newParentId, int sortOrder) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE library_categories SET parent_id = ?, sort_order = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, newParentId);
    sqlite3_bind_int(stmt, 2, sortOrder);
    sqlite3_bind_int(stmt, 3, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool LibraryDao::removeCategoryById(int id) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "WITH RECURSIVE cnt(x) AS ("
                      "  SELECT ? UNION ALL SELECT id FROM library_categories, cnt WHERE library_categories.parent_id = cnt.x"
                      ") DELETE FROM library_categories WHERE id IN (SELECT x FROM cnt);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    const char* sqlClean = "DELETE FROM library_category_paths WHERE category_id = ?;";
    if (sqlite3_prepare_v2(db, sqlClean, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "INSERT OR IGNORE INTO library_category_paths (category_id, path) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const QString& p : paths) {
        QString cleanP = QDir::toNativeSeparators(QDir::cleanPath(p));
        std::string pStd = cleanP.toStdString();
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}

bool LibraryDao::removePathsFromCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM library_category_paths WHERE category_id = ? AND path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const QString& p : paths) {
        QString cleanP = QDir::toNativeSeparators(QDir::cleanPath(p));
        std::string pStd = cleanP.toStdString();
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}

QStringList LibraryDao::getCategoryPaths(int id) {
    QStringList paths;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return paths;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT path FROM library_category_paths WHERE category_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return paths;

    sqlite3_bind_int(stmt, 1, id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (pStr) paths.append(QString::fromUtf8(pStr));
    }
    sqlite3_finalize(stmt);
    return paths;
}

} // namespace QuarkMeta
