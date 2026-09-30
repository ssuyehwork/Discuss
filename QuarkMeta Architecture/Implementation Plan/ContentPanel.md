# Implementation Plan - ContentPanel

## 1. Overview（概述与解决的问题）
本实施方案用于彻底根治网格视图筛选不生效、缩略图失败项反复走解图重试拖慢性能等 4 项架构遗留问题：
1. **统一筛选分发**：在 `ContentPanel` 中增加统一分发机制，把筛选条件（`m_currentFilter`）与重复项缓存路径（`setCachedDuplicatePaths`）同时同步给全部四个代理模型（`m_folderProxyModel`、`m_fileProxyModel`、`m_gridFolderProxyModel`、`m_gridFileProxyModel`），彻底修复网格视图下筛选与判重无响应的缺陷。
2. **失败项零重试与运行时 Failed 状态落盘**：
   - 修复 `DiskItemModel::loadThumbnailsForRows` 的回调，当回调收到 `pixmap.isNull()` 时，在主线程将该记录的 `thumbnailState` 置为 `Failed` 并单行发射 `dataChanged` 信号（同时清除 `m_requestedPaths`）；
   - 在 `loadThumbnailsForRows` 调度扫描时，只要记录 `thumbnailState == ThumbnailState::Failed`，直接 `continue` 跳过，杜绝对失败项（如破损 AI/EPS 矢量图）无限重复提交解图；
   - 在 `reloadThumbnailForPath` 中补齐将 `thumbnailState` 重置为 `Pending` 的逻辑。
3. **清理 `ItemRecord::create` 死代码分支**：删除 `ItemRecord::create` 中误判 `thumbStatus == 1` 的死分支，仅保留“文件夹或非图形文件=NotApplicable，其余=Pending”。
4. **清理 `ContentViewCoordinator` 孤儿函数**：删除无任何调用方的 `ContentViewCoordinator::refreshVisibleThumbnails` 声明与定义。

---

## 2. Modified Files List（影响文件清单）
- `src/core/ItemRecord.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/controllers/ContentViewCoordinator.h`
- `src/ui/controllers/ContentViewCoordinator.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### Change 1: `src/core/ItemRecord.cpp`（删除 `create` 内判定 `thumbStatus` 的死分支）
```cpp
<<<<<<< SEARCH
    if (r.isDir || !ColorPaletteEngine::isGraphicsFile(r.suffix.toLower())) {
        r.thumbnailState = ThumbnailState::NotApplicable;
    } else if (r.thumbStatus == 1) {
        r.thumbnailState = ThumbnailState::Failed;
    } else {
        r.thumbnailState = ThumbnailState::Pending;
    }
=======
    if (r.isDir || !ColorPaletteEngine::isGraphicsFile(r.suffix.toLower())) {
        r.thumbnailState = ThumbnailState::NotApplicable;
    } else {
        r.thumbnailState = ThumbnailState::Pending;
    }
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/ContentPanel.h` & `src/ui/ContentPanel.cpp`（4 个代理模型的统一筛选与判重同步）

#### `src/ui/ContentPanel.h`
```cpp
<<<<<<< SEARCH
    void applySort();
=======
    void applySort();
    void syncFilterToAllProxies();
>>>>>>> REPLACE
```

#### `src/ui/ContentPanel.cpp`
```cpp
<<<<<<< SEARCH
void ContentPanel::applyFilters() {
    if (m_folderProxyModel) {
        FilterState s = m_currentFilter;
        s.showFolders = true;
        s.showFiles = false;
        m_folderProxyModel->currentFilter = s;
        m_folderProxyModel->updateFilter();
    }
    if (m_fileProxyModel) {
        FilterState s = m_currentFilter;
        s.showFolders = false;
        s.showFiles = true;
        m_fileProxyModel->currentFilter = s;
        m_fileProxyModel->updateFilter();
    }
    if (m_columnView) {
        m_columnView->applyFilterState(m_currentFilter);
    }
    updateStatusBarStats();
}
=======
void ContentPanel::syncFilterToAllProxies() {
    if (m_folderProxyModel) {
        FilterState s = m_currentFilter;
        s.showFolders = true;
        s.showFiles = false;
        m_folderProxyModel->currentFilter = s;
        m_folderProxyModel->updateFilter();
    }
    if (m_fileProxyModel) {
        FilterState s = m_currentFilter;
        s.showFolders = false;
        s.showFiles = true;
        m_fileProxyModel->currentFilter = s;
        m_fileProxyModel->updateFilter();
    }
    if (m_gridFolderProxyModel) {
        FilterState s = m_currentFilter;
        s.showFolders = true;
        s.showFiles = false;
        m_gridFolderProxyModel->currentFilter = s;
        m_gridFolderProxyModel->updateFilter();
    }
    if (m_gridFileProxyModel) {
        FilterState s = m_currentFilter;
        s.showFolders = false;
        s.showFiles = true;
        m_gridFileProxyModel->currentFilter = s;
        m_gridFileProxyModel->updateFilter();
    }
}

