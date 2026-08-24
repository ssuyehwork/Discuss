#include "TagRepository.h"
#include "DatabaseManager.h"
#include "MetadataManager.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <vector>
#include <string>
#include <mutex>

namespace QuarkMeta {

QList<TagRepository::TagGroup> TagRepository::getAllGroups() {
    QList<TagGroup> results;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return results;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, color FROM tag_groups ORDER BY sort_order ASC";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            TagGroup tg;
            tg.id = sqlite3_column_int(stmt, 0);
            
            const wchar_t* wname = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
            const wchar_t* wcolor = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 2));
            if (wname) tg.name = QString::fromWCharArray(wname);
            if (wcolor) tg.color = QString::fromWCharArray(wcolor);

            // 读取组内的 tags
            sqlite3_stmt* itemStmt = nullptr;
            const char* itemSql = "SELECT tag_name FROM tag_group_items WHERE group_id = ?";
            if (sqlite3_prepare_v2(db, itemSql, -1, &itemStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(itemStmt, 1, tg.id);
                while (sqlite3_step(itemStmt) == SQLITE_ROW) {
                    const wchar_t* wtag = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(itemStmt, 0));
                    if (wtag) tg.tags << QString::fromWCharArray(wtag);
                }
                sqlite3_finalize(itemStmt);
            }
            results.append(tg);
        }
        sqlite3_finalize(stmt);
    }
    return results;
}

int TagRepository::createGroup(const QString& name, const QString& color) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return -1;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO tag_groups (name, color, sort_order) VALUES (?, ?, (SELECT IFNULL(MAX(sort_order), 0) + 1 FROM tag_groups))";
    int groupId = -1;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt, 1, name.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text16(stmt, 2, color.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            groupId = static_cast<int>(sqlite3_last_insert_rowid(db));
        }
        sqlite3_finalize(stmt);
    }
    return groupId;
}

bool TagRepository::renameGroup(int groupId, const QString& newName) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE tag_groups SET name = ? WHERE id = ?";
    bool success = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt, 1, newName.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, groupId);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    return success;
}

bool TagRepository::deleteGroup(int groupId) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    SqlTransaction trans(db);
    sqlite3_stmt* stmt1 = nullptr;
    sqlite3_stmt* stmt2 = nullptr;
    bool ok1 = false;
    bool ok2 = false;

    if (sqlite3_prepare_v2(db, "DELETE FROM tag_groups WHERE id = ?", -1, &stmt1, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt1, 1, groupId);
        ok1 = (sqlite3_step(stmt1) == SQLITE_DONE);
        sqlite3_finalize(stmt1);
    }

    if (sqlite3_prepare_v2(db, "DELETE FROM tag_group_items WHERE group_id = ?", -1, &stmt2, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt2, 1, groupId);
        ok2 = (sqlite3_step(stmt2) == SQLITE_DONE);
        sqlite3_finalize(stmt2);
    }

    if (ok1 && ok2) {
        return trans.commit();
    }
    return false;
}

bool TagRepository::addTagToGroup(const QString& tagName, int groupId) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO tag_group_items (group_id, tag_name) SELECT ?, ? WHERE NOT EXISTS (SELECT 1 FROM tag_group_items WHERE group_id = ? AND tag_name = ?)";
    bool success = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, groupId);
        sqlite3_bind_text16(stmt, 2, tagName.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, groupId);
        sqlite3_bind_text16(stmt, 4, tagName.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    return success;
}

bool TagRepository::removeTagFromGroup(const QString& tagName, int groupId) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (groupId == -1) {
        const char* sql = "DELETE FROM tag_group_items WHERE tag_name = ?";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text16(stmt, 1, tagName.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
            success = (sqlite3_step(stmt) == SQLITE_DONE);
            sqlite3_finalize(stmt);
        }
    } else {
        const char* sql = "DELETE FROM tag_group_items WHERE group_id = ? AND tag_name = ?";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, groupId);
            sqlite3_bind_text16(stmt, 2, tagName.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
            success = (sqlite3_step(stmt) == SQLITE_DONE);
            sqlite3_finalize(stmt);
        }
    }
    return success;
}

// -------------------------------------------------------------
// 🚨 全局独立标签主表实现（持久化到 global.db）
// -------------------------------------------------------------

bool TagRepository::createTag(const QString& tagName, const QString& color) {
    if (tagName.trimmed().isEmpty()) return false;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    const char* sql = 
        "INSERT INTO tags (name, color, last_used, use_count) VALUES (?, ?, ?, 1) "
        "ON CONFLICT(name) DO UPDATE SET last_used = excluded.last_used, use_count = use_count + 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt, 1, tagName.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text16(stmt, 2, color.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, now);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE) {
            sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
            return true;
        }
    }
    return false;
}

bool TagRepository::deleteTag(const QString& tagName) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    SqlTransaction trans(db);

    sqlite3_stmt* stmt1 = nullptr;
    sqlite3_stmt* stmt2 = nullptr;

    if (sqlite3_prepare_v2(db, "DELETE FROM tags WHERE name = ?", -1, &stmt1, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt1, 1, tagName.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt1);
        sqlite3_finalize(stmt1);
    }

    if (sqlite3_prepare_v2(db, "DELETE FROM tag_group_items WHERE tag_name = ?", -1, &stmt2, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt2, 1, tagName.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt2);
        sqlite3_finalize(stmt2);
    }

    return trans.commit();
}

QStringList TagRepository::getAllMasterTags() {
    QStringList result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT name FROM tags ORDER BY last_used DESC";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const wchar_t* wname = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
            if (wname) result << QString::fromWCharArray(wname);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

QStringList TagRepository::getRecentTags(int limit) {
    QStringList result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT name FROM tags WHERE last_used > 0 ORDER BY last_used DESC LIMIT ?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const wchar_t* wname = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
            if (wname) result << QString::fromWCharArray(wname);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

void TagRepository::recordTagUsage(const QString& tagName) {
    createTag(tagName);
}

} // namespace QuarkMeta