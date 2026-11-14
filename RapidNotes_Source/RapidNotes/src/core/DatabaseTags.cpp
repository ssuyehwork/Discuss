#include "DatabaseManager.h"
#include "DatabaseManagerInternal.h"

void DatabaseManager::ensureTagsExist(const QStringList& names) {
    if (names.isEmpty()) return;
    QSqlQuery q(m_db);
    q.prepare("INSERT OR IGNORE INTO tags(name) VALUES(?)");
    for (const QString& n : names) { q.bindValue(0, n); q.exec(); }
}

// 由 note_tags 重新生成 notes.tags 缓存：按批 IN 读取，一次事务内写回（调用方负责事务）
void DatabaseManager::refreshNoteTagsCache(const QList<int>& ids) {
    if (ids.isEmpty()) return;
    QSqlQuery up(m_db);
    up.prepare("UPDATE notes SET tags = ? WHERE id = ?");
    forEachIdChunk(ids, [&](const QList<int>& chunk) {
        QHash<int, QStringList> map;
        for (int id : chunk) map[id];
        QSqlQuery sel(m_db);
        sel.prepare("SELECT note_id, tag_name FROM note_tags WHERE note_id IN (" + inList(chunk.size()) + ") ORDER BY rowid");
        for (int id : chunk) sel.addBindValue(id);
        if (sel.exec()) while (sel.next()) map[sel.value(0).toInt()].append(sel.value(1).toString());
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            up.bindValue(0, RapidNotes::TagCodec::join(it.value()));
            up.bindValue(1, it.key());
            up.exec();
        }
    });
}

QStringList DatabaseManager::getAllTags() {
    QMutexLocker locker(&m_mutex);
    QStringList allTags;
    if (!m_db.isOpen()) return allTags;
    QSqlQuery query(m_db);
    if (query.exec("SELECT DISTINCT nt.tag_name FROM note_tags nt JOIN notes n ON n.id = nt.note_id "
                   "WHERE n.is_deleted = 0 ORDER BY nt.tag_name")) {
        while (query.next()) allTags.append(query.value(0).toString());
    }
    return allTags;
}

QList<QVariantMap> DatabaseManager::getRecentTagsWithCounts(int limit) {
    QMutexLocker locker(&m_mutex);
    QList<QVariantMap> results;
    if (!m_db.isOpen()) return results;
    QSqlQuery query(m_db);
    query.prepare("SELECT nt.tag_name, COUNT(*) AS c, MAX(n.updated_at) AS lu FROM note_tags nt "
                  "JOIN notes n ON n.id = nt.note_id WHERE n.is_deleted = 0 "
                  "GROUP BY nt.tag_name ORDER BY lu DESC, nt.tag_name ASC LIMIT ?");
    query.addBindValue(limit);
    if (query.exec()) {
        while (query.next()) {
            QVariantMap m; m["name"] = query.value(0).toString(); m["count"] = query.value(1).toInt();
            results.append(m);
        }
    }
    return results;
}

bool DatabaseManager::addTagsToNote(int noteId, const QStringList& tags) {
    return addTagsToNotesBatch({noteId}, tags);
}

bool DatabaseManager::addTagsToNotesBatch(const QList<int>& noteIds, const QStringList& tags) {
    if (noteIds.isEmpty() || tags.isEmpty()) return true;
    const QStringList norm = RapidNotes::TagCodec::normalize(tags);
    if (norm.isEmpty()) return true;
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    m_db.transaction();
    ensureTagsExist(norm);
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    forEachIdChunk(noteIds, [&](const QList<int>& chunk) {
        for (const QString& tg : norm) {
            QSqlQuery q(m_db);
            q.prepare("INSERT OR IGNORE INTO note_tags(note_id, tag_name) SELECT id, ? FROM notes WHERE id IN (" + inList(chunk.size()) + ")");
            q.addBindValue(tg);
            for (int id : chunk) q.addBindValue(id);
            q.exec();
        }
        QSqlQuery u(m_db);
        u.prepare("UPDATE notes SET updated_at = ? WHERE id IN (" + inList(chunk.size()) + ")");
        u.addBindValue(now);
        for (int id : chunk) u.addBindValue(id);
        u.exec();
    });
    refreshNoteTagsCache(noteIds);
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit noteUpdated(); }
    return ok;
}

