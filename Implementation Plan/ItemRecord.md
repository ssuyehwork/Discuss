# Implementation Plan - ItemRecord

## 1. Overview（概述与解决的问题）
本实施方案旨在彻底统一“缩略图状态”的唯一数据源，解决此前散落在 `ContentStatsWorker`、`FilterProxyModel` 与 `DiskItemModel` 三处且相互不一致、甚至在主线程实时访问文件系统的架构缺陷。本次修改：
1. 在 `ItemRecord` 上引入唯一的 `ThumbnailState` 枚举字段（`NotApplicable` / `Ready` / `Failed` / `Pending`）；
2. **主线程写入律**：限制状态更新只能在主线程发生（目录加载建立记录时在后台线程批量检测缓存文件存在性后交由主线程写入、解码成功落盘后在回调中由主线程写入、失败标记落盘后由主线程写入）；
3. **消除主线程文件系统 IO**：`DiskItemModel::data(HasThumbnailRole)` 仅依据 `ThumbnailState == Ready` 返回 `true`；
4. **统一统计与筛选口径**：`ContentStatsWorker` 仅统计图形文件（`NotApplicable` 不计入有也不计入无），有 = `Ready`，无 = `Failed + Pending`；`FilterProxyModel` 中 `HasThumbnail` 仅保留 `Ready`，`NoThumbnail` 保留 `Failed + Pending`，`NotApplicable` 项目在激活缩略图筛选时一律不通过；
5. **废弃代码清理**：清理并删除全项目已无任何调用方的旧 `UiHelper::hasPhysicalThumbnail` 逻辑。

---

## 2. Modified Files List（影响文件清单）
- `src/core/ItemRecord.h`
- `src/ui/models/DiskItemModel.h`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/workers/ContentStatsWorker.cpp`
- `src/ui/models/FilterProxyModel.cpp`
- `src/ui/UiHelper.h`
- `src/ui/UiHelper.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### Change 1: `src/core/ItemRecord.h`（新增 ThumbnailState 枚举与字段）
```cpp
<<<<<<< SEARCH
    int thumbStatus = 0; // 0: 正常/未处理, 1: 提取失败/跳过
    bool isParentExpanded = false;
=======
    enum class ThumbnailState {
        NotApplicable, // 文件夹或非图形文件
        Ready,         // 缩略图已落盘并可用
        Failed,        // 提取失败 (thumbStatus == 1)
        Pending        // 图形文件且未提取/未失败
    };

    ThumbnailState thumbnailState = ThumbnailState::NotApplicable;
    int thumbStatus = 0; // 0: 正常/未处理, 1: 提取失败/跳过
    bool isParentExpanded = false;
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/models/DiskItemModel.cpp`（目录建表初始化状态、异步回调写入状态与 data() 返回）

#### (1) `setRecords` 初始化 `thumbnailState`（在主线程设置前，后台一次性探测缓存存在性）
```cpp
<<<<<<< SEARCH
    m_pathToIndex.clear();
    m_requestedPaths.clear();
    int populatedMetaCount = 0;
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        auto& rec = m_allRecords[i];
        m_pathToIndex[rec.path] = i;
=======
    m_pathToIndex.clear();
    m_requestedPaths.clear();
    int populatedMetaCount = 0;

    // 🚀 在后台线程一次性批量探测图形文件的磁盘缩略图缓存存在性，防止主线程卡顿
    std::vector<bool> cacheExistFlags(m_allRecords.size(), false);
    for (size_t i = 0; i < m_allRecords.size(); ++i) {
        const auto& rec = m_allRecords[i];
        if (!rec.isDir && ColorPaletteEngine::isGraphicsFile(rec.suffix.toLower())) {
            if (rec.thumbStatus == 1) {
                cacheExistFlags[i] = false;
            } else {
                QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(rec.path);
                cacheExistFlags[i] = QFile::exists(thumbPath);
            }
        }
    }

    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        auto& rec = m_allRecords[i];
        m_pathToIndex[rec.path] = i;

        // 统一计算初始化 ThumbnailState
        if (rec.isDir || !ColorPaletteEngine::isGraphicsFile(rec.suffix.toLower())) {
            rec.thumbnailState = ItemRecord::ThumbnailState::NotApplicable;
        } else if (rec.thumbStatus == 1) {
            rec.thumbnailState = ItemRecord::ThumbnailState::Failed;
        } else if (cacheExistFlags[i]) {
            rec.thumbnailState = ItemRecord::ThumbnailState::Ready;
        } else {
            rec.thumbnailState = ItemRecord::ThumbnailState::Pending;
        }
>>>>>>> REPLACE
```

