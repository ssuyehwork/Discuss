# Implementation Plan - PresetTagsProgressAndLockFix

**Base Commit Hash**: `c6b97d6bd6005607baff5a68d45f068eb1c33dab`

---

## 一、 概述 (Overview)

本方案旨在解决点击“保存设置”按钮（`PresetTagsDialog`）时出现的两个关键缺陷：
1. **0ms 即时任务进度条响应**：在此前逻辑中，`TaskProgressService::createJob` 延迟在后台工作线程中调用，导致点击“保存设置”后无法第一时间在主界面顶端唤起 `TaskProgressStrip` 进度条。本方案通过在主线程调用 `saveLibraryPresetTags` 的瞬间（0ms）即时创建 `TaskProgressService` Job 并绑定到 `TaskProgressStrip` 浮层，确保视觉反馈毫秒级响应。
2. **`QuarkMetaJsonStore` 细粒度锁解耦**：此前的 `QuarkMetaJsonStore::addTagsBatch` 使用大锁 `std::lock_guard<std::mutex> lock(m_storeMutex)` 贯穿整个批量跨文件夹更新流程，导致后台异步打标签时长时间锁住 `m_storeMutex`，阻塞主线程/扫描线程的 `readFolderMeta` 操作，引发目录加载挂起并呈现“没有可显示的项目”的假死缺陷。本方案将锁范围缩小为“按文件夹粒度”迭代加锁/解锁，消除锁竞争与死锁风险。

---

## 二、 修改文件清单 (Modified Files List)

| 序号 | 物理文件路径 | 模块/组件 | 修改性质 |
| :---: | :--- | :--- | :--- |
| 1 | `src/core/PresetTagService.h` | Core / Domain | 拓展 API 签名与内部异步方法参数 |
| 2 | `src/core/PresetTagService.cpp` | Core / Domain | 主线程 0ms 创建 TaskProgressJob 并传递 JobId |
| 3 | `src/core/CoreEngine.h` | Core / Domain | 拓展 `addTagsToPathsAsync` 支持复用既有 JobId |
| 4 | `src/core/CoreEngine.cpp` | Core / Domain | 实现复用 JobId 及动态进度更新与 Job 结束 |
| 5 | `src/meta/QuarkMetaJsonStore.cpp` | Meta / Data | `addTagsBatch` 大锁拆分为按文件夹细粒度加锁 |

---

## 三、 逐行变更明细 (Detailed Line-by-Line Changes)

### 1. `src/core/PresetTagService.h`

```
<<<<<<< SEARCH
    void processAsyncPresetTagsUpdate(int categoryId, const QStringList& newTags, const QStringList& newlyAddedTags, std::function<void(int tagCount, int itemCount)> onTraceFinished);
=======
    void processAsyncPresetTagsUpdate(int jobId, int categoryId, const QStringList& newTags, const QStringList& newlyAddedTags, std::function<void(int tagCount, int itemCount)> onTraceFinished);
>>>>>>> REPLACE
```

---

### 2. `src/core/PresetTagService.cpp`

