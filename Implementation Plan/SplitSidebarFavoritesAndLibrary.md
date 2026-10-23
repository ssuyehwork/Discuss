# Implementation Plan - Split Sidebar Container into Dual Tabbed Components ("收藏夹" Bookmarks & "库" Library)

## Overview
This plan refactors the sidebar container (`FavoritePanel` / `FavoriteContainer`) by introducing a top tabbed switcher (`QTabBar` + `QStackedWidget`) that splits the panel into two completely isolated, non-interfering dual-track components:
1. **Tab 1: "库" (Library)**
   - Displays virtual classification categories/groups (`LibraryPanel` backed by `library` table in SQLite `global.db`).
   - Clicking a category item queries associated disk paths (or preset tags) and renders associated files/folders directly in `ContentPanel` via `ContentPanel::loadPaths(paths)`.
   - References original disk paths only; never copies/moves physical files.
2. **Tab 2: "收藏夹" (Bookmarks)**
   - Pure, direct-connect shortcut list (`FavoritePanel` backed by existing `favorites` table in `global.db`).
   - Clicking an item triggers standard navigation (`NavigationService::instance().navigateTo(path)`).

### Strict Isolation Rule (Highest Priority)
- **Zero Cross-Talk**: `FavoriteService` / `FavoriteDao` and `LibraryService` / `LibraryDao` operate on separate SQLite tables (`favorites` vs `library`).
- **No Signal Leaks**: `FavoritePanel` and `LibraryPanel` do not listen to each other's signals or trigger each other's reloads.
- **No Path Forgery**: Category nodes in the Library do not fake file system paths or trigger directory navigation signals.
- **No Shared Global State**: Independent data structures and isolated event Filter/Mediator bindings.

---

## Modified Files List
- `src/meta/LibraryDao.h` *(New)*
- `src/meta/LibraryDao.cpp` *(New)*
- `src/meta/LibraryService.h` *(New)*
- `src/meta/LibraryService.cpp` *(New)*
- `src/ui/LibraryPanel.h` *(New)*
- `src/ui/LibraryPanel.cpp` *(New)*
- `src/ui/SidebarContainerWidget.h` *(New Container replacing single FavoritePanel host)*
- `src/ui/SidebarContainerWidget.cpp` *(New Container)*
- `CMakeLists.txt`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/PanelLayoutManager.h`
- `src/ui/PanelLayoutManager.cpp`
- `src/ui/PanelMediator.h`
- `src/ui/PanelMediator.cpp`
- `resources/style.qss`

---

## Detailed Line-by-Line Changes

### 1. Update `CMakeLists.txt`

```diff
<<<<<<< SEARCH
    src/meta/FavoriteDao.h
    src/meta/FavoriteDao.cpp
    src/meta/FavoriteService.h
    src/meta/FavoriteService.cpp
=======
    src/meta/FavoriteDao.h
    src/meta/FavoriteDao.cpp
    src/meta/FavoriteService.h
    src/meta/FavoriteService.cpp
    src/meta/LibraryDao.h
    src/meta/LibraryDao.cpp
    src/meta/LibraryService.h
    src/meta/LibraryService.cpp
    src/ui/LibraryPanel.h
    src/ui/LibraryPanel.cpp
    src/ui/SidebarContainerWidget.h
    src/ui/SidebarContainerWidget.cpp
>>>>>>> REPLACE
```

---

### 2. Create `src/meta/LibraryDao.h`

```cpp
#pragma once

#include <QString>
#include <QStringList>
#include <QList>

