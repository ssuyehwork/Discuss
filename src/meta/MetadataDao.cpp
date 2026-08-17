#include "MetadataDao.h"
#include "DatabaseManager.h"
#include "MetadataManager.h"
#include "sqlite3.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

namespace ArcMeta {

static const char* kSqlInsertMetaDao =
    "INSERT OR REPLACE INTO metadata (folder_id, path, is_folder, rating, color, tags, note, url, "
    "ctime, mtime, atime, file_size, palettes, is_trash, original_path, "
    "width, height, ingestion_status, auto_color, base_name, ext, added_at, sha256) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

void MetadataDao::bindMetaHelper(sqlite3_stmt* stmt, const std::wstring& path, const RuntimeMeta& meta) {
    sqlite3_bind_text(stmt, 1, meta.folderId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 2, path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, meta.isFolder ? 1 : 0);
    sqlite3_bind_int(stmt, 4, meta.rating);
    sqlite3_bind_text16(stmt, 5, meta.manualColor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 6, meta.tags.join(",").toStdWString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 7, meta.note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 8, meta.url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 9, meta.ctime);
    sqlite3_bind_int64(stmt, 10, meta.mtime);
    sqlite3_bind_int64(stmt, 11, meta.atime);
    sqlite3_bind_int64(stmt, 12, meta.fileSize);

    QJsonArray arr;
    for (const auto& pe : meta.palettes) {
        QJsonObject obj;
        obj["color"] = pe.color.name();
        obj["ratio"] = (double)pe.ratio;
        arr.append(obj);
    }
    QByteArray ba = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    sqlite3_bind_blob(stmt, 13, ba.constData(), ba.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 14, meta.isTrash ? 1 : 0);
    sqlite3_bind_text16(stmt, 15, meta.originalPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 16, meta.width);
    sqlite3_bind_int(stmt, 17, meta.height);
    sqlite3_bind_int(stmt, 18, meta.ingestionStatus);
    sqlite3_bind_text16(stmt, 19, meta.autoColor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 20, meta.baseName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 21, meta.ext.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 22, meta.added_at);
    sqlite3_bind_text(stmt, 23, meta.sha256.c_str(), -1, SQLITE_TRANSIENT);
}

bool MetadataDao::insertOrReplace(sqlite3* db, const std::wstring& path, const RuntimeMeta& meta) {
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(db, kSqlInsertMetaDao, -1, &stmt, nullptr) == SQLITE_OK) {
        bindMetaHelper(stmt, path, meta);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success = true;
        }
        sqlite3_finalize(stmt);
    }
    return success;
}

bool MetadataDao::batchInsertOrReplace(sqlite3* db, const std::vector<std::pair<std::wstring, RuntimeMeta>>& items) {
    if (!db || items.empty()) return false;
    SqlTransaction trans(db);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kSqlInsertMetaDao, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& pair : items) {
            bindMetaHelper(stmt, pair.first, pair.second);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }
    return trans.commit();
}

bool MetadataDao::deleteByFolderIds(sqlite3* db, const std::vector<std::string>& folderIds) {
    if (!db || folderIds.empty()) return false;
    const char* sql = "DELETE FROM metadata WHERE folder_id = ?";
    SqlTransaction trans(db);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& fid : folderIds) {
            sqlite3_bind_text(stmt, 1, fid.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }
    return trans.commit();
}

bool MetadataDao::updatePathByFolderId(sqlite3* db, const std::string& folderId, const std::wstring& newPath) {
    if (!db || folderId.empty()) return false;
    const char* sql = "UPDATE metadata SET path = ? WHERE folder_id = ?";
    sqlite3_stmt* stmt = nullptr;
    bool ok = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt, 1, newPath.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, folderId.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            ok = true;
        }
        sqlite3_finalize(stmt);
    }
    return ok;
}

bool MetadataDao::checkExistsByFolderId(sqlite3* db, const std::string& folderId) {
    if (!db || folderId.empty()) return false;
    const char* sql = "SELECT 1 FROM metadata WHERE folder_id = ?";
    sqlite3_stmt* stmt = nullptr;
    bool exists = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, folderId.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            exists = true;
        }
        sqlite3_finalize(stmt);
    }
    return exists;
}

} // namespace ArcMeta
