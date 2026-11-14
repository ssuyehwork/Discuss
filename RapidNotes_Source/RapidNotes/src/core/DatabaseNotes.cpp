#include "DatabaseManager.h"
#include "DatabaseManagerInternal.h"

// 调用方已持锁并处于事务内：一批 ID 用一条 SQL 重算底色
void DatabaseManager::refreshNoteColors(const QList<int>& ids) {
    forEachIdChunk(ids, [&](const QList<int>& chunk) {
        QSqlQuery q(m_db);
        q.prepare(QString("UPDATE notes SET color = %1 WHERE id IN (%2)").arg(kNoteColorExpr, inList(chunk.size())));
        for (int id : chunk) q.addBindValue(id);
        q.exec();
    });
}

QList<QVariantMap> DatabaseManager::getNotesByIds(const QList<int>& ids) {
    QMutexLocker locker(&m_mutex);
    QList<QVariantMap> out;
    if (!m_db.isOpen() || ids.isEmpty()) return out;
    QHash<int, QVariantMap> byId;
    forEachIdChunk(ids, [&](const QList<int>& chunk) {
        QSqlQuery q(m_db);
        q.prepare("SELECT * FROM notes WHERE id IN (" + inList(chunk.size()) + ")");
        for (int id : chunk) q.addBindValue(id);
        if (!q.exec()) return;
        while (q.next()) {
            QSqlRecord rec = q.record();
            QVariantMap m;
            for (int i = 0; i < rec.count(); ++i) m[rec.fieldName(i).toLower()] = q.value(i);
            byId.insert(m.value("id").toInt(), m);
        }
    });
    out.reserve(ids.size());
    for (int id : ids) if (byId.contains(id)) out.append(byId.value(id));
    return out;
}

void DatabaseManager::addNoteAsync(const QString& title, const QString& content, const QStringList& tags,
                                  const QString& color, int categoryId,
                                  const QString& itemType, const QByteArray& dataBlob,
                                  const QString& sourceApp, const QString& sourceTitle,
                                  const QString& remark) {
    // [FIX] 彻底修复线程违规导致的闪退：
    // QSqlDatabase 的连接不是线程安全的，严禁在大规模并发采集时通过 QThreadPool 跨线程访问。
    // 如果此方法被外部线程（如剪贴板监控、后台任务）调用，必须调度到数据库所属的主线程执行。
    if (QThread::currentThread() != qApp->thread()) {
        QMetaObject::invokeMethod(this, [=]() {
            addNoteAsync(title, content, tags, color, categoryId, itemType, dataBlob, sourceApp, sourceTitle, remark);
        }, Qt::QueuedConnection);
        return;
    }

    // 此时已确保在主线程，执行同步添加逻辑
    addNote(title, content, tags, color, categoryId, itemType, dataBlob, sourceApp, sourceTitle, remark);
}

