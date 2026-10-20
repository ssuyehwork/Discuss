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

static bool evaluateRuleForRuntimeMeta(const SmartFolderRule& rule,
                                       const QString& filePath,
                                       const QString& fileName,
                                       const QString& suffix,
                                       const RuntimeMeta& meta) {
    if (rule.type == SmartRuleType::Rating) {
        int rVal = rule.value.toInt();
        if (rule.op == SmartRuleOperator::GreaterOrEqual) return (meta.rating >= rVal);
        if (rule.op == SmartRuleOperator::Equals) return (meta.rating == rVal);
        if (rule.op == SmartRuleOperator::LessOrEqual) return (meta.rating <= rVal);
        if (rule.op == SmartRuleOperator::Contains) return (meta.rating == rVal);
        if (rule.op == SmartRuleOperator::NotContains) return (meta.rating != rVal);
    } else if (rule.type == SmartRuleType::Extension) {
        QString targetExt = rule.value.trimmed();
        if (targetExt.startsWith('.')) targetExt.remove(0, 1);
        bool equals = (suffix.compare(targetExt, Qt::CaseInsensitive) == 0) ||
                      (fileName.endsWith("." + targetExt, Qt::CaseInsensitive));
        if (rule.op == SmartRuleOperator::NotContains) return !equals;
        return equals;
    } else if (rule.type == SmartRuleType::Color) {
        QString itemColor = QString::fromStdWString(meta.manualColor);
        if (itemColor.isEmpty()) itemColor = QString::fromStdWString(meta.autoColor);
        bool matchColor = (itemColor.compare(rule.value.trimmed(), Qt::CaseInsensitive) == 0);
        if (rule.op == SmartRuleOperator::NotContains) return !matchColor;
        return matchColor;
    } else if (rule.type == SmartRuleType::Tag) {
        bool hasTag = false;
        QString valTrimmed = rule.value.trimmed();
        for (const QString& t : meta.tags) {
            if (t.compare(valTrimmed, Qt::CaseInsensitive) == 0) {
                hasTag = true;
                break;
            }
        }
        if (rule.op == SmartRuleOperator::NotContains) return !hasTag;
        return hasTag;
    } else if (rule.type == SmartRuleType::Keyword) {
        QString note = QString::fromStdWString(meta.note);
        QString url = QString::fromStdWString(meta.url);
        QString kw = rule.value.trimmed();
        bool matchKw = fileName.contains(kw, Qt::CaseInsensitive) ||
                       filePath.contains(kw, Qt::CaseInsensitive) ||
                       note.contains(kw, Qt::CaseInsensitive) ||
                       url.contains(kw, Qt::CaseInsensitive);
        if (rule.op == SmartRuleOperator::NotContains) return !matchKw;
        return matchKw;
    }
    return false;
}

static bool evaluateRuleForItemMeta(const SmartFolderRule& rule,
                                    const QString& filePath,
                                    const QString& fileName,
                                    const QString& suffix,
                                    const ItemMeta& meta) {
    if (rule.type == SmartRuleType::Rating) {
        int rVal = rule.value.toInt();
        if (rule.op == SmartRuleOperator::GreaterOrEqual) return (meta.rating >= rVal);
        if (rule.op == SmartRuleOperator::Equals) return (meta.rating == rVal);
        if (rule.op == SmartRuleOperator::LessOrEqual) return (meta.rating <= rVal);
        if (rule.op == SmartRuleOperator::Contains) return (meta.rating == rVal);
        if (rule.op == SmartRuleOperator::NotContains) return (meta.rating != rVal);
    } else if (rule.type == SmartRuleType::Extension) {
        QString targetExt = rule.value.trimmed();
        if (targetExt.startsWith('.')) targetExt.remove(0, 1);
        bool equals = (suffix.compare(targetExt, Qt::CaseInsensitive) == 0) ||
                      (fileName.endsWith("." + targetExt, Qt::CaseInsensitive));
        if (rule.op == SmartRuleOperator::NotContains) return !equals;
        return equals;
    } else if (rule.type == SmartRuleType::Color) {
        QString itemColor = QString::fromStdWString(meta.color);
        if (itemColor.isEmpty()) itemColor = QString::fromStdWString(meta.autoColor);
        bool matchColor = (itemColor.compare(rule.value.trimmed(), Qt::CaseInsensitive) == 0);
        if (rule.op == SmartRuleOperator::NotContains) return !matchColor;
        return matchColor;
    } else if (rule.type == SmartRuleType::Tag) {
        bool hasTag = false;
        QString valTrimmed = rule.value.trimmed();
        for (const auto& t : meta.tags) {
            if (QString::fromStdWString(t).compare(valTrimmed, Qt::CaseInsensitive) == 0) {
                hasTag = true;
                break;
            }
        }
        if (rule.op == SmartRuleOperator::NotContains) return !hasTag;
        return hasTag;
    } else if (rule.type == SmartRuleType::Keyword) {
        QString note = QString::fromStdWString(meta.note);
        QString url = QString::fromStdWString(meta.url);
        QString kw = rule.value.trimmed();
        bool matchKw = fileName.contains(kw, Qt::CaseInsensitive) ||
                       filePath.contains(kw, Qt::CaseInsensitive) ||
                       note.contains(kw, Qt::CaseInsensitive) ||
                       url.contains(kw, Qt::CaseInsensitive);
        if (rule.op == SmartRuleOperator::NotContains) return !matchKw;
        return matchKw;
    }
    return false;
}