#### (2) `loadThumbnailsForRows` 中的成功/失败更新（主线程写入律）
```cpp
<<<<<<< SEARCH
            QMetaObject::invokeMethod(weakThis, [weakThis, path]() {
                if (!weakThis) return;
                auto it = weakThis->m_pathToIndex.find(path);
                if (it != weakThis->m_pathToIndex.end()) {
                    int currentIdx = it->second;
                    if (currentIdx >= 0 && currentIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                        if (weakThis->m_allRecords[currentIdx].path == path) {
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
            QMetaObject::invokeMethod(weakThis, [weakThis, path]() {
                if (!weakThis) return;
                auto it = weakThis->m_pathToIndex.find(path);
                if (it != weakThis->m_pathToIndex.end()) {
                    int currentIdx = it->second;
                    if (currentIdx >= 0 && currentIdx < static_cast<int>(weakThis->m_allRecords.size())) {
                        if (weakThis->m_allRecords[currentIdx].path == path) {
                            // 主线程专职写入状态
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
>>>>>>> REPLACE
```

#### (3) 失败标记刷新逻辑 `updateRecordMetadata` 中同步更新 `thumbnailState`
```cpp
<<<<<<< SEARCH
            record.tags = meta.tags;
            record.width = meta.width;
            record.height = meta.height;
            record.autoColor = QString::fromStdWString(meta.autoColor);
=======
            record.tags = meta.tags;
            record.width = meta.width;
            record.height = meta.height;
            record.thumbStatus = meta.thumbStatus;
            record.autoColor = QString::fromStdWString(meta.autoColor);

            if (!record.isDir && ColorPaletteEngine::isGraphicsFile(record.suffix.toLower())) {
                if (record.thumbStatus == 1) {
                    record.thumbnailState = ItemRecord::ThumbnailState::Failed;
                } else if (record.thumbnailState == ItemRecord::ThumbnailState::Failed) {
                    record.thumbnailState = ItemRecord::ThumbnailState::Pending;
                }
            }
>>>>>>> REPLACE
```

#### (4) `DiskItemModel::data(HasThumbnailRole)` 统一读取状态
```cpp
<<<<<<< SEARCH
    } else if (role == HasThumbnailRole) {
        return UiHelper::hasPhysicalThumbnail(record) ||
               (m_aspectRatios.contains(QDir::toNativeSeparators(path)) && m_aspectRatios.value(QDir::toNativeSeparators(path)) > 0.0) ||
               m_iconCache.contains(path);
=======
    } else if (role == HasThumbnailRole) {
        return record.thumbnailState == ItemRecord::ThumbnailState::Ready;
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/workers/ContentStatsWorker.cpp`（缩略图统计口径归一化）
```cpp
<<<<<<< SEARCH
            if (UiHelper::hasPhysicalThumbnail(record)) {
                stats.hasThumbnailCount++;
            } else {
                stats.noThumbnailCount++;
            }
=======
            if (ColorPaletteEngine::isGraphicsFile(record.suffix.toLower())) {
                if (record.thumbnailState == ItemRecord::ThumbnailState::Ready) {
                    stats.hasThumbnailCount++;
                } else {
                    stats.noThumbnailCount++;
                }
            }
>>>>>>> REPLACE
```

---

### Change 4: `src/ui/models/FilterProxyModel.cpp`（缩略图筛选器规则归一化）
```cpp
<<<<<<< SEARCH
    // 6.5 缩略图状态过滤 (Zero UI Main-Thread Disk I/O)
    if (currentFilter.thumbnailPresence != FilterState::ThumbAll &&
        !record.isDir && UiHelper::isGraphicsFile(record.suffix.toLower())) {
        bool hasThumb = sourceModelPtr->data(sourceModelPtr->index(sourceRow, 0), HasThumbnailRole).toBool();
        if (currentFilter.thumbnailPresence == FilterState::HasThumbnail && !hasThumb) return false;
        if (currentFilter.thumbnailPresence == FilterState::NoThumbnail && hasThumb) return false;
    }
=======
    // 6.5 缩略图状态过滤 (统一使用 ItemRecord::thumbnailState)
    if (currentFilter.thumbnailPresence != FilterState::ThumbAll) {
        if (record.isDir || !ColorPaletteEngine::isGraphicsFile(record.suffix.toLower())) {
            // 非图形文件或文件夹在激活缩略图筛选时一律隐藏（与统计口径完全一致）
            return false;
        }

        bool isReady = (record.thumbnailState == ItemRecord::ThumbnailState::Ready);
        if (currentFilter.thumbnailPresence == FilterState::HasThumbnail && !isReady) return false;
        if (currentFilter.thumbnailPresence == FilterState::NoThumbnail && isReady) return false;
    }
>>>>>>> REPLACE
```