int DatabaseManager::addNote(const QString& title, const QString& content, const QStringList& tags,
                            const QString& color, int categoryId,
                            const QString& itemType, const QByteArray& dataBlob,
                            const QString& sourceApp, const QString& sourceTitle,
                            const QString& remark) {
    // 2026-04-08 按照用户要求：物理提取多后缀关联
    QString fileExtensions = extractFileExtensions(itemType, content);

    // [USER_REQUEST] 2026-04-08 网址自动归档：发现 link 类型时自动在“我的分类”中创建 Link 组
    int finalCategoryId = categoryId;
    if (RapidNotes::ItemType::fromString(itemType) == RapidNotes::ItemType::Type::Link) {
        int linkCatId = getOrCreateCategoryByName("Link", -1, "#1abc9c");
        if (linkCatId > 0) finalCategoryId = linkCatId;
    }

    // 2026-03-xx 按照用户要求：正版化移除试用限制，不再拦截新增笔记的操作
    QVariantMap newNoteMap;
    bool success = false;
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    // 不再将 dataBlob 作为哈希数据源，统一只对 content 算哈希
    QString contentHash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Sha256).toHex();
    {   
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) { qDebug() << "[DB] 错误: 数据库未打开"; return 0; }

        QString finalColor = color.isEmpty() ? "#2d2d2d" : color;
        QStringList finalTags = RapidNotes::TagCodec::normalize(tags);

        // 查重：如果内容已存在，则更新标题、标签及分类
        QSqlQuery checkQuery(m_db);
        checkQuery.prepare("SELECT id, category_id, tags FROM notes WHERE content_hash = :hash AND is_deleted = 0 LIMIT 1");
        checkQuery.bindValue(":hash", contentHash);
        if (checkQuery.exec() && checkQuery.next()) {
            int existingId = checkQuery.value(0).toInt();
            QVariant oldCatVal = checkQuery.value(1);
            
            // 获取已有笔记的详细信息，用于智能判定是否需要更新标题等
            QVariantMap existingNote = getNoteById(existingId);
            QString existingTagsStr = existingNote.value("tags").toString();
            QStringList existingTags = RapidNotes::TagCodec::split(existingTagsStr);

            // 智能合并标签
            RapidNotes::TagCodec::merge(existingTags, finalTags);

            // 漂移保护逻辑：如果笔记已有明确分类，则优先保留原分类，防止在自动归档时发生分类位移
            int finalCatToUse = finalCategoryId;
            if (!oldCatVal.isNull() && oldCatVal.toInt() > 0) {
                finalCatToUse = oldCatVal.toInt(); 
            }

            // 获取新分类/旧分类的颜色
            QString finalColor = color;
            
            if (finalCatToUse != -1) {
                QSqlQuery catQuery(m_db);
                catQuery.prepare("SELECT color, preset_tags FROM categories WHERE id = :id");
                catQuery.bindValue(":id", finalCatToUse);
                if (catQuery.exec() && catQuery.next()) {
                    if (color.isEmpty()) finalColor = catQuery.value(0).toString();
                    QString preset = catQuery.value(1).toString();
                    if (!preset.isEmpty()) {
                        QStringList pTags = RapidNotes::TagCodec::split(preset);
                        RapidNotes::TagCodec::merge(existingTags, pTags);
                    }
                }
            }

            QSqlQuery updateQuery(m_db);
            // 重复内容时，不再更新更新时间 (updated_at)，而是当作最近访问 (last_accessed_at) 来处理。
            // 2026-05-09 按照用户要求：保持主列表位置不变，仅刷新最近访问排名。
            QString sql = "UPDATE notes SET tags = :tags, last_accessed_at = :now, source_app = :app, source_title = :stitle, category_id = :cat_id, file_extensions = :exts";
            if (!finalColor.isEmpty()) sql += ", color = :color";
            
            // [CRITICAL] 智能标题保护逻辑：禁止恢复“旧版全量覆盖标题”的傻逼行为。
            // 必须确保：仅当原标题是自动生成的通用标题，且新标题更有意义时才覆盖；否则必须保持笔记原始标题不变。
            // [OPTIMIZED] 扩展通用标题判定，支持网址形式标题的自动覆盖替换
            QString existingTitle = existingNote.value("title").toString().trimmed();
            bool isExistingGeneric = existingTitle.isEmpty() || existingTitle == "无标题灵感" || 
                                     existingTitle.startsWith("[截图]") || 
                                     existingTitle.startsWith("[截图]") ||
                                     existingTitle.startsWith("Copied ") ||
                                     existingTitle.startsWith("http://") ||
                                     existingTitle.startsWith("https://") ||
                                     existingTitle.startsWith("www.");
                                     
            bool isNewMeaningful = !title.isEmpty() && !title.startsWith("[拖入") && !title.startsWith("[图片");
            
            if (isExistingGeneric && isNewMeaningful && existingTitle != title) {
                sql += ", title = :title";
            }
            
            sql += " WHERE id = :id";

            updateQuery.prepare(sql);
            updateQuery.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(existingTags));
            updateQuery.bindValue(":now", currentTime);
            updateQuery.bindValue(":app", sourceApp);
            updateQuery.bindValue(":stitle", sourceTitle);
            updateQuery.bindValue(":cat_id", finalCatToUse == -1 ? QVariant(QMetaType::fromType<int>()) : finalCatToUse);
            updateQuery.bindValue(":exts", fileExtensions);
            if (!finalColor.isEmpty()) updateQuery.bindValue(":color", finalColor);
            if (sql.contains(":title")) updateQuery.bindValue(":title", title);
            updateQuery.bindValue(":id", existingId);
            
            if (updateQuery.exec()) success = true;
            if (success) { 
                qDebug() << "[DB] 命中重复记录，已更新 ID:" << existingId;
                locker.unlock(); 
                emit noteUpdated(); 
                return existingId; 
            }
        }
        if (categoryId != -1) {
            QSqlQuery catQuery(m_db);
            catQuery.prepare("SELECT color, preset_tags FROM categories WHERE id = :id");
            catQuery.bindValue(":id", categoryId);
            if (catQuery.exec() && catQuery.next()) {
                if (color.isEmpty()) finalColor = catQuery.value(0).toString();
                QString preset = catQuery.value(1).toString();
                if (!preset.isEmpty()) {
                    QStringList pTags = RapidNotes::TagCodec::split(preset);
                    RapidNotes::TagCodec::merge(finalTags, pTags);
                }
            }
        }
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO notes (title, content, tags, color, category_id, item_type, data_blob, content_hash, created_at, updated_at, source_app, source_title, remark, file_extensions, word_count) VALUES (:title, :content, :tags, :color, :category_id, :item_type, :data_blob, :hash, :created_at, :updated_at, :source_app, :source_title, :remark, :exts, :wc)");
        query.bindValue(":title", title);
        query.bindValue(":content", content);
        
        query.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(finalTags));
        
        query.bindValue(":color", finalColor);
        query.bindValue(":category_id", finalCategoryId == -1 ? QVariant(QMetaType::fromType<int>()) : finalCategoryId);
        query.bindValue(":item_type", itemType);
        query.bindValue(":data_blob", QVariant());
        query.bindValue(":hash", contentHash);
        query.bindValue(":created_at", currentTime);
        query.bindValue(":updated_at", currentTime);
        query.bindValue(":source_app", sourceApp);
        query.bindValue(":source_title", sourceTitle);
        query.bindValue(":remark", remark);
        query.bindValue(":exts", fileExtensions);
        query.bindValue(":wc", RapidNotes::WordBucket::calculate(content, itemType));
        if (query.exec()) {
            success = true;
            markDirty();
            qDebug() << "[DB] 新纪录插入成功";
            QVariant lastId = query.lastInsertId();
            
            // 2026-05-03 按照用户要求：新创建的笔记也要记录为访问
            recordAccess(lastId.toInt());
            
            // 2026-05-01 按照用户要求：同步物理化存储唯一标签
            syncNoteTags(lastId.toInt(), finalTags);

            QSqlQuery fetch(m_db);
            fetch.prepare("SELECT * FROM notes WHERE id = :id");
            fetch.bindValue(":id", lastId);
            if (fetch.exec() && fetch.next()) {
                QSqlRecord rec = fetch.record();
                for (int i = 0; i < rec.count(); ++i) newNoteMap[rec.fieldName(i).toLower()] = fetch.value(i);
            }
        }
    }
    if (success && !newNoteMap.isEmpty()) {
        int newId = newNoteMap["id"].toInt();
        // 2026-03-xx 按照用户要求：已启用 SQLite 触发器，移除冗余的 C++ 手动 FTS 同步逻辑
        // syncFts(newId, title, content, newNoteMap["tags"].toString());
        
        // [STABILITY] 跨线程信号同步加固：
        // 如果当前不在主线程执行（由 addNoteAsync 触发），则强制通过 QueuedConnection 发送信号，防止 UI 竞态崩溃
        if (QThread::currentThread() != qApp->thread()) {
            QMetaObject::invokeMethod(this, [this, newNoteMap](){ emit noteAdded(newNoteMap); }, Qt::QueuedConnection);
        } else {
            emit noteAdded(newNoteMap);
        }
        return newId;
    }
    return 0;
}

