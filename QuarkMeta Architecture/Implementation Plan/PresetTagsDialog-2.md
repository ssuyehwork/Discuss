# Implementation Plan: PresetTagsDialog Preset Tags Persistence Fix

## 1. Overview
When setting auto/preset tags (`设置自动标签`) in `PresetTagsDialog`, adding tags and clicking "保存设置" (Save) did not persist the selected tags to the database. Consequently, re-opening `PresetTagsDialog` for the same category or favorite item resulted in an empty tag container.

### Root Cause:
1. `FavoriteRecord` struct in `FavoriteDao.h` lacked a `presetTags` field.
2. The SQLite database table `favorites` lacked a `preset_tags` column.
3. `FavoriteDao` did not read `preset_tags` during `getAllFavorites()` or provide a update method to write `preset_tags` back to SQLite.
4. `PresetTagsDialog::loadTags()` did not assign `rec.presetTags` to `m_presetTags`.
5. `PresetTagsDialog::onSaveClicked()` only called `accept()` without persisting `m_presetTags` to `FavoriteDao`.

### Solution:
1. Add `QStringList presetTags` to `FavoriteRecord`.
2. Add `preset_tags` column to `favorites` table in SQLite with schema migration in `FavoriteDao::initTable()`.
3. Add `FavoriteDao::updatePresetTags(int id, const QStringList& tags)` and update `getAllFavorites()` to deserialize `preset_tags`.
4. Update `PresetTagsDialog::loadTags()` to populate `m_presetTags` from `rec.presetTags`.
5. Update `PresetTagsDialog::onSaveClicked()` to save `m_presetTags` via `FavoriteDao::updatePresetTags` before accepting.

---

## 2. Modified Files List
1. `src/meta/FavoriteDao.h`
2. `src/meta/FavoriteDao.cpp`
3. `src/ui/PresetTagsDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/meta/FavoriteDao.h`

```
<<<<<<< SEARCH
struct FavoriteRecord {
    int id = 0;
    int parentId = 0;
    FavoriteNodeType nodeType = FavoriteNodeType::RealPath;
    QString path;
    QString name;
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
};

class FavoriteDao {
public:
    static bool initTable();
    static QList<FavoriteRecord> getAllFavorites();
    static int addVirtualCategory(const QString& name, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888");
    static bool addFavorite(const QString& path, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888");
    static bool removeFavoriteById(int id);
    static bool removeFavorite(const QString& path);
    static bool updateFavorite(const QString& path, const QString& iconKey, const QString& colorHex);
    static bool updateFavoriteNode(int id, const QString& name, const QString& iconKey, const QString& colorHex);
    static bool updateNodeParentAndOrder(int id, int newParentId, int sortOrder);
    static bool containsPath(const QString& path);
    static bool updateSortOrders(const QList<QPair<int, int>>& orders);
};
=======
struct FavoriteRecord {
    int id = 0;
    int parentId = 0;
    FavoriteNodeType nodeType = FavoriteNodeType::RealPath;
    QString path;
    QString name;
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    QStringList presetTags;
};

class FavoriteDao {
public:
    static bool initTable();
    static QList<FavoriteRecord> getAllFavorites();
    static int addVirtualCategory(const QString& name, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888");
    static bool addFavorite(const QString& path, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888");
    static bool removeFavoriteById(int id);
    static bool removeFavorite(const QString& path);
    static bool updateFavorite(const QString& path, const QString& iconKey, const QString& colorHex);
    static bool updateFavoriteNode(int id, const QString& name, const QString& iconKey, const QString& colorHex);
    static bool updateNodeParentAndOrder(int id, int newParentId, int sortOrder);
    static bool updatePresetTags(int id, const QStringList& tags);
    static bool containsPath(const QString& path);
    static bool updateSortOrders(const QList<QPair<int, int>>& orders);
};
>>>>>>> REPLACE
```

### 3.2 `src/meta/FavoriteDao.cpp`