namespace QuarkMeta {

struct LibraryCategoryRecord {
    int id = 0;
    int parentId = 0;
    QString name;
    QString iconKey = "folder_filled";
    QString colorHex = "#888888";
    int sortOrder = 0;
    QStringList presetTags;
    QStringList associatedPaths;
};

class LibraryDao {
public:
    static bool initTable();
    static QList<LibraryCategoryRecord> getAllCategories();
    static int addCategory(const QString& name, int parentId = 0, const QString& iconKey = "folder_filled", const QString& colorHex = "#888888");
    static bool updateCategoryNode(int id, const QString& name, const QString& iconKey, const QString& colorHex);
    static bool updateNodeParentAndOrder(int id, int newParentId, int sortOrder);
    static bool removeCategoryById(int id);
    static bool updateCategoryPaths(int id, const QStringList& paths);
    static bool addPathsToCategory(int id, const QStringList& paths);
    static bool removePathsFromCategory(int id, const QStringList& paths);
    static QStringList getCategoryPaths(int id);
};

} // namespace QuarkMeta
```

---

### 3. Create `src/meta/LibraryDao.cpp`

```cpp
#include "LibraryDao.h"
#include "DatabaseManager.h"
#include <sqlite3.h>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QRandomGenerator>

namespace QuarkMeta {

bool LibraryDao::initTable() {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sqlCat = "CREATE TABLE IF NOT EXISTS library_categories ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "parent_id INTEGER DEFAULT 0, "
                         "name TEXT NOT NULL, "
                         "icon_key TEXT DEFAULT 'folder_filled', "
                         "color_hex TEXT DEFAULT '#888888', "
                         "sort_order INTEGER DEFAULT 0, "
                         "created_at INTEGER);";

    const char* sqlPaths = "CREATE TABLE IF NOT EXISTS library_category_paths ("
                           "category_id INTEGER, "
                           "path TEXT NOT NULL, "
                           "PRIMARY KEY(category_id, path));";

    char* errMsgs = nullptr;
    sqlite3_exec(db, sqlCat, nullptr, nullptr, &errMsgs);
    sqlite3_exec(db, sqlPaths, nullptr, nullptr, &errMsgs);
    return true;
}

QList<LibraryCategoryRecord> LibraryDao::getAllCategories() {
    QList<LibraryCategoryRecord> list;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return list;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "SELECT id, parent_id, name, icon_key, color_hex, sort_order FROM library_categories ORDER BY sort_order ASC, id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LibraryCategoryRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);
        rec.parentId = sqlite3_column_int(stmt, 1);
        const char* nameStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* iconStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* colorStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        rec.sortOrder = sqlite3_column_int(stmt, 5);

        if (nameStr) rec.name = QString::fromUtf8(nameStr);
        if (iconStr) rec.iconKey = QString::fromUtf8(iconStr);
        if (colorStr) rec.colorHex = QString::fromUtf8(colorStr);

        list.append(rec);
    }
    sqlite3_finalize(stmt);

    for (auto& rec : list) {
        rec.associatedPaths = getCategoryPaths(rec.id);
    }

    return list;
}

int LibraryDao::addCategory(const QString& name, int parentId, const QString& iconKey, const QString& colorHex) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return 0;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "INSERT INTO library_categories (parent_id, name, icon_key, color_hex, sort_order, created_at) "
                      "VALUES (?, ?, ?, ?, (SELECT COALESCE(MAX(sort_order), 0) + 1 FROM library_categories), ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    std::string nameStd = name.toStdString();
    std::string iconStd = iconKey.toStdString();
    std::string colorStd = colorHex.toStdString();

    sqlite3_bind_int(stmt, 1, parentId);
    sqlite3_bind_text(stmt, 2, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, QDateTime::currentSecsSinceEpoch());

    int newId = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    }
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return newId;
}

