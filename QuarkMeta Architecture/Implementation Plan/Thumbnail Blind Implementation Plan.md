# Folder Navigation Priority Preemption & 3-Level Thumbnail Degradation Plan

## Problem Summary & Root Cause Analysis

When a user double-clicks to open a folder (even one containing only 1 file), the application UI hangs ("Not Responding" state).
As identified by the user, this is caused by **unreasonable task priorities, background task contention, and un-cached failed extraction retries**:
1. When double-clicking a new folder, background worker threads (`MediaExtractorPipeline`, `DiskScanService`, and `QtConcurrent` jobs in `DiskItemModel`) continue extracting thumbnails and features for items in **previously visited folders**.
2. They retain locks (`CapsuleMediaExtractor::s_qtGuiMutex`, SQLite connection handles, and `MetadataManager` shard locks) and consume thread pool workers.
3. If an image fails to extract or is corrupt, repeated extraction retries on scroll re-trigger heavy parsing attempts.

---

## Technical Ironclad Directives

### 1. Mandatory Image Storage Format Rules
- **PNG Format Only**: All disk thumbnail caches **MUST** strictly use `.png` format (`QImage::save(..., "PNG")`).
- **JPEG Strictly Prohibited**: No `.jpg` or `.jpeg` formats are allowed for cached disk thumbnails.

### 2. 3-Level Graceful Degradation & Fallback Chain
When a thumbnail extraction is requested for any asset, the extraction pipeline executes the following 3-level fallback chain:

```text
1. Primary Strategy: Extract Embedded High-Resolution Thumbnail (512x512 PNG)
       │ (If corrupted, unparseable, or no embedded preview exists)
       ▼
2. Secondary Fallback: Retrieve System-Associated High-Resolution Icon (e.g., Native Shell Icons for AI/EPS/PSD/RAW)
       │ (If system shell icon is unavailable or invalid)
       ▼
3. Tertiary Fallback: Render Refined Extension-Based Vector Badge Icon (Guarantees elegant UI layout)
```

### 3. Anti-Reentry & Failed Extraction Caching (Zero-Lag Guarantee)
- When an asset fails at Step 1 and falls back to a substitute icon (Step 2 or Step 3), the result is **immediately recorded in the memory cache (`m_iconCache` / `m_requestedPaths`)** as processed.
- **Benefit**: Even if a user scrolls back and forth across thousands of corrupted or unsupported files, the pipeline **NEVER re-attempts extraction** for failed items.

---

## Detailed C++ Implementation Plan

### 1. Implemented Task Preemption & Cancellation in `ContentPanel::loadDirectory`
**Target File:** `src/ui/ContentPanel.cpp`

When `loadDirectory()` is called upon double-clicking or navigating to a new folder, immediately preempt and cancel all in-flight background extraction queues from past folders.

```cpp
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    restoreActiveView();

    // 🚨 物理抢占与中断：用户双击/切换新文件夹时，立刻拔除并清空上一文件夹残存的全部后台提图与特征提取队列
    MediaExtractorPipeline::instance().cancelAll();

    if (m_model != m_diskModel) {
        m_model = m_diskModel;
        m_proxyModel->setSourceModel(m_model);
    }

    m_isLoading = true;
    int reqId = ++m_loadRequestId;
```

---

### 2. PNG Format & 3-Level Fallback Pipeline in `CapsuleMediaExtractor`
**Target File:** `src/meta/CapsuleMediaExtractor.h` & `src/meta/CapsuleMediaExtractor.cpp`

Ensure disk thumbnail saving strictly enforces PNG format and caches fallback icons immediately upon extraction failure to prevent re-entry.

```cpp
bool CapsuleMediaExtractor::saveDiskThumbnail(const QString& rawPath, const QImage& img) {
    if (img.isNull()) return false;

    QString fileKey = QString("%1.png").arg(QString::fromStdString(calculateFileIdHash(rawPath)));
    QString thumbPath = getCacheDir() + "/" + fileKey;
    QFileInfo fi(thumbPath);
    QDir().mkpath(fi.absolutePath());

    // 🚨 强制要求：统一采用 PNG 格式保存缩略图，严禁使用 JPG！
    return img.save(thumbPath, "PNG");
}

QImage CapsuleMediaExtractor::getDiskThumbnailWithFallback(const QString& rawPath, int targetSize) {
    // 1. 尝试 512 PNG 缩略图
    QImage thumb = getDiskThumbnail(rawPath, targetSize);
    if (!thumb.isNull()) return thumb;

    // 2. 退化为：系统关联高清格式图标 (Win32 Shell Icon)
    QIcon shellIcon = ShellIconManager::instance().getIcon(rawPath);
    if (!shellIcon.isNull()) {
        QPixmap pix = shellIcon.pixmap(targetSize, targetSize);
        if (!pix.isNull()) return pix.toImage();
    }

    // 3. 退化为：基于扩展名的精致矢量徽章图标
    QString ext = QFileInfo(rawPath).suffix().toLower();
    return SvgIcons::getBadgeIconForExtension(ext, targetSize);
}
```

