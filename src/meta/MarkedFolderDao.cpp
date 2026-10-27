#include "MarkedFolderDao.h"
#include "DatabaseManager.h"
#include <sqlite3.h>
#include <QDir>
#include <mutex>

namespace QuarkMeta {

bool MarkedFolderDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "CREATE TABLE IF NOT EXISTS marked_folders ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "folder_path TEXT UNIQUE NOT NULL, "
                      "is_marked INTEGER NOT NULL DEFAULT 1, "
                      "updated_at INTEGER);";

    char* errMsgs = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        if (errMsgs) sqlite3_free(errMsgs);
        return false;
    }
    return true;
}

int MarkedFolderDao::getMarkedCount() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT COUNT(*) FROM marked_folders;";
    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

bool MarkedFolderDao::addMarkedFolder(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    // 1. 检查路径是否已经在表中
    const char* checkSql = "SELECT id, is_marked FROM marked_folders WHERE folder_path = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    bool existsInDb = false;

    if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            existsInDb = true;
        }
        sqlite3_finalize(stmt);
    }

    int nowTs = static_cast<int>(QDateTime::currentSecsSinceEpoch());

    if (existsInDb) {
        // 已存在：激活标记状态并更新时间
        const char* updateSql = "UPDATE marked_folders SET is_marked = 1, updated_at = ? WHERE folder_path = ?;";
        if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, nowTs);
            std::string pathStd = cleanPath.toStdString();
            sqlite3_bind_text(stmt, 2, pathStd.c_str(), -1, SQLITE_TRANSIENT);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return (rc == SQLITE_DONE);
        }
        return false;
    }

    // 2. 新记录：检查总容量上限（50条），执行淘汰规则
    const char* countSql = "SELECT COUNT(*) FROM marked_folders;";
    int totalCount = 0;
    if (sqlite3_prepare_v2(db, countSql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            totalCount = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    if (totalCount >= 50) {
        // 优先淘汰 is_marked = 0 的最旧记录
        const char* evictUnmarkedSql = "DELETE FROM marked_folders WHERE id = (SELECT id FROM marked_folders WHERE is_marked = 0 ORDER BY updated_at ASC LIMIT 1);";
        sqlite3_exec(db, evictUnmarkedSql, nullptr, nullptr, nullptr);

        int currentCountAfterEvict = 0;
        if (sqlite3_prepare_v2(db, countSql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                currentCountAfterEvict = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }

        if (currentCountAfterEvict >= 50) {
            // 仍满 50 条，淘汰处于标记状态的最旧记录
            const char* evictOldestSql = "DELETE FROM marked_folders WHERE id = (SELECT id FROM marked_folders ORDER BY updated_at ASC LIMIT 1);";
            sqlite3_exec(db, evictOldestSql, nullptr, nullptr, nullptr);
        }
    }

    // 3. 插入新记录
    const char* insertSql = "INSERT INTO marked_folders (folder_path, is_marked, updated_at) VALUES (?, 1, ?);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, nowTs);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    return false;
}

bool MarkedFolderDao::unmarkFolder(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE marked_folders SET is_marked = 0, updated_at = ? WHERE folder_path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        int nowTs = static_cast<int>(QDateTime::currentSecsSinceEpoch());
        sqlite3_bind_int(stmt, 1, nowTs);
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 2, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }
    return false;
}

bool MarkedFolderDao::unmarkAllFolders() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    int nowTs = static_cast<int>(QDateTime::currentSecsSinceEpoch());
    std::string sql = "UPDATE marked_folders SET is_marked = 0, updated_at = " + std::to_string(nowTs) + ";";
    return (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK);
}

bool MarkedFolderDao::removeFolderRecord(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM marked_folders WHERE folder_path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }
    return false;
}

bool MarkedFolderDao::clearAllFolderRecords() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    return (sqlite3_exec(db, "DELETE FROM marked_folders;", nullptr, nullptr, nullptr) == SQLITE_OK);
}

bool MarkedFolderDao::isMarked(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT is_marked FROM marked_folders WHERE folder_path = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    bool marked = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string pathStd = cleanPath.toStdString();
        sqlite3_bind_text(stmt, 1, pathStd.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            marked = (sqlite3_column_int(stmt, 0) == 1);
        }
        sqlite3_finalize(stmt);
    }
    return marked;
}

QStringList MarkedFolderDao::getAllMarkedFolders() {
    QStringList result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT folder_path FROM marked_folders ORDER BY is_marked DESC, updated_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* txt = sqlite3_column_text(stmt, 0);
            if (txt) {
                result.append(QString::fromUtf8(reinterpret_cast<const char*>(txt)));
            }
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

QStringList MarkedFolderDao::getActiveMarkedFolders() {
    QStringList result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT folder_path FROM marked_folders WHERE is_marked = 1 ORDER BY updated_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* txt = sqlite3_column_text(stmt, 0);
            if (txt) {
                result.append(QString::fromUtf8(reinterpret_cast<const char*>(txt)));
            }
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

} // namespace QuarkMeta
