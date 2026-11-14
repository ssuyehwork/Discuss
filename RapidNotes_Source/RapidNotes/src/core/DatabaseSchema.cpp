#include "DatabaseManager.h"
#include "DatabaseManagerInternal.h"

bool DatabaseManager::renameLegacyNoteTagDict() {
    // QuarkMeta 的 tags(name 主键) 与 RapidNotes 旧 tags(id 自增) 同名：旧表先改名，让出 tags 给统一标签字典
    QSqlQuery chk(m_db);
    bool legacy = false, dictExists = false;
    if (chk.exec("PRAGMA table_info(tags)")) {
        while (chk.next()) if (chk.value(1).toString().compare("id", Qt::CaseInsensitive) == 0) legacy = true;
    }
    QSqlQuery c2(m_db);
    if (c2.exec("SELECT 1 FROM sqlite_master WHERE type='table' AND name='note_tag_dict'") && c2.next()) dictExists = true;
    if (legacy && !dictExists) {
        QSqlQuery ren(m_db);
        if (!ren.exec("ALTER TABLE tags RENAME TO note_tag_dict")) {
            m_lastError = "旧标签表改名失败: " + ren.lastError().text();
            return false;
        }
    }
    return true;
}

bool DatabaseManager::migrateNoteTagsSchema() {
    // 统一后的标签模型：tags(字典，唯一) + note_tags(note_id, tag_name)。notes.tags 仅作为由它生成的展示/搜索缓存。
    QSqlQuery q(m_db);
    q.exec("CREATE TABLE IF NOT EXISTS system_config (key TEXT PRIMARY KEY, value TEXT)");

    bool legacyRel = false, hasDict = false;
    if (q.exec("PRAGMA table_info(note_tags)")) {
        while (q.next()) if (q.value(1).toString() == "tag_id") legacyRel = true;
    }
    QSqlQuery c(m_db);
    if (c.exec("SELECT 1 FROM sqlite_master WHERE type='table' AND name='note_tag_dict'") && c.next()) hasDict = true;

    m_db.transaction();
    QSqlQuery s(m_db);
    if (hasDict) s.exec("INSERT OR IGNORE INTO tags(name) SELECT name FROM note_tag_dict");
    if (legacyRel) s.exec("DROP TABLE note_tags");
    bool ok = s.exec("CREATE TABLE IF NOT EXISTS note_tags ("
                     "note_id INTEGER NOT NULL, tag_name TEXT NOT NULL COLLATE NOCASE, "
                     "PRIMARY KEY (note_id, tag_name), "
                     "FOREIGN KEY (note_id) REFERENCES notes(id) ON DELETE CASCADE, "
                     "FOREIGN KEY (tag_name) REFERENCES tags(name) ON DELETE CASCADE ON UPDATE CASCADE)");
    if (!ok) { m_lastError = "创建 note_tags 失败: " + s.lastError().text(); m_db.rollback(); return false; }
    s.exec("CREATE INDEX IF NOT EXISTS idx_note_tags_tag ON note_tags(tag_name, note_id)");

    QSqlQuery key(m_db);
    if (!(key.exec("SELECT 1 FROM system_config WHERE key = 'migration_20261113_tags_unify'") && key.next())) {
        // 以 notes.tags 字符串为准一次性重建关联（用户可见的标签就是它）
        QSqlQuery fetch(m_db);
        QSqlQuery insDict(m_db), insRel(m_db);
        insDict.prepare("INSERT OR IGNORE INTO tags(name) VALUES(?)");
        insRel.prepare("INSERT OR IGNORE INTO note_tags(note_id, tag_name) VALUES(?, ?)");
        if (fetch.exec("SELECT id, tags FROM notes WHERE tags IS NOT NULL AND tags != ''")) {
            while (fetch.next()) {
                int id = fetch.value(0).toInt();
                const QStringList names = RapidNotes::TagCodec::normalize(RapidNotes::TagCodec::split(fetch.value(1).toString()));
                for (const QString& n : names) {
                    insDict.bindValue(0, n); insDict.exec();
                    insRel.bindValue(0, id); insRel.bindValue(1, n); insRel.exec();
                }
            }
        }
        s.exec("INSERT OR REPLACE INTO system_config (key, value) VALUES ('migration_20261113_tags_unify', '1')");
    }
    if (hasDict) s.exec("DROP TABLE note_tag_dict");
    return m_db.commit();
}

