#include "TagRepository.h"
#include "DatabaseManager.h"
#include "MetadataManager.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <vector>
#include <string>
#include <mutex>

namespace QuarkMeta {

QList<TagRepository::TagGroup> TagRepository::getAllGroups() {
    // 确保数据已自动检查与迁移
    static std::once_flag s_migrateOnce;
    std::call_once(s_migrateOnce, []() { checkAndMigrate(); });

    QList<TagGroup> results;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return results;

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
    WriteGuard guard;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return -1;

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
    WriteGuard guard;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

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
    WriteGuard guard;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

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
    WriteGuard guard;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

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
    WriteGuard guard;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

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

void TagRepository::checkAndMigrate() {
    sqlite3* globalDb = DatabaseManager::instance().getGlobalDb();
    if (!globalDb) return;

    // 1. 优先强标记检查
    bool migrationCompleted = false;
    sqlite3_stmt* checkStmt = nullptr;
    const char* checkSql = "SELECT value FROM system_stats WHERE key = 'tag_migration_completed'";
    if (sqlite3_prepare_v2(globalDb, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            migrationCompleted = (sqlite3_column_int(checkStmt, 0) == 1);
        }
        sqlite3_finalize(checkStmt);
    }

    if (migrationCompleted) {
        return; // 已完成迁移，直接跳过
    }

    // 2. 检查全局库中是否已有数据，以防止覆盖
    bool globalHasGroups = false;
    sqlite3_stmt* gGroupStmt = nullptr;
    if (sqlite3_prepare_v2(globalDb, "SELECT 1 FROM tag_groups LIMIT 1", -1, &gGroupStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(gGroupStmt) == SQLITE_ROW) {
            globalHasGroups = true;
        }
        sqlite3_finalize(gGroupStmt);
    }

    (void)globalHasGroups;

    // 4. 写入强标记，即使没有历史数据
    sqlite3_stmt* markerStmt = nullptr;
    const char* markerSql = "INSERT OR REPLACE INTO system_stats (key, value) VALUES ('tag_migration_completed', 1)";
    if (sqlite3_prepare_v2(globalDb, markerSql, -1, &markerStmt, nullptr) == SQLITE_OK) {
        sqlite3_step(markerStmt);
        sqlite3_finalize(markerStmt);
    }
    // 并写入脏标记确保落盘
    DatabaseManager::instance().setDirty(true);
    DatabaseManager::instance().flushAll();
}

} // namespace QuarkMeta