```
<<<<<<< SEARCH
#include "PresetTagService.h"
#include "../meta/LibraryDao.h"
#include "CoreEngine.h"
#include <QSet>
#include <QtConcurrent>
#include <QApplication>
=======
#include "PresetTagService.h"
#include "../meta/LibraryDao.h"
#include "CoreEngine.h"
#include "TaskProgressService.h"
#include <QSet>
#include <QtConcurrent>
#include <QApplication>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool PresetTagService::saveLibraryPresetTags(int categoryId, const QStringList& newTags, const QStringList& initialTags, std::function<void(int tagCount, int itemCount)> onTraceFinished) {
    // 1. 计算相对 initialTags 新增的标签 (忽略大小写比较)
    QSet<QString> initialSetLower;
    for (const QString& tag : initialTags) {
        initialSetLower.insert(tag.toLower());
    }

    QStringList newlyAddedTags;
    for (const QString& tag : newTags) {
        if (!initialSetLower.contains(tag.toLower())) {
            newlyAddedTags.append(tag);
        }
    }

    // 2. 异步投递到后台工作线程池，主线程立即返回 0 毫秒无卡顿
    (void)QtConcurrent::run([this, categoryId, newTags, newlyAddedTags, onTraceFinished]() {
        processAsyncPresetTagsUpdate(categoryId, newTags, newlyAddedTags, onTraceFinished);
    });

    return true;
}

void PresetTagService::processAsyncPresetTagsUpdate(int categoryId, const QStringList& newTags, const QStringList& newlyAddedTags, std::function<void(int tagCount, int itemCount)> onTraceFinished) {
    // A. 后台写 SQLite 数据库
    if (!LibraryDao::updatePresetTags(categoryId, newTags)) {
        return;
    }

    if (newlyAddedTags.isEmpty()) {
        if (onTraceFinished) {
            QMetaObject::invokeMethod(qApp, [onTraceFinished]() {
                onTraceFinished(0, 0);
            }, Qt::QueuedConnection);
        }
        return;
    }

    // B. 后台线程高效检索分类下关联的所有文件路径
    QStringList paths = LibraryDao::getCategoryPaths(categoryId, false);
    paths.removeDuplicates();
    if (paths.isEmpty()) {
        if (onTraceFinished) {
            int newlyAddedCount = newlyAddedTags.size();
            QMetaObject::invokeMethod(qApp, [onTraceFinished, newlyAddedCount]() {
                onTraceFinished(newlyAddedCount, 0);
            }, Qt::QueuedConnection);
        }
        return;
    }

    // C. 定向批处理：在后台按物理父文件夹归类，分别直接读写各自的 .QuarkMeta.json
    int newlyAddedCount = newlyAddedTags.size();
    CoreEngine::instance().addTagsToPathsAsync(paths, newlyAddedTags, [onTraceFinished, newlyAddedCount](int pathCount) {
        if (onTraceFinished) {
            onTraceFinished(newlyAddedCount, pathCount);
        }
    });
}
=======
bool PresetTagService::saveLibraryPresetTags(int categoryId, const QStringList& newTags, const QStringList& initialTags, std::function<void(int tagCount, int itemCount)> onTraceFinished) {
    // 1. 计算相对 initialTags 新增的标签 (忽略大小写比较)
    QSet<QString> initialSetLower;
    for (const QString& tag : initialTags) {
        initialSetLower.insert(tag.toLower());
    }

    QStringList newlyAddedTags;
    for (const QString& tag : newTags) {
        if (!initialSetLower.contains(tag.toLower())) {
            newlyAddedTags.append(tag);
        }
    }

    // 2. 🚀【0ms 即时唤起进度条】：主线程被调用瞬间立即创建进度条 Job，保证视觉反馈零延迟
    int jobId = TaskProgressService::instance().createJob("正在应用预设标签...", 0);

    // 3. 异步投递到后台工作线程池，主线程立即返回 0 毫秒无卡顿
    (void)QtConcurrent::run([this, jobId, categoryId, newTags, newlyAddedTags, onTraceFinished]() {
        processAsyncPresetTagsUpdate(jobId, categoryId, newTags, newlyAddedTags, onTraceFinished);
    });

    return true;
}

void PresetTagService::processAsyncPresetTagsUpdate(int jobId, int categoryId, const QStringList& newTags, const QStringList& newlyAddedTags, std::function<void(int tagCount, int itemCount)> onTraceFinished) {
    // A. 后台写 SQLite 数据库
    if (!LibraryDao::updatePresetTags(categoryId, newTags)) {
        TaskProgressService::instance().finishJob(jobId);
        return;
    }

    if (newlyAddedTags.isEmpty()) {
        TaskProgressService::instance().finishJob(jobId);
        if (onTraceFinished) {
            QMetaObject::invokeMethod(qApp, [onTraceFinished]() {
                onTraceFinished(0, 0);
            }, Qt::QueuedConnection);
        }
        return;
    }

    // B. 后台线程高效检索分类下关联的所有文件路径
    QStringList paths = LibraryDao::getCategoryPaths(categoryId, false);
    paths.removeDuplicates();
    if (paths.isEmpty()) {
        TaskProgressService::instance().finishJob(jobId);
        if (onTraceFinished) {
            int newlyAddedCount = newlyAddedTags.size();
            QMetaObject::invokeMethod(qApp, [onTraceFinished, newlyAddedCount]() {
                onTraceFinished(newlyAddedCount, 0);
            }, Qt::QueuedConnection);
        }
        return;
    }

    // C. 定向批处理：在后台按物理父文件夹归类，复用现有 jobId 推进 TaskProgressStrip
    int newlyAddedCount = newlyAddedTags.size();
    CoreEngine::instance().addTagsToPathsAsync(paths, newlyAddedTags, [onTraceFinished, newlyAddedCount](int pathCount) {
        if (onTraceFinished) {
            onTraceFinished(newlyAddedCount, pathCount);
        }
    }, jobId);
}
>>>>>>> REPLACE
```

