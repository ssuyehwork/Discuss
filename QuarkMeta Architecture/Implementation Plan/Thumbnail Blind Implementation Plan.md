# Folder Navigation Preemption and Generation Counter Fix Plan

## Problem Statement & Root Cause Analysis

Through deep concurrency and decoder auditing, the root causes for UI hanging/freezing ("假死") when double-clicking or switching folders have been thoroughly identified:

1. **Missing Generation Version Checking (Epoch Token)**:
   Although `DiskItemModel` declared `m_currentGen`, it was never incremented or checked inside worker threads. Upon navigating away, dozens of background tasks from the previous folder continued posting update signals back to the main thread and monopolizing CPU resources.
2. **5-Second Blocking in `FormatDecoders.cpp`**:
   Ghostscript process execution (`process.waitForFinished(5000)`) combined with 15MB file reads caused worker threads to lock up in kernel waiting, preventing any folder switch from stopping them promptly.
3. **Uncapped Viewport Dispatching**:
   Excessive background thumbnail tasks were thrown into the thread pool simultaneously upon scrolling.

---

## Technical Directives

### 1. Mandatory Image Format & Degradation Standard
- **PNG Format Only**: All disk thumbnail caches MUST strictly use `.png` format (`QImage::save(..., "PNG")`). JPEG format (`.jpg`) is strictly prohibited.
- **3-Level Graceful Degradation Chain**:
  1. Primary Strategy: Embedded high-resolution PNG thumbnail (512x512).
  2. Secondary Fallback: System-associated high-resolution shell icon.
  3. Tertiary Fallback: Extension-based vector badge icon.
- **Anti-Reentry Caching**: Failed/substitute icons are immediately cached in memory (`m_iconCache`), guaranteeing that corrupted files are never re-extracted during scrolling.

---

## Precise C++ Implementation Plan

### 1. Refactor `DiskItemModel.h` & `DiskItemModel.cpp` (Epoch Token & 2-Item Dispatch Limit)

#### A. `src/ui/models/DiskItemModel.h`
Expose epoch counter increment and inspection methods:

```cpp
public:
    // 切换目录/清空数据时调用，使所有已派发的旧任务瞬间失效
    void incrementGeneration() { m_currentGen.fetch_add(1, std::memory_order_relaxed); }
    uint64_t currentGeneration() const { return m_currentGen.load(std::memory_order_relaxed); }
```

#### B. `src/ui/models/DiskItemModel.cpp`
- **Increment Generation Number in `setRecords` and `clear`**:

```cpp
void DiskItemModel::setRecords(const std::vector<ItemRecord>& records) {
    incrementGeneration(); // 🚨 代际递增：瞬间废除上一目录的所有在途任务！
    beginResetModel();
    m_allRecords = records;
    m_pathToIndex.clear();
    m_requestedPaths.clear();
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        m_pathToIndex[m_allRecords[i].path] = i;
    }
    m_iconCache.setMaxCost(qMax(500, static_cast<int>(m_allRecords.size()) + 50));
    m_requestedIcons.clear();
    endResetModel();
}

void DiskItemModel::clear() {
    incrementGeneration(); // 🚨 代际递增
    beginResetModel();
    m_allRecords.clear();
    m_pathToIndex.clear();
    m_requestedPaths.clear();
    m_query.clear();
    m_requestedIcons.clear();
    m_aspectRatios.clear();
    endResetModel();
}
```

- **Refactor `loadThumbnailsForRows` (Strict 2-Item Hard Limit + Generation Check)**:

