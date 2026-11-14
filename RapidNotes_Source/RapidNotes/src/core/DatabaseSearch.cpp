#include "DatabaseManager.h"
#include "DatabaseManagerInternal.h"

// 关键词按空白拆分，每个词都必须命中。≥3 字符走 FTS 索引（含正文）；更短的词用 LIKE 搜短字段。
void DatabaseManager::appendKeywordFilter(QString& whereClause, QVariantList& params, const QString& keyword) {
    const QStringList words = keyword.split(QRegularExpression(QStringLiteral(R"(\s+)")), Qt::SkipEmptyParts);
    for (const QString& kw : words) {
        if (m_ftsAvailable && kw.size() >= 3) {
            QString phrase = kw;
            phrase.replace('"', "\"\"");
            whereClause += "AND id IN (SELECT rowid FROM notes_search WHERE notes_search MATCH ?) ";
            params << ("\"" + phrase + "\"");
        } else {
            whereClause += "AND (title LIKE ? ESCAPE '\\' OR remark LIKE ? ESCAPE '\\' OR url LIKE ? ESCAPE '\\' OR tags LIKE ? ESCAPE '\\' OR file_extensions LIKE ? ESCAPE '\\') ";
            QString esc = kw;
            esc.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_");
            const QString pattern = "%" + esc + "%";
            params << pattern << pattern << pattern << pattern << pattern;
        }
    }
}

