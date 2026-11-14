#include "DatabaseManager.h"
#include "DatabaseManagerInternal.h"

QString DatabaseManager::getCategoryNameById(int id) {
    if (id <= 0) return "";
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("SELECT name FROM categories WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

QVariantMap DatabaseManager::getRootCategory(int catId) {
    if (catId <= 0) return QVariantMap();
    QMutexLocker locker(&m_mutex);
    
    int currentId = catId;
    QVariantMap result;
    
    // 递归向上查找父分类，直到顶级
    while (true) {
        QSqlQuery query(m_db);
        query.prepare("SELECT id, name, parent_id FROM categories WHERE id = :id");
        query.bindValue(":id", currentId);
        
        if (query.exec() && query.next()) {
            result["id"] = query.value("id");
            result["name"] = query.value("name");
            int parentId = query.value("parent_id").isNull() ? -1 : query.value("parent_id").toInt();
            
            if (parentId <= 0) {
                // 已经到达最顶层
                break;
            } else {
                currentId = parentId;
            }
        } else {
            break;
        }
    }
    
    return result;
}

bool DatabaseManager::reorderCategories(int parentId, bool ascending) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    QSqlQuery query(m_db);
    if (parentId <= 0) query.prepare("SELECT id, name FROM categories WHERE parent_id IS NULL OR parent_id <= 0");
    else { query.prepare("SELECT id, name FROM categories WHERE parent_id = :pid"); query.bindValue(":pid", parentId); }
    if (!query.exec()) return false;
    struct CatInfo { int id; QString name; };
    QList<CatInfo> list;
    while (query.next()) list.append({query.value(0).toInt(), query.value(1).toString()});
    if (list.isEmpty()) return true;
    std::sort(list.begin(), list.end(), [ascending](const CatInfo& a, const CatInfo& b) {
        if (ascending) return a.name.localeAwareCompare(b.name) < 0;
        return a.name.localeAwareCompare(b.name) > 0;
    });
    m_db.transaction();
    QSqlQuery update(m_db);
    for (int i = 0; i < list.size(); ++i) {
        update.prepare("UPDATE categories SET sort_order = :val WHERE id = :id");
        update.bindValue(":val", i);
        update.bindValue(":id", list[i].id);
        update.exec();
    }
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit categoriesChanged(); }
    return ok;
}

bool DatabaseManager::updateCategoryOrder(int parentId, const QList<int>& categoryIds) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    if (!m_db.transaction()) return false;
    QSqlQuery query(m_db);
    query.prepare("UPDATE categories SET parent_id = :pid, sort_order = :order WHERE id = :id");
    for (int i = 0; i < categoryIds.size(); ++i) {
        query.bindValue(":pid", parentId <= 0 ? QVariant() : parentId);
        query.bindValue(":order", i);
        query.bindValue(":id", categoryIds[i]);
        if (!query.exec()) { m_db.rollback(); return false; }
    }
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit categoriesChanged(); }
    return ok;
}

bool DatabaseManager::updatePinnedCategoryOrder(const QList<int>& categoryIds) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    if (!m_db.transaction()) return false;
    QSqlQuery query(m_db);
    query.prepare("UPDATE categories SET pinned_sort_order = :order WHERE id = :id");
    for (int i = 0; i < categoryIds.size(); ++i) {
        query.bindValue(":order", i);
        query.bindValue(":id", categoryIds[i]);
        if (!query.exec()) { m_db.rollback(); return false; }
    }
    bool ok = m_db.commit();
    if (ok) { markDirty(); emit categoriesChanged(); }
    return ok;
}

bool DatabaseManager::reorderAllCategories(bool ascending) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    QSqlQuery query(m_db);
    query.exec("SELECT DISTINCT parent_id FROM categories");
    QList<int> parents;
    bool hasRoot = false;
    while (query.next()) {
        if (query.value(0).isNull() || query.value(0).toInt() <= 0) hasRoot = true;
        else parents.append(query.value(0).toInt());
    }
    bool success = true;
    if (hasRoot) success &= reorderCategories(-1, ascending);
    for (int pid : parents) success &= reorderCategories(pid, ascending);
    return success;
}

QString DatabaseManager::getCategorySortMode(int catId) {
    if (catId <= 0) return "manual";
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("SELECT sort_mode FROM categories WHERE id = :id");
    query.bindValue(":id", catId);
    if (query.exec() && query.next()) {
        QString mode = query.value(0).toString();
        return mode.isEmpty() ? "manual" : mode;
    }
    return "manual";
}