```
<<<<<<< SEARCH
    // 数据库增量平滑迁移 (Add parent_id & node_type columns if upgrading from old schema)
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN parent_id INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN node_type INTEGER DEFAULT 1;", nullptr, nullptr, nullptr);

    return true;
=======
    // 数据库增量平滑迁移 (Add parent_id, node_type & preset_tags columns if upgrading from old schema)
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN parent_id INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN node_type INTEGER DEFAULT 1;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN preset_tags TEXT DEFAULT '';", nullptr, nullptr, nullptr);

    return true;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    const char* sql = "SELECT id, parent_id, node_type, path, name, icon_key, color_hex, sort_order FROM favorites ORDER BY sort_order ASC, id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FavoriteRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);
        rec.parentId = sqlite3_column_int(stmt, 1);
        rec.nodeType = static_cast<FavoriteNodeType>(sqlite3_column_int(stmt, 2));
        const char* pathStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        rec.sortOrder = sqlite3_column_int(stmt, 7);

        if (pathStr) rec.path = QString::fromUtf8(pathStr);
        if (nameStr) rec.name = QString::fromUtf8(nameStr);
        if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
        if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);

        list.append(rec);
    }
=======
    const char* sql = "SELECT id, parent_id, node_type, path, name, icon_key, color_hex, sort_order, preset_tags FROM favorites ORDER BY sort_order ASC, id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FavoriteRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);
        rec.parentId = sqlite3_column_int(stmt, 1);
        rec.nodeType = static_cast<FavoriteNodeType>(sqlite3_column_int(stmt, 2));
        const char* pathStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        rec.sortOrder = sqlite3_column_int(stmt, 7);
        const char* tagsStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));

        if (pathStr) rec.path = QString::fromUtf8(pathStr);
        if (nameStr) rec.name = QString::fromUtf8(nameStr);
        if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
        if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);
        if (tagsStr && strlen(tagsStr) > 0) {
            rec.presetTags = QString::fromUtf8(tagsStr).split(',', Qt::SkipEmptyParts);
        }

        list.append(rec);
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool FavoriteDao::updateNodeParentAndOrder(int id, int newParentId, int sortOrder) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE favorites SET parent_id = ?, sort_order = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, newParentId);
    sqlite3_bind_int(stmt, 2, sortOrder);
    sqlite3_bind_int(stmt, 3, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}
=======
bool FavoriteDao::updateNodeParentAndOrder(int id, int newParentId, int sortOrder) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE favorites SET parent_id = ?, sort_order = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, newParentId);
    sqlite3_bind_int(stmt, 2, sortOrder);
    sqlite3_bind_int(stmt, 3, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool FavoriteDao::updatePresetTags(int id, const QStringList& tags) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE favorites SET preset_tags = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string tagsStd = tags.join(",").toStdString();
    sqlite3_bind_text(stmt, 1, tagsStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/PresetTagsDialog.cpp`

```
<<<<<<< SEARCH
void PresetTagsDialog::loadTags() {
    auto list = FavoriteDao::getAllFavorites();
    for (const auto& rec : list) {
        if (rec.id == m_categoryId) {
            m_categoryName = rec.name;
            break;
        }
    }
    m_folderNameEdit->setText(m_categoryName);
}
=======
void PresetTagsDialog::loadTags() {
    auto list = FavoriteDao::getAllFavorites();
    for (const auto& rec : list) {
        if (rec.id == m_categoryId) {
            m_categoryName = rec.name;
            m_presetTags = rec.presetTags;
            break;
        }
    }
    m_folderNameEdit->setText(m_categoryName);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void PresetTagsDialog::onSaveClicked() {
    accept();
}
=======
void PresetTagsDialog::onSaveClicked() {
    FavoriteDao::updatePresetTags(m_categoryId, m_presetTags);
    accept();
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Right-click any category or favorite item in the left sidebar / favorite panel and choose "设置预设标签" (Set Preset Tags).
2. Click inside the tag container and select one or more tags (e.g. "测试", "提示").
3. Click "保存设置" (Save).
4. Re-open "设置预设标签" for the same category item.
5. Verify that all previously saved tags are restored and displayed as pills in the tag container.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Entry**: `FavoriteDao` serves as the Single Source of Truth for favorite items and category metadata in the central SQLite database.
- **Anti-Redundancy**: Reused existing database schema initialization and connection infrastructure in `FavoriteDao`.

---

## 6. Header API Signature Verification
- `FavoriteDao::getAllFavorites()` -> `QList<FavoriteRecord>`
- `FavoriteDao::updatePresetTags(int id, const QStringList& tags)` -> `bool`
- `FavoriteRecord::presetTags` -> `QStringList`