int SmartFolderDao::countMatchingItems(const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty()) return 0;

    QSet<QString> processedPaths;
    int count = 0;
    bool isAllMode = (rec.matchMode == "ALL");

    // 1. 获取库中配置的所有底层分类根路径与文件
    QStringList libPaths = LibraryDao::getCategoryPaths(-1);

    auto checkFileAndIncrement = [&](const QString& filePath) {
        if (filePath.isEmpty()) return;
        QFileInfo fi(filePath);
        if (fi.isDir()) return;

        QString canonical = fi.canonicalFilePath().toLower();
        if (canonical.isEmpty()) canonical = QDir::cleanPath(filePath).toLower();
        if (processedPaths.contains(canonical)) return;
        processedPaths.insert(canonical);

        QString fileName = fi.fileName();
        QString suffix = fi.suffix();

        if (fileName.endsWith(".QuarkMeta.json", Qt::CaseInsensitive) ||
            fileName.endsWith(".tmp", Qt::CaseInsensitive)) {
            return;
        }

        RuntimeMeta meta = MetadataManager::instance().getMeta(filePath.toStdWString());

        bool itemMatches = isAllMode;
        for (const auto& rule : rec.rules) {
            bool rulePassed = evaluateRuleForRuntimeMeta(rule, filePath, fileName, suffix, meta);
            if (isAllMode) {
                if (!rulePassed) { itemMatches = false; break; }
            } else {
                if (rulePassed) { itemMatches = true; break; }
            }
        }
        if (itemMatches) {
            count++;
        }
    };

    for (const QString& libPath : libPaths) {
        if (libPath.isEmpty()) continue;
        QFileInfo fi(libPath);
        if (fi.isDir()) {
            QDirIterator it(libPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                checkFileAndIncrement(it.filePath());
            }
        } else if (fi.isFile()) {
            checkFileAndIncrement(libPath);
        }
    }

    // 2. 补漏：检查数据库已关联但可能不在上述磁盘扫描路径里的条目
    auto indexedItems = LibraryDao::getCategoryIndexedItems(-1);
    for (const auto& item : indexedItems) {
        QString filePath = QDir::cleanPath(QString::fromStdWString(item.originalName));
        if (filePath.isEmpty()) continue;

        QFileInfo fi(filePath);
        if (fi.isDir()) continue;

        QString canonical = fi.canonicalFilePath().toLower();
        if (canonical.isEmpty()) canonical = filePath.toLower();
        if (processedPaths.contains(canonical)) continue;
        processedPaths.insert(canonical);

        QString fileName = fi.fileName();
        QString suffix = fi.suffix();

        bool itemMatches = isAllMode;
        for (const auto& rule : rec.rules) {
            bool rulePassed = evaluateRuleForItemMeta(rule, filePath, fileName, suffix, item);
            if (isAllMode) {
                if (!rulePassed) { itemMatches = false; break; }
            } else {
                if (rulePassed) { itemMatches = true; break; }
            }
        }
        if (itemMatches) {
            count++;
        }
    }

    return count;
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