bool DatabaseManager::updateNote(int id, const QString& title, const QString& content, const QStringList& tags, const QString& color, int categoryId,
                               const QString& itemType, const QByteArray& dataBlob,
                               const QString& sourceApp, const QString& sourceTitle,
                               const QString& remark) {
    // 2026-04-08 多后缀提取
    QString fileExtensions = extractFileExtensions(itemType, content);

    bool success = false;
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    
    // 重新计算内容哈希：统一只对 content 算哈希
    QString contentHash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Sha256).toHex();

    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        QSqlQuery query(m_db);
        
        // [CRITICAL] 锁定：更新笔记属性时必须全量同步所有元数据。严禁遗漏 hash 和 item_type。
        // 2026-05-02 同步修正：加入 word_count 显式更新
        QString sql = "UPDATE notes SET title=:title, content=:content, tags=:tags, updated_at=:updated_at, "
                      "category_id=:category_id, color=:color, last_accessed_at=:now, "
                      "content_hash=:hash, item_type=:type, data_blob=:blob, "
                      "source_app=:app, source_title=:stitle, remark=:remark, file_extensions=:exts, word_count=:wc";
        sql += " WHERE id=:id";

        query.prepare(sql);
        query.bindValue(":title", title);
        query.bindValue(":content", content);
        query.bindValue(":now", currentTime);
        query.bindValue(":hash", contentHash);
        query.bindValue(":type", itemType);
        query.bindValue(":blob", QVariant());
        query.bindValue(":app", sourceApp);
        query.bindValue(":stitle", sourceTitle);
        query.bindValue(":remark", remark);
        query.bindValue(":exts", fileExtensions);
        query.bindValue(":wc", RapidNotes::WordBucket::calculate(content, itemType));
        query.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(RapidNotes::TagCodec::normalize(tags)));
        
        query.bindValue(":updated_at", currentTime);
        query.bindValue(":category_id", categoryId == -1 ? QVariant() : categoryId);
        
        QString finalColor = color;
        if (finalColor.isEmpty()) {
            if (categoryId != -1) {
                QSqlQuery catQuery(m_db);
                catQuery.prepare("SELECT color FROM categories WHERE id = :id");
                catQuery.bindValue(":id", categoryId);
                if (catQuery.exec() && catQuery.next()) finalColor = catQuery.value(0).toString();
                else finalColor = "#0A362F";
            } else {
                finalColor = "#0A362F";
            }
        }
        query.bindValue(":color", finalColor);
        query.bindValue(":id", id);
        success = query.exec();
        
        if (success) {
            // 2026-05-01 按照用户要求：同步物理化更新唯一标签
            syncNoteTags(id, RapidNotes::TagCodec::normalize(tags));
            markDirty();
        }
    }
    if (success) { 
        // 2026-03-xx 按照用户要求：已启用 SQLite 触发器，移除冗余的 C++ 手动 FTS 同步逻辑
        
        // [STABILITY] 跨线程信号同步加固
        if (QThread::currentThread() != qApp->thread()) {
            QMetaObject::invokeMethod(this, [this](){ emit noteUpdated(); }, Qt::QueuedConnection);
        } else {
            emit noteUpdated();
        }
    }
    return success;
}

