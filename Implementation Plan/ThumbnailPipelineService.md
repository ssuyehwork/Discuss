# Implementation Plan - ThumbnailPipelineService

## 1. Overview（概述与解决的问题）
本实施方案旨在解决缩略图状态统一后的 4 处遗留补漏点：
1. **统计实时响应更新**：在 `ContentPanel` 的 `dataChanged` 监听触发条件中加入 `HasThumbnailRole`，复用既有 50ms 防抖定时器，解决筛选面板数字不随缩略图生成而实时增长的问题。
2. **运行时提取失败写入 Failed 状态**：扩展 `ThumbnailPipelineService::loadBatchAsync` 回调，在解码失败（`pixmap.isNull()`）时切回主线程通知模型；`DiskItemModel` 在主线程收到失败通知后将 `thumbnailState` 置为 `Failed` 并发 `dataChanged`（且清除 `m_requestedPaths` 锁，代际变化时熔断丢弃）。
3. **`dataChanged` 发射精准收窄**：将 `DiskItemModel` 中 `setRecords` 的异步缓存探测回调与 `flushPendingThumbDataChanged` 的批量刷新发出的 `dataChanged` 信号按“连续行段”拆分精准发射，杜绝使用粗暴的 `minRow~maxRow` 盲目重绘非变化行。
4. **状态初始化下沉**：将 `ThumbnailState` 基础判定（`NotApplicable` / `Failed` / `Pending`）下沉至 `ItemRecord::create` 和 `ItemRecord::fromMetadata`，彻底解决未初始化记录误归为 `NotApplicable` 的风险。

---

## 2. Modified Files List（影响文件清单）
- `src/core/ItemRecord.cpp`
- `src/util/ThumbnailPipelineService.cpp`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### Change 1: `src/core/ItemRecord.cpp`（初始化下沉）

```cpp
<<<<<<< SEARCH
void ItemRecord::fromMetadata(ItemRecord& r, const RuntimeMeta& meta) {
    r.rating = meta.rating;
    r.manualColor = QString::fromStdWString(meta.manualColor);
    r.autoColor = QString::fromStdWString(meta.autoColor);
    r.tags = meta.tags;
    r.pinned = meta.pinned;
    r.encrypted = meta.encrypted;
    r.url = QString::fromStdWString(meta.url);
    r.note = QString::fromStdWString(meta.note);
    r.sha256 = QString::fromStdWString(meta.sha256);
    r.width = meta.width;
    r.height = meta.height;
    r.thumbStatus = meta.thumbStatus;
    r.isDiskTrash = meta.isTrash;
    r.added_at = meta.added_at;

    r.palettes.clear();
    for (const auto& pe : meta.palettes) {
        r.palettes.push_back({pe.color, pe.ratio});
    }
}
=======
void ItemRecord::fromMetadata(ItemRecord& r, const RuntimeMeta& meta) {
    r.rating = meta.rating;
    r.manualColor = QString::fromStdWString(meta.manualColor);
    r.autoColor = QString::fromStdWString(meta.autoColor);
    r.tags = meta.tags;
    r.pinned = meta.pinned;
    r.encrypted = meta.encrypted;
    r.url = QString::fromStdWString(meta.url);
    r.note = QString::fromStdWString(meta.note);
    r.sha256 = QString::fromStdWString(meta.sha256);
    r.width = meta.width;
    r.height = meta.height;
    r.thumbStatus = meta.thumbStatus;
    r.isDiskTrash = meta.isTrash;
    r.added_at = meta.added_at;

    r.palettes.clear();
    for (const auto& pe : meta.palettes) {
        r.palettes.push_back({pe.color, pe.ratio});
    }

    if (!r.isDir && ColorPaletteEngine::isGraphicsFile(r.suffix.toLower())) {
        if (r.thumbStatus == 1) {
            r.thumbnailState = ThumbnailState::Failed;
        } else if (r.thumbnailState == ThumbnailState::NotApplicable || r.thumbnailState == ThumbnailState::Failed) {
            r.thumbnailState = ThumbnailState::Pending;
        }
    } else {
        r.thumbnailState = ThumbnailState::NotApplicable;
    }
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    r.filename = info.fileName();
    r.isManaged = false;
    r.isHidden = info.isHidden();

    return r;
}
=======
    r.filename = info.fileName();
    r.isManaged = false;
    r.isHidden = info.isHidden();

    if (r.isDir || !ColorPaletteEngine::isGraphicsFile(r.suffix.toLower())) {
        r.thumbnailState = ThumbnailState::NotApplicable;
    } else if (r.thumbStatus == 1) {
        r.thumbnailState = ThumbnailState::Failed;
    } else {
        r.thumbnailState = ThumbnailState::Pending;
    }

    return r;
}
>>>>>>> REPLACE
```

