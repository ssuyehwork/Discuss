# Implementation Plan - LibraryPanel-1.md

## 1. Overview
This implementation plan refactors `LibraryPanel` so that its `m_treeView` displays **only categories and subcategories**, completely removing the redundant rendering of individual favorited/bound items (files and folders) as tree child nodes in the sidebar.

Key objectives:
1. Pure category tree in `LibraryPanel`: `loadLibrary()` creates `QStandardItem` nodes solely for `library_categories` (and subcategories). No path child items are created under categories.
2. Clean up dead/redundant branches in `LibraryPanel`: Remove path-item detection (`UserRole + 4` / `isPathItem`), path-item context menu actions, and path-item click handling.
3. Drop onto category nodes: Drag-and-drop onto a category node (`DropTreeView::pathsDropped`) continues to bind paths to that category ID and apply preset tags.
4. ContentPanel display: Single-clicking a category node emits `categoryPathsSelected(paths)` to load and display all bound items in the central `ContentPanel`.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/LibraryPanel-1.md`.

## 2. Modified Files List
- `src/ui/LibraryPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/LibraryPanel.cpp`

#### Change 1: Simplify `onCategoryClicked` to only handle Category Nodes
```diff
<<<<<<< SEARCH
void LibraryPanel::onCategoryClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    bool isPathItem = index.data(Qt::UserRole + 4).toBool();

    if (isPathItem) {
        QString path = index.data(Qt::UserRole + 5).toString();
        if (!path.isEmpty()) {
            QFileInfo fi(path);
            if (!fi.isDir()) {
                emit requestLocateFile(path);
            }
        }
        return;
    }

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
    if (nodeId > 0) {
        QStringList paths = LibraryService::instance().getCategoryPaths(nodeId);
        emit categoryPathsSelected(paths);
    }
}
>>>>>>> REPLACE
```

#### Change 2: Remove path item handling from `onCategoryContextMenu`
```diff
<<<<<<< SEARCH
    bool isPathItem = index.data(Qt::UserRole + 4).toBool();
    if (isPathItem) {
        int catId = index.data(Qt::UserRole + 6).toInt();
        QString boundPath = index.data(Qt::UserRole + 5).toString();

        QAction* removePathAct = menu.addAction(UiHelper::getIcon("close", QColor("#EEEEEE")), "从分类中移除此路径");
        connect(removePathAct, &QAction::triggered, this, [catId, boundPath]() {
            LibraryService::instance().removePathsFromCategory(catId, {boundPath});
        });

        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }

    int nodeId = index.data(Qt::UserRole + 1).toInt();
=======
    int nodeId = index.data(Qt::UserRole + 1).toInt();
>>>>>>> REPLACE
```

#### Change 3: Simplify `onPathsDroppedToCategory`
```diff
<<<<<<< SEARCH
void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target) {
    if (!target.isValid() || paths.isEmpty()) return;
    int nodeId = target.data(Qt::UserRole + 1).toInt();
    bool isPathItem = target.data(Qt::UserRole + 4).toBool();
    if (isPathItem) {
        nodeId = target.data(Qt::UserRole + 6).toInt();
    }

    if (nodeId > 0) {
=======
void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target) {
    if (!target.isValid() || paths.isEmpty()) return;
    int nodeId = target.data(Qt::UserRole + 1).toInt();

    if (nodeId > 0) {
>>>>>>> REPLACE
```

#### Change 4: Purge Path Items from `loadLibrary()`
```diff
<<<<<<< SEARCH
    for (const auto& rec : list) {
        QIcon icon = UiHelper::getIcon(rec.iconKey, QColor(rec.colorHex), 18);
        QStandardItem* item = new QStandardItem(icon, rec.name);
        item->setData(rec.id, Qt::UserRole + 1);
        item->setData(rec.iconKey, Qt::UserRole + 2);
        item->setData(rec.colorHex, Qt::UserRole + 3);
        item->setData(false, Qt::UserRole + 4); // false = Category node

        // 挂载绑定的物理路径作为子项 Tree Items
        for (const QString& boundPath : rec.associatedPaths) {
            QFileInfo fi(boundPath);
            QIcon pathIcon = fi.isDir() ? UiHelper::getIcon("folder_filled", QColor("#378ADD"), 16) : ShellIconManager::getFileIcon(boundPath);
            QStandardItem* pathItem = new QStandardItem(pathIcon, fi.fileName().isEmpty() ? boundPath : fi.fileName());
            pathItem->setData(true, Qt::UserRole + 4); // true = Path node
            pathItem->setData(boundPath, Qt::UserRole + 5);
            pathItem->setData(rec.id, Qt::UserRole + 6);
            item->appendRow(pathItem);
        }

        itemMap.insert(rec.id, item);
    }
=======
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

#### Change 5: Remove `itemChanged` redundant checks
```diff
<<<<<<< SEARCH
    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        bool isPathItem = item->data(Qt::UserRole + 4).toBool();
        if (!isPathItem) {
            int nodeId = item->data(Qt::UserRole + 1).toInt();
            if (nodeId > 0) {
                QString name = item->text();
                QString iconKey = item->data(Qt::UserRole + 2).toString();
                QString colorHex = item->data(Qt::UserRole + 3).toString();
                LibraryDao::updateCategoryNode(nodeId, name, iconKey, colorHex);
            }
        }
    });
=======
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
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure CMake build directory if needed.
2. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
3. Run `QuarkMeta`, open the "库" (Library) tab in the left sidebar:
   - Confirm that the tree view displays ONLY category and subcategory nodes with clean icons.
   - Confirm that favorited/bound files and folders NO LONGER appear under category tree nodes in the sidebar.
   - Click a category node and confirm that all bound items load in the central `ContentPanel`.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: `LibraryService::instance().getCategoryPaths(nodeId)` is reused to retrieve paths for ContentPanel display upon clicking a category node.
- **Zero Redundancy**: Dead child-node creation and path-item metadata checks are completely removed.

## 6. Header API Signature Verification
- `LibraryPanel` class signature in `src/ui/LibraryPanel.h` remains 100% frozen and backward compatible.

## 7. Header Inclusion Chain & Type Completeness Check
- No headers removed or added. All Qt types (`QStandardItem`, `QModelIndex`, `QIcon`) and QuarkMeta services are completely declared.