bool DatabaseManager::updateNoteState(int id, const QString& column, const QVariant& value) {
    bool success = false;
    QString title, content, tags;
    bool needsFts = false;
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        // [CRITICAL] 必须包含 item_type 以支持从图片识别提取的文字类型标记，包含 color_label 支撑纯笔记色标
        QStringList allowedColumns = {"is_pinned", "is_favorite", "is_deleted", "tags", "rating", "category_id", "color", "content", "title", "item_type", "remark", "color_label"};
        if (!allowedColumns.contains(column)) return false;
        QSqlQuery query(m_db);
        bool colorDerived = false;
        if (column == "is_favorite") {
            // [CRITICAL] 锁定：修改属性必须同步更新 last_accessed_at。严禁移除。
            query.prepare("UPDATE notes SET is_favorite = :val, updated_at = :now, last_accessed_at = :now WHERE id = :id");
            colorDerived = true;
        } else if (column == "is_deleted") {
            // [CRITICAL] 锁定：删除状态变更必须同步更新 last_accessed_at。严禁移除。
            // 不清 category_id，以支持原位恢复
            query.prepare("UPDATE notes SET is_deleted = :val, updated_at = :now, last_accessed_at = :now WHERE id = :id");
            colorDerived = true;
        } else if (column == "category_id") {
            // [CRITICAL] 锁定：移动分类必须同步更新 last_accessed_at。严禁移除。
            query.prepare("UPDATE notes SET category_id = :val, is_deleted = 0, updated_at = :now, last_accessed_at = :now WHERE id = :id");
            colorDerived = true;
        } else {
            // [CRITICAL] 锁定：通用状态修改必须同步更新 last_accessed_at。严禁移除。
            query.prepare(QString("UPDATE notes SET %1 = :val, updated_at = :now, last_accessed_at = :now WHERE id = :id").arg(column));
        }
        query.bindValue(":val", value);
        query.bindValue(":now", currentTime);
        query.bindValue(":id", id);
        success = query.exec();
        if (success && colorDerived) refreshNoteColors({id});
        if (success) markDirty();
        if (success && (column == "content" || column == "title" || column == "tags" || column == "item_type")) {
            needsFts = true;
            QSqlQuery fetch(m_db);
            fetch.prepare("SELECT title, content, tags, item_type FROM notes WHERE id = ?");
            fetch.addBindValue(id);
            if (fetch.exec() && fetch.next()) { 
                title = fetch.value(0).toString(); 
                content = fetch.value(1).toString(); 
                tags = fetch.value(2).toString();
                QString itemType = fetch.value(3).toString();

                // 2026-05-02 同步修正：若涉及内容或类型变更，同步更新 C++ 字数绑定与标签实体
                if (column == "content" || column == "item_type") {
                    QSqlQuery up(m_db);
                    up.prepare("UPDATE notes SET word_count = ? WHERE id = ?");
                    up.addBindValue(RapidNotes::WordBucket::calculate(content, itemType));
                    up.addBindValue(id);
                    up.exec();
                }
                if (column == "tags") {
                    syncNoteTags(id, RapidNotes::TagCodec::split(tags));
                }
            }
        }
    } 
    if (success) { 
        emit noteUpdated(); 
    }
    return success;
}

