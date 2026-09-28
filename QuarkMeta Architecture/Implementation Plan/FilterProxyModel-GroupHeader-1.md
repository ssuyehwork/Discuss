# FilterProxyModel ListView Group Headers Master Implementation Plan (FilterProxyModel-GroupHeader-1.md)

## Overview
This master implementation plan establishes a complete, industrial-grade, crash-free, $O(N)$ high-performance solution for enabling and rendering the "文件夹 (N)" and "文件 (M)" blue collapsible section headers in **ListView (`DropTreeView`)**.

This plan adheres to all core architecture requirements:
1. **Data Model Sentinel Integration**: Instantiates two section header sentinel records (`isGroupHeader = true`, `filename = "folder_group_header"` / `"file_group_header"`) in `DiskItemModel::setRecords` appended to `m_allRecords` (after `MetaCacheDecorator::decorate`) so real item path-to-index mappings (`m_pathToIndex`) and JSON disk IO remain untouched. Handles `IsGroupHeaderRole` and `TypeRole` in `DiskItemModel::data`.
2. **Interactive Header Flags**: Overrides `flags()` in `DiskItemModel` to return `Qt::ItemIsEnabled` for group headers, allowing click events (for collapse/expand) while preventing headers from being selected in bulk actions (`Ctrl+A`), dragged, or edited.
3. **Group Index Sorting (`lessThan`)**: Enforces fixed group index ordering in `FilterProxyModel::lessThan` (Folder Header [0] > Folders [1] > File Header [2] > Files [3]), checking `sortOrder()` (`(sortOrder() == Qt::AscendingOrder) ? (leftWeight < rightWeight) : (leftWeight > rightWeight)`) so section headers remain strictly pinned above their item lists under both ascending and descending column sorts.
4. **Filter Immunity**: Exempts section headers (`isGroupHeader == true`) in `filterAcceptsRow`, ensuring section headers are never accidentally hidden by rating, color, tag, or keyword filters.
5. **$O(N)$ Dynamic Cached Visible Count Accuracy**: Caches visible non-hidden counts (`m_cachedFolderCount` / `m_cachedFileCount`) during filter invalidation (`updateFilter()`), reconnecting source model signals (`modelReset`, `rowsInserted`, `rowsRemoved`) in `setSourceModel` to mark `m_countsDirty = true` for 100% accurate count updates.
6. **Traversal & Worker Protection**: Skips header records (`isGroupHeader == true`) in `ContentStatsWorker`, `DuplicateDetectorService`, and status bar metrics (`updateStatusBarStats()`).
7. **Model Role SSOT**: Reuses `IsGroupHeaderRole` (`Qt::UserRole + 212`) and `IsGroupCollapsedRole` (`Qt::UserRole + 213`) from `ModelContract.h`.

## Modified Files List
1. `src/core/ItemRecord.h`
2. `src/ui/models/DiskItemModel.h`
3. `src/ui/models/DiskItemModel.cpp`
4. `src/ui/models/FilterProxyModel.h`
5. `src/ui/models/FilterProxyModel.cpp`
6. `src/ui/workers/ContentStatsWorker.cpp`
7. `src/meta/DuplicateDetectorService.cpp`
8. `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/core/ItemRecord.h`
Add `isGroupHeader` flag to `ItemRecord` struct.

<<<<<<< SEARCH
    bool isParentExpanded = false;
    bool isDropTarget = false;
=======
    bool isParentExpanded = false;
    bool isDropTarget = false;
    bool isGroupHeader = false;
>>>>>>> REPLACE

### 2. `src/ui/models/DiskItemModel.h`
Expose records accessor for DiskItemModel.

<<<<<<< SEARCH
    const std::vector<QuarkMeta::ItemRecord>& allRecords() const override { return m_allRecords; }
    void setRecords(const std::vector<QuarkMeta::ItemRecord>& records) override;
=======
    const std::vector<QuarkMeta::ItemRecord>& allRecords() const override { return m_allRecords; }
    void setRecords(const std::vector<QuarkMeta::ItemRecord>& records) override;