bool DatabaseManager::setCategorySortMode(int catId, const QString& mode) {
    if (catId <= 0) return false;
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        QSqlQuery query(m_db);
        query.prepare("UPDATE categories SET sort_mode = :mode WHERE id = :id");
        query.bindValue(":mode", mode);
        query.bindValue(":id", catId);
        success = query.exec();
        if (success) markDirty();
    }
    if (success) emit categoriesChanged();
    return success;
}

bool DatabaseManager::setCategoryPassword(int id, const QString& password, const QString& hint) {
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        QString hashedPassword = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
        QSqlQuery query(m_db);
        query.prepare("UPDATE categories SET password=:password, password_hint=:hint WHERE id=:id");
        query.bindValue(":password", hashedPassword);
        query.bindValue(":hint", hint);
        query.bindValue(":id", id);
        success = query.exec();
        if (success) markDirty();
    }
    if (success) emit categoriesChanged();
    return success;
}

bool DatabaseManager::removeCategoryPassword(int id) {
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        QSqlQuery query(m_db);
        query.prepare("UPDATE categories SET password=NULL, password_hint=NULL WHERE id=:id");
        query.bindValue(":id", id);
        success = query.exec();
        if (success) { markDirty(); m_unlockedCategories.remove(id); }
    }
    if (success) emit categoriesChanged();
    return success;
}

bool DatabaseManager::verifyCategoryPassword(int id, const QString& password) {
    bool correct = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        QString hashedPassword = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
        QSqlQuery query(m_db);
        query.prepare("SELECT password FROM categories WHERE id=:id");
        query.bindValue(":id", id);
        if (query.exec() && query.next()) {
            if (query.value(0).toString() == hashedPassword) correct = true;
        }
    }
    if (correct) unlockCategory(id);
    return correct;
}

bool DatabaseManager::isCategoryLocked(int id) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    if (m_unlockedCategories.contains(id)) return false;
    QSqlQuery query(m_db);
    query.prepare("SELECT password FROM categories WHERE id=:id");
    query.bindValue(":id", id);
    if (query.exec() && query.next()) return !query.value(0).toString().isEmpty();
    return false;
}

void DatabaseManager::lockCategory(int id) { { QMutexLocker locker(&m_mutex); m_unlockedCategories.remove(id); } emit categoriesChanged(); }
void DatabaseManager::lockAllCategories() { { QMutexLocker locker(&m_mutex); m_unlockedCategories.clear(); } emit categoriesChanged(); }
void DatabaseManager::toggleLockedCategoriesVisibility() {
    qDebug() << "[TRACE-DB] toggleLockedCategoriesVisibility 被调用。";
    // 2026-03-xx 按照用户要求：无论解锁/锁住状态，切换显示时立即全部重锁
    {
        QMutexLocker locker(&m_mutex);
        m_unlockedCategories.clear();
        m_lockedCategoriesHidden = !m_lockedCategoriesHidden;
        
        QSettings settings("RapidNotes", "QuickWindow");
        settings.setValue("lockedCategoriesHidden", m_lockedCategoriesHidden);
    }
    emit categoriesChanged();
}

void DatabaseManager::unlockCategory(int id) { { QMutexLocker locker(&m_mutex); m_unlockedCategories.insert(id); } emit categoriesChanged(); }

bool DatabaseManager::restoreAllFromTrash() {
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        m_db.transaction();
        
        QSqlQuery query(m_db);
        // 恢复所有分类
        query.exec("UPDATE categories SET is_deleted = 0 WHERE is_deleted = 1");
        // 恢复所有笔记，并恢复默认颜色（如果原分类已不存在，这部分逻辑在获取颜色时会处理）
        success = query.exec("UPDATE notes SET is_deleted = 0, updated_at = datetime('now','localtime') WHERE is_deleted = 1");
        
        success = m_db.commit();
    }
    if (success) { markDirty(); emit noteUpdated(); emit categoriesChanged(); }
    return success;
}