// [CRITICAL] 核心搜索逻辑：采用 FTS5 全文检索。禁止修改此处的 MATCH 语法及字段关联，以确保搜索结果的准确性与高性能。
QList<QVariantMap> DatabaseManager::searchNotes(const QString& keyword, const QString& filterType, const QVariant& filterValue, int page, int pageSize, const QVariantMap& criteria) {
    QMutexLocker locker(&m_mutex);
    QList<QVariantMap> results;
    if (!m_db.isOpen()) {
        qCritical() << "[DB] searchNotes 失败：数据库未打开";
        return results;
    }

    // [NEW] 处理回收站特殊视图：包含已删除的分类
    if (filterType == "trash" && keyword.isEmpty()) {
        // [OLD_VERSION_RECOVERY] 100% 还原旧版字段 SQL 结构，杜绝字段缺失报错
        QString sql = "SELECT id, title, content, tags, color, category_id, item_type, data_blob, created_at, updated_at, is_pinned, is_favorite, is_deleted, source_app, source_title, last_accessed_at, remark "
                      "FROM notes WHERE is_deleted = 1 "
                      "UNION ALL "
                      "SELECT id, name AS title, '(已删除的分类包)' AS content, '' AS tags, color, parent_id AS category_id, 'deleted_category' AS item_type, NULL AS data_blob, NULL AS created_at, NULL AS updated_at, 0 AS is_pinned, 0 AS is_favorite, 1 AS is_deleted, '' AS source_app, '' AS source_title, NULL AS last_accessed_at, '' AS remark "
                      "FROM categories WHERE is_deleted = 1 "
                      "ORDER BY is_pinned DESC, updated_at DESC";
        
        QSqlQuery query(m_db);
        if (query.exec(sql)) {
            while (query.next()) {
                QVariantMap map;
                QSqlRecord rec = query.record();
                for (int i = 0; i < rec.count(); ++i) map[rec.fieldName(i)] = query.value(i);
                results.append(map);
            }
        } else {
            qCritical() << "[DB] searchNotes(trash) failed:" << query.lastError().text() << "SQL:" << sql;
        }
        return results;
    }

    QString baseSql = "SELECT notes.* FROM notes ";
    QString whereClause;
    QVariantList params;
    applyCommonFilters(whereClause, params, filterType, filterValue, criteria);
    
    appendKeywordFilter(whereClause, params, keyword);
    
    QString finalSql = baseSql + whereClause + "ORDER BY ";
    
    // 检查自定义排序条件 - 优先级最高
    if (criteria.contains("sort_by") && criteria.contains("sort_order")) {
        QString sortBy = criteria.value("sort_by").toString();
        QString sortOrder = criteria.value("sort_order").toString();
        
        // 2026-05-04 修复：检查是否为RecentNotesWindow调用（通过criteria中的特殊标记）
        bool isRecentNotesWindow = criteria.value("recent_notes_window", false).toBool();
        
        if (isRecentNotesWindow) {
            // RecentNotesWindow专用：完全无视置顶逻辑
            finalSql += sortBy + " " + sortOrder;
            qDebug() << "[DB] 使用RecentNotesWindow排序（完全无视置顶）:" << sortBy << sortOrder;
        } else if (filterType == "all" || (filterType != "recently_visited" && filterType != "category")) {
            // "全部数据"和其他非分类视图必须保持置顶逻辑
            finalSql += "is_pinned DESC, " + sortBy + " " + sortOrder;
            qDebug() << "[DB] 使用自定义排序（保持置顶）:" << sortBy << sortOrder;
        } else {
            // "最近访问"可以无视置顶逻辑
            finalSql += sortBy + " " + sortOrder;
            qDebug() << "[DB] 使用自定义排序（无视置顶）:" << sortBy << sortOrder;
        }
    } else if (filterType == "recently_visited") {
        // 2026-05-04 锁定：最近访问只能按访问时间排序，不允许任何干扰
        finalSql += "last_accessed_at DESC";
        qDebug() << "[DB] 使用最近访问排序（锁定为 last_accessed_at DESC）";
    } else if (filterType == "category" && filterValue.toInt() > 0) {
        // 2026-05-04 按照用户要求修复：支持分类独立排序模式，必须遵循置顶逻辑
        QString mode = getCategorySortMode(filterValue.toInt());
        if (mode == "created_at_desc") {
            finalSql += "is_pinned DESC, created_at DESC";
        } else if (mode == "updated_at_desc") {
            finalSql += "is_pinned DESC, updated_at DESC";
        } else if (mode == "title_asc") {
            finalSql += "is_pinned DESC, title ASC";
        } else if (mode == "title_desc") {
            finalSql += "is_pinned DESC, title DESC";
        } else {
            // manual 或其他未知模式
            finalSql += "is_pinned DESC, sort_order ASC, updated_at DESC";
        }
        qDebug() << "[DB] 使用分类排序:" << mode;
    } else {
        // [CRITICAL] 锁定：所有视图严格遵循 置顶 > 排序值 > 更新时间 的排序准则，拒绝任何 rank 脑补。
        finalSql += "is_pinned DESC, sort_order ASC, updated_at DESC";
        qDebug() << "[DB] 使用默认排序";
    }
    
    qDebug() << "[DB] 最终SQL:" << finalSql;
    
    if (page > 0) finalSql += QString(" LIMIT %1 OFFSET %2").arg(pageSize).arg((page - 1) * pageSize);
    
    QSqlQuery query(m_db);
    query.prepare(finalSql);
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    
    if (query.exec()) { 
        while (query.next()) { 
            QVariantMap map; 
            QSqlRecord rec = query.record(); 
            for (int i = 0; i < rec.count(); ++i) map[rec.fieldName(i)] = query.value(i); 
            results.append(map); 
        } 
    }
    else qCritical() << "searchNotes failed:" << query.lastError().text();
    return results;
}

