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
    if (type == SmartRuleType::Tag) {
        QJsonArray tagArr;
        for (const QString& t : tags) {
            tagArr.append(t);
        }
        obj["tags"] = tagArr;
    } else {
        obj["value"] = value;
    }

    if (!date1.isEmpty()) obj["date1"] = date1;
    if (!date2.isEmpty()) obj["date2"] = date2;
    if (days > 0) obj["days"] = days;
    if (num1 != 0) obj["num1"] = num1;
    if (num2 != 0) obj["num2"] = num2;
    if (!unit.isEmpty()) obj["unit"] = unit;

    return obj;
}

SmartFolderRule SmartFolderRule::fromJson(const QJsonObject& obj) {
    SmartFolderRule rule;
    rule.type = static_cast<SmartRuleType>(obj.value("type").toInt(0));
    rule.op = static_cast<SmartRuleOperator>(obj.value("op").toInt(0));
    rule.value = obj.value("value").toString();

    if (obj.contains("tags") && obj.value("tags").isArray()) {
        QJsonArray tagArr = obj.value("tags").toArray();
        for (const auto& val : tagArr) {
            QString t = val.toString().trimmed();
            if (!t.isEmpty() && !rule.tags.contains(t)) {
                rule.tags.append(t);
            }
        }
    } else if (rule.type == SmartRuleType::Tag) {
        QString trimmed = rule.value.trimmed();
        if (!trimmed.isEmpty()) {
            rule.tags.append(trimmed);
        }
    }

    rule.date1 = obj.value("date1").toString();
    rule.date2 = obj.value("date2").toString();
    rule.days = obj.value("days").toInt(0);
    rule.num1 = obj.value("num1").toVariant().toLongLong();
    rule.num2 = obj.value("num2").toVariant().toLongLong();
    rule.unit = obj.value("unit").toString();

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

static QString escapeLikeString(const QString& rawInput) {
    QString escaped = rawInput;
    escaped.replace('\\', "\\\\");
    escaped.replace('%', "\\%");
    escaped.replace('_', "\\_");
    escaped.replace('\'', "''");
    return escaped;
}

static QString escapeSqlString(const QString& rawInput) {
    QString escaped = rawInput;
    escaped.replace('\'', "''");
    return escaped;
}

QString SmartFolderDao::buildRatingCondition(const SmartFolderRule& rule) {
    QString valStr = rule.value.trimmed();
    if (valStr == "0" || valStr == "无") {
        return "rating = 0";
    }
    bool ok = false;
    int r = valStr.toInt(&ok);
    if (!ok) return QString();

    if (rule.op == SmartRuleOperator::GreaterOrEqual) {
        return QString("rating >= %1").arg(r);
    } else if (rule.op == SmartRuleOperator::LessOrEqual) {
        return QString("rating <= %1").arg(r);
    } else {
        return QString("rating = %1").arg(r);
    }
}

QString SmartFolderDao::buildNameCondition(const SmartFolderRule& rule) {
    QString val = rule.value.trimmed();
    if (val.isEmpty()) return QString();

    if (rule.op == SmartRuleOperator::Equals) {
        return QString("LOWER(file_name) = LOWER('%1')").arg(escapeSqlString(val));
    } else if (rule.op == SmartRuleOperator::NotContains) {
        return QString("LOWER(file_name) NOT LIKE '%%%1%%' ESCAPE '\\'").arg(escapeLikeString(val));
    } else if (rule.op == SmartRuleOperator::StartsWith) {
        return QString("LOWER(file_name) LIKE '%1%%' ESCAPE '\\'").arg(escapeLikeString(val));
    } else if (rule.op == SmartRuleOperator::EndsWith) {
        return QString("LOWER(file_name) LIKE '%%%1' ESCAPE '\\'").arg(escapeLikeString(val));
    } else { // Contains
        return QString("LOWER(file_name) LIKE '%%%1%%' ESCAPE '\\'").arg(escapeLikeString(val));
    }
}

QString SmartFolderDao::buildFolderNameCondition(const SmartFolderRule& rule) {
    QString val = rule.value.trimmed();
    if (val.isEmpty()) return QString();

    if (rule.op == SmartRuleOperator::Equals) {
        return QString("(is_dir = 1 AND LOWER(file_name) = LOWER('%1'))").arg(escapeSqlString(val));
    } else if (rule.op == SmartRuleOperator::NotContains) {
        return QString("(is_dir = 1 AND LOWER(file_name) NOT LIKE '%%%1%%' ESCAPE '\\')").arg(escapeLikeString(val));
    } else if (rule.op == SmartRuleOperator::StartsWith) {
        return QString("(is_dir = 1 AND LOWER(file_name) LIKE '%1%%' ESCAPE '\\')").arg(escapeLikeString(val));
    } else if (rule.op == SmartRuleOperator::EndsWith) {
        return QString("(is_dir = 1 AND LOWER(file_name) LIKE '%%%1' ESCAPE '\\')").arg(escapeLikeString(val));
    } else { // Contains
        return QString("(is_dir = 1 AND LOWER(file_name) LIKE '%%%1%%' ESCAPE '\\')").arg(escapeLikeString(val));
    }
}

QString SmartFolderDao::buildFolderCondition(const SmartFolderRule& rule) {
    if (rule.op == SmartRuleOperator::IsNotFolder) {
        return "is_dir = 0";
    }
    return "is_dir = 1";
}

QString SmartFolderDao::buildDateCondition(const QString& colName, const SmartFolderRule& rule) {
    if (rule.op == SmartRuleOperator::RecentDays) {
        int d = rule.days > 0 ? rule.days : rule.value.toInt();
        if (d <= 0) d = 1;
        QDate today = QDate::currentDate();
        QDate startDay = today.addDays(-(d - 1));
        qint64 t1 = QDateTime(startDay, QTime(0, 0, 0, 0)).toMSecsSinceEpoch();
        return QString("(%1 > 0 AND %1 >= %2)").arg(colName).arg(t1);
    }

    QDate d1 = QDate::fromString(rule.date1, "yyyy-MM-dd");
    if (!d1.isValid() && !rule.value.isEmpty()) {
        d1 = QDate::fromString(rule.value.trimmed(), "yyyy-MM-dd");
    }
    if (!d1.isValid()) return QString();

    if (rule.op == SmartRuleOperator::Before) {
        qint64 t = QDateTime(d1, QTime(0, 0, 0, 0)).toMSecsSinceEpoch();
        return QString("(%1 > 0 AND %1 < %2)").arg(colName).arg(t);
    } else if (rule.op == SmartRuleOperator::After) {
        qint64 t = QDateTime(d1, QTime(23, 59, 59, 999)).toMSecsSinceEpoch();
        return QString("(%1 > 0 AND %1 > %2)").arg(colName).arg(t);
    } else if (rule.op == SmartRuleOperator::Between) {
        QDate d2 = QDate::fromString(rule.date2, "yyyy-MM-dd");
        if (!d2.isValid()) d2 = d1;
        if (d1 > d2) std::swap(d1, d2);
        qint64 t1 = QDateTime(d1, QTime(0, 0, 0, 0)).toMSecsSinceEpoch();
        qint64 t2 = QDateTime(d2, QTime(23, 59, 59, 999)).toMSecsSinceEpoch();
        return QString("(%1 >= %2 AND %1 <= %3)").arg(colName).arg(t1).arg(t2);
    } else { // Equals
        qint64 t1 = QDateTime(d1, QTime(0, 0, 0, 0)).toMSecsSinceEpoch();
        qint64 t2 = QDateTime(d1, QTime(23, 59, 59, 999)).toMSecsSinceEpoch();
        return QString("(%1 >= %2 AND %1 <= %3)").arg(colName).arg(t1).arg(t2);
    }
}

QString SmartFolderDao::buildNumericCondition(const QString& colName, const SmartFolderRule& rule) {
    qint64 n1 = rule.num1;
    qint64 n2 = rule.num2;
    if (n1 == 0 && !rule.value.isEmpty()) {
        n1 = rule.value.toLongLong();
    }

    if (rule.op == SmartRuleOperator::Between) {
        if (n1 > n2) std::swap(n1, n2);
        return QString("(%1 > 0 AND %1 >= %2 AND %1 <= %3)").arg(colName).arg(n1).arg(n2);
    } else if (rule.op == SmartRuleOperator::LessOrEqual) {
        return QString("(%1 > 0 AND %1 <= %2)").arg(colName).arg(n1);
    } else if (rule.op == SmartRuleOperator::Equals) {
        return QString("(%1 > 0 AND %1 = %2)").arg(colName).arg(n1);
    } else { // GreaterOrEqual
        return QString("(%1 > 0 AND %1 >= %2)").arg(colName).arg(n1);
    }
}

QString SmartFolderDao::buildFileSizeCondition(const SmartFolderRule& rule) {
    qint64 n1 = rule.num1;
    qint64 n2 = rule.num2;
    if (n1 == 0 && !rule.value.isEmpty()) {
        n1 = rule.value.toLongLong();
    }

    qint64 mult = 1024; // 默认 KB
    QString u = rule.unit.trimmed().toUpper();
    if (u == "MB") mult = 1024 * 1024;
    else if (u == "GB") mult = 1024LL * 1024LL * 1024LL;
    else if (u == "KB" || u.isEmpty()) mult = 1024;

    qint64 bytes1 = n1 * mult;
    qint64 bytes2 = n2 * mult;

    if (rule.op == SmartRuleOperator::Between) {
        if (bytes1 > bytes2) std::swap(bytes1, bytes2);
        return QString("(is_dir = 0 AND file_size >= %1 AND file_size <= %2)").arg(bytes1).arg(bytes2);
    } else if (rule.op == SmartRuleOperator::LessOrEqual) {
        return QString("(is_dir = 0 AND file_size <= %1)").arg(bytes1);
    } else if (rule.op == SmartRuleOperator::Equals) {
        return QString("(is_dir = 0 AND file_size = %1)").arg(bytes1);
    } else { // GreaterOrEqual
        return QString("(is_dir = 0 AND file_size >= %1)").arg(bytes1);
    }
}

QString SmartFolderDao::buildTextPropertyCondition(const QString& colName, const SmartFolderRule& rule) {
    if (rule.op == SmartRuleOperator::IsEmpty) {
        return QString("(%1 IS NULL OR %1 = '')").arg(colName);
    } else if (rule.op == SmartRuleOperator::IsNotEmpty) {
        return QString("(%1 IS NOT NULL AND %1 != '')").arg(colName);
    }

    QString val = rule.value.trimmed();
    if (val.isEmpty()) return QString();

    if (rule.op == SmartRuleOperator::NotContains) {
        return QString("(%1 IS NULL OR %1 = '' OR LOWER(%1) NOT LIKE '%%%2%%' ESCAPE '\\')")
            .arg(colName)
            .arg(escapeLikeString(val));
    } else { // Contains
        return QString("(%1 IS NOT NULL AND LOWER(%1) LIKE '%%%2%%' ESCAPE '\\')")
            .arg(colName)
            .arg(escapeLikeString(val));
    }
}

QString SmartFolderDao::buildColorCondition(const SmartFolderRule& rule) {
    QString colorVal = escapeSqlString(rule.value.trimmed());
    if (colorVal.isEmpty()) return QString();

    if (rule.op == SmartRuleOperator::NotContains) {
        return QString("color_hex != '%1'").arg(colorVal);
    } else {
        return QString("color_hex = '%1'").arg(colorVal);
    }
}

QString SmartFolderDao::buildExtensionCondition(const SmartFolderRule& rule) {
    QString rawValue = rule.value.trimmed();
    if (rawValue.isEmpty()) return QString();

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
            cleanExts << escapeSqlString(token);
        }
    }

    if (cleanExts.isEmpty()) return QString();

    if (cleanExts.size() == 1) {
        if (rule.op == SmartRuleOperator::NotContains) {
            return QString("LOWER(extension) != LOWER('%1')").arg(cleanExts.first());
        } else {
            return QString("LOWER(extension) = LOWER('%1')").arg(cleanExts.first());
        }
    } else {
        QStringList quotedList;
        for (const QString& ext : cleanExts) {
            quotedList << QString("LOWER('%1')").arg(ext);
        }
        if (rule.op == SmartRuleOperator::NotContains) {
            return QString("LOWER(extension) NOT IN (%1)").arg(quotedList.join(", "));
        } else {
            return QString("LOWER(extension) IN (%1)").arg(quotedList.join(", "));
        }
    }
}

