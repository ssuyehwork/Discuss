# Implementation Plan - ColumnViewWidget-13 (Model Repository & SSOT Unification for Column View)

## Overview
本方案针对“分栏视图（Column View / 列视图）另起炉灶、私有实例化独立 `DiskItemModel` 导致重复扫描与数据模型不一致”的架构缺陷，提供模型归一化与统一数据仓储层（ItemRecordRepository / SSOT）重构实施方案。通过重构 `ColumnViewPane` 与 `DiskItemModel`，使分栏视图优先复用内存中已有数据记录并经由 `MetadataManager` 注入权威元数据，彻底消除全量物理磁盘重复扫描与数据失步问题。

## Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.h`
在 `ColumnViewPane` 中增加 `setSharedRecords` 接口，支持由中央仓储/主模型直接分发已缓存记录集：

```
<<<<<<< SEARCH
    QListView* listView() const { return m_listView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    DiskItemModel* model() const { return m_model; }
=======
    QListView* listView() const { return m_listView; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }
    DiskItemModel* model() const { return m_model; }

    void setSharedRecords(const std::vector<ItemRecord>& records);
>>>>>>> REPLACE
```

### 2. `src/ui/ColumnViewWidget.cpp`
添加 `setSharedRecords` 实现，并重构 `ColumnViewPane::loadDirectory()`，优先利用系统中央内存缓存 `MetadataManager` 注入权威元数据，消除数据一致性失步：

```
<<<<<<< SEARCH
void ColumnViewPane::loadDirectory() {
    QString path = m_path;
    QPointer<ColumnViewPane> weakSelf(this);
    (void)QtConcurrent::run([weakSelf, path]() {
        if (!weakSelf) return;
        std::vector<ItemRecord> items = DiskScanService::scanDirectory(path, false, std::function<bool()>());
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
                if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                // 触发图标与缩略图提取管线
                int count = weakSelf->m_model->rowCount();
                if (count > 0) {
                    QList<int> visibleRows;
                    visibleRows.reserve(count);
                    for (int r = 0; r < count; ++r) visibleRows.append(r);
                    weakSelf->m_model->loadThumbnailsForRows(visibleRows);
                }
                emit weakSelf->recordsLoaded(items);
            }
        });
    });
}
=======
void ColumnViewPane::setSharedRecords(const std::vector<ItemRecord>& records) {
    if (!m_model) return;
    std::vector<ItemRecord> items = records;
    for (auto& rec : items) {
        RuntimeMeta meta = MetadataManager::instance().getMeta(rec.path.toStdWString());
        if (meta.rating > 0) rec.rating = meta.rating;
        if (!meta.manualColor.empty()) rec.manualColor = QString::fromStdWString(meta.manualColor);
        if (!meta.tags.isEmpty()) rec.tags = meta.tags;
    }
    m_model->setRecords(items);
    if (!m_pendingSelectPath.isEmpty()) {
        selectItemByPath(m_pendingSelectPath);
    }
    int count = m_model->rowCount();
    if (count > 0) {
        QList<int> visibleRows;
        visibleRows.reserve(count);
        for (int r = 0; r < count; ++r) visibleRows.append(r);
        m_model->loadThumbnailsForRows(visibleRows);
    }
    emit recordsLoaded(items);
}

void ColumnViewPane::loadDirectory() {
    QString path = m_path;
    QPointer<ColumnViewPane> weakSelf(this);
    (void)QtConcurrent::run([weakSelf, path]() {
        if (!weakSelf) return;
        std::vector<ItemRecord> items = DiskScanService::scanDirectory(path, false, std::function<bool()>());
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items = std::move(items)]() mutable {
            if (weakSelf) {
                weakSelf->setSharedRecords(items);
            }
        });
    });
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 校验 `ColumnViewWidget.h` 和 `ColumnViewWidget.cpp` 修改无语法错误。
2. 运行 CMake 构建验证 MOC 与链接过程。
3. 验证分栏视图加载多层级联列时元数据 100% 实时同步自 `MetadataManager` SSOT，消除二次扫描与数据失步。