---

### 3. `src/core/CoreEngine.h`

```
<<<<<<< SEARCH
    void addTagsToPathsAsync(const QStringList& paths, const QStringList& tags, std::function<void(int pathCount)> onFinished = nullptr);
=======
    void addTagsToPathsAsync(const QStringList& paths, const QStringList& tags, std::function<void(int pathCount)> onFinished = nullptr, int existingJobId = 0);
>>>>>>> REPLACE
```

---

### 4. `src/core/CoreEngine.cpp`

```
<<<<<<< SEARCH
void CoreEngine::addTagsToPathsAsync(const QStringList& paths, const QStringList& tags, std::function<void(int pathCount)> onFinished) {
    if (paths.isEmpty() || tags.isEmpty()) {
        if (onFinished) {
            QMetaObject::invokeMethod(qApp, [onFinished, pathCount = paths.size()]() {
                onFinished(pathCount);
            }, Qt::QueuedConnection);
        }
        return;
    }

    m_asyncTagPool.start([paths, tags, onFinished]() {
        int totalPaths = paths.size();
        int jobId = TaskProgressService::instance().createJob("正在应用预设标签...", totalPaths);

        TagLexiconService::instance().addTags(tags);

        constexpr int kBatchSize = 100;
        int processed = 0;
        for (int i = 0; i < totalPaths; i += kBatchSize) {
            QStringList batch = paths.mid(i, kBatchSize);
            MetadataManager::instance().addTagsBatch(batch, tags);
            processed += batch.size();
            TaskProgressService::instance().updateProgress(jobId, processed, totalPaths);
        }

        TaskProgressService::instance().finishJob(jobId);

        QMetaObject::invokeMethod(qApp, [paths, onFinished]() {
            AppEvent ev;
            ev.type = AppEventType::MetadataUpdated;
            ev.paths = paths;
            CentralEventHub::instance().publishEvent(ev);

            if (onFinished) {
                onFinished(paths.size());
            }
        }, Qt::QueuedConnection);
    });
}
=======
void CoreEngine::addTagsToPathsAsync(const QStringList& paths, const QStringList& tags, std::function<void(int pathCount)> onFinished, int existingJobId) {
    if (paths.isEmpty() || tags.isEmpty()) {
        if (existingJobId > 0) {
            TaskProgressService::instance().finishJob(existingJobId);
        }
        if (onFinished) {
            QMetaObject::invokeMethod(qApp, [onFinished, pathCount = paths.size()]() {
                onFinished(pathCount);
            }, Qt::QueuedConnection);
        }
        return;
    }

    m_asyncTagPool.start([paths, tags, onFinished, existingJobId]() {
        int totalPaths = paths.size();
        int jobId = existingJobId > 0 ? existingJobId : TaskProgressService::instance().createJob("正在应用预设标签...", totalPaths);

        TagLexiconService::instance().addTags(tags);

        constexpr int kBatchSize = 100;
        int processed = 0;
        for (int i = 0; i < totalPaths; i += kBatchSize) {
            QStringList batch = paths.mid(i, kBatchSize);
            MetadataManager::instance().addTagsBatch(batch, tags);
            processed += batch.size();
            TaskProgressService::instance().updateProgress(jobId, processed, totalPaths, QString("已应用 %1 / %2 项").arg(processed).arg(totalPaths));
        }

        TaskProgressService::instance().finishJob(jobId);

        QMetaObject::invokeMethod(qApp, [paths, onFinished]() {
            AppEvent ev;
            ev.type = AppEventType::MetadataUpdated;
            ev.paths = paths;
            CentralEventHub::instance().publishEvent(ev);

            if (onFinished) {
                onFinished(paths.size());
            }
        }, Qt::QueuedConnection);
    });
}
>>>>>>> REPLACE
```

