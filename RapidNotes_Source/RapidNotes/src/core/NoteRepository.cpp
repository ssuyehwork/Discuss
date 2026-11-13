#include "NoteRepository.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

namespace RapidNotes {

NoteRepository& NoteRepository::instance() {
    static NoteRepository repo;
    return repo;
}

NoteRepository::NoteRepository() {}

QMap<int, int> NoteRepository::getCategoryCounts() {
    QMap<int, int> counts;
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return counts;

    // 统计各正常分类的直属笔记数 (is_deleted = 0)
    QSqlQuery q(db);
    QString sql = "SELECT category_id, COUNT(*) FROM notes WHERE is_deleted = 0 AND category_id > 0 GROUP BY category_id";
    if (q.exec(sql)) {
        while (q.next()) {
            counts[q.value(0).toInt()] = q.value(1).toInt();
        }
    }

    // 统计未分类数量 (-2 代表未分类系统节点)
    QSqlQuery qUncat(db);
    if (qUncat.exec("SELECT COUNT(*) FROM notes WHERE is_deleted = 0 AND (category_id IS NULL OR category_id <= 0)")) {
        if (qUncat.next()) {
            counts[-2] = qUncat.value(0).toInt();
        }
    }

    // 统计全部数据 (-1 代表全部数据系统节点)
    QSqlQuery qAll(db);
    if (qAll.exec("SELECT COUNT(*) FROM notes WHERE is_deleted = 0")) {
        if (qAll.next()) {
            counts[-1] = qAll.value(0).toInt();
        }
    }

    return counts;
}

QList<NoteItemDTO> NoteRepository::loadNotesLight(int categoryId, int offset, int limit, const QString& searchKeyword) {
    QList<NoteItemDTO> list;
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen()) return list;

    QString sql = "SELECT id, title, SUBSTR(content, 1, 200) AS summary, tags, rating, "
                  "COALESCE(color_label, '') AS color_label, is_pinned, item_type, "
                  "created_at, updated_at, remark, word_count "
                  "FROM notes WHERE is_deleted = 0 ";
    if (categoryId > 0) sql += "AND category_id = :cat ";
    else if (categoryId == -2) sql += "AND (category_id IS NULL OR category_id <= 0) ";

    const QString kw = searchKeyword.trimmed();
    if (!kw.isEmpty()) {
        sql += "AND (title LIKE :kw ESCAPE '\\' OR remark LIKE :kw ESCAPE '\\' OR url LIKE :kw ESCAPE '\\' "
               "OR EXISTS (SELECT 1 FROM note_tags nt WHERE nt.note_id = notes.id AND nt.tag_name LIKE :kw ESCAPE '\\')) ";
    }
    sql += "ORDER BY is_pinned DESC, updated_at DESC ";
    if (limit > 0) sql += "LIMIT :lim OFFSET :off";

    QSqlQuery q(db);
    q.prepare(sql);
    if (categoryId > 0) q.bindValue(":cat", categoryId);
    if (!kw.isEmpty()) {
        QString esc = kw;
        esc.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_");
        q.bindValue(":kw", "%" + esc + "%");
    }
    if (limit > 0) { q.bindValue(":lim", limit); q.bindValue(":off", offset); }
    if (q.exec()) {
        while (q.next()) {
            NoteItemDTO item;
            item.id = q.value("id").toLongLong();
            item.title = q.value("title").toString();
            item.summary = q.value("summary").toString();
            item.tags = q.value("tags").toString();
            item.rating = q.value("rating").toInt();
            item.colorLabel = q.value("color_label").toString();
            item.isPinned = (q.value("is_pinned").toInt() == 1);
            item.itemType = q.value("item_type").toString();
            item.createdAt = q.value("created_at").toString();
            item.updatedAt = q.value("updated_at").toString();
            item.remark = q.value("remark").toString();
            item.wordCount = q.value("word_count").toInt();
            list.append(item);
        }
    }
    return list;
}

bool NoteRepository::updateRating(qint64 noteId, int rating) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE notes SET rating = :rating WHERE id = :id");
    q.bindValue(":rating", rating);
    q.bindValue(":id", noteId);
    return q.exec();
}