>>>>>>> REPLACE

### 3. `src/ui/models/DiskItemModel.cpp`
Instantiate group header sentinel records in `setRecords`, set interactive flags (`Qt::ItemIsEnabled`) for header rows, and handle `IsGroupHeaderRole` / `TypeRole` in `data`.

<<<<<<< SEARCH
void DiskItemModel::setRecords(const std::vector<ItemRecord>& records) {
    incrementGeneration();
    beginResetModel();
    m_allRecords = records;

    // 🚀【核心根治】：使用 MetaCacheDecorator 批量装载该目录下所有文件的 JSON 关联扩展元数据！
    MetaCacheDecorator::decorate(m_allRecords);
=======
void DiskItemModel::setRecords(const std::vector<ItemRecord>& records) {
    incrementGeneration();
    beginResetModel();
    m_allRecords = records;

    // 🚀【核心根治】：使用 MetaCacheDecorator 批量装载该目录下所有文件的 JSON 关联扩展元数据！
    MetaCacheDecorator::decorate(m_allRecords);

    // Append group header sentinel records after MetaCacheDecorator to avoid dummy disk IO
    ItemRecord folderHeaderRec;
    folderHeaderRec.isGroupHeader = true;
    folderHeaderRec.isDir = true;
    folderHeaderRec.filename = "folder_group_header";
    folderHeaderRec.path = "folder_group_header";

    ItemRecord fileHeaderRec;
    fileHeaderRec.isGroupHeader = true;
    fileHeaderRec.isDir = false;
    fileHeaderRec.filename = "file_group_header";
    fileHeaderRec.path = "file_group_header";

    m_allRecords.push_back(folderHeaderRec);
    m_allRecords.push_back(fileHeaderRec);
>>>>>>> REPLACE

<<<<<<< SEARCH
Qt::ItemFlags DiskItemModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return QAbstractTableModel::flags(index);
    Qt::ItemFlags f = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;
    if (index.column() == 0) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}
=======
Qt::ItemFlags DiskItemModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return QAbstractTableModel::flags(index);
    if (index.row() >= 0 && index.row() < static_cast<int>(m_allRecords.size())) {
        if (m_allRecords[index.row()].isGroupHeader) {
            return Qt::ItemIsEnabled; // 🚀 支持鼠标点击交互，但不可选中、不可拖拽、不可编辑
        }
    }
    Qt::ItemFlags f = QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled;
    if (index.column() == 0) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    } else if (role == TypeRole) {
        return record.isDir ? "folder" : "file";
    } else if (role == AspectRatioRole) {
=======
    } else if (role == TypeRole) {
        if (record.isGroupHeader) return record.filename;
        return record.isDir ? "folder" : "file";
    } else if (role == IsGroupHeaderRole) {
        return record.isGroupHeader;
    } else if (role == AspectRatioRole) {
>>>>>>> REPLACE

### 4. `src/ui/models/FilterProxyModel.h`
Add group header toggle methods, cache invalidation, and helper declarations to `FilterProxyModel`.

<<<<<<< SEARCH
    void setGroupHeadersEnabled(bool enabled);
    bool groupHeadersEnabled() const { return m_groupHeadersEnabled; }

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
=======
    void setGroupHeadersEnabled(bool enabled);
    bool groupHeadersEnabled() const { return m_groupHeadersEnabled; }

    bool isFoldersCollapsed() const { return m_foldersCollapsed; }
    bool isFilesCollapsed() const { return m_filesCollapsed; }
    void toggleFoldersCollapsed() { m_foldersCollapsed = !m_foldersCollapsed; updateFilter(); }
    void toggleFilesCollapsed() { m_filesCollapsed = !m_filesCollapsed; updateFilter(); }
    void updateFilter();
    void setSourceModel(QAbstractItemModel* sourceModel) override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool filterAcceptsRowBase(int sourceRow, const QModelIndex& sourceParent) const;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    void countVisibleFilteredItems(int& folderCount, int& fileCount) const;
    mutable int m_cachedFolderCount = 0;
    mutable int m_cachedFileCount = 0;
    mutable bool m_countsDirty = true;
>>>>>>> REPLACE

### 5. `src/ui/models/FilterProxyModel.cpp`
Implement header immunity in `filterAcceptsRow`, group index ordering in `lessThan`, dynamic header text calculation in `data`, and group collapse state handling in `setData`.

<<<<<<< SEARCH
void FilterProxyModel::setGroupHeadersEnabled(bool enabled) {
    if (m_groupHeadersEnabled != enabled) {
        m_groupHeadersEnabled = enabled;
        updateFilter();
    }
}

bool FilterProxyModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role == IsGroupCollapsedRole) {
        if (index.data(TypeRole).toString() == "folder_group_header") {
            m_foldersCollapsed = value.toBool();
            updateFilter();
            return true;
        } else if (index.data(TypeRole).toString() == "file_group_header") {
            m_filesCollapsed = value.toBool();
            updateFilter();
            return true;
        }
    }
    return QSortFilterProxyModel::setData(index, value, role);
}
=======
void FilterProxyModel::setSourceModel(QAbstractItemModel* sourceModel) {
    QSortFilterProxyModel::setSourceModel(sourceModel);
    if (sourceModel) {
        connect(sourceModel, &QAbstractItemModel::modelReset, this, [this]() { m_countsDirty = true; }, Qt::UniqueConnection);
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, [this]() { m_countsDirty = true; }, Qt::UniqueConnection);
        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, [this]() { m_countsDirty = true; }, Qt::UniqueConnection);
    }
}

