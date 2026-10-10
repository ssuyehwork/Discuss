# Implementation Plan - LibraryPanel System Categories & Drag-Drop Handling (`LibraryPanel-6.md`)

This plan details the exact changes needed to add 3 fixed system categories ("全部数据" -1, "未分类" -2, "未标签" -3) to the `LibraryPanel` in `QuarkMeta`, as well as drag-and-drop handling for blank canvas drops (assigning to "未分类" -2).

---

## 1. Overview
In the "Library" panel (`LibraryPanel`):
1. Add 3 built-in fixed system categories:
   - **全部数据** (`all`, ID: `-1`, icon: `all_data`, color: `#3498db`)
   - **未分类** (`uncategorized`, ID: `-2`, icon: `uncategorized`, color: `#95a5a6`)
   - **未标签** (`untagged`, ID: `-3`, icon: `untagged`, color: `#7f8c8d`)
2. Handle Drag and Drop behavior:
   - When dropping items to the **blank area** of the Library panel (`!target.isValid()`), assign the items to the "未分类" system category (ID: `-2`).
   - When dropping items onto a specific user category (`target.isValid()` and `nodeId > 0`), assign them to that target category.
3. Protect system items (ID < 0) from context menu modifications (renaming, deleting, color picking).

---

## 2. Modified Files List
1. `src/meta/LibraryDao.cpp` (Update `getCategoryPaths(int id)` for `id < 0` and update `addPathsToCategory` validation)
2. `src/ui/LibraryPanel.cpp` (Add 3 system items in `loadLibrary()`, protect negative IDs in context menu, and handle blank area drops in `onPathsDroppedToCategory`)

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/meta/LibraryDao.cpp`
Support negative IDs (`-1`, `-2`, `-3`) in `LibraryDao::getCategoryPaths(int id)` and allow `addPathsToCategory` for negative IDs like `-2`.

```diff
<<<<<<< SEARCH
bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0 || paths.isEmpty()) return false;
=======
bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id == 0 || paths.isEmpty()) return false;
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
QStringList LibraryDao::getCategoryPaths(int id) {
    QStringList paths;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return paths;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT path FROM library_category_paths WHERE category_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return paths;

    sqlite3_bind_int(stmt, 1, id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (pStr) paths.append(QString::fromUtf8(pStr));
    }
    sqlite3_finalize(stmt);
    return paths;
}
=======
QStringList LibraryDao::getCategoryPaths(int id) {
    QStringList paths;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id == 0) return paths;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    if (id == -1) {
        // 全部数据：获取库中所有关联路径 (去重)
        const char* sql = "SELECT DISTINCT path FROM library_category_paths;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else if (id == -2) {
        // 未分类：获取仅关联于 category_id <= 0 的路径，或存在于分类路径表中但不在正数 ID 分类中的路径
        const char* sql = "SELECT DISTINCT path FROM library_category_paths WHERE category_id <= 0 "
                          "EXCEPT "
                          "SELECT DISTINCT path FROM library_category_paths WHERE category_id > 0;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else if (id == -3) {
        // 未标签：获取 library_item_index 中 tags 为空/NULL 的文件路径，或库路径下无标签的文件
        const char* sql = "SELECT DISTINCT file_path FROM library_item_index WHERE tags IS NULL OR tags = '';";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else {
        // 常规用户分类 ID > 0
        const char* sql = "SELECT path FROM library_category_paths WHERE category_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return paths;

        sqlite3_bind_int(stmt, 1, id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (pStr) paths.append(QString::fromUtf8(pStr));
        }
        sqlite3_finalize(stmt);
    }
    return paths;
}
>>>>>>> REPLACE
```


### Change 2: `src/ui/LibraryPanel.cpp`
1) Protect negative IDs in `onCategoryContextMenu`.
2) In `loadLibrary()`, prepend the 3 fixed system items.
3) In `onPathsDroppedToCategory`, if `!target.isValid()` (dropped to blank area), default `targetNodeId = -2` ("未分类").

```diff
<<<<<<< SEARCH
    if (!index.isValid()) {
        QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建库分类");
        connect(newCatAct, &QAction::triggered, this, [this]() {
            createAndEditCategory(0);
        });
        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }

    int nodeId = index.data(Qt::UserRole + 1).toInt();
