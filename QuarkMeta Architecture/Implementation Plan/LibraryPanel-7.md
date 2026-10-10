# Implementation Plan - LibraryPanel Master Architectural Design (`LibraryPanel-7.md`)

This plan provides the complete, robust architectural solution for adding system categories ("全部数据" -1, "未分类" -2, "未标签" -3), displaying dynamic item counts without text truncation or database corruption, handling blank canvas drag-and-drop routing, and fixing hover drop target highlighting in `LibraryPanel`.

---

## 1. Overview & Architectural Principles

### 1.1 Core Objectives
1. **Built-in System Categories**: Inject 3 fixed system categories at the top of the Library tree:
   - **全部数据** (`all_data`, ID: `-1`, color: `#3498db`)
   - **未分类** (`uncategorized`, ID: `-2`, color: `#95a5a6`)
   - **未标签** (`untagged`, ID: `-3`, color: `#7f8c8d`)
2. **Blank Canvas Drag-and-Drop Routing**: Dropping files onto the blank canvas area (`!target.isValid()`) of the Library panel assigns them to **未分类** (ID: `-2`). Dropping onto a specific custom category (`nodeId > 0`) assigns them to that custom category.
3. **Dynamic Item Count Display**: Display item counts for both system categories and custom user categories in the form `Name (Count)` rendered via `LibraryItemDelegate::paint`. The item name stored in `Qt::DisplayRole` remains clean (e.g. `全部数据`, `测试`), preventing SQLite database name corruption during renaming.
4. **Hover Target Highlighting**: Fix `ViewDragDropHelper::isDropTarget` and `DragDropEventFilter` so hovering during drag operations over any targetable node (custom categories `nodeId > 0` or "未分类" `nodeId == -2`) accurately renders a semi-transparent blue highlight background (`#3498db`, alpha 0.35).
5. **Drag-and-Drop Event Lifecycle Protection**: Defer model reloading (`loadLibrary()`) using `QTimer::singleShot(0, ...)` upon `libraryChanged` signals so `m_model->clear()` is never executed synchronously inside an active Qt `dropEvent` call stack, completely eliminating segfault crashes.
6. **System Item Protection**: Prevent right-click context menu popup and inline renaming on system category nodes (`nodeId < 0`).

---

## 2. Modified Files List
1. `src/meta/LibraryDao.cpp` (Update SQL query for `getCategoryPaths(id)` and update `addPathsToCategory` ID validation)
2. `src/ui/ViewDragDropHelper.h` (Update `isDropTarget` parameter signature to accept `const QWidget* widget`)
3. `src/ui/ViewDragDropHelper.cpp` (Fix `isDropTarget` viewport matching and update `isTargetable` condition for drop hover highlighting)
4. `src/ui/LibraryPanel.cpp` (Inject system categories, set item role data, update delegate painting, protect negative IDs, and handle deferred model reload)

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/meta/LibraryDao.cpp`
Support negative IDs (`-1`, `-2`, `-3`) in `LibraryDao::getCategoryPaths(int id)` and update `addPathsToCategory` to allow `id == -2`.

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
        // 全部数据：获取库中所有关联路径与索引文件路径 (去重)
        const char* sql = "SELECT DISTINCT path FROM library_category_paths "
                          "UNION "
                          "SELECT DISTINCT file_path FROM library_item_index;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else if (id == -2) {
        // 未分类：获取关联于 category_id <= 0 的路径，或存在于库中但未归属于任何正数 ID 分类的路径
        const char* sql = "SELECT DISTINCT path FROM library_category_paths WHERE category_id <= 0 "
                          "UNION "
                          "SELECT DISTINCT file_path FROM library_item_index WHERE file_path NOT IN (SELECT path FROM library_category_paths WHERE category_id > 0) "
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
        // 未标签：获取 library_item_index 中 tags 为空/NULL 的文件路径
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

### Change 2: `src/ui/ViewDragDropHelper.h`
Update `isDropTarget` parameter signature to accept `const QWidget* widget`.

```diff
<<<<<<< SEARCH
    static bool isDropTarget(const QAbstractItemView* view, const QModelIndex& index);