bool DatabaseManager::updateNoteStateBatch(const QList<int>& ids, const QString& column, const QVariant& value) {
    if (ids.isEmpty()) return true;
    bool success = false;
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        QStringList allowedColumns = {"is_pinned", "is_favorite", "is_deleted", "tags", "rating", "category_id", "color", "content", "title", "item_type", "remark", "color_label", "url"};
        if (!allowedColumns.contains(column)) return false;
        m_db.transaction();

        // 同一条 SET 子句按 500 个 ID 一批执行
        auto runIn = [&](const QString& setClause, const QVariantList& setVals) {
            forEachIdChunk(ids, [&](const QList<int>& chunk) {
                QSqlQuery q(m_db);
                q.prepare("UPDATE notes SET " + setClause + " WHERE id IN (" + inList(chunk.size()) + ")");
                for (const QVariant& v : setVals) q.addBindValue(v);
                for (int id : chunk) q.addBindValue(id);
                q.exec();
            });
        };

        if (column == "category_id") {
            runIn("category_id = ?, is_deleted = 0, updated_at = ?, last_accessed_at = ?", {value, now, now});
            refreshNoteColors(ids);
        } else if (column == "is_favorite") {
            runIn("is_favorite = ?, updated_at = ?, last_accessed_at = ?", {value.toBool() ? 1 : 0, now, now});
            refreshNoteColors(ids);
        } else if (column == "is_deleted") {
            if (value.toBool()) runIn("is_deleted = 1, is_pinned = 0, is_favorite = 0, updated_at = ?, last_accessed_at = ?", {now, now});
            else runIn("is_deleted = 0, updated_at = ?, last_accessed_at = ?", {now, now});
            refreshNoteColors(ids);
        } else if (column == "tags") {
            // 标签的唯一真相是 note_tags：整批替换为同一组标签，再统一回写 notes.tags 缓存
            const QStringList clean = RapidNotes::TagCodec::normalize(RapidNotes::TagCodec::split(value.toString()));
            ensureTagsExist(clean);
            forEachIdChunk(ids, [&](const QList<int>& chunk) {
                QSqlQuery d(m_db);
                d.prepare("DELETE FROM note_tags WHERE note_id IN (" + inList(chunk.size()) + ")");
                for (int id : chunk) d.addBindValue(id);
                d.exec();
                for (const QString& tg : clean) {
                    QSqlQuery q(m_db);
                    q.prepare("INSERT OR IGNORE INTO note_tags(note_id, tag_name) SELECT id, ? FROM notes WHERE id IN (" + inList(chunk.size()) + ")");
                    q.addBindValue(tg);
                    for (int id : chunk) q.addBindValue(id);
                    q.exec();
                }
            });
            refreshNoteTagsCache(ids);
            runIn("updated_at = ?, last_accessed_at = ?", {now, now});
        } else {
            // column 已通过白名单校验
            runIn(column + " = ?, updated_at = ?, last_accessed_at = ?", {value, now, now});
            if (column == "content" || column == "item_type") {
                QSqlQuery up(m_db);
                up.prepare("UPDATE notes SET word_count = ? WHERE id = ?");
                forEachIdChunk(ids, [&](const QList<int>& chunk) {
                    QSqlQuery fetch(m_db);
                    fetch.prepare("SELECT id, content, item_type FROM notes WHERE id IN (" + inList(chunk.size()) + ")");
                    for (int id : chunk) fetch.addBindValue(id);
                    if (!fetch.exec()) return;
                    while (fetch.next()) {
                        up.bindValue(0, RapidNotes::WordBucket::calculate(fetch.value(1).toString(), fetch.value(2).toString()));
                        up.bindValue(1, fetch.value(0).toInt());
                        up.exec();
                    }
                });
            }
        }
        success = m_db.commit();
    }
    if (success) {
        markDirty();
        emit noteUpdated();
    }
    return success;
}