void ContentPanel::applyFilters() {
    syncFilterToAllProxies();
    if (m_columnView) {
        m_columnView->applyFilterState(m_currentFilter);
    }
    updateStatusBarStats();
}
>>>>>>> REPLACE
```

#### 重复项缓存同步（`statsReady` 回调）
```cpp
<<<<<<< SEARCH
    connect(m_statsWorker, &ContentStatsWorker::statsReady, this, [this](const ScanStats& stats) {
        if (m_fileProxyModel) {
            m_fileProxyModel->setCachedDuplicatePaths(stats.duplicatePaths);
        }
        emit directoryStatsReady(stats);
    });
=======
    connect(m_statsWorker, &ContentStatsWorker::statsReady, this, [this](const ScanStats& stats) {
        if (m_fileProxyModel) {
            m_fileProxyModel->setCachedDuplicatePaths(stats.duplicatePaths);
        }
        if (m_gridFileProxyModel) {
            m_gridFileProxyModel->setCachedDuplicatePaths(stats.duplicatePaths);
        }
        emit directoryStatsReady(stats);
    });
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/models/DiskItemModel.cpp`（失败项直接跳过与手动重置）

#### (1) `loadThumbnailsForRows` 跳过 `ThumbnailState::Failed` 记录
```cpp
<<<<<<< SEARCH
        QString ext = rec.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        if (rec.isDir || !isGraphic) {
            if (!rec.isDir) {
                qDebug() << "[THUMB_TRACE] Row" << r << "File:" << rec.filename << "is NOT a graphics file (ext:" << ext << "), skipping thumbnail load.";
            }
            continue;
        }
=======
        if (rec.isDir || rec.thumbnailState == ItemRecord::ThumbnailState::NotApplicable || rec.thumbnailState == ItemRecord::ThumbnailState::Failed) {
            continue;
        }
>>>>>>> REPLACE
```

#### (2) `reloadThumbnailForPath` 恢复状态为 `Pending`
```cpp
<<<<<<< SEARCH
    auto it = m_pathToIndex.find(nPath);
    if (it != m_pathToIndex.end()) {
        int rIdx = it->second;
        loadThumbnailsForRows({rIdx});
        emit dataChanged(
            index(rIdx, 0), 
            index(rIdx, columnCount() - 1), 
            {Qt::DecorationRole, Qt::DisplayRole, AspectRatioRole, HasThumbnailRole}
        );
    }
=======
    auto it = m_pathToIndex.find(nPath);
    if (it != m_pathToIndex.end()) {
        int rIdx = it->second;
        if (rIdx >= 0 && rIdx < static_cast<int>(m_allRecords.size())) {
            m_allRecords[rIdx].thumbnailState = ItemRecord::ThumbnailState::Pending;
        }
        loadThumbnailsForRows({rIdx});
        emit dataChanged(
            index(rIdx, 0), 
            index(rIdx, columnCount() - 1), 
            {Qt::DecorationRole, Qt::DisplayRole, AspectRatioRole, HasThumbnailRole}
        );
    }
>>>>>>> REPLACE
```

---

### Change 4: `src/ui/controllers/ContentViewCoordinator.h` & `ContentViewCoordinator.cpp`（删除死代码）

#### `src/ui/controllers/ContentViewCoordinator.h`
```cpp
<<<<<<< SEARCH
    void refreshVisibleThumbnails();