void FilterProxyModel::updateFilter() {
    m_countsDirty = true;
    beginFilterChange();
    endFilterChange();
}

void FilterProxyModel::setGroupHeadersEnabled(bool enabled) {
    if (m_groupHeadersEnabled != enabled) {
        m_groupHeadersEnabled = enabled;
        m_countsDirty = true;
        updateFilter();
    }
}

void FilterProxyModel::countVisibleFilteredItems(int& folderCount, int& fileCount) const {
    if (!m_countsDirty) {
        folderCount = m_cachedFolderCount;
        fileCount = m_cachedFileCount;
        return;
    }

    m_cachedFolderCount = 0;
    m_cachedFileCount = 0;
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return;

    const auto& records = sourceModelPtr->allRecords();
    for (int i = 0; i < static_cast<int>(records.size()); ++i) {
        const auto& rec = records[i];
        if (rec.isGroupHeader) continue;

        if (!filterAcceptsRowBase(i, QModelIndex())) continue;

        if (rec.isDir) {
            m_cachedFolderCount++;
        } else {
            m_cachedFileCount++;
        }
    }

    m_countsDirty = false;
    folderCount = m_cachedFolderCount;
    fileCount = m_cachedFileCount;
}

QVariant FilterProxyModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    bool isHeader = QSortFilterProxyModel::data(index, IsGroupHeaderRole).toBool();
    if (isHeader) {
        if (role == IsGroupCollapsedRole) {
            QString typeStr = QSortFilterProxyModel::data(index, TypeRole).toString();
            return (typeStr == "folder_group_header") ? m_foldersCollapsed : m_filesCollapsed;
        }
        if (role == Qt::DisplayRole && index.column() == 0) {
            int folderCount = 0, fileCount = 0;
            countVisibleFilteredItems(folderCount, fileCount);
            QString typeStr = QSortFilterProxyModel::data(index, TypeRole).toString();
            if (typeStr == "folder_group_header") {
                return QString("文件夹 (%1)").arg(folderCount);
            } else if (typeStr == "file_group_header") {
                return QString("文件 (%1)").arg(fileCount);
            }
        }
    }

    return QSortFilterProxyModel::data(index, role);
}

