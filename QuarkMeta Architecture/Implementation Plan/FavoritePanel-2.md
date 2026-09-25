# Implementation Plan: FavoritePanel Complete Context Menu & Inline Editing Integration

## 1. Overview
This implementation plan fully aligns `FavoritePanel` with the standard 6 context menu actions and inline editing workflow requested:
1. **New Folder (`onCreateCategory`)**: Creates a root virtual category and automatically invokes inline name editing.
2. **New Subfolder (`onCreateSubCategory`)**: Creates a child category under the current node and automatically invokes inline name editing.
3. **Preset Tags (`onSetPresetTags`)**: Adds "设置预设标签" menu item (with TODO overlay notice).
4. **Rename (`onRenameCategory`)**: Triggers `m_favoriteView->edit(index)` to open the inline name editor.
5. **Delete (`onDeleteCategory`)**: Cascading removal of categories or un-favoriting files.
6. **Sort (`onSortByName`)**: "排列" submenu supporting `按名称 (A→Z)` and `按名称 (Z→A)` recursive tree sorting.

---

## 2. Modified Files List
1. `src/ui/FavoritePanel.h`
2. `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/FavoritePanel.h`
```
<<<<<<< SEARCH
    void initUi();
    void updateItemThumbnail(const QString& path, const QPixmap& pix);

    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
    bool m_isLoading = false;
=======
    void initUi();
    void updateItemThumbnail(const QString& path, const QPixmap& pix);
    void createAndEditCategory(int parentId = 0);
    void sortItemsByName(bool ascending);

    QVBoxLayout* m_mainLayout = nullptr;
    DropTreeView* m_favoriteView = nullptr;
    QStandardItemModel* m_favoriteModel = nullptr;
    bool m_isLoading = false;
    int m_pendingEditNodeId = 0;
>>>>>>> REPLACE
```

---

### File 2: `src/ui/FavoritePanel.cpp`
```
<<<<<<< SEARCH
    connect(m_favoriteModel, &QStandardItemModel::rowsMoved, this, updateFavAndSave, Qt::QueuedConnection);
    connect(m_favoriteModel, &QStandardItemModel::rowsInserted, this, updateFavAndSave, Qt::QueuedConnection);
    connect(m_favoriteModel, &QStandardItemModel::rowsRemoved, this, updateFavAndSave, Qt::QueuedConnection);

    connect(&FavoriteService::instance(), &FavoriteService::favoriteChanged, this, [this](const QString& path, bool isFav) {
=======
    connect(m_favoriteModel, &QStandardItemModel::rowsMoved, this, updateFavAndSave, Qt::QueuedConnection);
    connect(m_favoriteModel, &QStandardItemModel::rowsInserted, this, updateFavAndSave, Qt::QueuedConnection);
    connect(m_favoriteModel, &QStandardItemModel::rowsRemoved, this, updateFavAndSave, Qt::QueuedConnection);

    connect(m_favoriteModel, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 6).toInt();
        if (nodeId > 0) {
            QString name = item->text();
            QString iconKey = item->data(Qt::UserRole + 2).toString();
            QString colorHex = item->data(Qt::UserRole + 3).toString();
            FavoriteDao::updateFavoriteNode(nodeId, name, iconKey, colorHex);
        }
    });

    connect(&FavoriteService::instance(), &FavoriteService::favoriteChanged, this, [this](const QString& path, bool isFav) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Right-click blank area or folder -> Click "新建文件夹" or "新建子文件夹".
2. Confirm new category folder is inserted, expanded, and inline name editor opens automatically.
3. Type new name and press Enter -> Confirm new name is saved immediately into SQLite.
4. Click "排列 -> 按名称 (A→Z)" -> Confirm items sort alphabetically and order is saved into SQLite.