=======
    static bool isDropTarget(const QWidget* widget, const QModelIndex& index);
>>>>>>> REPLACE
```

### Change 3: `src/ui/ViewDragDropHelper.cpp`
Check `widget` against `s_hoverView` and its viewport widget, and allow drop hover highlighting for `nodeId > 0` or `nodeId == -2`.

```diff
<<<<<<< SEARCH
                bool isTargetable = !hoverIdx.data(SectionHeaderRole).toBool() &&
                                    ((hoverIdx.data(TypeRole).toString() == "folder") ||
                                     (hoverIdx.data(TypeRole).toString() == "category") ||
                                     (hoverIdx.data(Qt::UserRole + 1).toInt() > 0) ||
                                     hoverIdx.data(Qt::UserRole + 2).toBool());
=======
                int nodeId = hoverIdx.data(Qt::UserRole + 1).toInt();
                bool isTargetable = !hoverIdx.data(SectionHeaderRole).toBool() &&
                                    ((hoverIdx.data(TypeRole).toString() == "folder") ||
                                     (hoverIdx.data(TypeRole).toString() == "category") ||
                                     (nodeId > 0 || nodeId == -2) ||
                                     hoverIdx.data(Qt::UserRole + 2).toBool());
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
bool ViewDragDropHelper::isDropTarget(const QAbstractItemView* view, const QModelIndex& index) {
    return view && s_hoverView == view && s_hoverIndex.isValid() && s_hoverIndex == index;
}
=======
bool ViewDragDropHelper::isDropTarget(const QWidget* widget, const QModelIndex& index) {
    if (!widget || !s_hoverView || !s_hoverIndex.isValid()) return false;
    if (widget == s_hoverView || widget == s_hoverView->viewport()) {
        return s_hoverIndex == index;
    }
    return false;
}
>>>>>>> REPLACE
```

### Change 4: `src/ui/LibraryPanel.cpp`
1) In `LibraryItemDelegate::paint`, append count `(%1)` when `count >= 0`. Pass `option.widget` to `ViewDragDropHelper::isDropTarget`.
2) In `initUi()`, wrap `loadLibrary()` calls in `QTimer::singleShot(0, ...)` to protect drop event stack execution.
3) In `onCategoryClicked()`, check `nodeId != 0` to enable file loading when clicking system items (`-1`, `-2`, `-3`).
4) In `onCategoryContextMenu()`, return early if `nodeId < 0`.
5) In `onPathsDroppedToCategory()`, default `nodeId = -2` when `!target.isValid()` (blank area drop).
6) In `loadLibrary()`, prepend system items, populate item data (`TypeRole`, `IdRole`, count data `Qt::UserRole + 9`), build custom category tree hierarchy, and set `m_isLoading = false;` at the end.

```diff
<<<<<<< SEARCH
#include <QFileInfo>
#include <QDir>
#include <QCursor>
#include <QPainter>
=======
#include <QFileInfo>
#include <QDir>
#include <QCursor>
#include <QPainter>
#include <QLineEdit>
#include <QTimer>
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    bool isDropTarget = index.data(IsDropTargetRole).toBool() ||
                       ViewDragDropHelper::isDropTarget(qobject_cast<const QAbstractItemView*>(option.widget), index);
=======
    bool isDropTarget = index.data(IsDropTargetRole).toBool() ||
                       ViewDragDropHelper::isDropTarget(option.widget, index);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
=======
    QString text = index.data(Qt::DisplayRole).toString();
    int count = index.data(Qt::UserRole + 9).toInt();
    if (count >= 0) {
        text += QString(" (%1)").arg(count);
    }

    painter->setPen((opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE"));
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    connect(&LibraryService::instance(), &LibraryService::libraryChanged, this, [this]() {
        loadLibrary();
    });

    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
        }
    });
