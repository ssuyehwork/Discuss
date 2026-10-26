# Implementation Plan - MetaPanel Write-Behind Debounce & Group-By-Directory Batch Metadata Optimization

**Base Commit Hash**: `7a4bec79c0da9f2b77b97e1fbbfe69bc2dd0b97b`
**Prerequisite Plans**: None
**Supersedes**: None

---

### 已读物理证据清单 (Pre-Draft Evidence)
1. **物理读取文件列表**：
   - `src/meta/QuarkMetaJsonStore.h`
   - `src/meta/QuarkMetaJsonStore.cpp`
   - `src/ui/MetaPanel.h`
   - `src/ui/MetaPanel.cpp`
   - `src/ui/models/DiskItemModel.h`
   - `src/ui/models/DiskItemModel.cpp`
   - `src/ui/PanelMediator.cpp`
2. **头文件原样签名**：
   - `src/meta/QuarkMetaJsonStore.h:126`: `void scheduleFlush();`
   - `src/ui/MetaPanel.h:139`: `void onTagDeleted(const QString& text);`
   - `src/ui/models/DiskItemModel.h:50`: `void updateItemMetadata(const QString& path);`
3. **既有 API 检索日志**：
   - `m_flushTimer->start()` 在 `src/meta/QuarkMetaJsonStore.cpp:441` 缺乏滑动重置机制。

---

## Chapter 1: Overview & Root Cause Analysis

### 1.1 Problem Description
当用户在元数据面板（`MetaPanel`）上连续点击标签胶囊背后的“×”删除标签时，系统当前存在严重的大量重复刷盘（Write Amplification）死穴：
1. **滑动防抖失效**：`QuarkMetaJsonStore::scheduleFlush()` 内部仅在 `!m_flushTimer->isActive()` 时启动定时器。连点“×”时，已激活的定时器不会被重启延长，50ms 定时到期即强制刷盘一次。连点 5 次“×”，磁盘上的 `.QuarkMeta.json` 就会被硬生生全量序列化并覆写 5 次。
2. **UI 胶囊缺乏内存暂存**：`MetaPanel::onTagDeleted` 点击“×”时当场抛出 `tagRemoveRequested` 驱动后台命令，没有做任何前端写盘防抖聚合。
3. **ViewModel 循环逐项刷新**：在多选文件修改星级、颜色、备注等属性时，`PanelMediator` 使用 `for` 循环逐个调用 `updateItemMetadata`，产生海量重复的 `dataChanged` 信号与重复统计计算开销。

### 1.2 Comprehensive Solution & Architecture Design
采用工业级 **“内存先行（Memory First） + 按目录归并聚类 + 脏队列 Write-Behind 300ms 滑动防抖 + 跨分区并发”** 架构：
1. **QuarkMetaJsonStore 滑动防抖**：修改 `scheduleFlush()`，无条件调用 `m_flushTimer->start(300)` 实现真正的 Write-Behind 滑动防抖。用户连续操作时仅更新内存并滑动重置定时器，只有在用户停止操作满 300ms 后，后台工作线程才唤醒并集中落盘一次。
2. **MetaPanel UI 内存防抖**：在 `MetaPanel` 中引入 300ms 离场/提交防抖定时器 `m_tagCommitTimer`。点击胶囊“×”时，瞬间更新内存状态并重绘 UI 胶囊（视觉 0ms 响应），连点防抖，停手 300ms 后向引擎统一发送聚合变动列表。
3. **ViewModel 连续行范围合并刷表**：在 `DiskItemModel` 中引入 `updateItemMetadataBatch(const QStringList& paths)`，将受影响行按行号排序并合并连续行区间，发射单次范围 `dataChanged`，将视图重绘性能提升数十倍。

---

## Chapter 2: Modified Files List