---

### 5. `src/meta/QuarkMetaJsonStore.cpp`

```
<<<<<<< SEARCH
    for (const auto& fPath : uniqueFoldersToPrefetch) {
        prefetchFolderJson(fPath);
    }

    {
        std::lock_guard<std::mutex> lock(m_storeMutex);

        for (auto& pair : folderGroups) {
            const std::wstring& normFolder = pair.first;
            const auto& itemsInFolder = pair.second;
            if (itemsInFolder.empty()) continue;

            auto jsonPtr = getOrLoadFolderLocked(normFolder, itemsInFolder.front().folderPath);

            for (const auto& item : itemsInFolder) {
                ItemMeta& meta = jsonPtr->items()[item.fileNameKey];
                if (meta.type.empty()) {
                    meta.type = item.info.isDir() ? L"folder" : L"file";
                }

                QStringList finalTags = mergeAddTagsInternal(meta.tags, tags);
                meta.tags = toStdWStringVector(finalTags);
                resultMap[item.nativePath] = finalTags;

                if (item.info.isDir()) {
                    std::wstring selfDirPath = item.info.absoluteFilePath().toStdWString();
                    std::wstring selfNorm = normalizeFolderPath(selfDirPath);
                    auto selfJsonPtr = getOrLoadFolderLocked(selfNorm, selfDirPath);
                    selfJsonPtr->folder().tags = meta.tags;
                    markDirtyLocked(selfNorm);
                }
            }

            markDirtyLocked(normFolder);
        }
    }

    scheduleFlush();
=======
    for (const auto& fPath : uniqueFoldersToPrefetch) {
        prefetchFolderJson(fPath);
    }

    // 🚀【细粒度锁优化】：避免用一个大锁贯穿所有文件夹循环，改为按文件夹独立加锁，防止挂起 readFolderMeta
    for (auto& pair : folderGroups) {
        const std::wstring& normFolder = pair.first;
        const auto& itemsInFolder = pair.second;
        if (itemsInFolder.empty()) continue;

        std::lock_guard<std::mutex> lock(m_storeMutex);

        auto jsonPtr = getOrLoadFolderLocked(normFolder, itemsInFolder.front().folderPath);

        for (const auto& item : itemsInFolder) {
            ItemMeta& meta = jsonPtr->items()[item.fileNameKey];
            if (meta.type.empty()) {
                meta.type = item.info.isDir() ? L"folder" : L"file";
            }

            QStringList finalTags = mergeAddTagsInternal(meta.tags, tags);
            meta.tags = toStdWStringVector(finalTags);
            resultMap[item.nativePath] = finalTags;

            if (item.info.isDir()) {
                std::wstring selfDirPath = item.info.absoluteFilePath().toStdWString();
                std::wstring selfNorm = normalizeFolderPath(selfDirPath);
                auto selfJsonPtr = getOrLoadFolderLocked(selfNorm, selfDirPath);
                selfJsonPtr->folder().tags = meta.tags;
                markDirtyLocked(selfNorm);
            }
        }

        markDirtyLocked(normFolder);
    }

    scheduleFlush();
>>>>>>> REPLACE
```