// [CRITICAL] 核心计数逻辑：必须与 searchNotes 的过滤条件保持 1:1 同步，禁止擅自改动。
QList<QVariantMap> DatabaseManager::searchNotesSummary(const QString& keyword, const QString& filterType, const QVariant& filterValue, int page, int pageSize, const QVariantMap& criteria) {
    QMutexLocker locker(&m_mutex);
    QList<QVariantMap> results;
    if (!m_db.isOpen()) {
        qCritical() << "[DB] searchNotesSummary 失败：数据库未打开";
        return results;
    }

    if (filterType == "trash" && keyword.isEmpty()) {
        QString sql = "SELECT id, title, substr(content, 1, 200) AS summary, tags, color, color_label, category_id, item_type, created_at, updated_at, is_pinned, is_favorite, is_deleted, rating, remark "
                      "FROM notes WHERE is_deleted = 1 "
                      "UNION ALL "
                      "SELECT id, name AS title, '(已删除的分类)' AS summary, '' AS tags, color, '' AS color_label, parent_id AS category_id, 'deleted_category' AS item_type, NULL AS created_at, NULL AS updated_at, 0 AS is_pinned, 0 AS is_favorite, 1 AS is_deleted, 0 AS rating, '' AS remark "
                      "FROM categories WHERE is_deleted = 1 "
                      "ORDER BY is_pinned DESC, updated_at DESC";
        QSqlQuery query(m_db);
        if (query.exec(sql)) {
            while (query.next()) {
                QVariantMap map;
                QSqlRecord rec = query.record();
                for (int i = 0; i < rec.count(); ++i) map[rec.fieldName(i)] = query.value(i);
                results.append(map);
            }
        }
        return results;
    }

    QString baseSql = "SELECT id, title, substr(content, 1, 200) AS summary, tags, color, color_label, category_id, item_type, rating, is_pinned, is_favorite, is_deleted, created_at, updated_at, remark, file_extensions FROM notes ";
    QString whereClause;
    QVariantList params;
    applyCommonFilters(whereClause, params, filterType, filterValue, criteria);

    appendKeywordFilter(whereClause, params, keyword);

    QString finalSql = baseSql + whereClause;

    // 补充排序与分页口径
    finalSql += " ORDER BY is_pinned DESC, sort_order ASC, updated_at DESC";
    if (page > 0) {
        finalSql += QString(" LIMIT %1 OFFSET %2").arg(pageSize).arg((page - 1) * pageSize);
    }

    QSqlQuery query(m_db);
    query.prepare(finalSql);
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap map;
            QSqlRecord rec = query.record();
            for (int i = 0; i < rec.count(); ++i) map[rec.fieldName(i)] = query.value(i);
            results.append(map);
        }
    } else {
        qCritical() << "searchNotesSummary failed:" << query.lastError().text() << "SQL:" << finalSql;
    }
    return results;
}

int DatabaseManager::getNotesCount(const QString& keyword, const QString& filterType, const QVariant& filterValue, const QVariantMap& criteria) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return 0;

    if (filterType == "trash") {
        // [OLD_VERSION_RECOVERY] 回归旧版简单计数逻辑
        int trashNotes = 0;
        QSqlQuery nQuery(m_db);
        nQuery.prepare("SELECT COUNT(*) FROM notes WHERE is_deleted = 1");
        if (nQuery.exec() && nQuery.next()) trashNotes = nQuery.value(0).toInt();

        int trashCats = 0;
        QSqlQuery cQuery(m_db);
        cQuery.prepare("SELECT COUNT(*) FROM categories WHERE is_deleted = 1");
        if (cQuery.exec() && cQuery.next()) trashCats = cQuery.value(0).toInt();

        return trashNotes + trashCats;
    }

    QString baseSql = "SELECT COUNT(*) FROM notes ";
    QString whereClause;
    QVariantList params;
    applyCommonFilters(whereClause, params, filterType, filterValue, criteria);
    
    appendKeywordFilter(whereClause, params, keyword);
    
    QSqlQuery query(m_db);
    query.prepare(baseSql + whereClause);
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    if (query.exec()) { if (query.next()) return query.value(0).toInt(); }
    else qCritical() << "getNotesCount failed:" << query.lastError().text();
    return 0;
}

