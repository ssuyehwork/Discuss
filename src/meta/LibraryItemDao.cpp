#include "LibraryItemDao.h"
#include "DatabaseManager.h"
#include "MetadataManager.h"
#include <QFileInfo>
#include <QDir>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

namespace QuarkMeta {

std::vector<ItemRecord> LibraryItemDao::getItemsByPaths(const QStringList& paths) {
    std::vector<ItemRecord> result;
    if (paths.isEmpty()) return result;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    for (const QString& path : paths) {
        QString normPath = QDir::toNativeSeparators(path);
        const char* sql = "SELECT file_id, file_path, file_name, base_name, extension, file_size, is_dir, is_empty, "
                          "is_hidden, is_managed, encrypted, pinned, rating, color_hex, auto_color, width, height, "
                          "thumb_status, note, url, sha256, created_at, updated_at, added_at "
                          "FROM library_items WHERE file_path = ? COLLATE NOCASE;";
        
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, normPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                ItemRecord rec;
                rec.fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                rec.path = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                rec.filename = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
                rec.suffix = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
                rec.size = sqlite3_column_int64(stmt, 5);
                rec.isDir = (sqlite3_column_int(stmt, 6) != 0);
                rec.isEmpty = (sqlite3_column_int(stmt, 7) != 0);
                rec.isHidden = (sqlite3_column_int(stmt, 8) != 0);
                rec.isManaged = (sqlite3_column_int(stmt, 9) != 0);
                rec.encrypted = (sqlite3_column_int(stmt, 10) != 0);
                rec.pinned = (sqlite3_column_int(stmt, 11) != 0);
                rec.rating = sqlite3_column_int(stmt, 12);
                rec.manualColor = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13)));
                rec.autoColor = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14)));
                rec.width = sqlite3_column_int(stmt, 15);
                rec.height = sqlite3_column_int(stmt, 16);
                rec.thumbStatus = sqlite3_column_int(stmt, 17);
                rec.note = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18)));
                rec.url = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 19)));
                rec.sha256 = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20)));
                rec.ctime = sqlite3_column_int64(stmt, 21);
                rec.mtime = sqlite3_column_int64(stmt, 22);
                rec.added_at = sqlite3_column_int64(stmt, 23);

                sqlite3_finalize(stmt);

                // 装配 file_tags
                const char* tagSql = "SELECT tag_name FROM file_tags WHERE file_id = ?;";
                if (sqlite3_prepare_v2(db, tagSql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, rec.fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        rec.tags.append(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
                    }
                    sqlite3_finalize(stmt);
                }

                // 装配 item_palettes
                const char* palSql = "SELECT color_hex, ratio FROM item_palettes WHERE file_id = ? ORDER BY idx ASC;";
                if (sqlite3_prepare_v2(db, palSql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, rec.fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        QString colorHex = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                        float ratio = static_cast<float>(sqlite3_column_double(stmt, 1));
                        rec.palettes.push_back({QColor(colorHex), ratio});
                    }
                    sqlite3_finalize(stmt);
                }

                result.push_back(rec);
            } else {
                sqlite3_finalize(stmt);
            }
        }
    }
    return result;
}

std::vector<ItemRecord> LibraryItemDao::getAllLibraryItems() {
    std::vector<ItemRecord> result;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    const char* sql = "SELECT file_path FROM library_items;";
    sqlite3_stmt* stmt = nullptr;
    QStringList paths;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            paths.append(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
        }
        sqlite3_finalize(stmt);
    }
    return getItemsByPaths(paths);
}