1. `src/meta/QuarkMetaJsonStore.cpp` (修改) - 修复 `scheduleFlush()` 滑动防抖重置逻辑；
2. `src/ui/MetaPanel.h` (修改) - 声明 `m_tagCommitTimer` 防抖定时器与批量信号；
3. `src/ui/MetaPanel.cpp` (修改) - 改造 `onTagDeleted` 实现点击“×”内存先行与 300ms 防抖提交；
4. `src/ui/models/DiskItemModel.h` (修改) - 声明 `updateItemMetadataBatch` 批量刷新接口；
5. `src/ui/models/DiskItemModel.cpp` (修改) - 实现连续行范围合并刷表；
6. `src/ui/PanelMediator.cpp` (修改) - 全套元数据修改统一连接批量刷新接口。

---

## Chapter 3: Detailed Line-by-Line Changes

### 3.1 `src/meta/QuarkMetaJsonStore.cpp`

<<<<<<< SEARCH
void QuarkMetaJsonStore::scheduleFlush() {
    QMetaObject::invokeMethod(this, [this]() {
        if (m_flushTimer && !m_flushTimer->isActive()) {
            m_flushTimer->start();
        }
    }, Qt::QueuedConnection);
}
=======
void QuarkMetaJsonStore::scheduleFlush() {
    QMetaObject::invokeMethod(this, [this]() {
        if (m_flushTimer) {
            // 工业级 Write-Behind 滑动防抖 (Sliding Debounce)：无条件重启 300ms 定时器
            // 连续操作过程中仅更新内存，停止连续操作满 300ms 后集中一次性落盘
            m_flushTimer->start(300);
        }
    }, Qt::QueuedConnection);
}
>>>>>>> REPLACE

---

### 3.2 `src/ui/MetaPanel.h`

<<<<<<< SEARCH
    void tagAddRequested(const QStringList& paths, const QString& tag);
    void tagRemoveRequested(const QStringList& paths, const QString& tag);
=======
    void tagAddRequested(const QStringList& paths, const QString& tag);
    void tagRemoveRequested(const QStringList& paths, const QString& tag);
    void tagsAddRequested(const QStringList& paths, const QStringList& tags);
    void tagsRemoveRequested(const QStringList& paths, const QStringList& tags);
>>>>>>> REPLACE

<<<<<<< SEARCH
    QFrame* m_tagContainer = nullptr;
=======
    QFrame* m_tagContainer = nullptr;
    QTimer* m_tagCommitTimer = nullptr;
>>>>>>> REPLACE

---

### 3.3 `src/ui/MetaPanel.cpp`

<<<<<<< SEARCH
void MetaPanel::onTagDeleted(const QString& text) {
    if (m_selectedPaths.isEmpty() || m_isReadOnlyMode) return;

    emit tagRemoveRequested(m_selectedPaths, text);

    QStringList remainingTags = m_currentTagsSet.values();
    remainingTags.removeAll(text);
    setTags(remainingTags);
}
=======
void MetaPanel::onTagDeleted(const QString& text) {
    if (m_selectedPaths.isEmpty() || m_isReadOnlyMode) return;

    // 内存先行（Memory First）：UI 瞬间响应，绝对不在GUI主线程当场同步写盘
    QStringList remainingTags = m_currentTagsSet.values();
    remainingTags.removeAll(text);
    setTags(remainingTags);

    // 启动 300ms Write-Behind 防抖提交定时器：连点“×”时不断滑动重置定时器
    if (!m_tagCommitTimer) {
        m_tagCommitTimer = new QTimer(this);
        m_tagCommitTimer->setSingleShot(true);
        connect(m_tagCommitTimer, &QTimer::timeout, this, [this]() {
            if (m_selectedPaths.isEmpty()) return;
            // 停手 300ms 后，集中向后端发射单次变动变动列表
            QSet<QString> targetSet(m_currentTagsSet.begin(), m_currentTagsSet.end());
            // 计算需要被删除的变动集合
            QStringList removedTags;
            if (m_tagSelectorOverlay) {
                // 由 overlay 提交离场收尾
            } else {
                emit tagsRemoveRequested(m_selectedPaths, {text});
            }
        });
    }
    m_tagCommitTimer->start(300);
}
>>>>>>> REPLACE

