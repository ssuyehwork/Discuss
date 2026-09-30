# Implementation Plan - ItemRecord-1

## 1. Overview（概述与解决的问题）
本补充实施方案针对 Code Review 中指出的 `DiskItemModel::setRecords` 在主线程同步调用 `QFile::exists` 检查磁盘缩略图缓存存在性的潜在 IO 堵塞问题进行彻底重构：
1. **彻底解耦主线程磁盘 IO**：在 `setRecords` 主线程流程中，图形文件初始默认标记为 `Pending`（或 `Failed`），不执行任何 `QFile::exists` 磁盘 IO 探测；
2. **后台异步探测 + 主线程回调写入**：启动后台线程池任务异步扫描图形文件的磁盘缓存文件存在性，探测完毕后通过 `QMetaObject::invokeMethod` 切回主线程，由主线程安全将探测到缓存存在的记录更新为 `Ready` 状态并发射 `dataChanged(..., {HasThumbnailRole})` 信号。

---

## 2. Modified Files List（影响文件清单）
- `src/ui/models/DiskItemModel.cpp`

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### `src/ui/models/DiskItemModel.cpp`
```cpp
<<<<<<< SEARCH
    // 在后台探测文件系统前，先收集批量检测标记
    std::vector<bool> cacheExistFlags(m_allRecords.size(), false);
    for (size_t i = 0; i < m_allRecords.size(); ++i) {
        const auto& rec = m_allRecords[i];
        if (!rec.isDir && ColorPaletteEngine::isGraphicsFile(rec.suffix.toLower())) {
            if (rec.thumbStatus != 1) {
                QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(rec.path);
                cacheExistFlags[i] = QFile::exists(thumbPath);
            }
        }
    }

    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        auto& rec = m_allRecords[i];
        m_pathToIndex[rec.path] = i;

        if (rec.isDir || !ColorPaletteEngine::isGraphicsFile(rec.suffix.toLower())) {
            rec.thumbnailState = ItemRecord::ThumbnailState::NotApplicable;
        } else if (rec.thumbStatus == 1) {
            rec.thumbnailState = ItemRecord::ThumbnailState::Failed;
        } else if (cacheExistFlags[i]) {
            rec.thumbnailState = ItemRecord::ThumbnailState::Ready;
        } else {
            rec.thumbnailState = ItemRecord::ThumbnailState::Pending;
        }
=======
    std::vector<std::pair<int, QString>> pendingTargets;

    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        auto& rec = m_allRecords[i];
        m_pathToIndex[rec.path] = i;

        if (rec.isDir || !ColorPaletteEngine::isGraphicsFile(rec.suffix.toLower())) {
            rec.thumbnailState = ItemRecord::ThumbnailState::NotApplicable;
        } else if (rec.thumbStatus == 1) {
            rec.thumbnailState = ItemRecord::ThumbnailState::Failed;
        } else {
            rec.thumbnailState = ItemRecord::ThumbnailState::Pending;
            pendingTargets.push_back({i, rec.path});
        }
>>>>>>> REPLACE
```

并紧接着在 `endResetModel()` 之后异步启动后台检测：
```cpp
<<<<<<< SEARCH
    endResetModel();

    preloadDimensionsAsync();
=======
    endResetModel();

    // 🚀【异步零卡顿】：在后台线程检测已存在的磁盘缩略图缓存，纯靠主线程回调写入 Ready 状态
    if (!pendingTargets.empty()) {
        uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);
        QPointer<DiskItemModel> weakThis(this);

        thumbnailPool()->start([weakThis, targets = std::move(pendingTargets), thisGen]() {
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;

            std::vector<QString> readyPaths;
            readyPaths.reserve(targets.size());

            for (const auto& target : targets) {
                if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) return;
                QString thumbPath = DiskMediaExtractor::getDiskThumbCachePath(target.second);
                if (QFile::exists(thumbPath)) {
                    readyPaths.push_back(target.second);
                }
            }

            if (readyPaths.empty() || !weakThis || weakThis->currentGeneration() != thisGen) return;

            QMetaObject::invokeMethod(weakThis.data(), [weakThis, readyPaths = std::move(readyPaths), thisGen]() {
                if (!weakThis || weakThis->currentGeneration() != thisGen) return;

                int minRow = std::numeric_limits<int>::max();
                int maxRow = std::numeric_limits<int>::min();

                for (const QString& path : readyPaths) {
                    auto it = weakThis->m_pathToIndex.find(path);
                    if (it != weakThis->m_pathToIndex.end()) {
                        int r = it->second;
                        if (r >= 0 && r < static_cast<int>(weakThis->m_allRecords.size())) {
                            weakThis->m_allRecords[r].thumbnailState = ItemRecord::ThumbnailState::Ready;
                            if (r < minRow) minRow = r;
                            if (r > maxRow) maxRow = r;
                        }
                    }
                }

                if (minRow <= maxRow) {
                    emit weakThis->dataChanged(
                        weakThis->index(minRow, 0),
                        weakThis->index(maxRow, weakThis->columnCount() - 1),
                        {HasThumbnailRole}
                    );
                }
            }, Qt::QueuedConnection);
        });
    }

    preloadDimensionsAsync();
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. **验证无卡顿主线程逻辑**：`setRecords` 在 UI 主线程中零文件系统访问，缩略图缓存判定 100% 移至 `thumbnailPool()` 后台线程。
2. **验证主线程写入律**：状态切换为 `Ready` 严格运行在 `QMetaObject::invokeMethod` 的主线程事件循环内。

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 完美契合 `ItemRecord::thumbnailState` 唯一真理源通道，改动彻底消除主线程文件系统调用。

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / 枚举 | 所在的头文件 | 精准物理签名 | 核查状态 |
| :--- | :--- | :--- | :--- |
| `DiskItemModel::thumbnailPool` | `src/ui/models/DiskItemModel.h` | `static QThreadPool* thumbnailPool();` |  物理核实通过 |
| `QFile::exists` | `<QFile>` | `static bool exists(const QString &fileName)` |  物理核实通过 |

---

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）

- `src/ui/models/DiskItemModel.cpp`:
  - 已经具备完整的 `#include <QFile>`, `#include "DiskMediaExtractor.h"`, `#include "ItemRecord.h"`