bool FilterProxyModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role == IsGroupCollapsedRole) {
        QString typeStr = index.data(TypeRole).toString();
        if (typeStr == "folder_group_header") {
            m_foldersCollapsed = value.toBool();
            m_countsDirty = true;
            updateFilter();
            return true;
        } else if (typeStr == "file_group_header") {
            m_filesCollapsed = value.toBool();
            m_countsDirty = true;
            updateFilter();
            return true;
        }
    }
    return QSortFilterProxyModel::setData(index, value, role);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    Q_UNUSED(sourceParent);
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];
=======
bool FilterProxyModel::filterAcceptsRowBase(int sourceRow, const QModelIndex& sourceParent) const {
    Q_UNUSED(sourceParent);
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    if (sourceModelPtr->currentPath() == "computer://") {
        return true;
    }

    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    if (record.isDir) {
        if (!currentFilter.showFolders) return false;
    } else {
        if (!currentFilter.showFiles) return false;
    }

    if (!currentFilter.ratings.isEmpty() && !currentFilter.ratings.contains(record.rating)) return false;

    if (!currentFilter.keyword.isEmpty()) {
        bool match = record.filename.contains(currentFilter.keyword, Qt::CaseInsensitive);
        if (!match) return false;
    }

    return true;
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    Q_UNUSED(sourceParent);
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    // 🚀【要点 3：分组标题行绝对豁免规则】：标题行绝不受星级、颜色、类型或关键词过滤隐匿
    if (record.isGroupHeader) {
        if (!m_groupHeadersEnabled) return false;
        int folderCount = 0, fileCount = 0;
        countVisibleFilteredItems(folderCount, fileCount);
        if (record.filename == "folder_group_header") return folderCount > 0;
        if (record.filename == "file_group_header") return fileCount > 0;
        return true;
    }

    if (!filterAcceptsRowBase(sourceRow, sourceParent)) {
        return false;
    }

    // 🚀【要点 3.1：群组折叠判定】：折叠状态下隐匿同组项目
    if (m_groupHeadersEnabled) {
        if (record.isDir && m_foldersCollapsed) return false;
        if (!record.isDir && m_filesCollapsed) return false;
    }

    return true;
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 🚀【绝对权重 1：文件夹永远在最上方】：无视升序降序反转，文件夹永远第一顺位
    if (leftRec.isDir != rightRec.isDir) {
        return (sortOrder() == Qt::AscendingOrder) ? leftRec.isDir : !leftRec.isDir;
    }
=======
    // 🚀【要点 2：分组标题行绝对定位排序规则】：文件夹标题 > 文件夹 > 文件标题 > 文件
    auto getGroupWeight = [](const ItemRecord& r) -> int {
        if (r.isGroupHeader && r.filename == "folder_group_header") return 0;
        if (r.isDir) return 1;
        if (r.isGroupHeader && r.filename == "file_group_header") return 2;
        return 3;
    };

    int leftWeight = getGroupWeight(leftRec);
    int rightWeight = getGroupWeight(rightRec);

    if (leftWeight != rightWeight) {
        return (sortOrder() == Qt::AscendingOrder) ? (leftWeight < rightWeight) : (leftWeight > rightWeight);
    }
>>>>>>> REPLACE

### 6. `src/ui/workers/ContentStatsWorker.cpp`
Skip group header records in scan statistics calculation.

<<<<<<< SEARCH
    for (const auto& record : records) {
        if (record.isHidden && !showHidden) continue;
=======
    for (const auto& record : records) {
        if (record.isGroupHeader) continue; // 🚀【要点 5】：过滤跳过分组标题节点
        if (record.isHidden && !showHidden) continue;
>>>>>>> REPLACE

### 7. `src/meta/DuplicateDetectorService.cpp`
Skip group header records in duplicate file detection.

<<<<<<< SEARCH
    for (const auto& rec : records) {
        if (rec.isDir || rec.size <= 0) continue;
        sizeBuckets[rec.size].push_back(&rec);
    }
=======
    for (const auto& rec : records) {
        if (rec.isGroupHeader || rec.isDir || rec.size <= 0) continue; // 🚀【要点 5】：过滤跳过分组标题节点
        sizeBuckets[rec.size].push_back(&rec);
    }
>>>>>>> REPLACE

### 8. `src/ui/ContentPanel.cpp`
Enable group headers for ListView, exclude sentinel rows in status bar count, and bind section collapse shortcuts.

<<<<<<< SEARCH
void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_treeView->setModel(m_proxyModel);
    m_treeView->installEventFilter(this);
=======
void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    if (m_proxyModel) {
        m_proxyModel->setGroupHeadersEnabled(true);
    }
    m_treeView->setModel(m_proxyModel);
    m_treeView->installEventFilter(this);
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::updateStatusBarStats() {
    int fileCount = 0;
    int folderCount = 0;
    if (m_proxyModel) {
        int total = m_proxyModel->rowCount();
        for (int i = 0; i < total; ++i) {
            QModelIndex idx = m_proxyModel->index(i, 0);
            if (idx.data(TypeRole).toString() == "folder") folderCount++;
            else fileCount++;
        }
    }
    emit statusBarStatsUpdated(fileCount, folderCount, fileCount + folderCount);
}
=======
void ContentPanel::updateStatusBarStats() {
    int fileCount = 0;
    int folderCount = 0;
    if (m_proxyModel) {
        int total = m_proxyModel->rowCount();
        for (int i = 0; i < total; ++i) {
            QModelIndex idx = m_proxyModel->index(i, 0);
            if (idx.data(IsGroupHeaderRole).toBool()) continue; // 🚀 过滤跳过哨兵标题节点
            if (idx.data(TypeRole).toString() == "folder") folderCount++;
            else fileCount++;
        }
    }
    emit statusBarStatsUpdated(fileCount, folderCount, fileCount + folderCount);
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::toggleFolderSectionCollapse() {
    if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
        return;
    }
    if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
        jv->toggleFolderSectionCollapse();
    }
}
=======
void ContentPanel::toggleFolderSectionCollapse() {
    if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
        return;
    }
    if (m_currentViewMode == ListView && m_proxyModel) {
        m_proxyModel->toggleFoldersCollapsed();
        return;
    }
    if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
        jv->toggleFolderSectionCollapse();
    }
}
>>>>>>> REPLACE