QString SmartFolderDao::buildTagCondition(const SmartFolderRule& rule) {
    QStringList cleanTags;
    for (const QString& t : rule.tags) {
        QString trimmed = escapeSqlString(t.trimmed());
        if (!trimmed.isEmpty() && !cleanTags.contains(trimmed)) {
            cleanTags << trimmed;
        }
    }

    if (cleanTags.isEmpty()) return QString();

    QStringList quotedTags;
    for (const QString& ct : cleanTags) {
        quotedTags << QString("'%1'").arg(ct);
    }
    QString tagListStr = quotedTags.join(", ");

    if (rule.op == SmartRuleOperator::NotContains) {
        return QString("file_id NOT IN (SELECT file_id FROM file_tags WHERE tag_name IN (%1))")
            .arg(tagListStr);
    } else {
        return QString("file_id IN (SELECT file_id FROM file_tags WHERE tag_name IN (%1) GROUP BY file_id HAVING COUNT(DISTINCT tag_name) = %2)")
            .arg(tagListStr)
            .arg(cleanTags.size());
    }
}

QString SmartFolderDao::buildWhereClause(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return QString();

    QStringList whereConditions;
    for (const auto& rule : rec.rules) {
        QString cond;
        switch (rule.type) {
        case SmartRuleType::Rating:
            cond = buildRatingCondition(rule);
            break;
        case SmartRuleType::Name:
            cond = buildNameCondition(rule);
            break;
        case SmartRuleType::FolderName:
            cond = buildFolderNameCondition(rule);
            break;
        case SmartRuleType::Folder:
            cond = buildFolderCondition(rule);
            break;
        case SmartRuleType::AddedDate:
            cond = buildDateCondition("added_at", rule);
            break;
        case SmartRuleType::ModifiedDate:
            cond = buildDateCondition("updated_at", rule);
            break;
        case SmartRuleType::CreatedDate:
            cond = buildDateCondition("created_at", rule);
            break;
        case SmartRuleType::Link:
            cond = buildTextPropertyCondition("url", rule);
            break;
        case SmartRuleType::Note:
            cond = buildTextPropertyCondition("note", rule);
            break;
        case SmartRuleType::Width:
            cond = buildNumericCondition("width", rule);
            break;
        case SmartRuleType::Height:
            cond = buildNumericCondition("height", rule);
            break;
        case SmartRuleType::FileSize:
            cond = buildFileSizeCondition(rule);
            break;
        case SmartRuleType::Color:
            cond = buildColorCondition(rule);
            break;
        case SmartRuleType::Extension:
            cond = buildExtensionCondition(rule);
            break;
        case SmartRuleType::Tag:
            cond = buildTagCondition(rule);
            break;
        }

        if (!cond.isEmpty()) {
            whereConditions << cond;
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

int SmartFolderDao::countMatchingItemsReadOnly(sqlite3* readOnlyDb, const SmartFolderRecord& rec) {
    if (!readOnlyDb || rec.rules.isEmpty()) return 0;

    QString whereClause = buildWhereClause(rec);
    if (whereClause.isEmpty()) return 0;

    QString sql = QString("SELECT COUNT(*) FROM library_items WHERE %1 AND sync_state != 2;")
                  .arg(whereClause);

    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(readOnlyDb, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
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