bool LibraryItemDao::upsertItem(const ItemRecord& record) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    SqlTransaction tx(db);

    QString fileId = record.fileId;
    if (fileId.isEmpty()) {
        const char* queryIdSql = "SELECT file_id FROM library_items WHERE file_path = ? COLLATE NOCASE;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, queryIdSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, record.path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            }
            sqlite3_finalize(stmt);
        }
    }
    if (fileId.isEmpty()) {
        fileId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    QFileInfo info(record.path);
    QString baseName = info.completeBaseName();
    QString ext = info.suffix();
    long long now = QDateTime::currentSecsSinceEpoch();

    const char* upsertSql = "INSERT INTO library_items ("
                            "file_id, file_path, file_name, base_name, extension, file_size, is_dir, is_empty, "
                            "is_hidden, is_managed, encrypted, pinned, rating, color_hex, auto_color, width, height, "
                            "thumb_status, note, url, sha256, created_at, updated_at, added_at, sync_state, synced_at) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?) "
                            "ON CONFLICT(file_id) DO UPDATE SET "
                            "file_path=excluded.file_path, file_name=excluded.file_name, base_name=excluded.base_name, "
                            "extension=excluded.extension, file_size=excluded.file_size, is_dir=excluded.is_dir, "
                            "is_empty=excluded.is_empty, is_hidden=excluded.is_hidden, is_managed=excluded.is_managed, "
                            "encrypted=excluded.encrypted, pinned=excluded.pinned, rating=excluded.rating, "
                            "color_hex=excluded.color_hex, auto_color=excluded.auto_color, width=excluded.width, "
                            "height=excluded.height, thumb_status=excluded.thumb_status, note=excluded.note, "
                            "url=excluded.url, sha256=excluded.sha256, created_at=excluded.created_at, "
                            "updated_at=excluded.updated_at, added_at=excluded.added_at, sync_state=0, synced_at=excluded.synced_at;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, upsertSql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, record.path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, record.filename.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, baseName.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, ext.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, record.size);
    sqlite3_bind_int(stmt, 7, record.isDir ? 1 : 0);
    sqlite3_bind_int(stmt, 8, record.isEmpty ? 1 : 0);
    sqlite3_bind_int(stmt, 9, record.isHidden ? 1 : 0);
    sqlite3_bind_int(stmt, 10, record.isManaged ? 1 : 0);
    sqlite3_bind_int(stmt, 11, record.encrypted ? 1 : 0);
    sqlite3_bind_int(stmt, 12, record.pinned ? 1 : 0);
    sqlite3_bind_int(stmt, 13, record.rating);
    sqlite3_bind_text(stmt, 14, record.manualColor.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 15, record.autoColor.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 16, record.width);
    sqlite3_bind_int(stmt, 17, record.height);
    sqlite3_bind_int(stmt, 18, record.thumbStatus);
    sqlite3_bind_text(stmt, 19, record.note.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 20, record.url.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 21, record.sha256.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 22, record.ctime);
    sqlite3_bind_int64(stmt, 23, record.mtime);
    sqlite3_bind_int64(stmt, 24, record.added_at > 0 ? record.added_at : now);
    sqlite3_bind_int64(stmt, 25, now);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);

    // 重建 file_tags
    const char* delTagsSql = "DELETE FROM file_tags WHERE file_id = ?;";
    if (sqlite3_prepare_v2(db, delTagsSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    const char* tagDictSql = "INSERT OR IGNORE INTO tags(name) VALUES(?);";
    const char* insTagSql = "INSERT OR REPLACE INTO file_tags(file_id, tag_name) VALUES(?, ?);";
    for (const QString& tag : record.tags) {
        if (tag.trimmed().isEmpty()) continue;
        if (sqlite3_prepare_v2(db, tagDictSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, tag.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        if (sqlite3_prepare_v2(db, insTagSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, tag.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // 重建 item_palettes
    const char* delPalSql = "DELETE FROM item_palettes WHERE file_id = ?;";
    if (sqlite3_prepare_v2(db, delPalSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    const char* insPalSql = "INSERT INTO item_palettes(file_id, idx, color_hex, ratio) VALUES(?, ?, ?, ?);";
    for (size_t i = 0; i < record.palettes.size(); ++i) {
        if (sqlite3_prepare_v2(db, insPalSql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, static_cast<int>(i));
            sqlite3_bind_text(stmt, 3, record.palettes[i].first.name().toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(stmt, 4, record.palettes[i].second);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    return tx.commit();
}

bool LibraryItemDao::removeItemsByPaths(const QStringList& paths) {
    if (paths.isEmpty()) return true;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    SqlTransaction tx(db);

    const char* sql = "DELETE FROM library_items WHERE file_path = ? COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    for (const QString& path : paths) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    return tx.commit();
}

bool LibraryItemDao::renameItemPath(const QString& oldPath, const QString& newPath) {
    return updatePath(oldPath, newPath);
}

bool LibraryItemDao::updatePath(const QString& oldPath, const QString& newPath) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || oldPath.isEmpty() || newPath.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    QFileInfo info(newPath);
    const char* sql = "UPDATE library_items SET file_path = ?, file_name = ?, base_name = ?, extension = ?, sync_state = 0 WHERE file_path = ? COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, info.fileName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, info.completeBaseName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, info.suffix().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, oldPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
    return false;
}

bool LibraryItemDao::updatePathPrefix(const QString& oldPrefix, const QString& newPrefix) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || oldPrefix.isEmpty() || newPrefix.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    QString cleanOld = QDir::toNativeSeparators(oldPrefix);
    QString cleanNew = QDir::toNativeSeparators(newPrefix);

    const char* sql = "UPDATE library_items SET file_path = ? || SUBSTR(file_path, ?) WHERE file_path LIKE ? || '%' COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, cleanNew.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, cleanOld.length() + 1);
        sqlite3_bind_text(stmt, 3, cleanOld.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
    return false;
}

bool LibraryItemDao::markSyncStateByPath(const QString& path, int syncState) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || path.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    const char* sql = "UPDATE library_items SET sync_state = ? WHERE file_path = ? COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, syncState);
        sqlite3_bind_text(stmt, 2, path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
    return false;
}

} // namespace QuarkMeta