int DatabaseManager::addCategory(const QString& name, int parentId, const QString& color) {
    int lastId = -1;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return -1;
        int maxOrder = 0;
        QSqlQuery orderQuery(m_db);
        if (parentId == -1) orderQuery.exec("SELECT MAX(sort_order) FROM categories WHERE parent_id IS NULL OR parent_id = -1");
        else { orderQuery.prepare("SELECT MAX(sort_order) FROM categories WHERE parent_id = :pid"); orderQuery.bindValue(":pid", parentId); orderQuery.exec(); }
        if (orderQuery.next()) maxOrder = orderQuery.value(0).toInt();
        QString chosenColor = color;
        if (chosenColor.isEmpty()) { static const QStringList palette = { "#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEEAD", "#D4A5A5", "#9B59B6", "#3498DB", "#E67E22", "#2ECC71", "#E74C3C", "#F1C40F", "#1ABC9C", "#34495E", "#95A5A6" }; chosenColor = palette.at(QRandomGenerator::global()->bounded(palette.size())); }
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO categories (name, parent_id, color, sort_order) VALUES (:name, :parent_id, :color, :sort_order)");
        query.bindValue(":name", name);
        query.bindValue(":parent_id", parentId == -1 ? QVariant(QMetaType::fromType<int>()) : parentId);
        query.bindValue(":color", chosenColor);
        query.bindValue(":sort_order", maxOrder + 1);
        if (query.exec()) { lastId = query.lastInsertId().toInt(); markDirty(); }
    }
    if (lastId != -1) emit categoriesChanged();
    return lastId;
}

int DatabaseManager::getOrCreateCategoryByName(const QString& name, int parentId, const QString& color) {
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return -1;
        
        QSqlQuery query(m_db);
        if (parentId <= 0) {
            query.prepare("SELECT id FROM categories WHERE name = :name AND (parent_id IS NULL OR parent_id <= 0) AND is_deleted = 0 LIMIT 1");
        } else {
            query.prepare("SELECT id FROM categories WHERE name = :name AND parent_id = :pid AND is_deleted = 0 LIMIT 1");
            query.bindValue(":pid", parentId);
        }
        query.bindValue(":name", name);
        
        if (query.exec() && query.next()) {
            return query.value(0).toInt();
        }
    }
    
    // 找不到则创建
    return addCategory(name, parentId, color);
}

bool DatabaseManager::toggleCategoryPinned(int id) {
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        QSqlQuery query(m_db);
        query.prepare("UPDATE categories SET is_pinned = NOT is_pinned WHERE id = :id");
        query.bindValue(":id", id);
        success = query.exec();
        if (success) markDirty();
    }
    if (success) emit categoriesChanged();
    return success;
}

bool DatabaseManager::renameCategory(int id, const QString& name) {
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        QSqlQuery query(m_db);
        query.prepare("UPDATE categories SET name=:name WHERE id=:id");
        query.bindValue(":name", name);
        query.bindValue(":id", id);
        success = query.exec();
        if (success) markDirty();
    }
    if (success) emit categoriesChanged();
    return success;
}

bool DatabaseManager::setCategoryColor(int id, const QString& color) {
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        m_db.transaction();
        QSqlQuery treeQuery(m_db);
        // [STABILITY] 增加递归深度限制（50层），防止循环引用导致的 SQL 执行器爆栈
        treeQuery.prepare(R"(
            WITH RECURSIVE category_tree(id, depth) AS (
                SELECT :id, 0 
                UNION ALL 
                SELECT c.id, ct.depth + 1 FROM categories c JOIN category_tree ct ON c.parent_id = ct.id 
                WHERE ct.depth < 50
            ) SELECT id FROM category_tree)");
        treeQuery.bindValue(":id", id);
        QList<int> allIds;
        if (treeQuery.exec()) { while (treeQuery.next()) allIds << treeQuery.value(0).toInt(); }
        if (!allIds.isEmpty()) {
            QString placeholders;
            for(int i=0; i<allIds.size(); ++i) placeholders += (i==0 ? "?" : ",?");
            QSqlQuery updateCats(m_db);
            updateCats.prepare(QString("UPDATE categories SET color = ? WHERE id IN (%1)").arg(placeholders));
            updateCats.addBindValue(color);
            for(int cid : allIds) updateCats.addBindValue(cid);
            updateCats.exec();
            QSqlQuery updateNotes(m_db);   // 先改分类色，再按统一规则重算其下笔记底色
            updateNotes.prepare(QString("UPDATE notes SET color = %1 WHERE category_id IN (%2)").arg(kNoteColorExpr, placeholders));
            for(int cid : allIds) updateNotes.addBindValue(cid);
            updateNotes.exec();
        }
        success = m_db.commit();
    }
    if (success) markDirty();
    if (success) { emit categoriesChanged(); emit noteUpdated(); }
    return success;
}