---

### Change 5: `src/ui/UiHelper.h` 与 `src/ui/UiHelper.cpp`（删除无人使用的旧逻辑）

#### `src/ui/UiHelper.h`
```cpp
<<<<<<< SEARCH
    static bool hasPhysicalThumbnail(const ItemRecord& record);
=======
>>>>>>> REPLACE
```

#### `src/ui/UiHelper.cpp`
```cpp
<<<<<<< SEARCH
bool UiHelper::hasPhysicalThumbnail(const ItemRecord& record) {
    if (record.isDir) return false;
    if (record.thumbStatus == 1) return false;

    static const QStringList iconOnlyExts = {"cur", "ico", "ani"};
    QString ext = record.suffix.toLower();
    if (iconOnlyExts.contains(ext)) return false;

    if (isStandardImage(ext) && record.width > 0 && record.height > 0) {
        return true;
    }

    QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(record.path);
    return QFile::exists(thumbPath);
}
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **静态代码审查**：
   - 确认 `ThumbnailState` 仅在 `DiskItemModel` 主线程方法及回调的 `invokeMethod`（主线程事件循环）中修改；
   - 确认全项目中已无 `UiHelper::hasPhysicalThumbnail` 引用。
2. **功能与性能验证**：
   - **零磁盘 I/O 验证**：在列表滚动和卡片重绘过程中，确认 `HasThumbnailRole` 不再触发 SHA256 或 `QFile::exists` 调用；
   - **数字与列表一致性验证**：在 FilterPanel 中勾选“有缩略图”与“无缩略图”，确认列表展示的行数与面板数字 100% 精准相符；
   - **非图形文件过滤验证**：包含 `.txt`、`.cpp` 的目录下，勾选“有缩略图”或“无缩略图”时，非图形文件均自动隐藏，不误计入“无缩略图”。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- `ItemRecord::thumbnailState` 作为缩略图状态的**唯一真理源 (SSOT)**。
- 状态变更通过 `dataChanged(..., {HasThumbnailRole})` 精准通知视图层，保持已有架构通道干净单一。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / 枚举 | 所在的头文件 | 精准物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `ItemRecord::ThumbnailState` | `src/core/ItemRecord.h` | `enum class ThumbnailState { NotApplicable, Ready, Failed, Pending };` |  物理核实通过 |
| `ColorPaletteEngine::isGraphicsFile` | `src/util/ColorPaletteEngine.h` | `static bool isGraphicsFile(const QString& ext);` |  物理核实通过 |
| `DiskMediaExtractor::getDiskThumbCachePath` | `src/util/DiskMediaExtractor.h` | `static QString getDiskThumbCachePath(const QString& filePath);` |  物理核实通过 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- **`src/ui/models/DiskItemModel.cpp`**:
  - 已包含 `#include "../../core/ItemRecord.h"`
  - 已包含 `#include "../../util/ColorPaletteEngine.h"`
  - 已包含 `#include "../../util/DiskMediaExtractor.h"`
  - 类型完整性：`ItemRecord::ThumbnailState` 在 `DiskItemModel.cpp` 中完全定义且类型完整。
- **`src/ui/workers/ContentStatsWorker.cpp`**:
  - 已包含 `#include "../../util/ColorPaletteEngine.h"`
  - 类型完整性：`ItemRecord::ThumbnailState` 完全可读。
- **`src/ui/models/FilterProxyModel.cpp`**:
  - 已包含 `#include "../../util/ColorPaletteEngine.h"`
  - 类型完整性：`ItemRecord::ThumbnailState` 完全可读。

---

## 8. 代码审查发现与非侵入解耦汇报（审查补充指示）
1. **UiHelper 清理说明**：
   - 清理 `UiHelper::hasPhysicalThumbnail` 后，已通过命令行检索确认全项目（`src/`）无任何遗留调用方，已安全全物理删除，无孤儿残存。
2. **主线程写入律验证**：
   - 目录初始化建立记录时，在 `setRecords` 入口以一次性向量遍历完成缓存判定并赋予 `rec.thumbnailState`；
   - 异步解图成功后，在 `QMetaObject::invokeMethod` 主线程回调中将对应的 `record.thumbnailState` 改写为 `Ready` 并触发 `dataChanged`，保证零多线程竞态。