bool DatabaseManager::removeTagsFromNotesBatch(const QList<int>& noteIds, const QStringList& tags) {
    if (noteIds.isEmpty() || tags.isEmpty()) return true;
    const QStringList norm = RapidNotes::TagCodec::normalize(tags);
    if (norm.isEmpty()) return true;
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    m_db.transaction();
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    forEachIdChunk(noteIds, [&](const QList<int>& chunk) {
        QSqlQuery q(m_db);
        q.prepare("DELETE FROM note_tags WHERE note_id IN (" + inList(chunk.size()) + ") AND tag_name IN (" + inList(norm.size()) + ")");
        for (int id : chunk) q.addBindValue(id);
        for (const QString& tg : norm) q.addBindValue(tg);
        q.exec();
        QSqlQuery u(m_db);
        u.prepare("UPDATE notes SET updated_at = ? WHERE id IN (" + inList(chunk.size()) + ")");
        u.addBindValue(now);
        for (int id : chunk) u.addBindValue(id);
        u.exec();
    });
    refreshNoteTagsCache(noteIds);
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit noteUpdated(); }
    return ok;
}

bool DatabaseManager::renameTagGlobally(const QString& oldName, const QString& newName) {
    const QString o = oldName.trimmed(), n = newName.trimmed();
    if (o.isEmpty() || n.isEmpty() || o == n) return true;
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    m_db.transaction();
    QList<int> ids;
    {
        QSqlQuery s(m_db);
        s.prepare("SELECT DISTINCT note_id FROM note_tags WHERE tag_name IN (?, ?)");
        s.addBindValue(o); s.addBindValue(n);
        if (s.exec()) while (s.next()) ids << s.value(0).toInt();
    }
    if (o.compare(n, Qt::CaseInsensitive) == 0) {
        QSqlQuery u(m_db);                       // 仅大小写变化：直接改字典，关联表由外键级联
        u.prepare("UPDATE tags SET name = ? WHERE name = ?");
        u.addBindValue(n); u.addBindValue(o); u.exec();
    } else {
        ensureTagsExist({n});
        QSqlQuery mv(m_db);
        mv.prepare("INSERT OR IGNORE INTO note_tags(note_id, tag_name) SELECT note_id, ? FROM note_tags WHERE tag_name = ?");
        mv.addBindValue(n); mv.addBindValue(o); mv.exec();
        QSqlQuery d1(m_db); d1.prepare("DELETE FROM note_tags WHERE tag_name = ?"); d1.addBindValue(o); d1.exec();
        QSqlQuery d2(m_db); d2.prepare("DELETE FROM tags WHERE name = ?"); d2.addBindValue(o); d2.exec();
    }
    refreshNoteTagsCache(ids);
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit noteUpdated(); }
    return ok;
}

bool DatabaseManager::deleteTagGlobally(const QString& tagName) {
    const QString tg = tagName.trimmed();
    if (tg.isEmpty()) return true;
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    m_db.transaction();
    QList<int> ids;
    {
        QSqlQuery s(m_db);
        s.prepare("SELECT DISTINCT note_id FROM note_tags WHERE tag_name = ?");
        s.addBindValue(tg);
        if (s.exec()) while (s.next()) ids << s.value(0).toInt();
    }
    QSqlQuery d1(m_db); d1.prepare("DELETE FROM note_tags WHERE tag_name = ?"); d1.addBindValue(tg); d1.exec();
    QSqlQuery d2(m_db); d2.prepare("DELETE FROM tags WHERE name = ?"); d2.addBindValue(tg); d2.exec();
    refreshNoteTagsCache(ids);
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit noteUpdated(); }
    return ok;
}

void DatabaseManager::syncNoteTags(int noteId, const QStringList& tags) {
    if (noteId <= 0) return;
    const QStringList clean = RapidNotes::TagCodec::normalize(tags);
    QSqlQuery del(m_db);
    del.prepare("DELETE FROM note_tags WHERE note_id = ?");
    del.addBindValue(noteId); del.exec();
    ensureTagsExist(clean);
    QSqlQuery rel(m_db);
    rel.prepare("INSERT OR IGNORE INTO note_tags(note_id, tag_name) VALUES(?, ?)");
    for (const QString& n : clean) { rel.bindValue(0, noteId); rel.bindValue(1, n); rel.exec(); }
    QSqlQuery up(m_db);   // 同步回写 notes.tags 缓存（统一用纯逗号分隔）
    up.prepare("UPDATE notes SET tags = ? WHERE id = ?");
    up.addBindValue(RapidNotes::TagCodec::join(clean)); up.addBindValue(noteId); up.exec();
}