// 搜索索引：FTS5 trigram（支持中英文子串匹配，大小写不敏感）。不可用时自动退回 LIKE。
bool DatabaseManager::ensureSearchIndex() {
    QSqlQuery q(m_db);
    m_ftsAvailable = false;
    bool exists = q.exec("SELECT 1 FROM sqlite_master WHERE type='table' AND name='notes_search'") && q.next();
    if (!exists) {
        QSqlQuery c(m_db);
        if (!c.exec("CREATE VIRTUAL TABLE notes_search USING fts5("
                    "title, content, remark, url, tags, file_extensions, "
                    "content='notes', content_rowid='id', tokenize='trigram')")) {
            qWarning() << "[DB] FTS5 trigram 不可用，搜索退回 LIKE:" << c.lastError().text();
            return true;   // 不影响启动
        }
    }
    QSqlQuery d(m_db);
    d.exec("DROP TRIGGER IF EXISTS notes_search_ai");
    d.exec("DROP TRIGGER IF EXISTS notes_search_ad");
    d.exec("DROP TRIGGER IF EXISTS notes_search_au");
    d.exec("CREATE TRIGGER notes_search_ai AFTER INSERT ON notes BEGIN "
           "INSERT INTO notes_search(rowid, title, content, remark, url, tags, file_extensions) "
           "VALUES (new.id, new.title, new.content, new.remark, new.url, new.tags, new.file_extensions); END");
    d.exec("CREATE TRIGGER notes_search_ad AFTER DELETE ON notes BEGIN "
           "INSERT INTO notes_search(notes_search, rowid, title, content, remark, url, tags, file_extensions) "
           "VALUES ('delete', old.id, old.title, old.content, old.remark, old.url, old.tags, old.file_extensions); END");
    // 仅在参与检索的列变化时才重写索引，改星级/颜色/访问时间不触发
    d.exec("CREATE TRIGGER notes_search_au AFTER UPDATE OF title, content, remark, url, tags, file_extensions ON notes BEGIN "
           "INSERT INTO notes_search(notes_search, rowid, title, content, remark, url, tags, file_extensions) "
           "VALUES ('delete', old.id, old.title, old.content, old.remark, old.url, old.tags, old.file_extensions); "
           "INSERT INTO notes_search(rowid, title, content, remark, url, tags, file_extensions) "
           "VALUES (new.id, new.title, new.content, new.remark, new.url, new.tags, new.file_extensions); END");

    QSqlQuery key(m_db);
    if (!(key.exec("SELECT 1 FROM system_config WHERE key = 'fts_trigram_v1'") && key.next())) {
        QSqlQuery rb(m_db);
        if (!rb.exec("INSERT INTO notes_search(notes_search) VALUES('rebuild')")) {
            qWarning() << "[DB] 搜索索引重建失败:" << rb.lastError().text();
            return true;
        }
        QSqlQuery mark(m_db);
        mark.exec("INSERT OR REPLACE INTO system_config (key, value) VALUES ('fts_trigram_v1', '1')");
    }
    m_ftsAvailable = true;
    return true;
}