QVariantMap DatabaseManager::getCounts() {
    QMutexLocker locker(&m_mutex);
    QVariantMap counts;
    if (!m_db.isOpen()) return counts;
    QSqlQuery query(m_db);
    auto getCount = [&](const QString& where, bool applySecurity = true) {
        QString sql = "SELECT COUNT(*) FROM notes WHERE " + where;
        QVariantList params;
        if (applySecurity) { QString securityClause; applySecurityFilter(securityClause, params, "all"); sql += " " + securityClause; }
        QSqlQuery q(m_db);
        q.prepare(sql);
        for(int i=0; i<params.size(); ++i) q.bindValue(i, params[i]);
        if (q.exec()) { if (q.next()) return q.value(0).toInt(); }
        return 0;
    };
    QString todayStr = QDate::currentDate().toString("yyyy-MM-dd");
    QString yesterdayStr = QDate::currentDate().addDays(-1).toString("yyyy-MM-dd");

    counts["all"] = getCount("is_deleted = 0");
    counts["today"] = getCount(QString("is_deleted = 0 AND date(created_at) = '%1'").arg(todayStr));
    counts["yesterday"] = getCount(QString("is_deleted = 0 AND date(created_at) = '%1'").arg(yesterdayStr));
    counts["recently_visited"] = getCount(QString("is_deleted = 0 AND (date(last_accessed_at) >= '%1' OR date(created_at) = '%2')").arg(QDate::currentDate().addDays(-7).toString("yyyy-MM-dd")).arg(QDate::currentDate().toString("yyyy-MM-dd")));
    // 2026-03-xx 按照用户要求修复傻逼逻辑：统一“未分类”判定口径，兼容 NULL 和 -1（分类物理删除后的残留）
    counts["uncategorized"] = getCount("is_deleted = 0 AND (category_id IS NULL OR category_id <= 0)");
    counts["untagged"] = getCount("is_deleted = 0 AND (tags IS NULL OR tags = '')");
    counts["bookmark"] = getCount("is_deleted = 0 AND is_favorite = 1");
    
    // [MODIFIED] 统一回收站统计口径：包含已删除笔记 + 已删除分类包
    int trashNotes = getCount("is_deleted = 1", false);
    int trashCats = 0;
    QSqlQuery catTrashQuery(m_db);
    if (catTrashQuery.exec("SELECT COUNT(*) FROM categories WHERE is_deleted = 1")) {
        if (catTrashQuery.next()) trashCats = catTrashQuery.value(0).toInt();
    }
    counts["trash"] = trashNotes + trashCats;

    // [MODIFIED] 按照用户要求切换为“物理隔离计数”逻辑：分类仅统计直接归属于自身的笔记，不再递归累加子分类。
    QMap<int, int> directCounts;
    if (query.exec("SELECT category_id, COUNT(*) FROM notes WHERE is_deleted = 0 AND category_id IS NOT NULL GROUP BY category_id")) {
        while (query.next()) {
            directCounts[query.value(0).toInt()] = query.value(1).toInt();
        }
    }

    for (auto it = directCounts.begin(); it != directCounts.end(); ++it) {
        counts["cat_" + QString::number(it.key())] = it.value();
    }

    return counts;
}

