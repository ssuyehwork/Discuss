#include "SmartFolderDao.h"
#include "DatabaseManager.h"
#include "../core/FilterStateJson.h"
#include <QJsonDocument>
#include <QSqlQuery>
#include <QSqlError>

namespace QuarkMeta {

bool SmartFolderDao::initTable() {
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery checkQuery(db);
    checkQuery.exec("PRAGMA table_info(smart_folders);");
    bool hasFilterJson = false;
    while (checkQuery.next()) {
        if (checkQuery.value("name").toString() == "filter_json") {
            hasFilterJson = true;
            break;
        }
    }

    if (!hasFilterJson) {
        QSqlQuery dropQuery(db);
        dropQuery.exec("DROP TABLE IF EXISTS smart_folders;");
    }

    QSqlQuery query(db);
    QString sql = "CREATE TABLE IF NOT EXISTS smart_folders ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "name TEXT NOT NULL, "
                  "icon_key TEXT, "
                  "color_hex TEXT, "
                  "sort_order INTEGER DEFAULT 0, "
                  "filter_json TEXT);";
    return query.exec(sql);
}

QList<SmartFolderRecord> SmartFolderDao::getAllSmartFolders() {
    QList<SmartFolderRecord> list;
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.exec("SELECT id, name, icon_key, color_hex, sort_order, filter_json FROM smart_folders ORDER BY sort_order ASC, id ASC;");

    while (query.next()) {
        SmartFolderRecord rec;
        rec.id = query.value("id").toInt();
        rec.name = query.value("name").toString();
        rec.iconKey = query.value("icon_key").toString();
        rec.colorHex = query.value("color_hex").toString();
        rec.sortOrder = query.value("sort_order").toInt();

        QString filterStr = query.value("filter_json").toString();
        if (!filterStr.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(filterStr.toUtf8());
            if (doc.isObject()) {
                rec.filter = FilterStateJson::fromJson(doc.object());
            }
        }
        list.append(rec);
    }
    return list;
}

SmartFolderRecord SmartFolderDao::getSmartFolderById(int id) {
    SmartFolderRecord rec;
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.prepare("SELECT id, name, icon_key, color_hex, sort_order, filter_json FROM smart_folders WHERE id = ?;");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        rec.id = query.value("id").toInt();
        rec.name = query.value("name").toString();
        rec.iconKey = query.value("icon_key").toString();
        rec.colorHex = query.value("color_hex").toString();
        rec.sortOrder = query.value("sort_order").toInt();

        QString filterStr = query.value("filter_json").toString();
        if (!filterStr.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(filterStr.toUtf8());
            if (doc.isObject()) {
                rec.filter = FilterStateJson::fromJson(doc.object());
            }
        }
    }
    return rec;
}

int SmartFolderDao::addSmartFolder(const SmartFolderRecord& rec) {
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.prepare("INSERT INTO smart_folders (name, icon_key, color_hex, sort_order, filter_json) VALUES (?, ?, ?, ?, ?);");
    query.addBindValue(rec.name);
    query.addBindValue(rec.iconKey);
    query.addBindValue(rec.colorHex);
    query.addBindValue(rec.sortOrder);

    QJsonObject json = FilterStateJson::toJson(rec.filter);
    QJsonDocument doc(json);
    query.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    if (query.exec()) {
        return query.lastInsertId().toInt();
    }
    return 0;
}

bool SmartFolderDao::updateSmartFolder(const SmartFolderRecord& rec) {
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.prepare("UPDATE smart_folders SET name = ?, icon_key = ?, color_hex = ?, sort_order = ?, filter_json = ? WHERE id = ?;");
    query.addBindValue(rec.name);
    query.addBindValue(rec.iconKey);
    query.addBindValue(rec.colorHex);
    query.addBindValue(rec.sortOrder);

    QJsonObject json = FilterStateJson::toJson(rec.filter);
    QJsonDocument doc(json);
    query.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    query.addBindValue(rec.id);

    return query.exec();
}

bool SmartFolderDao::removeSmartFolderById(int id) {
    auto db = DatabaseManager::instance().getDatabase();
    QSqlQuery query(db);
    query.prepare("DELETE FROM smart_folders WHERE id = ?;");
    query.addBindValue(id);
    return query.exec();
}

} // namespace QuarkMeta
