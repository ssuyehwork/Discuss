# Implementation Plan: FavoritePanel Hierarchical Tree & Virtual Category Architecture

## 1. Overview
This implementation plan upgrades `FavoritePanel` from a flat single-level list into a multi-tiered, hierarchical tree structure (`QTreeView` + `QStandardItemModel`). It introduces the ability for users to dynamically create **Virtual Categories** (logical folders that exist only in QuarkMeta metadata, not on the physical file system).

### Key Technical Improvements:
- **Database Extension**: Upgrades SQLite `favorites` table with `parent_id` (default 0 for root) and `node_type` (0 = VirtualCategory, 1 = RealPath) with zero-data-loss ALTER TABLE migration.
- **DAO & Service Layer SSOT**: Extends `FavoriteDao` and `FavoriteService` to handle hierarchical nodes, tree node creation/removal, and parent-child relationship management.
- **Hierarchical UI & Tree Model**: Refactors `FavoritePanel` to populate a tree model according to `parent_id`, supporting expanding/collapsing, custom SVG icons and colors for virtual categories, and context menu commands ("New Category", "Rename", "Change Icon", "Set Color", "Remove").
- **Drag & Drop Re-parenting**: Supports dragging real files/folders or categories onto virtual category nodes to re-parent them dynamically.

---

## 2. Modified Files List
1. `src/meta/FavoriteDao.h`
2. `src/meta/FavoriteDao.cpp`
3. `src/meta/FavoriteService.h`
4. `src/meta/FavoriteService.cpp`
5. `src/ui/FavoritePanel.h`
6. `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/meta/FavoriteDao.h`
```
<<<<<<< SEARCH
struct FavoriteRecord {
    int id = 0;
    QString path;
    QString name;
    QString iconKey = "folder";
    QString colorHex = "#888888";
    int sortOrder = 0;
};

class FavoriteDao {
public:
    static bool initTable();
    static QList<FavoriteRecord> getAllFavorites();
    static bool addFavorite(const QString& path, const QString& iconKey = "folder", const QString& colorHex = "#888888");
    static bool removeFavorite(const QString& path);
    static bool updateFavorite(const QString& path, const QString& iconKey, const QString& colorHex);
    static bool containsPath(const QString& path);
    static bool updateSortOrders(const QList<QPair<QString, int>>& orders);
};
=======
enum class FavoriteNodeType {
    VirtualCategory = 0,
    RealPath = 1
};

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
>>>>>>> REPLACE
```

---

### File 2: `src/meta/FavoriteDao.cpp`
```
<<<<<<< SEARCH
bool FavoriteDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "CREATE TABLE IF NOT EXISTS favorites ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "path TEXT UNIQUE NOT NULL, "
                      "name TEXT, "
                      "icon_key TEXT DEFAULT 'folder', "
                      "color_hex TEXT DEFAULT '#888888', "
                      "sort_order INTEGER DEFAULT 0, "
                      "created_at INTEGER);";

    char* errMsgs = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        if (errMsgs) sqlite3_free(errMsgs);
        return false;
    }
    return true;
}
=======
bool FavoriteDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "CREATE TABLE IF NOT EXISTS favorites ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "parent_id INTEGER DEFAULT 0, "
                      "node_type INTEGER DEFAULT 1, "
                      "path TEXT UNIQUE NOT NULL, "
                      "name TEXT, "
                      "icon_key TEXT DEFAULT 'folder_filled', "
                      "color_hex TEXT DEFAULT '#888888', "
                      "sort_order INTEGER DEFAULT 0, "
                      "created_at INTEGER);";

    char* errMsgs = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        if (errMsgs) sqlite3_free(errMsgs);
        return false;
    }

    // 数据库增量平滑迁移 (Add parent_id & node_type columns if upgrading from old schema)
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN parent_id INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE favorites ADD COLUMN node_type INTEGER DEFAULT 1;", nullptr, nullptr, nullptr);

    return true;
}
>>>>>>> REPLACE
```

---

