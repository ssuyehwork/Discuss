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
                         "preset_tags TEXT DEFAULT '', "
                         "created_at INTEGER);";

    const char* sqlPaths = "CREATE TABLE IF NOT EXISTS library_category_paths ("
                           "category_id INTEGER, "
                           "path TEXT NOT NULL, "
                           "PRIMARY KEY(category_id, path));";

    const char* sqlIndex = "CREATE TABLE IF NOT EXISTS library_item_index ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "category_id INTEGER NOT NULL, "
                           "file_path TEXT NOT NULL, "
                           "rating INTEGER DEFAULT 0, "
                           "manual_color TEXT DEFAULT '', "
                           "auto_color TEXT DEFAULT '', "
                           "palettes TEXT DEFAULT '', "
                           "tags TEXT DEFAULT '', "
                           "note TEXT DEFAULT '', "
                           "link TEXT DEFAULT '', "
                           "ratio INTEGER DEFAULT 0, "
                           "width INTEGER DEFAULT 0, "
                           "height INTEGER DEFAULT 0, "
                           "updated_at INTEGER, "
                           "UNIQUE(category_id, file_path));";

    char* errMsgs = nullptr;
    sqlite3_exec(db, sqlCat, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPaths, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlIndex, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, "ALTER TABLE library_categories ADD COLUMN preset_tags TEXT DEFAULT '';", nullptr, nullptr, nullptr);
    return true;
}

bool LibraryDao::initItemIndexTable() {
    return initTable();
}

bool LibraryDao::indexItemMetadata(int categoryId, const QString& filePath, const ItemMeta& meta) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || categoryId <= 0 || filePath.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "INSERT INTO library_item_index "
                      "(category_id, file_path, rating, manual_color, auto_color, palettes, tags, note, link, ratio, width, height, updated_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
                      "ON CONFLICT(category_id, file_path) DO UPDATE SET "
                      "rating=excluded.rating, manual_color=excluded.manual_color, "
                      "auto_color=excluded.auto_color, palettes=excluded.palettes, "
                      "tags=excluded.tags, note=excluded.note, link=excluded.link, "
                      "ratio=excluded.ratio, width=excluded.width, height=excluded.height, updated_at=excluded.updated_at;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string pathStd = QDir::toNativeSeparators(QDir::cleanPath(filePath)).toStdString();
    std::string mColorStd = QString::fromStdWString(meta.color).toStdString();
    std::string aColorStd = QString::fromStdWString(meta.autoColor).toStdString();

    QStringList tagList;
    for (const auto& t : meta.tags) tagList.append(QString::fromStdWString(t));
    std::string tagsStd = tagList.join(",").toStdString();

    std::string noteStd = QString::fromStdWString(meta.note).toStdString();
    std::string linkStd = QString::fromStdWString(meta.url).toStdString();

    sqlite3_bind_int(stmt, 1, categoryId);
    sqlite3_bind_text(stmt, 2, pathStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, meta.rating);
    sqlite3_bind_text(stmt, 4, mColorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, aColorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, tagsStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, noteStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, linkStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, 0);
    sqlite3_bind_int(stmt, 11, meta.width);
    sqlite3_bind_int(stmt, 12, meta.height);
    sqlite3_bind_int64(stmt, 13, QDateTime::currentSecsSinceEpoch());

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return ok;
}

bool LibraryDao::removeIndexedItem(int categoryId, const QString& filePath) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || categoryId <= 0 || filePath.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM library_item_index WHERE category_id = ? AND file_path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string pathStd = QDir::toNativeSeparators(QDir::cleanPath(filePath)).toStdString();
    sqlite3_bind_int(stmt, 1, categoryId);
    sqlite3_bind_text(stmt, 2, pathStd.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return ok;
}

QList<ItemMeta> LibraryDao::getCategoryIndexedItems(int categoryId) {
    QList<ItemMeta> list;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || categoryId <= 0) return list;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT file_path, rating, manual_color, auto_color, tags, note, link, width, height "
                      "FROM library_item_index WHERE category_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, categoryId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ItemMeta meta;
        const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (pStr) meta.originalName = QString::fromUtf8(pStr).toStdWString();
        meta.rating = sqlite3_column_int(stmt, 1);
        const char* mcStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* acStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* tagsStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* noteStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* linkStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        meta.width = sqlite3_column_int(stmt, 7);
        meta.height = sqlite3_column_int(stmt, 8);

        if (mcStr) meta.color = QString::fromUtf8(mcStr).toStdWString();
        if (acStr) meta.autoColor = QString::fromUtf8(acStr).toStdWString();
        if (tagsStr && strlen(tagsStr) > 0) {
            QStringList tList = QString::fromUtf8(tagsStr).split(',', Qt::SkipEmptyParts);
            for (const auto& t : tList) meta.tags.push_back(t.toStdWString());
        }
        if (noteStr) meta.note = QString::fromUtf8(noteStr).toStdWString();
        if (linkStr) meta.url = QString::fromUtf8(linkStr).toStdWString();

        list.append(meta);
    }
    sqlite3_finalize(stmt);
    return list;
}

QList<LibraryCategoryRecord> LibraryDao::getAllCategories() {
    QList<LibraryCategoryRecord> list;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return list;

    {
        std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

        const char* sql = "SELECT id, parent_id, name, icon_key, color_hex, sort_order, preset_tags FROM library_categories ORDER BY sort_order ASC, id ASC;";
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
            const char* tagsStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

            if (nameStr) rec.name = QString::fromUtf8(nameStr);
            if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
            if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);
            if (tagsStr && strlen(tagsStr) > 0) {
                rec.presetTags = QString::fromUtf8(tagsStr).split(',', Qt::SkipEmptyParts);
            }

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
    if (!db || id == 0 || paths.isEmpty()) return false;

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

bool LibraryDao::updatePresetTags(int id, const QStringList& tags) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE library_categories SET preset_tags = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string tagsStd = tags.join(",").toStdString();
    sqlite3_bind_text(stmt, 1, tagsStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

QStringList LibraryDao::getCategoryPaths(int id) {
    QStringList paths;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id == 0) return paths;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    if (id == -1) {
        // 全部数据：获取库中所有关联路径与索引文件路径 (去重)
        const char* sql = "SELECT DISTINCT path FROM library_category_paths "
                          "UNION "
                          "SELECT DISTINCT file_path FROM library_item_index;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else if (id == -2) {
        // 未分类：获取关联于 category_id <= 0 的路径，或存在于库中但未归属于任何正数 ID 分类的路径
        const char* sql = "SELECT DISTINCT path FROM library_category_paths WHERE category_id <= 0 "
                          "UNION "
                          "SELECT DISTINCT file_path FROM library_item_index WHERE file_path NOT IN (SELECT path FROM library_category_paths WHERE category_id > 0) "
                          "EXCEPT "
                          "SELECT DISTINCT path FROM library_category_paths WHERE category_id > 0;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else if (id == -3) {
        // 未标签：获取 library_item_index 中 tags 为空/NULL 的文件路径
        const char* sql = "SELECT DISTINCT file_path FROM library_item_index WHERE tags IS NULL OR tags = '';";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else {
        // 常规用户分类 ID > 0
        const char* sql = "SELECT path FROM library_category_paths WHERE category_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return paths;

        sqlite3_bind_int(stmt, 1, id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (pStr) paths.append(QString::fromUtf8(pStr));
        }
        sqlite3_finalize(stmt);
    }
    return paths;
}

} // namespace QuarkMeta