---

### 3. Reset Pipeline Cancel Token in `MediaExtractorPipeline`
**Target File:** `src/meta/MediaExtractorPipeline.h` & `src/meta/MediaExtractorPipeline.cpp`

Update `MediaExtractorPipeline` to safely cancel all running tasks, clear the queue, and automatically reset the cancellation token when new folder items are enqueued.

```cpp
void MediaExtractorPipeline::enqueue(const std::wstring& path) {
    m_isCanceled.store(false); // Reset cancel token for active directory
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.push_back(path);
    dispatchWorkersIfNeeded();
}

void MediaExtractorPipeline::cancelAll() {
    m_isCanceled.store(true);
    {
        std::lock_guard<std::mutex> queueLock(m_queueMutex);
        m_queue.clear();
    }
    m_activeCount.store(0);
    SyncStatusService::instance().updateMediaPending(0);
}
```

---

### 4. Invalidate Thumbnail Generation Counter & Anti-Reentry in `DiskItemModel`
**Target File:** `src/ui/models/DiskItemModel.cpp`

When setting new records for a folder (`setRecords` / `clear`), increment a generation counter (`m_currentGen`) to immediately invalidate and discard any running thumbnail extraction callbacks from previous folders.

```cpp
void DiskItemModel::setRecords(const std::vector<ItemRecord>& records) {
    beginResetModel();
    m_allRecords = records;
    m_pathToIndex.clear();
    m_requestedPaths.clear();
    uint64_t currentGen = m_currentGen.fetch_add(1, std::memory_order_relaxed) + 1;
    for (int i = 0; i < static_cast<int>(m_allRecords.size()); ++i) {
        m_pathToIndex[m_allRecords[i].path] = i;
    }
    m_iconCache.setMaxCost(qMax(500, static_cast<int>(m_allRecords.size()) + 50));
    m_requestedIcons.clear();
    endResetModel();
}
```

In `loadThumbnailsForRows`:
```cpp
    QPointer<DiskItemModel> weakThis(this);
    uint64_t gen = m_currentGen.load(std::memory_order_relaxed);
    (void)QtConcurrent::run([weakThis, newQueue, gen]() {
        for (const auto& task : newQueue) {
            // Discard stale thumbnail extraction if folder generation changed
            if (!weakThis || CoreController::isShuttingDown() || weakThis->m_currentGen.load(std::memory_order_relaxed) != gen) break;

            QString path = task.first;
            QImage img = CapsuleMediaExtractor::getDiskThumbnailWithFallback(path, 512);

            // 🚨 防重入核心：即使退化为替补图标，也立即记入 m_iconCache，不再重复重试解析损坏文件！
            QIcon icon(QPixmap::fromImage(img));
            QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, path, icon, gen]() {
                if (weakThis && weakThis->m_currentGen.load(std::memory_order_relaxed) == gen) {
                    weakThis->m_iconCache.insert(path, new QIcon(icon));
                    int row = weakThis->m_pathToIndex.value(path, -1);
                    if (row >= 0) {
                        emit weakThis->dataChanged(weakThis->index(row, 0), weakThis->index(row, 0), {Qt::DecorationRole});
                    }
                }
            });
        }
    });
```

---

## Verification Plan

### Test Steps
1. **PNG Only File Verification:**
   - Run thumbnail extraction for an image folder and inspect disk cache under `.QuarkMeta/thumbnails/`.
   - Confirm all cached thumbnail files end in `.png` and no `.jpg`/`.jpeg` files exist.
2. **3-Level Degradation & Fallback Test:**
   - Test corrupt or unparseable image files (`.psd`, `.ai`, broken `.png`).
   - Confirm step 1 fails gracefully -> falls back to system shell icon -> falls back to badge icon.
3. **Anti-Reentry & Scroll Performance Test:**
   - Scroll up and down repeatedly over hundreds of corrupt/unparseable files.
   - Confirm extraction is only performed once and cached immediately, maintaining zero lag.