---

### Change 2: `src/util/ThumbnailPipelineService.cpp`（失败通知扩展）

```cpp
<<<<<<< SEARCH
            if (!finalImg.isNull()) {
                if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                    return;
                }

                QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                    if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                        return;
                    }

                    QPixmap pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }

                        if (onSingleLoaded) {
                            onSingleLoaded(path, pix);
                        }
                    }
                }, Qt::QueuedConnection);
            }
=======
            if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                return;
            }

            QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                    return;
                }

                if (!finalImg.isNull()) {
                    QPixmap pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }

                        if (onSingleLoaded) {
                            onSingleLoaded(path, pix);
                        }
                    } else {
                        if (onSingleLoaded) {
                            onSingleLoaded(path, QPixmap());
                        }
                    }
                } else {
                    if (onSingleLoaded) {
                        onSingleLoaded(path, QPixmap());
                    }
                }
            }, Qt::QueuedConnection);
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/models/DiskItemModel.cpp`（收窄 `dataChanged` 连续块与失败处理）

#### (1) `flushPendingThumbDataChanged` 连续块拆分
```cpp
<<<<<<< SEARCH
void DiskItemModel::flushPendingThumbDataChanged() {
    if (m_pendingThumbRows.isEmpty()) return;

    QSet<int> rowsToEmit = m_pendingThumbRows;
    m_pendingThumbRows.clear();

    int minRow = std::numeric_limits<int>::max();
    int maxRow = std::numeric_limits<int>::min();

    for (int r : rowsToEmit) {
        if (r >= 0 && r < static_cast<int>(m_allRecords.size())) {
            if (r < minRow) minRow = r;
            if (r > maxRow) maxRow = r;
        }
    }

    if (minRow <= maxRow) {
        emit dataChanged(index(minRow, 0), index(maxRow, columnCount() - 1),
                          {Qt::DecorationRole, HasThumbnailRole});
    }
}
=======
void DiskItemModel::flushPendingThumbDataChanged() {
    if (m_pendingThumbRows.isEmpty()) return;

    QList<int> sortedRows = m_pendingThumbRows.values();
    m_pendingThumbRows.clear();
    std::sort(sortedRows.begin(), sortedRows.end());

    int startRow = -1;
    int prevRow = -1;

    for (int r : sortedRows) {
        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        if (startRow == -1) {
            startRow = r;
            prevRow = r;
        } else if (r == prevRow + 1) {
            prevRow = r;
        } else {
            emit dataChanged(index(startRow, 0), index(prevRow, columnCount() - 1),
                             {Qt::DecorationRole, HasThumbnailRole});
            startRow = r;
            prevRow = r;
        }
    }
    if (startRow != -1) {
        emit dataChanged(index(startRow, 0), index(prevRow, columnCount() - 1),
                         {Qt::DecorationRole, HasThumbnailRole});
    }
}
>>>>>>> REPLACE
```