// [CRITICAL] 核心统计逻辑：2026-04-09 按照用户要求回归 LIKE 逻辑。
QVariantMap DatabaseManager::getFilterStats(const QString& keyword, const QString& filterType, const QVariant& filterValue, const QVariantMap& criteria) {
    QMutexLocker locker(&m_mutex);
    QVariantMap stats;
    if (!m_db.isOpen()) return stats;

    QString baseSql = "FROM notes ";
    QString whereClause;
    QVariantList params;
    applyCommonFilters(whereClause, params, filterType, filterValue, criteria);
    
    appendKeywordFilter(whereClause, params, keyword);

    QSqlQuery query(m_db);

    // 1.1 收藏统计 (2026-05-03 [NEW] 按照用户要求：新增收藏状态聚合)
    QMap<int, int> favorites;
    query.prepare("SELECT is_favorite, COUNT(*) " + baseSql + whereClause + " GROUP BY is_favorite");
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    if (query.exec()) { while (query.next()) favorites[query.value(0).toInt()] = query.value(1).toInt(); }
    QVariantMap favMap;
    for (auto it = favorites.begin(); it != favorites.end(); ++it) favMap[QString::number(it.key())] = it.value();
    stats["favorite"] = favMap;

    QMap<int, int> stars;
    query.prepare("SELECT rating, COUNT(*) " + baseSql + whereClause + " GROUP BY rating");
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    if (query.exec()) { while (query.next()) stars[query.value(0).toInt()] = query.value(1).toInt(); }
    QVariantMap starsMap;
    for (auto it = stars.begin(); it != stars.end(); ++it) starsMap[QString::number(it.key())] = it.value();
    stats["stars"] = starsMap;

    // 1.5 阶梯字数聚合统计 (2026-05-01 按照用户要求：直接读取绑定字段，实现高性能归类)
    QString wcQuerySql = "SELECT word_count, COUNT(*) " + baseSql + whereClause + " GROUP BY word_count";
    QSqlQuery wcQuery(m_db);
    wcQuery.prepare(wcQuerySql);
    for (int i = 0; i < params.size(); ++i) wcQuery.bindValue(i, params[i]);
    
    QVariantMap wcMap;
    if (wcQuery.exec()) {
        while (wcQuery.next()) {
            int wc = wcQuery.value(0).toInt();
            // 物理过滤：仅在 Map 中保留有效的字数阶梯
            wcMap[QString::number(wc)] = wcQuery.value(1).toInt();
        }
    }
    stats["word_count"] = wcMap;

    QMap<QString, int> colors;
    query.prepare("SELECT color_label, COUNT(*) " + baseSql + whereClause + " AND color_label != '' GROUP BY color_label");
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    if (query.exec()) { while (query.next()) colors[query.value(0).toString()] = query.value(1).toInt(); }
    QVariantMap colorsMap;
    for (auto it = colors.begin(); it != colors.end(); ++it) colorsMap[it.key()] = it.value();
    stats["colors"] = colorsMap;

    // 2.5 物理级多后缀统计 (2026-04-08 按照用户要求：多对多关联统计)
    QMap<QString, int> bizTypes;
    QSqlQuery typeQuery(m_db);
    // [PERFORMANCE] 直接扫描 file_extensions 字段，实现精准的后缀名聚合
    typeQuery.prepare("SELECT item_type, file_extensions " + baseSql + whereClause);
    for (int i = 0; i < params.size(); ++i) typeQuery.bindValue(i, params[i]);
    if (typeQuery.exec()) {
        while (typeQuery.next()) {
            QString itemType = typeQuery.value(0).toString();
            QString exts = typeQuery.value(1).toString();
            if (!exts.isEmpty()) {
                QStringList parts = RapidNotes::TagCodec::split(exts);
                for (const QString& e : parts) bizTypes[e.trimmed()]++;
            } else {
                // 回退逻辑：处理非文件类的语义化类型
                RapidNotes::ItemType::Type type = RapidNotes::ItemType::fromString(itemType);
                if (type == RapidNotes::ItemType::Type::Image) bizTypes["截图"]++;
                else if (type == RapidNotes::ItemType::Type::Code) bizTypes["脚本代码"]++;
                else if (type == RapidNotes::ItemType::Type::Text) bizTypes["纯文本"]++;
                else if (type == RapidNotes::ItemType::Type::Link) bizTypes["Link"]++;
                else if (type == RapidNotes::ItemType::Type::File) bizTypes["数据库附件"]++;
                else if (type == RapidNotes::ItemType::Type::LocalFile) bizTypes["本地文件"]++;
                else if (type == RapidNotes::ItemType::Type::LocalFolder || type == RapidNotes::ItemType::Type::Folder) bizTypes["文件夹"]++;
                else if (type == RapidNotes::ItemType::Type::LocalBatch) bizTypes["批量托管"]++;
                else bizTypes["其他"]++;
            }
        }
    }
    QVariantMap typesMap;
    for (auto it = bizTypes.begin(); it != bizTypes.end(); ++it) typesMap[it.key()] = it.value();
    stats["types"] = typesMap;

    // 2.7 物理化唯一标签统计 (2026-05-01 按照用户要求：直接通过实体关联表聚合，杜绝模糊匹配误差)
    QMap<QString, int> tags;
    // 为实现精准统计，我们必须将主表的过滤条件(whereClause)应用到 JOIN 查询中
    // 采用别名 n 以确保字段引用不冲突
    QString refinedWhere = whereClause;
    refinedWhere.replace("is_deleted", "n.is_deleted")
                .replace("category_id", "n.category_id")
                .replace("is_favorite", "n.is_favorite")
                .replace("item_type", "n.item_type")
                .replace("color", "n.color")
                .replace("word_count", "n.word_count")
                .replace("rating", "n.rating");

    QString tagStatsSql = "SELECT nt.tag_name, COUNT(*) "
                          "FROM note_tags nt "
                          "JOIN notes n ON nt.note_id = n.id "
                          + refinedWhere + " GROUP BY nt.tag_name";

    QSqlQuery tagQuery(m_db);
    tagQuery.prepare(tagStatsSql);
    for (int i = 0; i < params.size(); ++i) tagQuery.bindValue(i, params[i]);
    
    if (tagQuery.exec()) {
        while (tagQuery.next()) {
            tags[tagQuery.value(0).toString()] = tagQuery.value(1).toInt();
        }
    }
    QVariantMap tagsMap;
    for (auto it = tags.begin(); it != tags.end(); ++it) tagsMap[it.key()] = it.value();
    
    // 2.7.5 未标签统计 (2026-05-03 [NEW] 按照用户要求：物理统计 tags 为空的记录)
    QSqlQuery untaggedQuery(m_db);
    untaggedQuery.prepare("SELECT COUNT(*) " + baseSql + whereClause + " AND (tags IS NULL OR tags = '')");
    for (int i = 0; i < params.size(); ++i) untaggedQuery.bindValue(i, params[i]);
    if (untaggedQuery.exec() && untaggedQuery.next()) {
        int untaggedCount = untaggedQuery.value(0).toInt();
        if (untaggedCount > 0) tagsMap["__untagged__"] = untaggedCount;
    }
    stats["tags"] = tagsMap;

    // 5. 创建日期统计
    QMap<QString, int> createDateCounts;
    query.prepare("SELECT date(created_at), COUNT(*) " + baseSql + whereClause + " GROUP BY date(created_at) ORDER BY date(created_at) DESC");
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    if (query.exec()) {
        while (query.next()) {
            createDateCounts[query.value(0).toString()] = query.value(1).toInt();
        }
    }
    QVariantMap createDateStats;
    for (auto it = createDateCounts.begin(); it != createDateCounts.end(); ++it) createDateStats[it.key()] = it.value();
    stats["date_create"] = createDateStats;

    // 6. 修改日期统计
    QMap<QString, int> updateDateCounts;
    query.prepare("SELECT date(updated_at), COUNT(*) " + baseSql + whereClause + " GROUP BY date(updated_at) ORDER BY date(updated_at) DESC");
    for (int i = 0; i < params.size(); ++i) query.bindValue(i, params[i]);
    if (query.exec()) {
        while (query.next()) {
            updateDateCounts[query.value(0).toString()] = query.value(1).toInt();
        }
    }
    QVariantMap updateDateStats;
    for (auto it = updateDateCounts.begin(); it != updateDateCounts.end(); ++it) updateDateStats[it.key()] = it.value();
    stats["date_update"] = updateDateStats;

    return stats;
}