bool DatabaseManager::hardDeleteCategories(const QList<int>& ids) {
    // 2026-03-xx 按照用户要求：分类物理删除，笔记软删除（删除并重置 category_id）
    if (ids.isEmpty()) return true;
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;

    if (!m_db.transaction()) {
        qWarning() << "[DB] hardDeleteCategories 开启事务失败";
        return false;
    }

    QList<int> allIds;
    for (int startId : ids) {
        // [MODIFIED] 必须包含递归逻辑，确保所有子分类被物理清除，笔记被正确移出
        QSqlQuery treeQuery(m_db);
        treeQuery.prepare(R"(
            WITH RECURSIVE category_tree(id, depth) AS (
                SELECT :id, 0
                UNION ALL
                SELECT c.id, ct.depth + 1 FROM categories c JOIN category_tree ct ON c.parent_id = ct.id
                WHERE ct.depth < 50
            ) SELECT id FROM category_tree
        )");
        treeQuery.bindValue(":id", startId);
        if (treeQuery.exec()) {
            while (treeQuery.next()) {
                int cid = treeQuery.value(0).toInt();
                if (!allIds.contains(cid)) allIds << cid;
            }
        }
    }

    QStringList idStrings;
    for(int cid : allIds) idStrings << QString::number(cid);
    QString joinedIds = idStrings.join(",");

    // 1. 软删除关联笔记：标记 is_deleted=1，并将 category_id 设为 -1 (未分类)，防止孤儿记录
    QSqlQuery softDelNotes(m_db);
    QString softDelSql = QString(
        "UPDATE notes SET is_deleted = 1, category_id = -1, color = '#2d2d2d', "
        "is_pinned = 0, is_favorite = 0, updated_at = datetime('now','localtime') "
        "WHERE category_id IN (%1)"
    ).arg(joinedIds);

    if (!softDelNotes.exec(softDelSql)) {
        qWarning() << "[DB] 混合删除-笔记软处理失败:" << softDelNotes.lastError().text();
        m_db.rollback();
        return false;
    }

    // 2. 物理删除分类自身
    QSqlQuery query(m_db);
    bool ok = query.exec(QString("DELETE FROM categories WHERE id IN (%1)").arg(joinedIds));

    if (ok) {
        m_db.commit();
        qDebug() << "[DB] 成功执行混合删除：物理清除分类" << allIds.size() << "个，笔记移入回收站" << softDelNotes.numRowsAffected() << "条";
        markDirty();
        emit categoriesChanged();
        emit noteUpdated();
    } else {
        m_db.rollback();
        qWarning() << "[DB] 混合删除-分类物理清除失败:" << query.lastError().text();
    }
    return ok;
}

bool DatabaseManager::softDeleteCategories(const QList<int>& ids) {
    // 2026-03-xx 增加详尽日志，排查删除失效问题
    qDebug() << "[DB] softDeleteCategories 入口参数 ids:" << ids;
    if (ids.isEmpty()) return true;
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        if (!m_db.transaction()) {
            qWarning() << "[DB] 无法开启事务:" << m_db.lastError().text();
            return false;
        }
        
        QSqlQuery query(m_db);
        for (int id : ids) {
            // 使用递归 CTE 找到所有子分类 ID
            QSqlQuery treeQuery(m_db);
            treeQuery.prepare(R"(
                WITH RECURSIVE category_tree(id, depth) AS (
                    SELECT :id, 0
                    UNION ALL
                    SELECT c.id, ct.depth + 1 FROM categories c JOIN category_tree ct ON c.parent_id = ct.id
                    WHERE ct.depth < 50
                ) SELECT id FROM category_tree
            )");
            treeQuery.bindValue(":id", id);
            QList<int> allIds;
            if (treeQuery.exec()) {
                while (treeQuery.next()) allIds << treeQuery.value(0).toInt();
            } else {
                qWarning() << "[DB] 递归查询失败:" << treeQuery.lastError().text();
            }

            qDebug() << "[DB] 分类 ID:" << id << "递归展开后的 IDs:" << allIds;

            if (!allIds.isEmpty()) {
                QStringList idStrings;
                for(int cid : allIds) idStrings << QString::number(cid);
                QString joinedIds = idStrings.join(",");

                // 1. 标记分类为已删除 (2026-03-xx 放弃 prepare 以解决 Parameter count mismatch)
                QSqlQuery delCat(m_db);
                if (!delCat.exec(QString("UPDATE categories SET is_deleted = 1, updated_at = datetime('now','localtime') WHERE id IN (%1)").arg(joinedIds))) {
                    qWarning() << "[DB] 更新 categories 状态失败:" << delCat.lastError().text();
                    m_db.rollback();
                    return false;
                } else {
                    qDebug() << "[DB] 成功标记" << delCat.numRowsAffected() << "个分类为已删除";
                }

                // 2. 标记所属笔记为已删除
                QSqlQuery delNotes(m_db);
                if (!delNotes.exec(QString("UPDATE notes SET is_deleted = 1, color = '#2d2d2d', is_pinned = 0, is_favorite = 0, updated_at = datetime('now','localtime'), last_accessed_at = datetime('now','localtime') WHERE category_id IN (%1)").arg(joinedIds))) {
                    qWarning() << "[DB] 更新 notes 状态失败:" << delNotes.lastError().text();
                    m_db.rollback();
                    return false;
                } else {
                    qDebug() << "[DB] 成功标记所属分类下的" << delNotes.numRowsAffected() << "条笔记为已删除";
                }
            }
        }
        success = m_db.commit();
        if (!success) {
            qWarning() << "[DB] 事务提交失败:" << m_db.lastError().text();
            m_db.rollback();
        } else {
            qDebug() << "[DB] softDeleteCategories 事务提交成功";
        }
    }
    if (success) {
        markDirty();
        emit categoriesChanged();
        emit noteUpdated();
    }
    return success;
}

