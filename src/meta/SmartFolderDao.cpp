#include "SmartFolderDao.h"
#include "DatabaseManager.h"
#include "LibraryDao.h"
#include "MetadataManager.h"
#include <sqlite3.h>
#include <QJsonDocument>
#include <QDateTime>
#include <QFileInfo>
#include <QDirIterator>
#include <QSet>
#include <QRegularExpression>

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
                      "icon_key TEXT DEFAULT 'smart-folder-1.svg', "
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
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("SmartFolderDao::getAllSmartFolders prepare", db, sql);
        return list;
    }

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
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("SmartFolderDao::getSmartFolderById prepare", db, sql);
        return rec;
    }

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
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("SmartFolderDao::addSmartFolder prepare", db, sql);
        return 0;
    }

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
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("SmartFolderDao::updateSmartFolder prepare", db, sql);
        return false;
    }

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

QString SmartFolderDao::buildWhereClause(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return QString();

    QStringList whereConditions;
    for (const auto& rule : rec.rules) {
        if (rule.type == SmartRuleType::Rating) {
            QString op = (rule.op == SmartRuleOperator::GreaterOrEqual) ? ">=" :
                         (rule.op == SmartRuleOperator::LessOrEqual) ? "<=" : "=";
            whereConditions << QString("rating %1 %2").arg(op).arg(rule.value.toInt());
        } 
        else if (rule.type == SmartRuleType::Extension) {
            QString rawValue = rule.value.trimmed();
            if (!rawValue.isEmpty()) {
                static const QRegularExpression splitRegex("[,;\\s|]+");
                QStringList rawTokens = rawValue.split(splitRegex, Qt::SkipEmptyParts);
                QStringList cleanExts;
                for (QString token : rawTokens) {
                    token = token.trimmed();
                    if (token.startsWith('.')) {
                        token.remove(0, 1);
                    }
                    token = token.trimmed();
                    if (!token.isEmpty()) {
                        cleanExts << token.replace("'", "''");
                    }
                }

                if (!cleanExts.isEmpty()) {
                    if (cleanExts.size() == 1) {
                        if (rule.op == SmartRuleOperator::NotContains) {
                            whereConditions << QString("LOWER(extension) != LOWER('%1')").arg(cleanExts.first());
                        } else {
                            whereConditions << QString("LOWER(extension) = LOWER('%1')").arg(cleanExts.first());
                        }
                    } else {
                        QStringList quotedList;
                        for (const QString& ext : cleanExts) {
                            quotedList << QString("LOWER('%1')").arg(ext);
                        }
                        if (rule.op == SmartRuleOperator::NotContains) {
                            whereConditions << QString("LOWER(extension) NOT IN (%1)").arg(quotedList.join(", "));
                        } else {
                            whereConditions << QString("LOWER(extension) IN (%1)").arg(quotedList.join(", "));
                        }
                    }
                }
            }
        } 
        else if (rule.type == SmartRuleType::Color) {
            QString colorVal = rule.value.trimmed().replace("'", "''");
            if (!colorVal.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("color_hex != '%1'").arg(colorVal);
                } else {
                    whereConditions << QString("color_hex = '%1'").arg(colorVal);
                }
            }
        }
        else if (rule.type == SmartRuleType::Tag) {
            // 利用 file_tags 多对多关联表实现秒级查询 (以 file_id 解耦匹配)
            QString tagVal = rule.value.trimmed().replace("'", "''");
            if (!tagVal.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                       .arg(tagVal);
                } else {
                    whereConditions << QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name = '%1')")
                                       .arg(tagVal);
                }
            }
        }
        else if (rule.type == SmartRuleType::Keyword) {
            QString kw = rule.value.trimmed().replace("'", "''");
            if (!kw.isEmpty()) {
                if (rule.op == SmartRuleOperator::NotContains) {
                    whereConditions << QString("file_name NOT LIKE '%%%1%%'").arg(kw);
                } else {
                    whereConditions << QString("file_name LIKE '%%%1%%'").arg(kw);
                }
            }
        }
    }

    if (whereConditions.isEmpty()) return QString();

    QString connector = (rec.matchMode.toUpper() == "ALL") ? " AND " : " OR ";
    return "(" + whereConditions.join(connector) + ")";
}

int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return 0;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString whereClause = buildWhereClause(rec);
    if (whereClause.isEmpty()) return 0;

    QString sql = QString("SELECT COUNT(*) FROM library_items WHERE %1 AND sync_state != 2;")
                  .arg(whereClause);

    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    } else {
        DatabaseManager::logSqlError("SmartFolderDao::countMatchingItems prepare", db, sql);
    }
    return count;
}

QStringList SmartFolderDao::getMatchingPaths(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return {};

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return {};

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString whereClause = buildWhereClause(rec);
    if (whereClause.isEmpty()) return {};

    QString sql = QString("SELECT file_path FROM library_items WHERE %1 AND sync_state != 2;")
                  .arg(whereClause);

    sqlite3_stmt* stmt = nullptr;
    QStringList paths;
    if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* p = sqlite3_column_text(stmt, 0);
            if (p) {
                paths.append(QString::fromUtf8(reinterpret_cast<const char*>(p)));
            }
        }
        sqlite3_finalize(stmt);
    } else {
        DatabaseManager::logSqlError("SmartFolderDao::getMatchingPaths prepare", db, sql);
    }
    return paths;
}

bool SmartFolderDao::removeSmartFolderById(int id) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM smart_folders WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        DatabaseManager::logSqlError("SmartFolderDao::removeSmartFolderById prepare", db, sql);
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

} // namespace QuarkMeta