bool NoteRepository::updateColorLabel(qint64 noteId, const QString& colorLabel) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE notes SET color_label = :color_label WHERE id = :id");
    q.bindValue(":color_label", colorLabel);
    q.bindValue(":id", noteId);
    return q.exec();
}

bool NoteRepository::updateTags(qint64 noteId, const QString& tags) {
    return DatabaseManager::instance().updateNoteStateBatch({int(noteId)}, "tags", tags);
}

bool NoteRepository::updateRemark(qint64 noteId, const QString& remark) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE notes SET remark = :remark WHERE id = :id");
    q.bindValue(":remark", remark);
    q.bindValue(":id", noteId);
    return q.exec();
}

bool NoteRepository::updatePinned(qint64 noteId, bool isPinned) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE notes SET is_pinned = :pinned WHERE id = :id");
    q.bindValue(":pinned", isPinned ? 1 : 0);
    q.bindValue(":id", noteId);
    return q.exec();
}

bool NoteRepository::updateTitle(qint64 noteId, const QString& title) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE notes SET title = :title WHERE id = :id");
    q.bindValue(":title", title);
    q.bindValue(":id", noteId);
    return q.exec();
}

bool NoteRepository::batchUpdateRating(const QList<qint64>& noteIds, int rating) {
    QList<int> ids; for (qint64 i : noteIds) ids << int(i);
    return DatabaseManager::instance().updateNoteStateBatch(ids, "rating", rating);
}

bool NoteRepository::batchUpdateColorLabel(const QList<qint64>& noteIds, const QString& colorLabel) {
    QList<int> ids; for (qint64 i : noteIds) ids << int(i);
    return DatabaseManager::instance().updateNoteStateBatch(ids, "color_label", colorLabel);
}

bool NoteRepository::batchUpdateTags(const QList<qint64>& noteIds, const QString& tags) {
    QList<int> ids; for (qint64 i : noteIds) ids << int(i);
    return DatabaseManager::instance().updateNoteStateBatch(ids, "tags", tags);   // 经标签统一入口，写 note_tags
}

bool NoteRepository::moveNotesToCategory(const QList<qint64>& noteIds, int targetCategoryId) {
    QList<int> ids; for (qint64 i : noteIds) ids << int(i);
    return DatabaseManager::instance().moveNotesToCategory(ids, targetCategoryId > 0 ? targetCategoryId : -1);
}

bool NoteRepository::updateCategoryOrder(const QList<int>& orderedIds) {
    if (orderedIds.isEmpty()) return true;
    QSqlDatabase db = DatabaseManager::instance().database();
    db.transaction();
    QSqlQuery q(db);
    q.prepare("UPDATE categories SET sort_order = :ord WHERE id = :id");
    for (int i = 0; i < orderedIds.size(); ++i) {
        q.bindValue(":ord", i);
        q.bindValue(":id", orderedIds[i]);
        q.exec();
    }
    return db.commit();
}

bool NoteRepository::renameCategory(int categoryId, const QString& newName) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE categories SET name = :name WHERE id = :id");
    q.bindValue(":name", newName);
    q.bindValue(":id", categoryId);
    return q.exec();
}

bool NoteRepository::setCategoryIcon(int categoryId, const QString& iconKey) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE categories SET icon_key = :icon WHERE id = :id");
    q.bindValue(":icon", iconKey);
    q.bindValue(":id", categoryId);
    return q.exec();
}

bool NoteRepository::toggleCategoryPinned(int categoryId, bool isPinned) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("UPDATE categories SET is_pinned = :pinned WHERE id = :id");
    q.bindValue(":pinned", isPinned ? 1 : 0);
    q.bindValue(":id", categoryId);
    return q.exec();
}

QVariantMap NoteRepository::getNoteById(int id) {
    return DatabaseManager::instance().getNoteById(id);
}

QString NoteRepository::getDatabasePath() {
    QSqlDatabase db = DatabaseManager::instance().database();
    return db.databaseName();
}

} // namespace RapidNotes