bool DatabaseManager::restoreCategories(const QList<int>& ids) {
    if (ids.isEmpty()) return true;
    bool success = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        m_db.transaction();
        
        QSqlQuery query(m_db);
        for (int id : ids) {
            // 同样递归找到所有子项，确保整树恢复
            QSqlQuery treeQuery(m_db);
            treeQuery.prepare(R"(
                WITH RECURSIVE category_tree(id, depth) AS (
                    SELECT :id, 0
                    UNION ALL
                    SELECT c.id, ct.depth + 1 FROM categories c JOIN category_tree ct ON c.parent_id = ct.id
                    WHERE ct.depth < 50
                ) SELECT id FROM category_tree
            )");
            treeQuery.bindValue(":id", id);
            QList<int> allIds;
            if (treeQuery.exec()) {
                while (treeQuery.next()) allIds << treeQuery.value(0).toInt();
            }

            if (!allIds.isEmpty()) {
                QStringList placeholders;
                for(int i=0; i<allIds.size(); ++i) placeholders << "?";
                QString joined = placeholders.join(",");

                // 1. 恢复分类
                QSqlQuery resCat(m_db);
                resCat.prepare(QString("UPDATE categories SET is_deleted = 0 WHERE id IN (%1)").arg(joined));
                for(int cid : allIds) resCat.addBindValue(cid);
                resCat.exec();

                // 2. 恢复笔记。同步更新最后访问时间。
                QSqlQuery resNotes(m_db);
                resNotes.prepare(QString("UPDATE notes SET is_deleted = 0, updated_at = datetime('now','localtime'), last_accessed_at = datetime('now','localtime') WHERE category_id IN (%1)").arg(joined));
                for(int cid : allIds) resNotes.addBindValue(cid);
                resNotes.exec();
            }
        }
        success = m_db.commit();
    }
    if (success) {
        markDirty();
        emit categoriesChanged();
        emit noteUpdated();
    }
    return success;
}

bool DatabaseManager::moveCategory(int id, DatabaseManager::MoveDirection direction) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return false;
    int parentId = -1;
    QSqlQuery parentQuery(m_db);
    parentQuery.prepare("SELECT parent_id FROM categories WHERE id = :id");
    parentQuery.bindValue(":id", id);
    if (parentQuery.exec() && parentQuery.next()) parentId = parentQuery.value(0).isNull() ? -1 : parentQuery.value(0).toInt();
    else return false;
    QSqlQuery siblingsQuery(m_db);
    if (parentId == -1) siblingsQuery.prepare("SELECT id FROM categories WHERE parent_id IS NULL OR parent_id = -1 ORDER BY sort_order ASC");
    else { siblingsQuery.prepare("SELECT id FROM categories WHERE parent_id = :pid ORDER BY sort_order ASC"); siblingsQuery.bindValue(":pid", parentId); }
    if (!siblingsQuery.exec()) return false;
    QList<int> siblings;
    while (siblingsQuery.next()) siblings << siblingsQuery.value(0).toInt();
    int currentIndex = siblings.indexOf(id);
    if (currentIndex == -1) return false;
    switch (direction) {
        case Up: if (currentIndex > 0) std::swap(siblings[currentIndex], siblings[currentIndex - 1]); else return false; break;
        case Down: if (currentIndex < siblings.size() - 1) std::swap(siblings[currentIndex], siblings[currentIndex + 1]); else return false; break;
        case Top: if (currentIndex > 0) { siblings.removeAt(currentIndex); siblings.prepend(id); } else return false; break;
        case Bottom: if (currentIndex < siblings.size() - 1) { siblings.removeAt(currentIndex); siblings.append(id); } else return false; break;
    }
    return updateCategoryOrder(parentId, siblings);
}