---

## 四、 构建与验证步骤 (Build & Verification Steps)

1. **编译确认**：
   在 Qt Creator 或命令行工具中使用 C++20 标准进行增量构建，确保零警告、零错误通过。
2. **0ms 进度条响应验证**：
   打开“库”分类的右键菜单“设置预设标签”，添加新标签并点击“保存设置”。验证 `TaskProgressStrip`（顶端 5px 进度条）在点击保存的 **0ms 瞬间** 立即在顶部弹显并顺畅显示进度。
3. **并发浏览零假死验证**：
   在设置大批次预设标签保存的同时，快速在侧边栏“收藏夹”中频繁点击切换多个包含大量文件的物理文件夹。验证 `ContentPanel` 内容即刻流畅加载，不再出现因锁竞争导致“没有可显示的项目”挂起假死问题。

---

## 五、 官方 API 复用与防冗余声明 (Official API Reuse & Anti-Redundancy Declaration)

- **直接复用 `TaskProgressService`**：严格复用项目既有的单例服务 `TaskProgressService::instance().createJob(...)` / `updateProgress(...)` / `finishJob(...)` 与 `TaskProgressStrip`，禁止重新新建任何进度通知 UI。
- **直接复用既有批处理架构**：直接拓展 `CoreEngine::addTagsToPathsAsync` 参数，无需另开平行逻辑。

---

## 六、 头文件 API 签名校验 (Header API Signature Verification)

- `PresetTagService.h`：增加私有函数 `processAsyncPresetTagsUpdate` 的 `jobId` 参数，为私有辅助接口，不影响对外公开调用契约。
- `CoreEngine.h`：`addTagsToPathsAsync` 的新增参数 `int existingJobId = 0` 附带默认值 `0`，完美向后兼容既有所有外部调用方。

---

## 七、 头文件包含链与类型完整性检查 (Header Inclusion Chain & Type Completeness Check)

- `PresetTagService.cpp` 中已包含 `#include "TaskProgressService.h"`，保证 `TaskProgressService` 类型完整。
- `CoreEngine.cpp` 中已包含 `#include "TaskProgressService.h"`，保证 `TaskProgressService` 类型完整。

---

## 八、 数据 SSOT 声明表 (Data SSOT Declaration Table)

| 数据项 | SSOT 权威数据源 | 读写路径 |
| :--- | :--- | :--- |
| **任务进度状态** | `TaskProgressService` | `TaskProgressService::instance()` 单例数据源 ➔ 驱动 `TaskProgressStrip` UI |
| **侧车数据与锁** | `QuarkMetaJsonStore` | `m_storeMutex` 控制内存缓存访问，磁盘写回经 `m_flushPool` 异步刷盘 |

---

## 九、 架构与红线合规自检表 (Compliance Self-Check Table)

| 检查项 | 是否合规 | 判定说明 |
| :--- | :---: | :--- |
| **无未授权源码修改红线** | **是** | 所有代码修改仅作为标准 Merge Diff 存放在本实施方案中，源代码保持不变 |
| **无非 Reentrant 锁死风险** | **是** | 将全局连续大锁拆分为独立循环按文件夹细粒度加锁，死锁风险归零 |
| **无主线程 IO 阻塞** | **是** | 所有写磁盘与 DB 均在后台工作线程池中执行，主线程 0ms 响应 |

---

## 十、 行为保持与零退化声明表 (Behavior Preservation Declaration Table)

1. **现有 preset_tags 读写逻辑**：对 SQLite 数据库 `library_categories` 和 `.QuarkMeta.json` 的侧车写入规则保持 100% 相同。
2. **外部接口兼容**：所有对外公开 API 签名均未破坏，默认参数确保既有模块调用无需任何修改。