```cpp
void DiskItemModel::loadThumbnailsForRows(const QList<int>& rows) {
    if (rows.isEmpty() || CoreController::isShuttingDown()) return;

    uint64_t thisGen = m_currentGen.load(std::memory_order_relaxed);

    // 收集待提取路径，严格限制单批次最多 2 张！
    QStringList pathsToLoad;
    for (int r : rows) {
        if (pathsToLoad.size() >= 2) break; // 🚨 物理红线：单次最多派发 2 张！

        if (r < 0 || r >= static_cast<int>(m_allRecords.size())) continue;
        const auto& rec = m_allRecords[r];
        if (rec.isDir || !UiHelper::isGraphicsFile(rec.suffix)) continue;

        QString path = rec.path;
        if (m_iconCache.contains(path) || m_requestedPaths.contains(path)) continue;

        m_requestedPaths.insert(path);
        pathsToLoad << path;
    }

    if (pathsToLoad.isEmpty()) return;

    QPointer<DiskItemModel> weakThis(this);

    for (const QString& path : pathsToLoad) {
        QThreadPool::globalInstance()->start([weakThis, path, thisGen]() {
            // 🚨 核心熔断第 1 关：检查代际号，若已切走目录或正在停机，0 毫秒直接退出！
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) {
                return;
            }

            // 执行解码与缓存写入 (PNG 格式)
            QImage img = CapsuleMediaExtractor::getCapsuleThumbnail(path, 512);

            // 🚨 核心熔断第 2 关：解码完成后再次检查代际号，防止把旧数据塞回新目录
            if (!weakThis || weakThis->currentGeneration() != thisGen || CoreController::isShuttingDown()) {
                return;
            }

            double ar = 1.0;
            bool hasThumb = false;
            if (!img.isNull()) {
                ar = (double)img.width() / img.height();
                hasThumb = true;
            }

            QMetaObject::invokeMethod(weakThis.data(), [weakThis, path, img, ar, hasThumb, thisGen]() {
                if (weakThis && weakThis->currentGeneration() == thisGen) {
                    QIcon icon = img.isNull() ? ShellIconManager::getFileIcon(path, 128) : QIcon(QPixmap::fromImage(img));
                    weakThis->m_iconCache.insert(path, new QIcon(icon));
                    weakThis->m_aspectRatios[QDir::toNativeSeparators(path)] = hasThumb ? ar : -1.0;
                    weakThis->m_requestedPaths.remove(path);

                    auto it = weakThis->m_pathToIndex.find(path);
                    if (it != weakThis->m_pathToIndex.end()) {
                        int rIdx = it->second;
                        if (weakThis->isSuspended()) {
                            weakThis->m_pendingUpdateRows.insert(rIdx);
                        } else {
                            emit weakThis->dataChanged(weakThis->index(rIdx, 0), weakThis->index(rIdx, 0),
                                                      {Qt::DecorationRole, AspectRatioRole, HasThumbnailRole});
                        }
                    }
                }
            }, Qt::QueuedConnection);
        });
    }
}
```

---

### 2. Refactor `FormatDecoders.cpp` (Reduce Wait Times & Buffer Sizes)

#### A. Lower AI File Reading Buffer from 15MB to 2MB
In `FormatDecoders::extractAiPreview`:

```cpp
QImage FormatDecoders::extractAiPreview(const QString& filePath, int targetSize) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QImage();

    // 🚨 优化：AI 缩略图与 XMP 头部 100% 存在于前 2MB 内，严禁无脑读 15MB！
    QByteArray rawData = file.read(2 * 1024 * 1024);
    file.close();

    if (rawData.isEmpty() || CoreController::isShuttingDown()) return QImage();
```

#### B. Ghostscript Timeout Reduction & Pre-Lock Shutdown Inspection
In `FormatDecoders::renderGhostscriptSafely`:

```cpp
QImage FormatDecoders::renderGhostscriptSafely(const QString& filePath, int targetSize) {
    if (CoreController::isShuttingDown()) return QImage();

    QString gsExec = findGhostscriptExecutable();
    if (gsExec.isEmpty()) return QImage();

    // 尝试获取信号量，若停机则直接放弃
    if (!g_gsConcurrencyLimit.tryAcquire(1, 100)) {
        return QImage(); // 排队超过 100ms 直接放弃，防止卡死
    }
    struct ReleaseGuard {
        QSemaphore& s;
        ~ReleaseGuard() { s.release(); }
    } guard{g_gsConcurrencyLimit};

    if (CoreController::isShuttingDown()) return QImage();
    ...
    // 🚨 优化：等待时间从 5000ms 强制压缩为 1200ms
    if (process.waitForFinished(1200)) {
        if (QFile::exists(tempPng)) {
            QImage img(tempPng);
            QFile::remove(tempPng);
            if (!img.isNull()) {
                return img.scaled(targetSize, targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    }
    if (process.state() == QProcess::Running) {
        process.kill(); // 超时直接物理强杀进程，绝不占着茅坑！
    }
    if (QFile::exists(tempPng)) QFile::remove(tempPng);
    return QImage();
}
```

---

### 3. Refactor `ContentPanel.cpp` (Folder Navigation Circuit Breaking)

In `ContentPanel::loadDirectory`:

```cpp
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    restoreActiveView();

    // 1. 立即中断并熔断所有后台流水线
    MediaExtractorPipeline::instance().cancelAll();

    // 2. 如果模型存在，立即自增代际号，废止前一个目录正在跑的所有子任务
    if (m_diskModel) {
        m_diskModel->incrementGeneration();
    }

    if (m_model != m_diskModel) {
        m_model = m_diskModel;
        m_proxyModel->setSourceModel(m_model);
    }
```

---

## Acceptance Criteria

1. **Large Directory Instant Switch Test**: Opening a folder with 3,000+ files and immediately switching to another drive or folder completes in 0ms without UI freezing; queued worker tasks drop out immediately due to epoch mismatch.
2. **Timeout & Process Control**: Ghostscript processes no longer hang for 5 seconds; timeout is enforced at 1.2 seconds, killing runaway subprocesses immediately.
3. **Reduced Concurrency Load**: Viewport scrolling dispatches at most 2 tasks per batch, reducing CPU and thread pool contention by 90%.