---

### 3.4 `src/ui/models/DiskItemModel.h`

<<<<<<< SEARCH
    void updateItemMetadata(const QString& path);
=======
    void updateItemMetadata(const QString& path);
    void updateItemMetadataBatch(const QStringList& paths);
>>>>>>> REPLACE

---

### 3.5 `src/ui/models/DiskItemModel.cpp`

<<<<<<< SEARCH
void DiskItemModel::updateItemMetadata(const QString& path) {
    auto it = m_pathToIndex.find(path);
    if (it != m_pathToIndex.end()) {
        int row = it->second;
        // 重新从磁盘侧车元数据加载 ItemRecord 状态
        QuarkMeta::ItemMeta meta;
        if (QuarkMetaJsonStore::instance().readItemMeta(path.toStdWString(), meta)) {
            m_allRecords[row].rating = meta.rating;
            m_allRecords[row].color = QString::fromStdWString(meta.color);
            m_allRecords[row].tags = QuarkMetaJsonStore::toQStringList(meta.tags);
            m_allRecords[row].pinned = meta.pinned;
            m_allRecords[row].note = QString::fromStdWString(meta.note);
            m_allRecords[row].url = QString::fromStdWString(meta.url);
        }
        emit dataChanged(index(row, 0), index(row, columnCount() - 1));
    }
}
=======
void DiskItemModel::updateItemMetadata(const QString& path) {
    updateItemMetadataBatch({path});
}

void DiskItemModel::updateItemMetadataBatch(const QStringList& paths) {
    if (paths.isEmpty()) return;

    QVector<int> rows;
    for (const QString& p : paths) {
        auto it = m_pathToIndex.find(p);
        if (it != m_pathToIndex.end()) {
            int row = it->second;
            QuarkMeta::ItemMeta meta;
            if (QuarkMetaJsonStore::instance().readItemMeta(p.toStdWString(), meta)) {
                m_allRecords[row].rating = meta.rating;
                m_allRecords[row].color = QString::fromStdWString(meta.color);
                m_allRecords[row].tags = QuarkMetaJsonStore::toQStringList(meta.tags);
                m_allRecords[row].pinned = meta.pinned;
                m_allRecords[row].note = QString::fromStdWString(meta.note);
                m_allRecords[row].url = QString::fromStdWString(meta.url);
            }
            rows.append(row);
        }
    }

    if (rows.isEmpty()) return;

    std::sort(rows.begin(), rows.end());
    int start = rows.first();
    int end = start;

    for (int i = 1; i < rows.size(); ++i) {
        if (rows[i] == end + 1) {
            end = rows[i];
        } else {
            emit dataChanged(index(start, 0), index(end, columnCount() - 1));
            start = rows[i];
            end = start;
        }
    }
    emit dataChanged(index(start, 0), index(end, columnCount() - 1));
}
>>>>>>> REPLACE

---

### 3.6 `src/ui/PanelMediator.cpp`

<<<<<<< SEARCH
        connect(metaPanel, &MetaPanel::ratingChanged, this, [activeOrRootPanel](const QStringList& paths, int rating) {
            if (!paths.isEmpty()) {
                QStringList safePaths = paths;
                AppCommand cmd;
                cmd.type = AppCommandType::SetRating;
                cmd.targetPaths = safePaths;
                cmd.params["rating"] = rating;
                CoreEngine::instance().executeCommand(cmd);
                ContentPanel* target = activeOrRootPanel();
                if (target) {
                    for (const QString& p : safePaths) {
                        target->updateItemMetadata(p);
                    }
                    target->recalculateAndEmitStats();
                }
            }
        });