void DatabaseManager::applySecurityFilter(QString& whereClause, QVariantList& params, const QString& filterType) {
    QSqlQuery catQuery(m_db);
    catQuery.exec("SELECT id FROM categories WHERE password IS NOT NULL AND password != ''");
    QList<int> lockedIds;
    while (catQuery.next()) { int cid = catQuery.value(0).toInt(); if (!m_unlockedCategories.contains(cid)) lockedIds.append(cid); }
    if (!lockedIds.isEmpty()) {
        QStringList placeholders; for (int i = 0; i < lockedIds.size(); ++i) placeholders << "?";
        // 2026-03-xx 按照用户要求修复逻辑：在排除锁定分类时，必须确保"未分类"项目（NULL 或 <=0）始终可见，不被误杀
        whereClause += QString("AND (category_id IS NULL OR category_id <= 0 OR category_id NOT IN (%1)) ").arg(placeholders.join(", "));
        for (int id : lockedIds) params << id;
    }
}

// [CRITICAL] 通用过滤引擎：recently_visited 必须包含排除今日新建笔记的日期判定条件。此逻辑涉及业务分类的严谨性，禁止删除。
void DatabaseManager::applyCommonFilters(QString& whereClause, QVariantList& params, const QString& filterType, const QVariant& filterValue, const QVariantMap& criteria) {
    if (filterType == "trash") {
        whereClause = "WHERE is_deleted = 1 ";
    } else {
        whereClause = "WHERE is_deleted = 0 ";
        applySecurityFilter(whereClause, params, filterType);
        
        if (filterType == "category") { 
            // 2026-03-xx 按照用户要求修复傻逼逻辑：分类 ID 为 -1 时应视为“未分类”，统一查询口径
            if (filterValue.toInt() == -1) whereClause += "AND (category_id IS NULL OR category_id <= 0) "; 
            else { whereClause += "AND category_id = ? "; params << filterValue.toInt(); } 
        }
        else if (filterType == "uncategorized") {
            // 2026-03-xx 按照用户要求修复傻逼逻辑：统一“未分类”判定，防止物理删除分类后的笔记在恢复时变成“幽灵数据”
            whereClause += "AND (category_id IS NULL OR category_id <= 0) ";
        }
        else if (filterType == "today") {
            whereClause += "AND date(created_at) = ? ";
            params << QDate::currentDate().toString("yyyy-MM-dd");
        }
        else if (filterType == "yesterday") {
            whereClause += "AND date(created_at) = ? ";
            params << QDate::currentDate().addDays(-1).toString("yyyy-MM-dd");
        }
        else if (filterType == "recently_visited") {
            // 2026-05-05 按照用户要求：扩展"最近访问"范围，包含今日创建的笔记和过去7天内的访问记录
            whereClause += "AND (date(last_accessed_at) >= ? OR date(created_at) = ?) ";
            params << QDate::currentDate().addDays(-7).toString("yyyy-MM-dd");
            params << QDate::currentDate().toString("yyyy-MM-dd");
        }
        else if (filterType == "bookmark") whereClause += "AND is_favorite = 1 ";
        else if (filterType == "untagged") whereClause += "AND (tags IS NULL OR tags = '') ";
    }
    
    if (filterType != "trash" && !criteria.isEmpty()) {
        if (criteria.contains("stars")) { 
            QStringList stars = criteria.value("stars").toStringList(); 
            if (!stars.isEmpty()) whereClause += QString("AND rating IN (%1) ").arg(stars.join(", ")); 
        }
        if (criteria.contains("favorite")) {
            QStringList favs = criteria.value("favorite").toStringList();
            if (!favs.isEmpty()) {
                QStringList favConds;
                for (const auto& f : favs) {
                    favConds << "is_favorite = ?";
                    params << f.toInt();
                }
                whereClause += QString("AND (%1) ").arg(favConds.join(" OR "));
            }
        }
        if (criteria.contains("word_count")) {
            // 2026-05-01 按照用户要求：彻底废除实时计算，改为精准阶梯匹配
            QStringList buckets = criteria.value("word_count").toStringList();
            if (!buckets.isEmpty()) {
                QStringList wcConds;
                for (const auto& b : buckets) {
                    wcConds << "word_count = ?";
                    params << b.toInt();
                }
                whereClause += QString("AND (%1) ").arg(wcConds.join(" OR "));
            }
        }
        if (criteria.contains("types")) { 
            // [CRITICAL] 2026-04-08 核心过滤引擎升级：支持多对多后缀关联检索
            QStringList typeCriteria = criteria.value("types").toStringList(); 
            if (!typeCriteria.isEmpty()) { 
                QStringList bizConds;
                for (const QString& label : typeCriteria) {
                    if (label == "截图") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Image));
                    else if (label == "脚本代码") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Code));
                    else if (label == "纯文本") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Text));
                    else if (label == "Link" || label == "网页链接") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Link));
                    else if (label == "数据库附件") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File));
                    else if (label == "本地文件") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalFile));
                    else if (label == "文件夹") {
                        // 2026-05-03 按照用户要求：修正文件夹筛选逻辑，同步检查 item_type 与物理后缀标记
                        bizConds << QString("(item_type IN ('%1', '%2') OR file_extensions = '文件夹' OR file_extensions LIKE '文件夹, %' OR file_extensions LIKE '%, 文件夹' OR file_extensions LIKE '%, 文件夹, %')")
                                    .arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalFolder),
                                         RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Folder));
                    }
                    else if (label == "批量托管") bizConds << QString("item_type = '%1'").arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalBatch));
                    else if (label == "其他") {
                        QString notInList = QString("'%1', '%2', '%3', '%4', '%5', '%6', '%7', '%8', '%9'")
                            .arg(RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Image),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Code),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Text),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Link),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::File),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalFile),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalFolder),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::Folder),
                                 RapidNotes::ItemType::toString(RapidNotes::ItemType::Type::LocalBatch));
                        bizConds << QString("(file_extensions = '' AND item_type NOT IN (%1))").arg(notInList);
                    }
                    else {
                        // [OPTIMIZATION] 2026-04-08：物理移除消耗性能的 REPLACE，直接利用存储时的规范化格式 (, ext1, ext2,)
                        // 注：在 addNote 时我们已经确保 file_extensions 存储格式为 "ext1, ext2"
                        // 为实现毫秒级响应，我们采用标准 LIKE 匹配
                        bizConds << "(file_extensions = ? OR file_extensions LIKE ? OR file_extensions LIKE ? OR file_extensions LIKE ?)";
                        params << label.toLower();
                        params << label.toLower() + ", %";
                        params << "%, " + label.toLower();
                        params << "%, " + label.toLower() + ", %";
                    }
                }

                if (!bizConds.isEmpty()) {
                    whereClause += QString("AND (%1) ").arg(bizConds.join(" OR ")); 
                }
            } 
        }
        if (criteria.contains("colors")) { 
            QStringList colors = criteria.value("colors").toStringList(); 
            if (!colors.isEmpty()) { 
                QStringList placeholders; 
                for (const auto& c : colors) { placeholders << "?"; params << c; } 
                whereClause += QString("AND color_label IN (%1) ").arg(placeholders.join(", ")); 
            } 
        }
        if (criteria.contains("tags")) { 
            // 2026-05-01 彻底废除字符串 LIKE 匹配，改用基于实体关联表的精准 EXISTS 查询
            // 这能 100% 解决多语言混合、特殊分隔符导致的筛选失效问题
            QStringList selectedTags = criteria.value("tags").toStringList(); 
            if (!selectedTags.isEmpty()) { 
                QStringList tagExists;
                for (const auto& t : selectedTags) {
                    if (t == "__untagged__") {
                        tagExists << "(tags IS NULL OR tags = '')";
                    } else {
                        tagExists << "EXISTS (SELECT 1 FROM note_tags nt WHERE nt.note_id = notes.id AND nt.tag_name = ?)";
                        params << t.trimmed();
                    }
                }
                whereClause += QString("AND (%1) ").arg(tagExists.join(" OR ")); 
            } 
        }
        if (criteria.contains("date_create")) { 
            QStringList dates = criteria.value("date_create").toStringList(); 
            if (!dates.isEmpty()) { 
                QStringList dateConds; 
                for (const auto& d : dates) { 
                    dateConds << "date(created_at) = ?";
                    params << d;
                } 
                if (!dateConds.isEmpty()) whereClause += QString("AND (%1) ").arg(dateConds.join(" OR ")); 
            } 
        }
        if (criteria.contains("date_update")) { 
            QStringList dates = criteria.value("date_update").toStringList(); 
            if (!dates.isEmpty()) { 
                QStringList dateConds; 
                for (const auto& d : dates) { 
                    dateConds << "date(updated_at) = ?";
                    params << d;
                } 
                if (!dateConds.isEmpty()) whereClause += QString("AND (%1) ").arg(dateConds.join(" OR ")); 
            } 
        }
    }
}
