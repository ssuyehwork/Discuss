#include "SmartFolderDao.h"
#include "DatabaseManager.h"
#include <sqlite3.h>
#include <QJsonDocument>
#include <QDateTime>

namespace QuarkMeta {

QJsonObject SmartFolderRule::toJson() const {
    QJsonObject obj;
    obj["type"] = static_cast<int>(type);
    obj["op"] = static_cast<int>(op);
    obj["value"] = value;
    return obj;
}

SmartFolderRule SmartFolderRule::fromJson(const QJsonObject& obj) {
    SmartFolderRule rule;
    rule.type = static_cast<SmartRuleType>(obj.value("type").toInt(0));
    rule.op = static_cast<SmartRuleOperator>(obj.value("op").toInt(0));
    rule.value = obj.value("value").toString();
    return rule;
}

bool SmartFolderDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "CREATE TABLE IF NOT EXISTS smart_folders ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "name TEXT NOT NULL, "
                      "match_mode TEXT DEFAULT 'ALL', "
                      "icon_key TEXT DEFAULT 'folder_filled', "
                      "color_hex TEXT DEFAULT '#888888', "
                      "sort_order INTEGER DEFAULT 0, "
                      "rules_json TEXT DEFAULT '[]', "
                      "created_at INTEGER);";

    char* errMsgs = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsgs);
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

    const char* sql = "SELECT id, name, match_mode, icon_key, color_hex, sort_order, rules_json "
                      "FROM smart_folders ORDER BY sort_order ASC, id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SmartFolderRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);

        const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* matchStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        rec.sortOrder = sqlite3_column_int(stmt, 5);
        const char* rulesJsonStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        if (nameStr) rec.name = QString::fromUtf8(nameStr);
        if (matchStr) rec.matchMode = QString::fromUtf8(matchStr);
        if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
        if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);

        if (rulesJsonStr && strlen(rulesJsonStr) > 0) {
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray(rulesJsonStr));
            if (doc.isArray()) {
                QJsonArray arr = doc.array();
                for (const QJsonValue& val : arr) {
                    if (val.isObject()) {
                        rec.rules.append(SmartFolderRule::fromJson(val.toObject()));
                    }
                }
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

    const char* sql = "SELECT id, name, match_mode, icon_key, color_hex, sort_order, rules_json "
                      "FROM smart_folders WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return rec;

    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        rec.id = sqlite3_column_int(stmt, 0);

        const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* matchStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        rec.sortOrder = sqlite3_column_int(stmt, 5);
        const char* rulesJsonStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

        if (nameStr) rec.name = QString::fromUtf8(nameStr);
        if (matchStr) rec.matchMode = QString::fromUtf8(matchStr);
        if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
        if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);

        if (rulesJsonStr && strlen(rulesJsonStr) > 0) {
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray(rulesJsonStr));
            if (doc.isArray()) {
                QJsonArray arr = doc.array();
                for (const QJsonValue& val : arr) {
                    if (val.isObject()) {
                        rec.rules.append(SmartFolderRule::fromJson(val.toObject()));
                    }
                }
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

    const char* sql = "INSERT INTO smart_folders (name, match_mode, icon_key, color_hex, sort_order, rules_json, created_at) "
                      "VALUES (?, ?, ?, ?, (SELECT COALESCE(MAX(sort_order), 0) + 1 FROM smart_folders), ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    QJsonArray rulesArray;
    for (const auto& rule : rec.rules) {
        rulesArray.append(rule.toJson());
    }
    QJsonDocument doc(rulesArray);
    std::string jsonStd = doc.toJson(QJsonDocument::Compact).toStdString();

    std::string nameStd = rec.name.toStdString();
    std::string matchStd = rec.matchMode.toStdString();
    std::string iconStd = rec.iconKey.toStdString();
    std::string colorStd = rec.colorHex.toStdString();

    sqlite3_bind_text(stmt, 1, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, matchStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, jsonStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, QDateTime::currentSecsSinceEpoch());

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

    const char* sql = "UPDATE smart_folders SET name = ?, match_mode = ?, icon_key = ?, color_hex = ?, rules_json = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    QJsonArray rulesArray;
    for (const auto& rule : rec.rules) {
        rulesArray.append(rule.toJson());
    }
    QJsonDocument doc(rulesArray);
    std::string jsonStd = doc.toJson(QJsonDocument::Compact).toStdString();

    std::string nameStd = rec.name.toStdString();
    std::string matchStd = rec.matchMode.toStdString();
    std::string iconStd = rec.iconKey.toStdString();
    std::string colorStd = rec.colorHex.toStdString();

    sqlite3_bind_text(stmt, 1, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, matchStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, jsonStd.c_str(), -1, SQLITE_TRANSIENT);
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