bool DatabaseManager::createAllTables() {
    QSqlQuery query(m_db);
    QString createNotesTable = R"(
        CREATE TABLE IF NOT EXISTS notes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT,
            content TEXT,
            tags TEXT,
            color TEXT DEFAULT '#2d2d2d',
            category_id INTEGER,
            item_type TEXT DEFAULT 'text',
            data_blob BLOB,
            content_hash TEXT,
            rating INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_pinned INTEGER DEFAULT 0,
            is_locked INTEGER DEFAULT 0,
            is_favorite INTEGER DEFAULT 0,
            is_deleted INTEGER DEFAULT 0,
            source_app TEXT,
            source_title TEXT,
            last_accessed_at DATETIME,
            sort_order INTEGER DEFAULT 0,
            remark TEXT DEFAULT '',
            file_extensions TEXT DEFAULT ''
        )
    )";
    if (!query.exec(createNotesTable)) return false;

    // 2026-03-xx 按照用户要求：部署核心业务索引，确保大数据量下的检索性能
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_main_filter ON notes(is_deleted, category_id, is_pinned, updated_at)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_rating ON notes(rating) WHERE rating > 0");
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_color_label ON notes(color_label) WHERE is_deleted = 0 AND color_label != ''");

    QString createCategoriesTable = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            parent_id INTEGER,
            color TEXT DEFAULT '#808080',
            sort_order INTEGER DEFAULT 0,
            preset_tags TEXT,
            password TEXT,
            password_hint TEXT,
            is_deleted INTEGER DEFAULT 0,
            is_pinned INTEGER DEFAULT 0,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    query.exec(createCategoriesTable);
    // [FIX] 彻底强化迁移：严禁跳过。必须确保 categories 拥有 updated_at 字段，否则回收站统合查询必挂。
    {
        auto addCol = [&](const QString& table, const QString& col, const QString& def) -> bool {
            QStringList existingCols;
            QSqlQuery check(m_db);
            if (check.exec(QString("PRAGMA table_info(%1)").arg(table))) {
                while (check.next()) existingCols << check.value(1).toString().toLower();
            }
            if (!existingCols.contains(col.toLower())) {
                QSqlQuery alter(m_db);
                if (alter.exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table, col, def))) {
                    return true;
                }
            }
            return false;
        };
        addCol("categories", "updated_at", "DATETIME DEFAULT CURRENT_TIMESTAMP");
        addCol("categories", "is_deleted", "INTEGER DEFAULT 0");
        addCol("categories", "is_pinned", "INTEGER DEFAULT 0");
        addCol("categories", "sort_order", "INTEGER DEFAULT 0");
        addCol("categories", "color", "TEXT DEFAULT '#808080'");
        addCol("categories", "parent_id", "INTEGER");
        addCol("categories", "preset_tags", "TEXT");
        addCol("categories", "password", "TEXT");
        addCol("categories", "password_hint", "TEXT");
        addCol("categories", "pinned_sort_order", "INTEGER DEFAULT 0"); // [NEW] 2026-04-20 增加镜像组拉伸排序
        addCol("categories", "sort_mode", "TEXT DEFAULT 'manual'"); // [NEW] 2026-04-xx 增加分类独立排序模式
        addCol("categories", "icon_key", "TEXT DEFAULT 'Data-Classification.svg'"); // [NEW] 分类矢量图标键
    }
    if (!migrateNoteTagsSchema()) return false;
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_content_hash ON notes(content_hash)");
    
    // 2026-05-02 彻底摧毁弱智的 SQL 触发器架构，字数计算权全面收回归 C++ 引擎
    query.exec("DROP TRIGGER IF EXISTS trg_notes_insert_wc");
    query.exec("DROP TRIGGER IF EXISTS trg_notes_update_wc");

    // 试用期与使用次数表
    query.exec("CREATE TABLE IF NOT EXISTS system_config (key TEXT PRIMARY KEY, value TEXT)");
    
    // 初始化试用信息
    QSqlQuery checkLaunch(m_db);
    checkLaunch.prepare("SELECT value FROM system_config WHERE key = 'first_launch_date'");
    if (checkLaunch.exec() && !checkLaunch.next()) {
        QSqlQuery initQuery(m_db);
        initQuery.prepare("INSERT INTO system_config (key, value) VALUES ('first_launch_date', :date)");
        initQuery.bindValue(":date", QDateTime::currentDateTime().toString(Qt::ISODate));
        initQuery.exec();
        
        initQuery.prepare("INSERT INTO system_config (key, value) VALUES ('usage_count', '0')");
        initQuery.exec();
    }

    // [CRITICAL] 待办事项表：扩展支持联动、循环和子任务。
    QString createTodosTable = R"(
        CREATE TABLE IF NOT EXISTS todos (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            content TEXT,
            start_time DATETIME,
            end_time DATETIME,
            status INTEGER DEFAULT 0, -- 0:待办, 1:已完成, 2:已逾期
            reminder_time DATETIME,
            priority INTEGER DEFAULT 0,
            color TEXT,
            note_id INTEGER DEFAULT -1,
            repeat_mode INTEGER DEFAULT 0, -- 0:None, 1:Daily, 2:Weekly, 3:Monthly
            parent_id INTEGER DEFAULT -1,
            progress INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    if (query.exec(createTodosTable)) {
        // 增量升级逻辑
        QSqlQuery upgrade(m_db);
        QStringList newCols = {"note_id", "repeat_mode", "parent_id", "progress"};
        for (const auto& col : newCols) {
            upgrade.exec(QString("ALTER TABLE todos ADD COLUMN %1 INTEGER DEFAULT 0").arg(col));
        }
    }

    // [MODIFIED] 强化版迁移：确保 notes 表字段完整
    {
        auto addCol = [&](const QString& table, const QString& col, const QString& def) {
            QStringList existingCols;
            QSqlQuery check(m_db);
            if (check.exec(QString("PRAGMA table_info(%1)").arg(table))) {
                while (check.next()) existingCols << check.value(1).toString().toLower();
            }
            if (!existingCols.contains(col.toLower())) {
                qDebug() << "[DB] 迁移检测：正在补齐" << table << "表的缺失字段 ->" << col;
                QSqlQuery alter(m_db);
                if (!alter.exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table, col, def))) {
                    qCritical() << "[DB] 严重错误：补齐字段失败 ->" << col << alter.lastError().text();
                    return false;
                } else {
                    qDebug() << "[DB] 迁移成功：字段" << col << "已加入" << table;
                    return true;
                }
            }
            return false;
        };
        addCol("notes", "sort_order", "INTEGER DEFAULT 0");
        addCol("notes", "is_deleted", "INTEGER DEFAULT 0");
        addCol("notes", "updated_at", "DATETIME DEFAULT CURRENT_TIMESTAMP");
        addCol("notes", "last_accessed_at", "DATETIME");
        addCol("notes", "is_pinned", "INTEGER DEFAULT 0");
        addCol("notes", "is_locked", "INTEGER DEFAULT 0");
        addCol("notes", "is_favorite", "INTEGER DEFAULT 0");
        addCol("notes", "source_app", "TEXT");
        addCol("notes", "source_title", "TEXT");
        addCol("notes", "rating", "INTEGER DEFAULT 0");
        addCol("notes", "content_hash", "TEXT");
        addCol("notes", "item_type", "TEXT DEFAULT 'text'");
        addCol("notes", "category_id", "INTEGER");
        addCol("notes", "color", "TEXT DEFAULT '#2d2d2d'");
        addCol("notes", "data_blob", "BLOB");
        addCol("notes", "tags", "TEXT");
        addCol("notes", "title", "TEXT");
        addCol("notes", "content", "TEXT");
        addCol("notes", "created_at", "DATETIME DEFAULT CURRENT_TIMESTAMP");
        addCol("notes", "remark", "TEXT DEFAULT ''"); // [NEW] 备注字段
        addCol("notes", "file_extensions", "TEXT DEFAULT ''"); // [NEW] 2026-04-08 多后缀关联字段
        addCol("notes", "word_count", "INTEGER DEFAULT 0");
        addCol("notes", "color_label", "TEXT DEFAULT ''"); // [NEW] 纯笔记独立手动颜色标记字段
        addCol("notes", "url", "TEXT DEFAULT ''"); // 链接独立字段（原先与备注共用 remark 导致互相覆盖）

        // 索引（放在补列之后，旧库缺列时也能建成）
        query.exec("CREATE INDEX IF NOT EXISTS idx_notes_main_filter ON notes(is_deleted, category_id, is_pinned, updated_at)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_notes_rating ON notes(rating) WHERE rating > 0");
        query.exec("CREATE INDEX IF NOT EXISTS idx_notes_color_label ON notes(color_label) WHERE is_deleted = 0 AND color_label != ''");
        query.exec("CREATE INDEX IF NOT EXISTS idx_notes_recent ON notes(last_accessed_at DESC) WHERE is_deleted = 0");
        query.exec("CREATE INDEX IF NOT EXISTS idx_notes_fav ON notes(is_favorite) WHERE is_favorite = 1 AND is_deleted = 0");
        query.exec("CREATE INDEX IF NOT EXISTS idx_notes_cat_sort ON notes(category_id, is_deleted, sort_order)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_notes_updated ON notes(updated_at DESC)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_notes_type ON notes(item_type)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_cat_parent ON categories(parent_id, is_deleted)");

        // 2026-05-03 按照用户要求：执行 C++ 引擎全量字数精准校准迁移 v3 (修复 0 字逻辑并应用绿色收藏)
        QSqlQuery checkAccurateMigV3(m_db);
        checkAccurateMigV3.exec("SELECT 1 FROM system_config WHERE key = 'migration_20260503_wc_color_fix'");
        if (!checkAccurateMigV3.next()) {
            qDebug() << "[DB] 正在启动全量字数与收藏颜色校准 v3...";
            QSqlQuery fetch(m_db);
            if (fetch.exec("SELECT id, content, item_type, is_favorite FROM notes")) {
                m_db.transaction();
                QSqlQuery upWC(m_db);
                upWC.prepare("UPDATE notes SET word_count = ? WHERE id = ?");
                
                QSqlQuery upColor(m_db);
                upColor.prepare("UPDATE notes SET color = '#2ECC71' WHERE id = ? AND is_favorite = 1");

                while (fetch.next()) {
                    int id = fetch.value(0).toInt();
                    upWC.addBindValue(RapidNotes::WordBucket::calculate(fetch.value(1).toString(), fetch.value(2).toString()));
                    upWC.addBindValue(id);
                    upWC.exec();
                    
                    if (fetch.value(3).toInt() == 1) {
                        upColor.addBindValue(id);
                        upColor.exec();
                    }
                }
                m_db.commit();
                query.exec("INSERT OR REPLACE INTO system_config (key, value) VALUES ('migration_20260503_wc_color_fix', '1')");
                qDebug() << "[DB] 迁移 v3 完成。";
            }
        }

        // 2026-05-01 按照用户要求：执行架构级标签物理化迁移（仅执行一次）
        QSqlQuery checkTagMig(m_db);
        checkTagMig.exec("SELECT 1 FROM system_config WHERE key = 'migration_20260501_tag_normalization'");
        if (!checkTagMig.next()) {
            qDebug() << "[DB] 正在启动标签架构实体化迁移，校准历史遗留数据...";
            QSqlQuery fetchNotes(m_db);
            if (fetchNotes.exec("SELECT id, tags FROM notes WHERE tags IS NOT NULL AND tags != ''")) {
                int count = 0;
                while (fetchNotes.next()) {
                    int noteId = fetchNotes.value(0).toInt();
                    QStringList tagList = RapidNotes::TagCodec::split(fetchNotes.value(1).toString());
                    syncNoteTags(noteId, tagList);
                    count++;
                }
                query.exec("INSERT OR REPLACE INTO system_config (key, value) VALUES ('migration_20260501_tag_normalization', '1')");
                qDebug() << "[DB] 标签实体化迁移完成，共校准" << count << "条笔记。";
            }
        }
        // 2026-05-09 按照用户要求：执行标签格式校准迁移（去除 ", " 中的空格）
        QSqlQuery checkTagSpaceMig(m_db);
        checkTagSpaceMig.exec("SELECT 1 FROM system_config WHERE key = 'migration_20260509_tag_space_cleanup'");
        if (!checkTagSpaceMig.next()) {
            qDebug() << "[DB] 正在启动标签格式校准迁移，将 ', ' 统一替换为 ','...";
            m_db.transaction();
            QSqlQuery up(m_db);
            // 使用 SQLite 的 replace 函数进行全量替换
            up.exec("UPDATE notes SET tags = REPLACE(tags, ', ', ',') WHERE tags LIKE '%, %'");
            m_db.commit();
            query.exec("INSERT OR REPLACE INTO system_config (key, value) VALUES ('migration_20260509_tag_space_cleanup', '1')");
            qDebug() << "[DB] 标签格式校准完成。";
        }

        // 存量数据库一次性清理（清空历史遗留的 BLOB）
        QSqlQuery checkBlobMig(m_db);
        checkBlobMig.exec("SELECT 1 FROM system_config WHERE key = 'migration_20260515_purge_all_blobs'");
        if (!checkBlobMig.next()) {
            qDebug() << "[DB] 正在释放历史 BLOB 占用的体积...";
            m_db.exec("UPDATE notes SET data_blob = NULL WHERE data_blob IS NOT NULL");
            m_db.exec("INSERT OR REPLACE INTO system_config (key, value) VALUES ('migration_20260515_purge_all_blobs', '1')");
        }
    }

    if (!ensureSearchIndex()) return false;
    return true;
}