bool LibraryDao::updateCategoryNode(int id, const QString& name, const QString& iconKey, const QString& colorHex) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE library_categories SET name = ?, icon_key = ?, color_hex = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string nameStd = name.toStdString();
    std::string iconStd = iconKey.toStdString();
    std::string colorStd = colorHex.toStdString();

    sqlite3_bind_text(stmt, 1, nameStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, iconStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, colorStd.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool LibraryDao::updateNodeParentAndOrder(int id, int newParentId, int sortOrder) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "UPDATE library_categories SET parent_id = ?, sort_order = ? WHERE id = ?;";
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

bool LibraryDao::removeCategoryById(int id) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "WITH RECURSIVE cnt(x) AS ("
                      "  SELECT ? UNION ALL SELECT id FROM library_categories, cnt WHERE library_categories.parent_id = cnt.x"
                      ") DELETE FROM library_categories WHERE id IN (SELECT x FROM cnt);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, id);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    const char* sqlClean = "DELETE FROM library_category_paths WHERE category_id = ?;";
    if (sqlite3_prepare_v2(db, sqlClean, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return success;
}

bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "INSERT OR IGNORE INTO library_category_paths (category_id, path) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const QString& p : paths) {
        QString cleanP = QDir::toNativeSeparators(QDir::cleanPath(p));
        std::string pStd = cleanP.toStdString();
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}

bool LibraryDao::removePathsFromCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id <= 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "DELETE FROM library_category_paths WHERE category_id = ? AND path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const QString& p : paths) {
        QString cleanP = QDir::toNativeSeparators(QDir::cleanPath(p));
        std::string pStd = cleanP.toStdString();
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}

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

} // namespace QuarkMeta
```

---

### 4. Create `src/meta/LibraryService.h`

```cpp
#pragma once

#include <QObject>
#include "LibraryDao.h"

namespace QuarkMeta {

class LibraryService : public QObject {
    Q_OBJECT

public:
    static LibraryService& instance();

    int createCategory(const QString& name, int parentId = 0);
    bool removeCategory(int id);
    bool addPathsToCategory(int id, const QStringList& paths);
    bool removePathsFromCategory(int id, const QStringList& paths);
    QStringList getCategoryPaths(int id) const;

signals:
    void libraryChanged();

private:
    explicit LibraryService(QObject* parent = nullptr);
    ~LibraryService() override = default;
};

} // namespace QuarkMeta
```

---

### 5. Create `src/meta/LibraryService.cpp`

```cpp
#include "LibraryService.h"

namespace QuarkMeta {

LibraryService& LibraryService::instance() {
    static LibraryService s_instance;
    return s_instance;
}

LibraryService::LibraryService(QObject* parent) : QObject(parent) {
    LibraryDao::initTable();
}

int LibraryService::createCategory(const QString& name, int parentId) {
    int id = LibraryDao::addCategory(name, parentId);
    if (id > 0) {
        emit libraryChanged();
    }
    return id;
}

bool LibraryService::removeCategory(int id) {
    bool ok = LibraryDao::removeCategoryById(id);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}

bool LibraryService::addPathsToCategory(int id, const QStringList& paths) {
    bool ok = LibraryDao::addPathsToCategory(id, paths);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}

bool LibraryService::removePathsFromCategory(int id, const QStringList& paths) {
    bool ok = LibraryDao::removePathsFromCategory(id, paths);
    if (ok) {
        emit libraryChanged();
    }
    return ok;
}

QStringList LibraryService::getCategoryPaths(int id) const {
    return LibraryDao::getCategoryPaths(id);
}

} // namespace QuarkMeta
```

---

### 6. Create `src/ui/LibraryPanel.h`

```cpp
#pragma once

#include <QFrame>
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include "DropTreeView.h"

namespace QuarkMeta {

class LibraryPanel : public QFrame {
    Q_OBJECT

public:
    explicit LibraryPanel(QWidget* parent = nullptr);
    ~LibraryPanel() override = default;

    void loadLibrary();
    void saveLibrary();

signals:
    void categoryPathsSelected(const QStringList& paths);

private slots:
    void onCategoryClicked(const QModelIndex& index);
    void onCategoryContextMenu(const QPoint& pos);
    void onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target);

private:
    void initUi();
    void createAndEditCategory(int parentId = 0);

    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
    bool m_isLoading = false;
    int m_pendingEditNodeId = 0;
};

} // namespace QuarkMeta
```

---

### 7. Create `src/ui/LibraryPanel.cpp`

```cpp
#include "LibraryPanel.h"
#include "UiHelper.h"
#include "ToolTipOverlay.h"
#include "../meta/LibraryDao.h"
#include "../meta/LibraryService.h"
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QHeaderView>
#include <QCursor>

