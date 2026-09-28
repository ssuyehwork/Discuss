# FilterProxyModel ListView Group Headers Implementation Plan (FilterProxyModel-GroupHeader-1.md)

## Overview
This implementation plan provides an enhanced, crash-free, and stable architectural solution for enabling and displaying "文件夹 (N)" and "文件 (M)" blue section headers with collapsible/expandable vector arrow widgets in **ListView (列表模式)**.

This iteration builds upon `FilterProxyModel-GroupHeader.md` and addresses all runtime safety & persistence requirements:
1. **Full Filtering Logic Preservation**: Preserves 100% of the pre-existing filter conditions (ratings, colors, types, dates, tags, keywords, duplicate check) inside `filterAcceptsRowBase`.
2. **Recursion Safety**: Calculates header counts via direct `sourceModel()->allRecords()` scanning and `filterAcceptsRowBase()` without invoking recursive virtual index queries.
3. **Header Row Persistence**: Decouples section header count calculation (`calculateBaseCounts`) from collapse state (`m_foldersCollapsed` / `m_filesCollapsed`). Section headers stay visible and fully interactive when collapsed.
4. **Safe Proxy Indexing**: Correctly delegates data queries for non-header items to `QSortFilterProxyModel::data()`, preserving Qt's internal model index contracts.

## Modified Files List
1. `src/ui/models/FilterProxyModel.h`
2. `src/ui/models/FilterProxyModel.cpp`
3. `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/models/FilterProxyModel.h`
Add group header toggle methods, base acceptance filter, and virtual row mapping declarations in `FilterProxyModel`.

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

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool filterAcceptsRowBase(int sourceRow, const QModelIndex& sourceParent) const;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    void calculateBaseCounts(int& folderCount, int& fileCount) const;
>>>>>>> REPLACE

### 2. `src/ui/models/FilterProxyModel.cpp`
Implement non-recursive count calculation, complete base filtering, stable header index mapping, and single-definition `setData`.

<<<<<<< SEARCH
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
void FilterProxyModel::calculateBaseCounts(int& folderCount, int& fileCount) const {
    folderCount = 0;
    fileCount = 0;
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return;

    const auto& records = sourceModelPtr->allRecords();
    for (int i = 0; i < static_cast<int>(records.size()); ++i) {
        if (!filterAcceptsRowBase(i, QModelIndex())) continue;
        if (records[i].isDir) {
            folderCount++;
        } else {
            fileCount++;
        }
    }
}

int FilterProxyModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    int baseCount = QSortFilterProxyModel::rowCount(parent);
    if (!m_groupHeadersEnabled) return baseCount;

    int folderCount = 0, fileCount = 0;
    calculateBaseCounts(folderCount, fileCount);

    int extraRows = 0;
    if (folderCount > 0) extraRows++;
    if (fileCount > 0) extraRows++;

    return baseCount + extraRows;
}

QModelIndex FilterProxyModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() || row < 0 || row >= rowCount(parent)) {
        return QModelIndex();
    }
    if (!m_groupHeadersEnabled) {
        return QSortFilterProxyModel::index(row, column, parent);
    }
    return createIndex(row, column);
}

QModelIndex FilterProxyModel::parent(const QModelIndex& child) const {
    Q_UNUSED(child);
    return QModelIndex();
}