bool DatabaseManager::recordAccess(int id) {
    bool success = false;
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        QSqlQuery query(m_db);
        query.prepare("UPDATE notes SET last_accessed_at = :now WHERE id = :id");
        query.bindValue(":now", currentTime);
        query.bindValue(":id", id);
        success = query.exec();
        if (success) markDirty();
    }
    if (success) emit noteUpdated(); // 发射信号通知界面刷新
    return success;
}

bool DatabaseManager::toggleNoteState(int id, const QString& column) {
    QVariant currentVal;
    {
        QMutexLocker locker(&m_mutex);
        QSqlQuery query(m_db);
        query.prepare(QString("SELECT %1 FROM notes WHERE id = :id").arg(column));
        query.bindValue(":id", id);
        if (query.exec() && query.next()) currentVal = query.value(0);
    }
    if (currentVal.isValid()) return updateNoteState(id, column, !currentVal.toBool());
    return false;
}

bool DatabaseManager::moveNotesToCategory(const QList<int>& noteIds, int catId) {
    if (noteIds.isEmpty()) return true;
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        m_db.transaction();
        QStringList presetTags;
        if (catId != -1) {
            QSqlQuery catQuery(m_db);
            catQuery.prepare("SELECT preset_tags FROM categories WHERE id = :id");
            catQuery.bindValue(":id", catId);
            if (catQuery.exec() && catQuery.next()) {
                presetTags = RapidNotes::TagCodec::normalize(RapidNotes::TagCodec::split(catQuery.value(0).toString()));
            }
        }
        const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        ensureTagsExist(presetTags);
        forEachIdChunk(noteIds, [&](const QList<int>& chunk) {
            QSqlQuery q(m_db);   // 移动分类同步更新 last_accessed_at
            q.prepare("UPDATE notes SET category_id = ?, is_deleted = 0, updated_at = ?, last_accessed_at = ? WHERE id IN (" + inList(chunk.size()) + ")");
            q.addBindValue(catId == -1 ? QVariant() : QVariant(catId));
            q.addBindValue(now); q.addBindValue(now);
            for (int id : chunk) q.addBindValue(id);
            q.exec();
            for (const QString& tg : presetTags) {   // 分类预设标签：集合化补齐
                QSqlQuery r(m_db);
                r.prepare("INSERT OR IGNORE INTO note_tags(note_id, tag_name) SELECT id, ? FROM notes WHERE id IN (" + inList(chunk.size()) + ")");
                r.addBindValue(tg);
                for (int id : chunk) r.addBindValue(id);
                r.exec();
            }
        });
        if (!presetTags.isEmpty()) refreshNoteTagsCache(noteIds);
        refreshNoteColors(noteIds);
        success = m_db.commit();
    }
    if (success) {
        markDirty();
        emit noteUpdated();
    }
    return success;
}

bool DatabaseManager::deleteNotesBatch(const QList<int>& ids) {
    if (ids.isEmpty()) return true;
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        m_db.transaction();
        forEachIdChunk(ids, [&](const QList<int>& chunk) {   // note_tags 由外键级联清理
            QSqlQuery q(m_db);
            q.prepare("DELETE FROM notes WHERE id IN (" + inList(chunk.size()) + ")");
            for (int id : chunk) q.addBindValue(id);
            q.exec();
        });
        success = m_db.commit();
    }
    if (success) {
        markDirty();
        ClipboardMonitor::instance().clearLastHash();
        emit noteUpdated();
    }
    return success;
}

bool DatabaseManager::softDeleteNotes(const QList<int>& ids) {
    if (ids.isEmpty()) return true;
    bool success = false;
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        m_db.transaction();
        forEachIdChunk(ids, [&](const QList<int>& chunk) {   // 不清 category_id，便于原位恢复
            QSqlQuery q(m_db);
            q.prepare("UPDATE notes SET is_deleted = 1, is_pinned = 0, is_favorite = 0, updated_at = ?, last_accessed_at = ? WHERE id IN (" + inList(chunk.size()) + ")");
            q.addBindValue(now); q.addBindValue(now);
            for (int id : chunk) q.addBindValue(id);
            q.exec();
        });
        refreshNoteColors(ids);
        success = m_db.commit();
    }
    if (success) {
        markDirty();
        ClipboardMonitor::instance().clearLastHash();
        emit noteUpdated();
    }
    return success;
}