#### (2) `setRecords` 缓存判定连续块拆分
```cpp
<<<<<<< SEARCH
                if (minRow <= maxRow) {
                    emit weakThis->dataChanged(
                        weakThis->index(minRow, 0),
                        weakThis->index(maxRow, weakThis->columnCount() - 1),
                        {HasThumbnailRole}
                    );
                }
=======
                std::vector<int> sortedRows;
                sortedRows.reserve(readyPaths.size());
                for (const QString& path : readyPaths) {
                    auto it = weakThis->m_pathToIndex.find(path);
                    if (it != weakThis->m_pathToIndex.end() && it->second >= 0 && it->second < static_cast<int>(weakThis->m_allRecords.size())) {
                        sortedRows.push_back(it->second);
                    }
                }
                std::sort(sortedRows.begin(), sortedRows.end());
                sortedRows.erase(std::unique(sortedRows.begin(), sortedRows.end()), sortedRows.end());

                int startRow = -1;
                int prevRow = -1;
                for (int r : sortedRows) {
                    if (startRow == -1) {
                        startRow = r;
                        prevRow = r;
                    } else if (r == prevRow + 1) {
                        prevRow = r;
                    } else {
                        emit weakThis->dataChanged(
                            weakThis->index(startRow, 0),
                            weakThis->index(prevRow, weakThis->columnCount() - 1),
                            {HasThumbnailRole}
                        );
                        startRow = r;
                        prevRow = r;
                    }
                }
                if (startRow != -1) {
                    emit weakThis->dataChanged(
                        weakThis->index(startRow, 0),
                        weakThis->index(prevRow, weakThis->columnCount() - 1),
                        {HasThumbnailRole}
                    );
                }
>>>>>>> REPLACE
```

#### (3) `loadThumbnailsForRows` 运行时失败写入通知处理
```cpp
<<<<<<< SEARCH
            QMetaObject::invokeMethod(weakThis, [weakThis, path]() {
                if (!weakThis) return;
                auto it = weakThis->m_pathToIndex.find(path);
                if (it != weakThis->m_pathToIndex.end()) {
                    int currentIdx = it->second;
                    if (currentIdx >= 0 && currentIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                        if (weakThis->m_allRecords[currentIdx].path == path) {
                            weakThis->m_allRecords[currentIdx].thumbnailState = ItemRecord::ThumbnailState::Ready;
                            weakThis->m_pendingThumbRows.insert(currentIdx);
                            if (weakThis->m_thumbBatchTimer && !weakThis->m_thumbBatchTimer->isActive()) {
                                weakThis->m_thumbBatchTimer->start();
                            }
                            emit weakThis->thumbnailLoaded(currentIdx);
                        }
                    }
                }
            }, Qt::QueuedConnection);
=======
            QMetaObject::invokeMethod(weakThis, [weakThis, path, pixmap]() {
                if (!weakThis) return;
                weakThis->m_requestedPaths.remove(path);
                auto it = weakThis->m_pathToIndex.find(path);
                if (it != weakThis->m_pathToIndex.end()) {
                    int currentIdx = it->second;
                    if (currentIdx >= 0 && currentIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                        if (weakThis->m_allRecords[currentIdx].path == path) {
                            if (!pixmap.isNull()) {
                                weakThis->m_allRecords[currentIdx].thumbnailState = ItemRecord::ThumbnailState::Ready;
                                weakThis->m_pendingThumbRows.insert(currentIdx);
                                if (weakThis->m_thumbBatchTimer && !weakThis->m_thumbBatchTimer->isActive()) {
                                    weakThis->m_thumbBatchTimer->start();
                                }
                                emit weakThis->thumbnailLoaded(currentIdx);
                            } else {
                                // 运行时解码失败写入 Failed 状态，发送精准单行 dataChanged
                                weakThis->m_allRecords[currentIdx].thumbnailState = ItemRecord::ThumbnailState::Failed;
                                emit weakThis->dataChanged(
                                    weakThis->index(currentIdx, 0),
                                    weakThis->index(currentIdx, weakThis->columnCount() - 1),
                                    {HasThumbnailRole}
                                );
                            }
                        }
                    }
                }
            }, Qt::QueuedConnection);
>>>>>>> REPLACE
```

---

### Change 4: `src/ui/ContentPanel.cpp`（监听 `HasThumbnailRole` 驱动防抖重算）