=======
    connect(&LibraryService::instance(), &LibraryService::libraryChanged, this, [this]() {
        QTimer::singleShot(0, this, [this]() {
            loadLibrary();
        });
    });

    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
            QTimer::singleShot(0, this, [this]() {
                loadLibrary();
            });
        }
    });
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId > 0) {
        QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
        emit categoryPathsSelected(paths);
    }
}
=======
void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId != 0) {
        QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
        emit categoryPathsSelected(paths);
    }
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();
=======
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId < 0) {
        // 系统分类禁止弹出修改菜单
        return;
    }

    QString curIconKey = index.data(Qt::UserRole + 2).toString();
    QString curColorHex = index.data(Qt::UserRole + 3).toString();
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

    if (m_treeView) m_treeView->expandAll();
    m_isLoading = false;
=======
void LibraryPanel::loadLibrary() {
    if (!m_model) return;
    m_isLoading = true;
    m_model->clear();

    LibraryDao::initTable();

    // 1. 注入 3 个固定系统分类 (带动态计数)
    auto addSystemItem = [this](const QString& name, const QString& iconKey, const QString& colorHex, int sysId) {
        int count = LibraryDao::getCategoryPaths(sysId).size();
        QIcon icon = UiHelper::getIcon(iconKey, QColor(colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, name);
        item->setData("system", TypeRole);
        item->setData(sysId, IdRole);
        item->setData(sysId, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);
        item->setEditable(false);
        m_model->appendRow(item);
    };

    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);
    addSystemItem("未标签", "untagged", "#7f8c8d", -3);

    // 2. 加载用户自定义分类 (带动态计数与完整树构建)
    auto list = LibraryDao::getAllCategories();

    QMap<int, QStandardItem*> itemMap;
    for (const auto& rec : list) {
        int count = LibraryDao::getCategoryPaths(rec.id).size();
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData("category", TypeRole);
        item->setData(rec.id, IdRole);
        item->setData(rec.id, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(count, Qt::UserRole + 9);

        itemMap.insert(rec.id, item);
    }

    for (const auto& rec : list) {
        if (!itemMap.contains(rec.id)) continue;
        QStandardItem* item = itemMap.value(rec.id);

        if (rec.parentId > 0 && itemMap.contains(rec.parentId)) {
            itemMap.value(rec.parentId)->appendRow(item);
        } else {
            m_model->appendRow(item);
        }
    }

    if (m_treeView) m_treeView->expandAll();
    m_isLoading = false;
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Re-build the project using CMake.
2. Launch QuarkMeta and navigate to the "库" (Library) panel.
3. Confirm that "全部数据", "未分类", and "未标签" appear at the top displaying item counts (e.g. `(0)` or `(5)`).
4. Drag a file or folder over a custom category or "未分类" and confirm that the target category displays a smooth blue hover highlight (`#3498db`).
5. Drop the file onto the blank canvas area and confirm it is assigned to "未分类" without crashing.
6. Click custom and system categories and verify that files are displayed in the main content panel.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `LibraryDao::getCategoryPaths(id)` as the unified path query API.
- Reused `ViewDragDropHelper::isDropTarget` and `DragDropEventFilter` across views for drag target hover detection.
- Maintained Clean Architecture: `LibraryDao` handles SQL and domain state, `LibraryPanel` presents UI.

---

## 6. Header API Signature Verification
- `ViewDragDropHelper::isDropTarget(const QWidget* widget, const QModelIndex& index)` in `src/ui/ViewDragDropHelper.h`: Exact match.
- `LibraryDao::getCategoryPaths(int id)` in `src/meta/LibraryDao.h`: Exact match.
- `LibraryPanel::loadLibrary()` in `src/ui/LibraryPanel.h`: Exact match.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `ViewDragDropHelper.cpp` includes `ViewDragDropHelper.h`, `ModelContract.h`, `<QMimeData>`, `<QDrag>`, `<QPixmap>`.
- `LibraryPanel.cpp` includes `LibraryPanel.h`, `UiHelper.h`, `LibraryDao.h`, `LibraryService.h`, `<QLineEdit>`, `<QTimer>`.
- All required types are fully defined.