bool DatabaseManager::moveNote(int id, DatabaseManager::MoveDirection direction, const QString& filterType, const QVariant& filterValue, const QVariantMap& criteria) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;

    // 1. 获取当前视图下的所有笔记 ID (按当前排序)
    QString baseSql = "SELECT id FROM notes ";
    QString whereClause;
    QVariantList params;
    applyCommonFilters(whereClause, params, filterType, filterValue, criteria);
    
    // 排除置顶干扰，只在同类（置顶或非置顶）内移动，或者简单处理：按当前最终显示顺序获取列表
    QString finalSql = baseSql + whereClause;
    if (filterType == "recently_visited") finalSql += " ORDER BY last_accessed_at DESC";
    else finalSql += " ORDER BY is_pinned DESC, sort_order ASC, updated_at DESC";
    
    QSqlQuery query(m_db);
    query.prepare(finalSql);
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    
    QList<int> ids;
    if (query.exec()) {
        while (query.next()) ids << query.value(0).toInt();
    } else return false;

    int currentIndex = ids.indexOf(id);
    if (currentIndex == -1) return false;

    // 2. 调整位置
    switch (direction) {
        case Up: 
            if (currentIndex > 0) std::swap(ids[currentIndex], ids[currentIndex - 1]); 
            else return false; 
            break;
        case Down: 
            if (currentIndex < ids.size() - 1) std::swap(ids[currentIndex], ids[currentIndex + 1]); 
            else return false; 
            break;
        case Top: 
            if (currentIndex > 0) { ids.removeAt(currentIndex); ids.prepend(id); } 
            else return false; 
            break;
        case Bottom: 
            if (currentIndex < ids.size() - 1) { ids.removeAt(currentIndex); ids.append(id); } 
            else return false; 
            break;
    }

    // 3. 批量更新 sort_order
    m_db.transaction();
    QSqlQuery update(m_db);
    for (int i = 0; i < ids.size(); ++i) {
        update.prepare("UPDATE notes SET sort_order = :val WHERE id = :id");
        update.bindValue(":val", i);
        update.bindValue(":id", ids[i]);
        update.exec();
    }
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit noteUpdated(); }
    return ok;
}

bool DatabaseManager::moveNotesToRow(const QList<int>& idsToMove, int targetRow, const QString& filterType, const QVariant& filterValue, const QVariantMap& criteria) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;

    // 1. 获取当前视图下的完整 ID 列表 (按当前排序)
    QString baseSql = "SELECT id FROM notes ";
    QString whereClause;
    QVariantList params;
    applyCommonFilters(whereClause, params, filterType, filterValue, criteria);
    
    QString finalSql = baseSql + whereClause;
    if (filterType == "recently_visited") finalSql += " ORDER BY last_accessed_at DESC";
    else if (filterType == "trash") finalSql += " ORDER BY updated_at DESC";
    else finalSql += " ORDER BY is_pinned DESC, sort_order ASC, updated_at DESC";
    
    QSqlQuery query(m_db);
    query.prepare(finalSql);
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    
    QList<int> fullList;
    if (query.exec()) {
        while (query.next()) fullList << query.value(0).toInt();
    } else return false;

    // 2. 执行逻辑上的移动
    // 先移除要移动的项
    for (int id : idsToMove) {
        fullList.removeAll(id);
    }
    
    // 在目标位置重新插入 (注意范围限制)
    int actualTarget = qBound(0, targetRow, fullList.size());
    for (int i = 0; i < idsToMove.size(); ++i) {
        fullList.insert(actualTarget + i, idsToMove[i]);
    }

    // 3. 批量更新 sort_order
    m_db.transaction();
    QSqlQuery update(m_db);
    for (int i = 0; i < fullList.size(); ++i) {
        update.prepare("UPDATE notes SET sort_order = :val WHERE id = :id");
        update.bindValue(":val", i);
        update.bindValue(":id", fullList[i]);
        update.exec();
    }
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit noteUpdated(); }
    return ok;
}