QList<QVariantMap> DatabaseManager::getAllCategories() {
    QMutexLocker locker(&m_mutex);
    QList<QVariantMap> results;
    if (!m_db.isOpen()) return results;
    QSqlQuery query(m_db);
    // [MODIFIED] 彻底分离排序：实体组纯靠 sort_order 排序，不再强制使用 is_pinned 置顶干扰
    if (query.exec("SELECT * FROM categories WHERE is_deleted = 0 ORDER BY sort_order ASC")) { 
        while (query.next()) { 
            QVariantMap map; 
            QSqlRecord rec = query.record(); 
            for (int i = 0; i < rec.count(); ++i) map[rec.fieldName(i)] = query.value(i); 
            results.append(map); 
        } 
    }
    return results;
}

QList<QVariantMap> DatabaseManager::getChildCategories(int parentId) {
    QMutexLocker locker(&m_mutex);
    QList<QVariantMap> results;
    if (!m_db.isOpen()) return results;
    QSqlQuery query(m_db);
    if (parentId <= 0) {
        query.prepare("SELECT * FROM categories WHERE (parent_id IS NULL OR parent_id <= 0) AND is_deleted = 0 ORDER BY is_pinned DESC, sort_order ASC");
    } else {
        query.prepare("SELECT * FROM categories WHERE parent_id = ? AND is_deleted = 0 ORDER BY is_pinned DESC, sort_order ASC");
        query.addBindValue(parentId);
    }
    if (query.exec()) {
        while (query.next()) {
            QVariantMap map;
            QSqlRecord rec = query.record();
            for (int i = 0; i < rec.count(); ++i) map[rec.fieldName(i).toLower()] = query.value(i);
            results.append(map);
        }
    }
    return results;
}

bool DatabaseManager::setCategoryPresetTags(int catId, const QString& tags) {
    bool ok = false;
    QList<int> affectedIds;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_db.isOpen()) return false;
        m_db.transaction();
        QSqlQuery query(m_db);
        query.prepare("UPDATE categories SET preset_tags=:tags WHERE id=:id");
        query.bindValue(":tags", tags);
        query.bindValue(":id", catId);
        if (!query.exec()) { m_db.rollback(); return false; }
        if (!tags.isEmpty()) {
            QStringList newTagsList = RapidNotes::TagCodec::split(tags);
            QSqlQuery fetchNotes(m_db);
            fetchNotes.prepare("SELECT id, tags FROM notes WHERE category_id = :catId AND is_deleted = 0");
            fetchNotes.bindValue(":catId", catId);
            if (fetchNotes.exec()) {
                while (fetchNotes.next()) {
                    int noteId = fetchNotes.value(0).toInt();
                    QString existingTagsStr = fetchNotes.value(1).toString();
                    QStringList existingTags = RapidNotes::TagCodec::split(existingTagsStr);
                    bool changed = false;
                    RapidNotes::TagCodec::merge(existingTags, newTagsList, &changed);
                    if (changed) { 
                        affectedIds << noteId;
                        QSqlQuery updateNote(m_db); 
                        updateNote.prepare("UPDATE notes SET tags = :tags WHERE id = :id"); 
                        updateNote.bindValue(":tags", RapidNotes::TagCodec::joinDisplay(existingTags)); 
                        updateNote.bindValue(":id", noteId); 
                        updateNote.exec(); 
                        
                        // 2026-05-02 补齐预设标签同步漏洞
                        syncNoteTags(noteId, existingTags);
                    }
                }
            }
        }
        ok = m_db.commit();
    }
    if (ok) markDirty();
    if (ok) { 
        emit categoriesChanged(); 
        emit noteUpdated(); 
    }
    return ok;
}

QString DatabaseManager::getCategoryPresetTags(int catId) {
    QMutexLocker locker(&m_mutex);
    if (!m_db.isOpen()) return "";
    QSqlQuery query(m_db);
    query.prepare("SELECT preset_tags FROM categories WHERE id=:id");
    query.bindValue(":id", catId);
    if (query.exec() && query.next()) return query.value(0).toString();
    return "";
}