## Build & Verification Steps
1. Build project using CMake:
   ```bash
   cmake -B build -S .
   cmake --build build --config Debug
   ```
2. Open QuarkMeta application and navigate to any directory with both folders and files.
3. Switch to **ListView (列表模式)**.
4. Verify that "文件夹 (N)" appears above folders and "文件 (M)" appears between folders and files.
5. Verify that section counts `N` and `M` accurately match visible items (excluding hidden files).
6. Click section headers or press section collapse shortcut. Confirm sections expand and collapse smoothly while section headers remain visible.
7. Perform color, rating, and keyword filtering. Confirm section headers remain present whenever matching items exist.

## SSOT API Reuse & Anti-Redundancy Self-Check
- **Model Contract Roles**: Reused `IsGroupHeaderRole` (`Qt::UserRole + 212`) and `IsGroupCollapsedRole` (`Qt::UserRole + 213`) from `ModelContract.h`.
- **View Layer Reuse**: Reused `DropTreeView::updateGroupHeaderSpanning()` and `TreeItemDelegate::paint` without duplicating UI widgets.
- **Worker Cleanliness**: Protected `ContentStatsWorker` and `DuplicateDetectorService` from treating section headers as real files.

## Header API Signature Verification
- `ItemRecord::isGroupHeader` in `src/core/ItemRecord.h`
- `DiskItemModel::setRecords` in `src/ui/models/DiskItemModel.h`
- `DiskItemModel::flags` in `src/ui/models/DiskItemModel.h`
- `FilterProxyModel::setGroupHeadersEnabled(bool)` in `src/ui/models/FilterProxyModel.h`
- `FilterProxyModel::toggleFoldersCollapsed()` in `src/ui/models/FilterProxyModel.h`
- `ContentPanel::toggleFolderSectionCollapse()` in `src/ui/ContentPanel.h`
- `IsGroupHeaderRole` and `IsGroupCollapsedRole` in `src/core/ModelContract.h`