=======
    if (!index.isValid()) {
        QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建库分类");
        connect(newCatAct, &QAction::triggered, this, [this]() {
            createAndEditCategory(0);
        });
        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }

    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId < 0) {
        // 系统分类禁止弹出修改菜单
        return;
    }
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target) {
    if (!target.isValid() || paths.isEmpty()) return;
    int nodeId = target.data(Qt::UserRole + 1).toInt();

    if (nodeId > 0) {
        LibraryService::instance().addPathsToCategory(nodeId, paths);
=======
void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target) {
    if (paths.isEmpty()) return;

    // 拖拽到空白处时默认为“未分类” (-2)，拖到具体分类上则为对应的分类 ID
    int nodeId = -2;
    if (target.isValid()) {
        nodeId = target.data(Qt::UserRole + 1).toInt();
    }

    if (nodeId > 0 || nodeId == -2) {
        LibraryService::instance().addPathsToCategory(nodeId, paths);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();

    LibraryDao::initTable();
    auto list = LibraryDao::getAllCategories();

    QMap<int, QStandardItem*> itemMap;
    for (const auto& rec : list) {
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData(rec.id, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);

        itemMap.insert(rec.id, item);
    }
=======
void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();

    LibraryDao::initTable();

    // 1. 注入 3 个固定系统分类
    auto addSystemItem = [this](const QString& name, const QString& iconKey, const QString& colorHex, int sysId) {
        QIcon icon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, name);
        item->setData(sysId, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(colorHex, Qt::UserRole + 3);
        item->setEditable(false);
        m_model->appendRow(item);
    };

    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);
    addSystemItem("未标签", "untagged", "#7f8c8d", -3);

    // 2. 加载用户自定义分类
    auto list = LibraryDao::getAllCategories();

    QMap<int, QStandardItem*> itemMap;
    for (const auto& rec : list) {
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData(rec.id, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);

        itemMap.insert(rec.id, item);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Re-build the project using CMake.
2. Launch QuarkMeta and switch to the "库" (Library) sidebar panel.
3. Drag files/folders onto the **blank area** of the Library panel and verify they are assigned to "未分类" (`category_id = -2`).
4. Drag files/folders onto a user-created category and verify they are assigned to that specific category.
5. Click each system category ("全部数据", "未分类", "未标签") and verify filtered results.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `LibraryDao::addPathsToCategory(nodeId, paths)` for both positive IDs and `-2` (未分类).
- Reused `LibraryDao::getCategoryPaths(id)` as the unified path query API.
- Maintained Clean Architecture: Domain/Data layer in `LibraryDao`, UI presenting layer in `LibraryPanel`.

---

## 6. Header API Signature Verification
- `LibraryDao::addPathsToCategory(int id, const QStringList& paths)` in `src/meta/LibraryDao.h`: Exact match.
- `LibraryDao::getCategoryPaths(int id)` in `src/meta/LibraryDao.h`: Exact match.
- `LibraryService::getCategoryPaths(int id)` in `src/meta/LibraryService.h`: Exact match.
- `LibraryPanel::loadLibrary()` in `src/ui/LibraryPanel.h`: Exact match.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `LibraryDao.cpp` includes `LibraryDao.h`, `DatabaseManager.h`, `<sqlite3.h>`.
- `LibraryPanel.cpp` includes `LibraryPanel.h`, `UiHelper.h`, `LibraryDao.h`, `LibraryService.h`.
- All required types (`QStringList`, `QStandardItem`, `QIcon`, `QColor`) are fully defined.