bool DatabaseManager::reorderNotes(const QString& filterType, const QVariant& filterValue, bool ascending, const QVariantMap& criteria) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;

    // 2026-05-04 按照用户要求：支持按更新时间排序
    QString sortBy = criteria.value("sort_by", "title").toString();
    QString sortOrder = criteria.value("sort_order", ascending ? "ASC" : "DESC").toString();
    
    QString baseSql;
    if (sortBy == "updated_at") {
        baseSql = "SELECT id, title, updated_at FROM notes ";
    } else {
        baseSql = "SELECT id, title FROM notes ";
    }
    
    QString whereClause;
    QVariantList params;
    applyCommonFilters(whereClause, params, filterType, filterValue, criteria);
    
    QSqlQuery query(m_db);
    query.prepare(baseSql + whereClause);
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    
    struct NoteSortInfo { 
        int id; 
        QString title; 
        QDateTime updatedAt; 
    };
    QList<NoteSortInfo> list;
    if (query.exec()) {
        while (query.next()) {
            if (sortBy == "updated_at") {
                list.append({query.value(0).toInt(), query.value(1).toString(), query.value(2).toDateTime()});
            } else {
                list.append({query.value(0).toInt(), query.value(1).toString(), QDateTime()});
            }
        }
    } else return false;

    if (list.isEmpty()) return true;

    if (sortBy == "updated_at") {
        std::sort(list.begin(), list.end(), [&sortOrder](const NoteSortInfo& a, const NoteSortInfo& b) {
            if (sortOrder == "DESC") return a.updatedAt > b.updatedAt;
            return a.updatedAt < b.updatedAt;
        });
    } else {
        std::sort(list.begin(), list.end(), [ascending](const NoteSortInfo& a, const NoteSortInfo& b) {
            if (ascending) return a.title.localeAwareCompare(b.title) < 0;
            return a.title.localeAwareCompare(b.title) > 0;
        });
    }

    m_db.transaction();
    QSqlQuery update(m_db);
    for (int i = 0; i < list.size(); ++i) {
        update.prepare("UPDATE notes SET sort_order = :val WHERE id = :id");
        update.bindValue(":val", i);
        update.bindValue(":id", list[i].id);
        update.exec();
    }
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit noteUpdated(); }
    return ok;
}

bool DatabaseManager::emptyTrash() {
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        m_db.transaction();
        
        QSqlQuery query(m_db);
        // 1. 物理删除笔记
        query.exec("DELETE FROM notes WHERE is_deleted = 1");
        
        // 2. 物理删除分类
        query.exec("DELETE FROM categories WHERE is_deleted = 1");
        
        success = m_db.commit();
    }
    if (success) { markDirty(); emit noteUpdated(); }
    return success;
}

QVariantMap DatabaseManager::getNoteById(int id) {
    QMutexLocker locker(&m_mutex);
    QVariantMap map;
    if (!m_db.isOpen()) return map;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM notes WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next()) {
        QSqlRecord rec = query.record();
        for (int i = 0; i < rec.count(); ++i) {
            map[rec.fieldName(i).toLower()] = query.value(i);
        }
    }
    return map;
}

int DatabaseManager::getLastCreatedNoteId() {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return 0;
    QSqlQuery query(m_db);
    // [USER_REQUEST] 核心修复：直接通过 ID 倒序获取最后创建的一条非删除笔记，彻底杜绝排序干扰
    if (query.exec("SELECT id FROM notes WHERE is_deleted = 0 ORDER BY id DESC LIMIT 1")) {
        if (query.next()) return query.value(0).toInt();
    }
    return 0;
}

bool DatabaseManager::recordAccessBatch(const QList<int>& ids) {
    if (ids.isEmpty()) return true;
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    m_db.transaction();
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    forEachIdChunk(ids, [&](const QList<int>& chunk) {
        QSqlQuery q(m_db);
        q.prepare("UPDATE notes SET last_accessed_at = ? WHERE id IN (" + inList(chunk.size()) + ")");
        q.addBindValue(now);
        for (int id : chunk) q.addBindValue(id);
        q.exec();
    });
    return m_db.commit();
}

bool DatabaseManager::toggleNoteFavoriteBatch(const QList<int>& ids) {
    if (ids.isEmpty()) return true;
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    m_db.transaction();
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    forEachIdChunk(ids, [&](const QList<int>& chunk) {   // 每行按自己当前状态翻转，一条 SQL 搞定一批
        QSqlQuery q(m_db);
        q.prepare("UPDATE notes SET is_favorite = CASE WHEN is_favorite = 1 THEN 0 ELSE 1 END, updated_at = ?, last_accessed_at = ? "
                  "WHERE id IN (" + inList(chunk.size()) + ")");
        q.addBindValue(now); q.addBindValue(now);
        for (int id : chunk) q.addBindValue(id);
        q.exec();
    });
    refreshNoteColors(ids);
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit noteUpdated(); }
    return ok;
}