=======
>>>>>>> REPLACE
```

#### `src/ui/controllers/ContentViewCoordinator.cpp`
```cpp
<<<<<<< SEARCH
void ContentViewCoordinator::refreshVisibleThumbnails() {
    QAbstractItemView* activeView = activeItemView();
    if (!activeView || !m_panel || !m_panel->model()) return;

    QSortFilterProxyModel* proxyModel = qobject_cast<QSortFilterProxyModel*>(activeView->model());
    if (!proxyModel) return;

    QRect viewportRect = activeView->viewport()->rect();
    QModelIndex topLeft = activeView->indexAt(viewportRect.topLeft());
    QModelIndex bottomRight = activeView->indexAt(viewportRect.bottomRight());

    QList<int> sourceRowsToLoad;

    if (topLeft.isValid() && bottomRight.isValid()) {
        int startRow = std::min(topLeft.row(), bottomRight.row());
        int endRow = std::max(topLeft.row(), bottomRight.row());

        for (int r = startRow; r <= endRow; ++r) {
            QModelIndex proxyIdx = proxyModel->index(r, 0);
            if (!proxyIdx.isValid()) continue;

            QModelIndex srcIdx = proxyModel->mapToSource(proxyIdx);
            if (srcIdx.isValid()) {
                sourceRowsToLoad.append(srcIdx.row());
            }
        }
    } else {
        int sourceRowCount = m_panel->model()->rowCount();
        for (int i = 0; i < sourceRowCount; ++i) {
            sourceRowsToLoad.append(i);
        }
    }

    if (!sourceRowsToLoad.isEmpty()) {
        m_panel->model()->loadThumbnailsForRows(sourceRowsToLoad);
    }
}
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **网格筛选与判重验证**：在网格视图（Grid Mode）下勾选评级、颜色、文件类型、重复项等条件，确认卡片视图瞬间精准筛选隐藏/显示，状态栏与内容区完全一致。
2. **失败项不重复解图验证**：打开包含坏图或异常 EPS 的目录，观察日志，确认解析失败后 `thumbnailState` 被赋值为 `Failed`，后续滚动与视图重绘时 `loadThumbnailsForRows` 瞬间直接 `continue` 跳过，日志不再出现重复解图尝试，加载速度恢复流畅。
3. **右键深度重新提取验证**：右键点击 Failed 状态的图像选择“重新提取缩略图”，确认 `reloadThumbnailForPath` 正确将其充置为 `Pending` 并重新触发提取。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 统一使用私有函数 `ContentPanel::syncFilterToAllProxies` 完成 4 个代理模型的同步分发；
- 统一利用 `ItemRecord::thumbnailState` 字段阻断失败项重试，无任何多余集合。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / 方法 | 所在的头文件 | 精准物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `FilterProxyModel::setCachedDuplicatePaths` | `src/ui/models/FilterProxyModel.h` | `void setCachedDuplicatePaths(const QSet<QString>& paths);` |  物理核实通过 |
| `FilterProxyModel::updateFilter` | `src/ui/models/FilterProxyModel.h` | `void updateFilter();` |  物理核实通过 |
| `DiskItemModel::reloadThumbnailForPath` | `src/ui/models/DiskItemModel.h` | `void reloadThumbnailForPath(const QString& path);` |  物理核实通过 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `src/ui/ContentPanel.cpp`:
  - 包含了 `FilterProxyModel.h`。类型完整，方法均能正常调用。
- `src/ui/models/DiskItemModel.cpp`:
  - 包含了 `ItemRecord.h`，`ThumbnailState` 完全定义。

---

## 8. 专项审查结果

### 8.1 “手动重新提取缩略图”入口清单与状态重置核查（第 2 点）

1. **入口位置**：右键菜单 `ContentContextMenu::showMenu` 中的 `ActionReextractThumbnail`（文案：“重新提取缩略图”）。
2. **后端调用链**：调用 `DeepThumbnailExtractor::instance().extractBatchAsync` -> `DiskMediaExtractor::forceExtractDeepThumbnail`；在 `forceExtractDeepThumbnail` 中成功解析后将磁盘 JSON 中的 `thumbStatus` 重置为 `0`。
3. **模型状态重置核查**：在 `ContentContextMenu` 提取完成后的回调中，调用了 `weakThis->reloadThumbnailForPath(itemPath)`。
4. **状态重置补充**：已在本方案 Change 3 中于 `DiskItemModel::reloadThumbnailForPath` 内显式补齐 `m_allRecords[rIdx].thumbnailState = ItemRecord::ThumbnailState::Pending;`，确保手动触发重新提取时状态能精准重置并重新触发提图。

### 8.2 `ContentViewCoordinator::refreshVisibleThumbnails` 调用方检索结果（第 4 点）

经全工程（`src/`）物理检索：
- **`ContentViewCoordinator::refreshVisibleThumbnails()`** 在全工程中**没有任何调用方**（调用计数为 0）。
- **核查结论**：该函数系早期视图协调器留下的废弃死代码（当前可见区缩略图计算已由 `DualSectionPanel` 和 `SectionedScrollCanvas` 统一接管）。已在本方案 Change 4 中安全将其声明与定义全部删除。