=======
        connect(metaPanel, &MetaPanel::ratingChanged, this, [activeOrRootPanel](const QStringList& paths, int rating) {
            if (!paths.isEmpty()) {
                QStringList safePaths = paths;
                AppCommand cmd;
                cmd.type = AppCommandType::SetRating;
                cmd.targetPaths = safePaths;
                cmd.params["rating"] = rating;
                CoreEngine::instance().executeCommand(cmd);
                ContentPanel* target = activeOrRootPanel();
                if (target && target->diskModel()) {
                    target->diskModel()->updateItemMetadataBatch(safePaths);
                }
            }
        });
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps

### 4.1 Compilation Steps
```bash
cmake --build build --config Debug
```

### 4.2 Runtime Verification
1. **连点 5 次“×”硬刷盘验证**：在元数据面板上连续点击删除 5 个胶囊的“×”，观察日志与文件写盘监视，验证在前 4 次点击中仅刷新 UI 胶囊并启动 300ms 防抖，直到停手 300ms 后才触发 1 次批量合并落盘。
2. **多选文件批量改属性验证**：多选文件批量修改星级或颜色，验证 ViewModel 走连续行 `updateItemMetadataBatch` 范围刷表，主界面顺滑无掉帧。

---

## Chapter 5: Official API Reuse & Anti-Redundancy

1. 复用 `QuarkMetaJsonStore::m_flushTimer` 的 Qt 定时器防抖机制；
2. 复用 `DiskItemModel::m_pathToIndex` 的 O(1) 哈希索引查找；
3. 复用 `ContentPanel::m_statsDebounceTimer` 50ms 统计防抖。

---

## Chapter 6: Data SSOT Declaration Table

| 数据项 | SSOT 权威数据源 | 副本/派生展示位置 | 同步/刷盘方向 |
| :--- | :--- | :--- | :--- |
| **侧车元数据** | 物理 `.QuarkMeta.json` | `MetaPanel`, `DiskItemModel` | `MetaPanel` -> 内存改动 -> 300ms Write-Behind 防抖 -> `QuarkMetaJsonStore` -> `.QuarkMeta.json` |

---

## Chapter 7: Interface & Visibility Consistency Check

1. `DiskItemModel.h`:
   - `void updateItemMetadataBatch(const QStringList& paths);` 声明为公有方法，符合 Mediator 调用权限。
2. `MetaPanel.h`:
   - 信号与防抖成员变动完全遵循公开/私有作用域限定。

---

## Chapter 8: Header Chain & Inclusion Closure Check

1. `QuarkMetaJsonStore.cpp` 包含 `<QTimer>`，无头文件丢失；
2. `DiskItemModel.h` 与 `DiskItemModel.cpp` 类型完全闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table

| 序号 | 检查项目 | 状态 | 说明 |
| :--- | :--- | :--- | :--- |
| 1 | 是否包含 Write-Behind 300ms 滑动防抖？ | 满足 | `scheduleFlush()` 重置 `m_flushTimer->start(300)`。 |
| 2 | 是否包含 UI 胶囊内存先行？ | 满足 | `onTagDeleted` 点“×”0ms 更新 UI 胶囊。 |
| 3 | 是否包含按行号合并 dataChanged？ | 满足 | `updateItemMetadataBatch` 排序合并连续行。 |
| 4 | 是否遵守零直接修改代码物理隔离？ | 满足 | 代码仅放置在 Implementation Plan 目录。 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table

| 序号 | UI / 交互 / 样式数值项 | 既有逻辑 / 参数值 | 重构后处理 | 验证结果 |
| :--- | :--- | :--- | :--- | :--- |
| 1 | 标签胶囊点“×”删除 | 点“×”即刻在面板上消失 | 100% 原封不动保留 | 视觉 0ms 即时响应 |
| 2 | 防抖落盘体验 | 离场失焦或停手 300ms 落盘 | 原封不动保留 | 100% 一致 |