### File 3: `src/meta/FavoriteService.h`
```
<<<<<<< SEARCH
    bool isFavorite(const QString& path) const;
    bool addFavorite(const QString& path);
    bool removeFavorite(const QString& path);
    bool toggleFavorite(const QString& path);
=======
    bool isFavorite(const QString& path) const;
    bool addFavorite(const QString& path, int parentId = 0);
    int addVirtualCategory(const QString& name, int parentId = 0);
    bool removeFavorite(const QString& path);
    bool removeFavoriteById(int id);
    bool toggleFavorite(const QString& path);
>>>>>>> REPLACE
```

---

### File 4: `src/ui/FavoritePanel.h`
```
<<<<<<< SEARCH
    void removeFavoriteItem(const QString& path);
    void addFavoriteItem(const QString& path);
    void loadFavorites();
    void saveFavorites();
=======
    void removeFavoriteItem(const QString& path);
    void addFavoriteItem(const QString& path, int parentId = 0);
    void addVirtualCategory(const QString& name, int parentId = 0);
    void loadFavorites();
    void saveFavorites();
>>>>>>> REPLACE
```

---

### File 5: `src/ui/FavoritePanel.cpp`
```
<<<<<<< SEARCH
void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_isLoading = true;
    m_favoriteModel->clear();

    FavoriteDao::initTable();
    auto list = FavoriteDao::getAllFavorites();

    QStringList pathsToExtract;

    for (const auto& rec : list) {
        QString nativePath = QDir::toNativeSeparators(QDir::cleanPath(rec.path));
        QFileInfo fi(nativePath);
        if (!fi.exists()) continue;

        QColor itemColor = QColor(rec.colorHex);
        if (!itemColor.isValid()) itemColor = QColor("#888888");

        QString iconKey = rec.iconKey.isEmpty() ? "folder_filled" : rec.iconKey;
        if (iconKey == "folder") iconKey = "folder_filled";

        bool isDir = fi.isDir();
        QIcon icon;

        if (isDir) {
            icon = UiHelper::getIcon(iconKey, itemColor, 18);
        } else {
            icon = ShellIconManager::getFileIcon(nativePath);
            QString ext = fi.suffix().toLower();
            if (UiHelper::isGraphicsFile(ext) || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf" || ext == "svg") {
                pathsToExtract << nativePath;
            }
        }

        QStandardItem* item = new QStandardItem(icon, rec.name.isEmpty() ? fi.fileName() : rec.name);
        item->setData(nativePath, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(isDir, Qt::UserRole + 4);
        item->setData(false, Qt::UserRole + 5);

        m_favoriteModel->appendRow(item);
    }

    m_isLoading = false;

    if (!pathsToExtract.isEmpty()) {
        QPointer<FavoritePanel> weakThis(this);
        for (const QString& path : pathsToExtract) {
            (void)QtConcurrent::run([weakThis, path]() {
                if (!weakThis) return;
                QImage img = DiskMediaExtractor::getCapsuleThumbnail(path, 128);
                if (!img.isNull()) {
                    QPixmap pix = QPixmap::fromImage(img);
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, path, pix]() {
                        if (weakThis) {
                            weakThis->updateItemThumbnail(path, pix);
                        }
                    }, Qt::QueuedConnection);
                }
            });
        }
    }
}
=======
void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_isLoading = true;
    m_favoriteModel->clear();

    FavoriteDao::initTable();
    auto list = FavoriteDao::getAllFavorites();

    QStringList pathsToExtract;
    QMap<int, QStandardItem*> itemMap;

    // First Pass: Create QStandardItems
    for (const auto& rec : list) {
        bool isVirtual = (rec.nodeType == FavoriteNodeType::VirtualCategory);
        QString nativePath = isVirtual ? rec.path : QDir::toNativeSeparators(QDir::cleanPath(rec.path));

        if (!isVirtual) {
            QFileInfo fi(nativePath);
            if (!fi.exists()) continue;
        }

        QColor itemColor = QColor(rec.colorHex);
        if (!itemColor.isValid()) itemColor = QColor("#888888");

        QString iconKey = rec.iconKey.isEmpty() ? "folder_filled" : rec.iconKey;
        if (iconKey == "folder") iconKey = "folder_filled";

        bool isDir = isVirtual ? true : QFileInfo(nativePath).isDir();
        QIcon icon;

        if (isDir) {
            icon = UiHelper::getIcon(iconKey, itemColor, 18);
        } else {
            icon = ShellIconManager::getFileIcon(nativePath);
            QString ext = QFileInfo(nativePath).suffix().toLower();
            if (UiHelper::isGraphicsFile(ext) || ext == "psd" || ext == "ai" || ext == "eps" || ext == "pdf" || ext == "svg") {
                pathsToExtract << nativePath;
            }
        }

        QString displayName = rec.name;
        if (displayName.isEmpty() && !isVirtual) {
            displayName = QFileInfo(nativePath).fileName();
        }

        QStandardItem* item = new QStandardItem(icon, displayName);
        item->setData(nativePath, Qt::UserRole + 1);
        item->setData(iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(isDir, Qt::UserRole + 4);
        item->setData(false, Qt::UserRole + 5);
        item->setData(rec.id, Qt::UserRole + 6);
        item->setData(isVirtual, Qt::UserRole + 7);
        item->setData(rec.parentId, Qt::UserRole + 8);

        itemMap.insert(rec.id, item);
    }

    // Second Pass: Build Tree Hierarchy
    for (const auto& rec : list) {
        if (!itemMap.contains(rec.id)) continue;
        QStandardItem* item = itemMap.value(rec.id);

        if (rec.parentId > 0 && itemMap.contains(rec.parentId)) {
            itemMap.value(rec.parentId)->appendRow(item);
        } else {
            m_favoriteModel->appendRow(item);
        }
    }

    if (m_favoriteView) {
        m_favoriteView->expandAll();
    }

    m_isLoading = false;

    if (!pathsToExtract.isEmpty()) {
        QPointer<FavoritePanel> weakThis(this);
        for (const QString& path : pathsToExtract) {
            (void)QtConcurrent::run([weakThis, path]() {
                if (!weakThis) return;
                QImage img = DiskMediaExtractor::getCapsuleThumbnail(path, 128);
                if (!img.isNull()) {
                    QPixmap pix = QPixmap::fromImage(img);
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, path, pix]() {
                        if (weakThis) {
                            weakThis->updateItemThumbnail(path, pix);
                        }
                    }, Qt::QueuedConnection);
                }
            });
        }
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Step 1: Build Verification
Run standard CMake build:
```bash
cmake --build build
```

### Step 2: Functional Verification
1. **Right-click Blank Area in FavoritePanel**: Click "新建虚拟分类" -> Observe new Virtual Category folder created at root level.
2. **Right-click Virtual Category**: Click "新建子分类" -> Observe nested child category appended under the parent node.
3. **Change Icon & Color**: Choose a color from `ColorStripPicker` -> Observe live preview update on virtual category icon.
4. **Drag and Drop**: Drag a file or folder from `ContentPanel` onto a Virtual Category -> Observe item nested inside the Virtual Category.
5. **Database Persistence**: Restart application -> Confirm tree hierarchy, virtual categories, custom icons, and colors are 100% restored from SQLite.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Access**: All favorite additions, deletions, and checks strictly route through `FavoriteService::instance()` and `FavoriteDao`.
- **Anti-Redundancy**: No parallel local lists created. `m_favoriteModel` is populated strictly from SQLite `FavoriteDao::getAllFavorites()` with `parent_id` hierarchy mapping.

---

## 6. Header API Signature Verification Table

| Calling Class | Called Class / Method | Header File Source | Verified Signature |
| :--- | :--- | :--- | :--- |
| `FavoritePanel` | `FavoriteDao::initTable()` | `src/meta/FavoriteDao.h` | `static bool initTable();` |
| `FavoritePanel` | `FavoriteDao::getAllFavorites()` | `src/meta/FavoriteDao.h` | `static QList<FavoriteRecord> getAllFavorites();` |
| `FavoritePanel` | `FavoriteService::addVirtualCategory()` | `src/meta/FavoriteService.h` | `int addVirtualCategory(const QString& name, int parentId = 0);` |
| `FavoritePanel` | `FavoriteService::removeFavoriteById()` | `src/meta/FavoriteService.h` | `bool removeFavoriteById(int id);` |
| `FavoritePanel` | `UiHelper::getIcon()` | `src/ui/UiHelper.h` | `static QIcon getIcon(const QString& key, const QColor& color, int size = 18);` |
| `FavoritePanel` | `UiHelper::applyMenuStyle()` | `src/ui/UiHelper.h` | `static void applyMenuStyle(QWidget* menu);` |