QVariant FilterProxyModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    if (!m_groupHeadersEnabled) {
        return QSortFilterProxyModel::data(index, role);
    }

    int row = index.row();
    int folderCount = 0, fileCount = 0;
    calculateBaseCounts(folderCount, fileCount);

    bool hasFolderHeader = (folderCount > 0);
    bool hasFileHeader = (fileCount > 0);

    int folderHeaderRow = hasFolderHeader ? 0 : -1;
    int fileHeaderRow = -1;

    if (hasFileHeader) {
        int folderVisibleRows = m_foldersCollapsed ? 0 : folderCount;
        fileHeaderRow = hasFolderHeader ? (1 + folderVisibleRows) : 0;
    }

    if (row == folderHeaderRow) {
        if (role == IsGroupHeaderRole) return true;
        if (role == IsGroupCollapsedRole) return m_foldersCollapsed;
        if (role == TypeRole) return "folder_group_header";
        if (role == Qt::DisplayRole && index.column() == 0) return QString("文件夹 (%1)").arg(folderCount);
        return QVariant();
    }

    if (row == fileHeaderRow) {
        if (role == IsGroupHeaderRole) return true;
        if (role == IsGroupCollapsedRole) return m_filesCollapsed;
        if (role == TypeRole) return "file_group_header";
        if (role == Qt::DisplayRole && index.column() == 0) return QString("文件 (%1)").arg(fileCount);
        return QVariant();
    }

    // Map virtual row to base proxy index
    int baseRow = row;
    if (hasFolderHeader && row > folderHeaderRow) {
        baseRow--;
    }
    if (hasFileHeader && fileHeaderRow != -1 && row > fileHeaderRow) {
        baseRow--;
    }

    QModelIndex baseIdx = QSortFilterProxyModel::index(baseRow, index.column(), parent());
    return QSortFilterProxyModel::data(baseIdx, role);
}