namespace QuarkMeta {

LibraryPanel::LibraryPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("LibraryContainer");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
    loadLibrary();
}

void LibraryPanel::initUi() {
    m_treeView = new DropTreeView(this);
    m_treeView->setObjectName("LibraryTreeView");
    m_treeView->setHeaderHidden(true);
    if (m_treeView->header()) {
        m_treeView->header()->setStretchLastSection(true);
        m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    m_treeView->setIndentation(15);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(true);
    m_treeView->setDropIndicatorShown(true);
    m_treeView->setDefaultDropAction(Qt::MoveAction);
    m_treeView->setDragDropMode(QAbstractItemView::DragDrop);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);

    m_mainLayout->addWidget(m_treeView, 1);

    connect(m_treeView, &QTreeView::clicked, this, &LibraryPanel::onCategoryClicked);
    connect(m_treeView, &QWidget::customContextMenuRequested, this, &LibraryPanel::onCategoryContextMenu);
    connect(m_treeView, &DropTreeView::pathsDropped, this, &LibraryPanel::onPathsDroppedToCategory);

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
}

void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    int nodeId = index.data(Qt::UserRole + 1).toInt();
    if (nodeId > 0) {
        QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
        emit categoryPathsSelected(paths);
    }
}

void LibraryPanel::onCategoryContextMenu(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);

    QMenu menu(this);
    UiHelper::applyMenuStyle(&menu);

    int parentId = 0;
    if (index.isValid()) {
        parentId = index.data(Qt::UserRole + 1).toInt();
    }

    QAction* newCatAct = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "新建库分类");
    connect(newCatAct, &QAction::triggered, this, [this, parentId]() {
        createAndEditCategory(parentId);
    });

    if (index.isValid()) {
        int nodeId = index.data(Qt::UserRole + 1).toInt();
        QAction* renameAct = menu.addAction(UiHelper::getIcon("edit", QColor("#EEEEEE")), "重命名");
        connect(renameAct, &QAction::triggered, this, [this, index]() {
            if (m_treeView) m_treeView->edit(index);
        });

        QAction* removeAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "删除分类");
        connect(removeAct, &QAction::triggered, this, [this, nodeId]() {
            LibraryService::instance().removeCategory(nodeId);
        });
    }

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target) {
    if (!target.isValid()) return;
    int nodeId = target.data(Qt::UserRole + 1).toInt();
    if (nodeId > 0 && !paths.isEmpty()) {
        LibraryService::instance().addPathsToCategory(nodeId, paths);
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已关联 %1 个路径到当前分类").arg(paths.size()), 1500, QColor("#2ecc71"));
    }
}

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
}

void LibraryPanel::createAndEditCategory(int parentId) {
    int newId = LibraryService::instance().createCategory("新建分类", parentId);
    if (newId > 0) {
        loadLibrary();
    }
}

} // namespace QuarkMeta
```

---

### 8. Create `src/ui/SidebarContainerWidget.h` and `SidebarContainerWidget.cpp`

`SidebarContainerWidget` wraps `FavoritePanel` and `LibraryPanel` in a `QStackedWidget` with a top `QTabBar`:

```cpp
#pragma once

#include <QFrame>
#include <QTabBar>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "FavoritePanel.h"
#include "LibraryPanel.h"

namespace QuarkMeta {

class SidebarContainerWidget : public QFrame {
    Q_OBJECT

public:
    explicit SidebarContainerWidget(QWidget* parent = nullptr);
    ~SidebarContainerWidget() override = default;

    FavoritePanel* favoritePanel() const { return m_favoritePanel; }
    LibraryPanel* libraryPanel() const { return m_libraryPanel; }