```cpp
<<<<<<< SEARCH
    // 核心架构闭环：监听底层模型元数据变更（卡片点击、列表点击、快捷键赋予、F4重复等），自动防抖驱动统计重算与筛选器同步
    connect(m_diskModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
        if (roles.isEmpty() || roles.contains(RatingRole) || roles.contains(ColorRole) || roles.contains(TagsRole)) {
            if (m_statsDebounceTimer) {
                m_statsDebounceTimer->start();
            }
        }
    });
=======
    // 核心架构闭环：监听底层模型元数据变更（卡片点击、缩略图就绪/失败、颜色与标签赋予等），自动防抖驱动统计重算与筛选器同步
    connect(m_diskModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
        if (roles.isEmpty() || roles.contains(RatingRole) || roles.contains(ColorRole) || roles.contains(TagsRole) || roles.contains(HasThumbnailRole)) {
            if (m_statsDebounceTimer) {
                m_statsDebounceTimer->start();
            }
        }
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **编译核验**：确保 `ItemRecord.cpp`、`ThumbnailPipelineService.cpp`、`DiskItemModel.cpp` 和 `ContentPanel.cpp` 编译无任何警告或错误。
2. **实时统计与筛选验证**：
   - 打开包含多张未生成缩略图的文件夹，观察 FilterPanel 的“有缩略图/无缩略图”数字随着后台提图不断完成而防抖实时递增；
   - 故意导入破坏损坏的图像文件，确认其提图失败后 `thumbnailState` 变为 `Failed`，计入“无缩略图”且不再重复排查。
3. **连续行段 `dataChanged` 验证**：
   - 检查 `dataChanged` 的 `minRow/maxRow` 区间，确认断开行已被拆分为独立区间发射。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- `ItemRecord::thumbnailState` 保持作为全局唯一数据源；
- 统计重算复用 `ContentPanel` 现有的 50ms 防抖定时器，完全没有另起炉灶。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / 枚举 | 所在的头文件 | 精准物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `ItemRecord::create` | `src/core/ItemRecord.h` | `static ItemRecord create(const QString& path, const RuntimeMeta* providedMeta = nullptr);` |  物理核实通过 |
| `ItemRecord::fromMetadata` | `src/core/ItemRecord.h` | `static void fromMetadata(ItemRecord& r, const RuntimeMeta& meta);` |  物理核实通过 |
| `ThumbnailPipelineService::loadBatchAsync` | `src/util/ThumbnailPipelineService.h` | `void loadBatchAsync(..., std::function<void(const QString& path, const QPixmap& pixmap)> onSingleLoaded);` |  物理核实通过 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `src/core/ItemRecord.cpp`:
  - 包含了 `#include "ItemRecord.h"`, `#include "../util/ColorPaletteEngine.h"`
  - 类型完整性：`ColorPaletteEngine::isGraphicsFile` 可直接调用。

---

## 8. 模型初始化与装载路径审查清单（第 4 点）

### 8.1 继承 `ItemModelBase` 的子类全排查
经过全项目代码库（`src/`）审查：
- **`DiskItemModel`** 为项目中**唯一的 `ItemModelBase` 继承实现**（其他视图或模式如列视图 `ColumnViewPane` 均直接实例化使用 `DiskItemModel` 作为其数据 SourceModel）。
- 故只需确保 `ItemRecord::create` / `fromMetadata` 规范化，全网数据装载路径将自然 100% 继承正确的 `thumbnailState` 初始值。

### 8.2 全项目 `ItemRecord::create` 调用点排查
1. `src/core/DiskScanService.cpp:38`:
   - 调用：`ItemRecord itemRec = ItemRecord::create(absPath, nullptr);`
   - 初始化情况：下沉后将依据扩展名自动设为 `NotApplicable` 或 `Pending`/`Failed`。
2. `src/ui/controllers/ContentDataLoader.cpp:63`:
   - 调用：`driveRecords.push_back(ItemRecord::create(drive.absolutePath()));`
   - 初始化情况：盘符设为 `NotApplicable`。
3. `src/ui/controllers/ContentDataLoader.cpp:146`:
   - 调用：`records.push_back(ItemRecord::create(p));`
   - 初始化情况：文件依据扩展名下沉判定。
4. `src/ui/controllers/ContentDataLoader.cpp:174`:
   - 调用：`newRecs.push_back(ItemRecord::create(p));`
   - 初始化情况：文件依据扩展名下沉判定。
5. `src/ui/ColumnViewPane.cpp:597`:
   - 调用：`items.push_back(ItemRecord::create(drive.absolutePath()));`
   - 初始化情况：列视图驱动器根节点初始化为 `NotApplicable`。

所有装载路径均已确认可以 100% 完全套用下沉至 `ItemRecord::create` 的初始逻辑。