bool FilterProxyModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role == IsGroupCollapsedRole) {
        QString typeStr = index.data(TypeRole).toString();
        if (typeStr == "folder_group_header") {
            m_foldersCollapsed = value.toBool();
            updateFilter();
            return true;
        } else if (typeStr == "file_group_header") {
            m_filesCollapsed = value.toBool();
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

    // 🚀【此电脑根路径豁免准则】：当加载“此电脑”(computer://)盘符列表时，盘符属于系统硬件层介质，100% 必须始终放行显示，不受常规文件夹/文件显隐或星级筛选器的过滤关断！
    if (sourceModelPtr->currentPath() == "computer://") {
        return true;
    }

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    bool isTrashView = contentPanel && (contentPanel->getCurrentCategoryType() == "trash");

    // 0. 隐藏属性过滤
    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    // 1. 文件夹与文件显隐控制 (showFolders/showFiles 为顶栏切换按钮的绝对关断最高优先级)
    if (!isTrashView) {
        if (record.isDir) {
            if (!currentFilter.showFolders) return false;
        } else {
            if (!currentFilter.showFiles) return false;
        }
    }
=======
bool FilterProxyModel::filterAcceptsRowBase(int sourceRow, const QModelIndex& sourceParent) const {
    Q_UNUSED(sourceParent);
    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    // 🚀【此电脑根路径豁免准则】：当加载“此电脑”(computer://)盘符列表时，盘符属于系统硬件层介质，100% 必须始终放行显示，不受常规文件夹/文件显隐或星级筛选器的过滤关断！
    if (sourceModelPtr->currentPath() == "computer://") {
        return true;
    }

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    bool isTrashView = contentPanel && (contentPanel->getCurrentCategoryType() == "trash");

    // 0. 隐藏属性过滤
    if (record.isHidden && !currentFilter.showHidden) {
        return false;
    }

    // 1. 文件夹与文件显隐控制 (showFolders/showFiles 为顶栏切换按钮的绝对关断最高优先级)
    if (!isTrashView) {
        if (record.isDir) {
            if (!currentFilter.showFolders) return false;
        } else {
            if (!currentFilter.showFiles) return false;
        }
    }

    // 2. 评级过滤
    if (!currentFilter.ratings.isEmpty()) {
        if (!currentFilter.ratings.contains(record.rating)) return false;
    }

    // 3. 颜色标记过滤
    if (!currentFilter.colors.isEmpty()) {
        bool matchColor = false;
        static const QMap<QString, QString> s_colorHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };

        for (const QString& colName : currentFilter.colors) {
            if (colName == "无色标" || colName.isEmpty()) {
                if (record.manualColor.isEmpty() && record.autoColor.isEmpty()) {
                    matchColor = true;
                    break;
                }
            } else {
                QString targetHex = s_colorHexMap.value(colName, colName);
                if (record.manualColor.compare(targetHex, Qt::CaseInsensitive) == 0 ||
                    record.manualColor.contains(colName, Qt::CaseInsensitive) ||
                    record.autoColor.contains(colName, Qt::CaseInsensitive)) {
                    matchColor = true;
                    break;
                }
            }
        }
        if (!matchColor) return false;
    }

    // 4. 类型过滤
    if (!currentFilter.types.isEmpty() || !currentFilter.typeFilterText.isEmpty()) {
        QString type = record.isDir ? "folder" : "file";
        QString ext = record.suffix.toUpper();
        bool matchType = false;

        if (!currentFilter.typeFilterText.isEmpty()) {
            QString searchText = currentFilter.typeFilterText.trimmed();
            if (searchText == "文件夹" || searchText.toLower() == "folder") {
                if (type == "folder") matchType = true;
            } else if (searchText == "空文件夹") {
                if (type == "folder" && record.isEmpty) matchType = true;
            } else {
                if (ext.contains(searchText.toUpper())) matchType = true;
            }
            if (!matchType) return false;
        }

        if (!currentFilter.types.isEmpty()) {
            matchType = false;
            for (const QString& fType : currentFilter.types) {
                if (fType == "folder") {
                    if (type == "folder") { matchType = true; break; }
                } else if (fType == "file") {
                    if (type != "folder") { matchType = true; break; }
                } else if (fType == "空文件夹") {
                    if (type == "folder" && record.isEmpty) { matchType = true; break; }
                } else {
                    if (ext == fType.toUpper()) { matchType = true; break; }
                }
            }
            if (!matchType) return false;
        }
    }

    // 5. 日期过滤
    if (!currentFilter.createDates.isEmpty() || !currentFilter.createDateFilterText.isEmpty()) {
        QString dStr = QDateTime::fromMSecsSinceEpoch(record.ctime).date().toString("dd-MM-yyyy");
        if (!currentFilter.createDateFilterText.isEmpty() && !dStr.contains(currentFilter.createDateFilterText.trimmed())) {
            return false;
        }
        if (!currentFilter.createDates.isEmpty() && !currentFilter.createDates.contains(dStr)) {
            return false;
        }
    }

    if (!currentFilter.modifyDates.isEmpty() || !currentFilter.modifyDateFilterText.isEmpty()) {
        QString dStr = QDateTime::fromMSecsSinceEpoch(record.mtime).date().toString("dd-MM-yyyy");
        if (!currentFilter.modifyDateFilterText.isEmpty() && !dStr.contains(currentFilter.modifyDateFilterText.trimmed())) {
            return false;
        }
        if (!currentFilter.modifyDates.isEmpty() && !currentFilter.modifyDates.contains(dStr)) {
            return false;
        }
    }

    // 6. 附加属性过滤 (链接、备注、标签、尺寸、判重)
    if (currentFilter.linkPresence != FilterState::All) {
        bool hasLink = !record.url.isEmpty();
        if (currentFilter.linkPresence == FilterState::Yes && !hasLink) return false;
        if (currentFilter.linkPresence == FilterState::No && hasLink) return false;
    }

    if (currentFilter.notePresence != FilterState::All) {
        bool hasNote = !record.note.isEmpty();
        if (currentFilter.notePresence == FilterState::Yes && !hasNote) return false;
        if (currentFilter.notePresence == FilterState::No && hasNote) return false;
    }

    if (currentFilter.tagPresence != FilterState::All) {
        bool hasTags = !record.tags.isEmpty();
        if (currentFilter.tagPresence == FilterState::Yes && !hasTags) return false;
        if (currentFilter.tagPresence == FilterState::No && hasTags) return false;
    }

    if (currentFilter.minSize != -1 && record.size < currentFilter.minSize) return false;
    if (currentFilter.maxSize != -1 && record.size > currentFilter.maxSize) return false;

    if (currentFilter.ratio != FilterState::AspectAny) {
        if (record.width > 0 && record.height > 0) {
            double r = static_cast<double>(record.width) / record.height;
            if (currentFilter.ratio == FilterState::Horizontal && record.width <= record.height) return false;
            if (currentFilter.ratio == FilterState::Vertical && record.height <= record.width) return false;
            if (currentFilter.ratio == FilterState::Square && std::abs(r - 1.0) > 0.05) return false;
            if (currentFilter.ratio == FilterState::Ratio169 && std::abs(r - 1.77) > 0.05) return false;
        } else {
            return false;
        }
    }

    if (currentFilter.duplicatePresence != FilterState::DupAll) {
        if (record.isDir) return false;
        bool isDuplicate = m_cachedDuplicatePaths.contains(record.path);
        if (currentFilter.duplicatePresence == FilterState::DuplicateOnly && !isDuplicate) return false;
        if (currentFilter.duplicatePresence == FilterState::UniqueOnly && isDuplicate) return false;
    }

    // 7. 搜索关键词匹配
    if (!currentFilter.keyword.isEmpty()) {
        const QString& kw = currentFilter.keyword;
        bool match = record.filename.contains(kw, Qt::CaseInsensitive);

        if (!match) {
            for (const QString& tag : record.tags) {
                if (tag.contains(kw, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }

        if (!match && !record.note.isEmpty()) {
            if (record.note.contains(kw, Qt::CaseInsensitive)) {
                match = true;
            }
        }

        if (!match) return false;
    }

    return true;
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (!filterAcceptsRowBase(sourceRow, sourceParent)) {
        return false;
    }

    const auto* sourceModelPtr = qobject_cast<const ItemModelBase*>(sourceModel());
    if (!sourceModelPtr) return true;

    const auto& records = sourceModelPtr->allRecords();
    if (sourceRow < 0 || sourceRow >= static_cast<int>(records.size())) return false;
    const auto& record = records[sourceRow];

    auto* contentPanel = qobject_cast<ContentPanel*>(parent());
    bool isTrashView = contentPanel && (contentPanel->getCurrentCategoryType() == "trash");

    // Section Collapse check
    if (!isTrashView && m_groupHeadersEnabled) {
        if (record.isDir && m_foldersCollapsed) return false;
        if (!record.isDir && m_filesCollapsed) return false;
    }

    return true;
}
>>>>>>> REPLACE

### 3. `src/ui/ContentPanel.cpp`
Enable group headers in ListView initialization and route toggleFolderSectionCollapse in ContentPanel.

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
1. Run `cmake -B build && cmake --build build` on Windows environment with MSVC / Qt6.
2. Launch QuarkMeta app and navigate to any directory containing both folders and files.
3. Switch to **ListView (列表模式)**.
4. Verify that "文件夹 (N)" and "文件 (M)" blue section headers with vector arrows span across all list columns.
5. Click on the "文件夹 (N)" section header or press the collapse shortcut to toggle section collapse. Confirm folders are smoothly collapsed/expanded while the section header remains visible and interactive.

## SSOT API Reuse & Anti-Redundancy Self-Check
- **Model Role SSOT**: Reused `IsGroupHeaderRole` (`Qt::UserRole + 212`) and `IsGroupCollapsedRole` (`Qt::UserRole + 213`) from `ModelContract.h`.
- **View Delegate Reuse**: 100% reused pre-existing `DropTreeView::updateGroupHeaderSpanning()` and `TreeItemDelegate::paint` header rendering branches without creating duplicate widgets.
- **Refresh Entrypoint**: Reused `FilterProxyModel::updateFilter()` for triggering layout recalculation during section collapse/expansion.

## Header API Signature Verification
- `FilterProxyModel::setGroupHeadersEnabled(bool)` in `src/ui/models/FilterProxyModel.h`
- `FilterProxyModel::updateFilter()` in `src/ui/models/FilterProxyModel.h`
- `ContentPanel::toggleFolderSectionCollapse()` in `src/ui/ContentPanel.h`
- `DropTreeView::updateGroupHeaderSpanning()` in `src/ui/DropTreeView.h`
- `IsGroupHeaderRole` & `IsGroupCollapsedRole` in `src/core/ModelContract.h`