    QSize minimumSizeHint() const override { return QSize(230, 100); }

private:
    void initUi();

    QVBoxLayout* m_mainLayout = nullptr;
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
    FavoritePanel* m_favoritePanel = nullptr;
    LibraryPanel* m_libraryPanel = nullptr;
};

} // namespace QuarkMeta
```

```cpp
#include "SidebarContainerWidget.h"
#include "UiHelper.h"

namespace QuarkMeta {

SidebarContainerWidget::SidebarContainerWidget(QWidget* parent) : QFrame(parent) {
    setObjectName("SidebarContainerWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(230);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initUi();
}

void SidebarContainerWidget::initUi() {
    QWidget* header = new QWidget(this);
    header->setObjectName("ContainerHeader");
    header->setFixedHeight(32);

    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(5, 0, 5, 0);
    headerLayout->setSpacing(0);

    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->addTab("收藏夹");
    m_tabBar->addTab("库");
    m_tabBar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_tabBar);
    headerLayout->addStretch();
    m_mainLayout->addWidget(header);

    m_stackedWidget = new QStackedWidget(this);
    m_favoritePanel = new FavoritePanel(this);
    m_libraryPanel = new LibraryPanel(this);

    m_stackedWidget->addWidget(m_favoritePanel);
    m_stackedWidget->addWidget(m_libraryPanel);

    m_mainLayout->addWidget(m_stackedWidget, 1);

    connect(m_tabBar, &QTabBar::currentChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);
}

} // namespace QuarkMeta
```

---

### 9. Update `src/ui/PanelMediator.cpp`

Wire `LibraryPanel::categoryPathsSelected` to `ContentPanel::loadPaths` without path forgery:

```diff
<<<<<<< SEARCH
    if (favoritePanel) {
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(favoritePanel, &FavoritePanel::requestLocateFile, this, [this, contentPanel](const QString& path) {
            QFileInfo fi(path);
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->setPendingSelectName(fi.fileName(), false);
            }
            NavigationService::instance().navigateTo(fi.absolutePath());
        });
    }
=======
    if (favoritePanel) {
        connect(favoritePanel, &FavoritePanel::directorySelected, &NavigationService::instance(), [](const QString& path) {
            NavigationService::instance().navigateTo(path);
        });

        connect(favoritePanel, &FavoritePanel::requestLocateFile, this, [this, contentPanel](const QString& path) {
            QFileInfo fi(path);
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->setPendingSelectName(fi.fileName(), false);
            }
            NavigationService::instance().navigateTo(fi.absolutePath());
        });
    }

    if (components.libraryPanel) {
        connect(components.libraryPanel, &LibraryPanel::categoryPathsSelected, this, [this, contentPanel](const QStringList& paths) {
            ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
            if (target) {
                target->loadPaths(paths);
            }
        });
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Recompile standard C++ build target.
2. Launch QuarkMeta and observe top sidebar tab switcher showing "收藏夹" and "库".
3. Verify Tab 1 "收藏夹": Clicking items navigates directly to disk paths.
4. Verify Tab 2 "库": Creating categories and dropping paths renders associated files/folders directly in `ContentPanel` via `loadPaths`, leaving disk paths untouched.
5. Verify total isolation: Modifying items in "库" does not trigger reloads or signals in "收藏夹".

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `ContentPanel::loadPaths` SSOT data loading channel without path forgery.
- Reuses `DatabaseManager::getGlobalDb()` for clean isolated table creation.

---

## Header API Signature Verification
- `ContentPanel::loadPaths(const QStringList &paths, int reqId)`: verified signature in `src/ui/ContentPanel.h`.
- `DatabaseManager::getGlobalDb()`: verified signature in `src/meta/DatabaseManager.h`.

---

## Header Inclusion Chain & Type Completeness Check
- All new files explicitly include necessary headers (`<QWidget>`, `<QTabBar>`, `<QStackedWidget>`, `LibraryService.h`, `LibraryDao.h`).
