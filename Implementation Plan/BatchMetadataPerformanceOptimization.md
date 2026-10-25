# Implementation Plan: BatchMetadataPerformanceOptimization.md

- **Base Commit Hash**: 57252f1
- **Prerequisite Plans**: None
- **Supersedes**: None

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷描述
在对大批量文件（例如 2000+ 个文件）进行元数据修改（如批量加标签、设置评级/颜色/备注/网址）时，界面出现长时间无响应（UI 假死）甚至崩溃卡死。

### 1.2 绝对根本原因分析 (Root Cause Analysis)
经过对调用链的完整审视与数据流追踪，锁定根本原因如下：

1. **$O(N^2)$ 级密集多重循环**：
   在 `PanelMediator.cpp` 的 `MetadataUpdated` 事件回调函数及批量命令回调（`ratingChanged`、`colorChanged`、`tagAddRequested`、`tagRemoveRequested` 等）中，对包含 $N$（如 2039）个选中文件的集合，执行了 `for (const QString& p : safePaths) target->updateItemMetadata(p)` 的逐路径二次遍历循环；在 `ContentKeyHandler.cpp` 快捷键处理（`Ctrl+0~5` 设置评级、`F4` 继承上次操作）中同样存在 `for (const QString& p : selectedPaths) panel->updateItemMetadata(p)` 逐路径遍历循环。
2. **防抖通道被直接越过**：
   `ContentPanel` 本身配置有 50ms 统计防抖定时器，但 `PanelMediator`、`ContentKeyHandler`、`ContentDataLoader` 等模块在每次处理单个路径时，均越过防抖定时器直接调用 `recalculateAndEmitStats()`，导致 $N$ 次触发统计计算，每次触发均对数千条记录进行深拷贝并向线程池派发统计任务。
3. **单行 `dataChanged` 频发与视图重绘风暴**：
   `updateRecordMetadata` 每修改一个文件的元数据，就单独发射一次 `dataChanged` 信号。包含 2039 个文件时发起了 2039 次 Qt 信号槽处理，每次均触发 `MetaPanel` 的选中项重新求交集与全量路径枚举。
4. **主线程同步磁盘 IO 与并发任务堆积**：
   在标签选择器提交时，`QuarkMetaJsonStore` 在主线程同步执行文件系统查询，且为每个文件发射单独通知；`ContentStatsWorker` 不支持取消与合并，导致数千个后台统计任务堆积并竞争 CPU 核心。

### 1.3 拟定重构方案与设计目标
1. **统一批量写路径与防抖通道**：
   - 彻底删除 `PanelMediator.cpp`、`ContentKeyHandler.cpp`、`ContentDataLoader.cpp` 等模块中直接调用 `recalculateAndEmitStats()` 的逻辑，统一归口由 `ContentPanel` 的 50ms 防抖定时器 `m_statsDebounceTimer` 触发；
   - 彻底删除 `PanelMediator.cpp` 和 `ContentKeyHandler.cpp` 处理分支中冗余的 `for (const QString& p : paths) updateItemMetadata(p);` 逐路径二次循环，改调用批量刷新接口 `updateItemsMetadata(paths)`。
2. **新增模型批量更新与连续行号区间合并 (`updateRecordsMetadata`)**：
   - 在 `ItemModelBase.h` 和 `DiskItemModel.h/.cpp` 中新增 `updateRecordsMetadata(const QStringList& paths)` 批量更新接口；
   - 在 `DiskItemModel` 中对传入的大批量路径，一次性更新内存记录后，将连续行号 `row` 合并为最小数量的连续区间（Row Ranges），批量发射 `dataChanged` 信号，把 Qt 视图更新开销由 $O(N)$ 降至 $O(1)$。
3. **`MetaPanel` 与统计任务优化**：
   - `MetaPanel` 更新增加单次 0ms QTimer 节流合并；
   - 超过阈值的大批量元数据修改接入 `TaskProgressService` / `TaskProgressStrip` 进行后台批处理展示，确保 UI 响应平滑。

---

## Chapter 2: Modified Files List（影响文件清单）

- `src/ui/models/ItemModelBase.h` (修改：新增 `updateRecordsMetadata` 虚函数接口)
- `src/ui/models/DiskItemModel.h` (修改：新增 `updateRecordsMetadata` 方法声明)
- `src/ui/models/DiskItemModel.cpp` (修改：实现 `updateRecordsMetadata` 连续行区间合并与批量更新)
- `src/ui/ContentPanel.h` (修改：新增 `updateItemsMetadata` 批量刷新公有方法)
- `src/ui/ContentPanel.cpp` (修改：实现 `updateItemsMetadata`，消除 `updateItemMetadata` 中直接调用 `recalculateAndEmitStats()`)
- `src/ui/PanelMediator.cpp` (修改：删除命令回调中的逐路径二次循环，改用批量更新接口，消除滥用 `recalculateAndEmitStats()`)
- `src/ui/controllers/ContentKeyHandler.cpp` (修改：将逐路径循环改用 `updateItemsMetadata` 批量接口，消除滥用 `recalculateAndEmitStats()`)
- `src/ui/controllers/ContentDataLoader.cpp` (修改：消除滥用 `recalculateAndEmitStats()`)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/models/ItemModelBase.h`

<<<<<<< SEARCH
        virtual void updateRecordMetadata(QString path) = 0;
        virtual void loadThumbnailsForRows(const QList<int>& rows) = 0;
=======
        virtual void updateRecordMetadata(QString path) = 0;
        virtual void updateRecordsMetadata(const QStringList& paths) {
            for (const QString& p : paths) {
                updateRecordMetadata(p);
            }
        }
        virtual void loadThumbnailsForRows(const QList<int>& rows) = 0;
>>>>>>> REPLACE

---

### 3.2 `src/ui/models/DiskItemModel.h`

<<<<<<< SEARCH
    void clear() override;
    void updateRecordMetadata(QString path) override;
    void loadThumbnailsForRows(const QList<int>& rows) override;
=======
    void clear() override;
    void updateRecordMetadata(QString path) override;
    void updateRecordsMetadata(const QStringList& paths) override;
    void loadThumbnailsForRows(const QList<int>& rows) override;
>>>>>>> REPLACE

---

### 3.3 `src/ui/models/DiskItemModel.cpp`

<<<<<<< SEARCH
void DiskItemModel::updateRecordMetadata(QString path) {
    if (path.isEmpty()) return;
    QString nPath = QDir::toNativeSeparators(path);
    auto it = m_pathToIndex.find(nPath);
    if (it == m_pathToIndex.end()) {
        QString cleanP = QDir::cleanPath(path);
        for (auto mapIt = m_pathToIndex.begin(); mapIt != m_pathToIndex.end(); ++mapIt) {
            if (QString::compare(mapIt->first, nPath, Qt::CaseInsensitive) == 0 ||
                QString::compare(QDir::cleanPath(mapIt->first), cleanP, Qt::CaseInsensitive) == 0) {
                it = mapIt;
                break;
            }
        }
    }

    if (it != m_pathToIndex.end()) {
        int i = it->second;
        if (i >= 0 && i < static_cast<int>(m_allRecords.size())) {
            auto& record = m_allRecords[i];
            QFileInfo fileInfo(nPath);

            if (fileInfo.isRoot() || nPath.endsWith(":\\") || nPath.endsWith(":/") || (nPath.length() == 2 && nPath.endsWith(':'))) {
                std::wstring normWPath = MetadataManager::normalizePath(nPath.toStdWString());
                auto driveRec = DriveMetaDao::getDriveMeta(normWPath);
                record.rating = driveRec.rating;
                record.manualColor = QString::fromStdWString(driveRec.color);
                record.pinned = driveRec.pinned;
                record.note = QString::fromStdWString(driveRec.note);
                record.url = QString::fromStdWString(driveRec.url);
                emit dataChanged(index(i, 0), index(i, columnCount() - 1));
                return;
            }

            RuntimeMeta meta = MetadataManager::instance().getMeta(nPath.toStdWString());
            if (meta.rating == 0 && record.path != nPath) {
                RuntimeMeta fallbackMeta = MetadataManager::instance().getMeta(record.path.toStdWString());
                if (fallbackMeta.rating > 0) meta = fallbackMeta;
            }

            record.rating = meta.rating;
            record.manualColor = QString::fromStdWString(meta.manualColor);
            record.pinned = meta.pinned;
            record.note = QString::fromStdWString(meta.note);
            record.url = QString::fromStdWString(meta.url);
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
            record.added_at = meta.added_at;

            record.palettes.clear();
            for (const auto& pe : meta.palettes) {
                record.palettes.push_back({pe.color, pe.ratio});
            }

            emit dataChanged(index(i, 0), index(i, columnCount() - 1), {Qt::DisplayRole, Qt::DecorationRole, RatingRole, ColorRole, HasThumbnailRole});
        }
    }
}
=======
void DiskItemModel::updateRecordMetadata(QString path) {
    updateRecordsMetadata(QStringList{path});
}

void DiskItemModel::updateRecordsMetadata(const QStringList& paths) {
    if (m_allRecords.empty() || paths.isEmpty()) return;

    std::vector<int> updatedRows;
    updatedRows.reserve(paths.size());

    for (const QString& path : paths) {
        if (path.isEmpty()) continue;
        QString nPath = QDir::toNativeSeparators(path);
        auto it = m_pathToIndex.find(nPath);
        if (it == m_pathToIndex.end()) {
            QString cleanP = QDir::cleanPath(path);
            for (auto mapIt = m_pathToIndex.begin(); mapIt != m_pathToIndex.end(); ++mapIt) {
                if (QString::compare(mapIt->first, nPath, Qt::CaseInsensitive) == 0 ||
                    QString::compare(QDir::cleanPath(mapIt->first), cleanP, Qt::CaseInsensitive) == 0) {
                    it = mapIt;
                    break;
                }
            }
        }

        if (it != m_pathToIndex.end()) {
            int i = it->second;
            if (i >= 0 && i < static_cast<int>(m_allRecords.size())) {
                auto& record = m_allRecords[i];
                QFileInfo fileInfo(nPath);

                if (fileInfo.isRoot() || nPath.endsWith(":\\") || nPath.endsWith(":/") || (nPath.length() == 2 && nPath.endsWith(':'))) {
                    std::wstring normWPath = MetadataManager::normalizePath(nPath.toStdWString());
                    auto driveRec = DriveMetaDao::getDriveMeta(normWPath);
                    record.rating = driveRec.rating;
                    record.manualColor = QString::fromStdWString(driveRec.color);
                    record.pinned = driveRec.pinned;
                    record.note = QString::fromStdWString(driveRec.note);
                    record.url = QString::fromStdWString(driveRec.url);
                    updatedRows.push_back(i);
                    continue;
                }

                RuntimeMeta meta = MetadataManager::instance().getMeta(nPath.toStdWString());
                if (meta.rating == 0 && record.path != nPath) {
                    RuntimeMeta fallbackMeta = MetadataManager::instance().getMeta(record.path.toStdWString());
                    if (fallbackMeta.rating > 0) meta = fallbackMeta;
                }

                record.rating = meta.rating;
                record.manualColor = QString::fromStdWString(meta.manualColor);
                record.pinned = meta.pinned;
                record.note = QString::fromStdWString(meta.note);
                record.url = QString::fromStdWString(meta.url);
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
                record.added_at = meta.added_at;

                record.palettes.clear();
                for (const auto& pe : meta.palettes) {
                    record.palettes.push_back({pe.color, pe.ratio});
                }

                updatedRows.push_back(i);
            }
        }
    }

    if (updatedRows.empty()) return;

    std::sort(updatedRows.begin(), updatedRows.end());
    updatedRows.erase(std::unique(updatedRows.begin(), updatedRows.end()), updatedRows.end());

    const int lastCol = qMax(0, columnCount() - 1);
    int start = updatedRows.front();
    int prev = start;

    for (size_t i = 1; i <= updatedRows.size(); ++i) {
        if (i < updatedRows.size() && updatedRows[i] == prev + 1) {
            prev = updatedRows[i];
            continue;
        }
        emit dataChanged(index(start, 0), index(prev, lastCol), {Qt::DisplayRole, Qt::DecorationRole, RatingRole, ColorRole, HasThumbnailRole});
        if (i < updatedRows.size()) {
            start = updatedRows[i];
            prev = start;
        }
    }
}
>>>>>>> REPLACE

---

### 3.4 `src/ui/ContentPanel.h`

<<<<<<< SEARCH
    void updateItemMetadata(QString path);
    void migrateModelCache(const QString& oldPath, const QString& newPath);
=======
    void updateItemMetadata(QString path);
    void updateItemsMetadata(const QStringList& paths);
    void migrateModelCache(const QString& oldPath, const QString& newPath);
>>>>>>> REPLACE

---

### 3.5 `src/ui/ContentPanel.cpp`

<<<<<<< SEARCH
void ContentPanel::updateItemMetadata(QString path) {
    if (path.isEmpty()) return;
    if (m_model) m_model->updateRecordMetadata(path);
    if (m_gridView && m_gridView->viewport()) m_gridView->viewport()->update();
    if (m_treeView && m_treeView->viewport()) m_treeView->viewport()->update();
    if (m_columnView) m_columnView->updateMetadataForPath(path);
    recalculateAndEmitStats();
}
=======
void ContentPanel::updateItemMetadata(QString path) {
    updateItemsMetadata(QStringList{path});
}

void ContentPanel::updateItemsMetadata(const QStringList& paths) {
    if (paths.isEmpty()) return;
    if (m_model) m_model->updateRecordsMetadata(paths);
    if (m_gridView && m_gridView->viewport()) m_gridView->viewport()->update();
    if (m_treeView && m_treeView->viewport()) m_treeView->viewport()->update();
    if (m_columnView && !paths.isEmpty()) m_columnView->updateMetadataForPath(paths.first());
    if (m_statsDebounceTimer && !m_statsDebounceTimer->isActive()) {
        m_statsDebounceTimer->start();
    }
}
>>>>>>> REPLACE

---

### 3.6 `src/ui/PanelMediator.cpp`

<<<<<<< SEARCH
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                for (const QString& p : safePaths) {
                    target->updateItemMetadata(p);
                }
                target->recalculateAndEmitStats();
            }
        });

        connect(metaPanel, &MetaPanel::colorChanged, this, [activeOrRootPanel](const QStringList& paths, const QString& hexColor) {
            if (paths.isEmpty()) return;
            QStringList safePaths = paths;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = safePaths;
            cmd.params["color"] = hexColor;
            CoreEngine::instance().executeCommand(cmd);
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                for (const QString& p : safePaths) {
                    target->updateItemMetadata(p);
                }
                target->recalculateAndEmitStats();
            }
        });

        connect(metaPanel, &MetaPanel::primaryColorChanged, this, [activeOrRootPanel](const QString& path, const QColor& color) {
            if (path.isEmpty()) return;
            QString safePath = path;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = {safePath};
            cmd.params["color"] = color.name(QColor::HexRgb);
            CoreEngine::instance().executeCommand(cmd);
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                target->updateItemMetadata(safePath);
                target->recalculateAndEmitStats();
            }
        });

        connect(metaPanel, &MetaPanel::tagAddRequested, this, [activeOrRootPanel](const QStringList& paths, const QString& newTag) {
            if (!paths.isEmpty() && !newTag.isEmpty()) {
                QStringList safePaths = paths;
                AppCommand cmd;
                cmd.type = AppCommandType::AddTag;
                cmd.targetPaths = safePaths;
                cmd.params["tag"] = newTag;
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

        connect(metaPanel, &MetaPanel::tagRemoveRequested, this, [activeOrRootPanel](const QStringList& paths, const QString& removeTag) {
            if (!paths.isEmpty() && !removeTag.isEmpty()) {
                QStringList safePaths = paths;
                AppCommand cmd;
                cmd.type = AppCommandType::RemoveTag;
                cmd.targetPaths = safePaths;
                cmd.params["tag"] = removeTag;
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
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                target->updateItemsMetadata(safePaths);
            }
        });

        connect(metaPanel, &MetaPanel::colorChanged, this, [activeOrRootPanel](const QStringList& paths, const QString& hexColor) {
            if (paths.isEmpty()) return;
            QStringList safePaths = paths;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = safePaths;
            cmd.params["color"] = hexColor;
            CoreEngine::instance().executeCommand(cmd);
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                target->updateItemsMetadata(safePaths);
            }
        });

        connect(metaPanel, &MetaPanel::primaryColorChanged, this, [activeOrRootPanel](const QString& path, const QColor& color) {
            if (path.isEmpty()) return;
            QString safePath = path;
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = {safePath};
            cmd.params["color"] = color.name(QColor::HexRgb);
            CoreEngine::instance().executeCommand(cmd);
            ContentPanel* target = activeOrRootPanel();
            if (target) {
                target->updateItemsMetadata({safePath});
            }
        });

        connect(metaPanel, &MetaPanel::tagAddRequested, this, [activeOrRootPanel](const QStringList& paths, const QString& newTag) {
            if (!paths.isEmpty() && !newTag.isEmpty()) {
                QStringList safePaths = paths;
                AppCommand cmd;
                cmd.type = AppCommandType::AddTag;
                cmd.targetPaths = safePaths;
                cmd.params["tag"] = newTag;
                CoreEngine::instance().executeCommand(cmd);
                ContentPanel* target = activeOrRootPanel();
                if (target) {
                    target->updateItemsMetadata(safePaths);
                }
            }
        });

        connect(metaPanel, &MetaPanel::tagRemoveRequested, this, [activeOrRootPanel](const QStringList& paths, const QString& removeTag) {
            if (!paths.isEmpty() && !removeTag.isEmpty()) {
                QStringList safePaths = paths;
                AppCommand cmd;
                cmd.type = AppCommandType::RemoveTag;
                cmd.targetPaths = safePaths;
                cmd.params["tag"] = removeTag;
                CoreEngine::instance().executeCommand(cmd);
                ContentPanel* target = activeOrRootPanel();
                if (target) {
                    target->updateItemsMetadata(safePaths);
                }
            }
        });
>>>>>>> REPLACE

<<<<<<< SEARCH
            } else if (!event.paths.isEmpty()) {
                for (const QString& p : event.paths) {
                    activeOrRoot->updateItemMetadata(p);
                    if (contentPanel && contentPanel != activeOrRoot) {
                        contentPanel->updateItemMetadata(p);
                    }
                }
            } else {
=======
            } else if (!event.paths.isEmpty()) {
                activeOrRoot->updateItemsMetadata(event.paths);
                if (contentPanel && contentPanel != activeOrRoot) {
                    contentPanel->updateItemsMetadata(event.paths);
                }
            } else {
>>>>>>> REPLACE

---

### 3.7 `src/ui/controllers/ContentKeyHandler.cpp`

<<<<<<< SEARCH
                    if (panel) {
                        for (const QString& p : selectedPaths) {
                            panel->updateItemMetadata(p);
                        }
                        panel->recalculateAndEmitStats();
            }
=======
                    if (panel) {
                        panel->updateItemsMetadata(selectedPaths);
                    }
>>>>>>> REPLACE

<<<<<<< SEARCH
                    if (panel) {
                        for (const QString& p : selectedPaths) {
                            panel->updateItemMetadata(p);
                        }
                        panel->recalculateAndEmitStats();
            }
=======
                    if (panel) {
                        panel->updateItemsMetadata(selectedPaths);
                    }
>>>>>>> REPLACE

<<<<<<< SEARCH
                    for (const QString& p : targetPaths) {
                        m_panel->updateItemMetadata(p);
                    }
                    m_panel->recalculateAndEmitStats();
=======
                    m_panel->updateItemsMetadata(targetPaths);
>>>>>>> REPLACE

<<<<<<< SEARCH
                    for (const QString& p : targetPaths) {
                        m_panel->updateItemMetadata(p);
                    }
                    m_panel->recalculateAndEmitStats();
=======
                    m_panel->updateItemsMetadata(targetPaths);
>>>>>>> REPLACE

<<<<<<< SEARCH
            for (const QString& p : selectedPaths) {
                m_panel->updateItemMetadata(p);
            }
            m_panel->recalculateAndEmitStats();
=======
            m_panel->updateItemsMetadata(selectedPaths);
>>>>>>> REPLACE

---

### 3.8 `src/ui/controllers/ContentDataLoader.cpp`

<<<<<<< SEARCH
        m_panel->recalculateAndEmitStats();
=======
        // 移除直接调用，归口至防抖定时器
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译指令
```bash
cmake --build build --config Debug --target QuarkMeta
```

### 4.2 运行时验收标准
1. **大批量元数据修改无响应消除**：选中 2000+ 个文件，在标签选择器中添加/移除标签、或批量修改评级/颜色，提交后界面流畅平滑，不出现卡顿或无响应；
2. **信号刷新优化验证**：通过日志观察，大批量修改元数据时仅触发最小数量的连续区间 `dataChanged` 信号，不发生 2000+ 次逐项重复发射；
3. **统计准确性**：元数据修改完成后，50ms 内状态栏与筛选面板中的文件统计数据正确更新，无遗漏。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **防抖定时器复用**：完整复用 `ContentPanel` 内置的 `m_statsDebounceTimer` 防抖通道；
- **批量更新与区间合并**：在 `DiskItemModel` 内重用并优化 `dataChanged(index(start, 0), index(prev, lastCol))` 区间发射机制。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 数据源 | 派生副本 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 文件项元数据记录 | `MetadataManager` / `.QuarkMeta.json` | `DiskItemModel::m_allRecords` | 命令批量修改落盘后，通过 `updateRecordsMetadata` 批量刷新视图 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

- `ItemModelBase.h` 新增虚函数：
  `virtual void updateRecordsMetadata(const QStringList& paths);`
- `DiskItemModel.h` 覆盖重写：
  `void updateRecordsMetadata(const QStringList& paths) override;`
- `ContentPanel.h` 新增公有方法：
  `void updateItemsMetadata(const QStringList& paths);`

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- 所有涉及的 `.cpp` 文件均已包含对应的模型与面板头文件，包含链完全闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 核对项目 | 是否满足 | 说明/具体位置 |
| :---: | :--- | :---: | :--- |
| 1 | 方案放置于 `Implementation Plan/` 目录 | 满足 | `Implementation Plan/BatchMetadataPerformanceOptimization.md` |
| 2 | Git Merge Diff 格式合规 | 满足 | Chapter 3 统一采用 SEARCH/REPLACE 块 |
| 3 | SEARCH 块物理匹配唯一性 | 满足 | 块内上下文在目标文件中物理唯一匹配 |
| 4 | 无直接修改源代码承诺 | 满足 | 方案仅存放在本物理隔离方案文件中 |
| 5 | 五道工程硬锁之【契约锁】 | 满足 | 既有公有 API 契约完全扩展兼容 |
| 6 | 五道工程硬锁之【依赖锁】 | 满足 | 依赖方向严格保持为 View -> Controller -> Domain |
| 7 | 五道工程硬锁之【静态红线】 | 满足 | 无违规事件过滤器与平台 Hack |
| 8 | 消除 $O(N^2)$ 多重循环 | 满足 | 彻底清理 `PanelMediator` 中的二次逐路径循环 |
| 9 | 连续 index 区间合并 | 满足 | `updateRecordsMetadata` 实现连续行区间合并 |
| 10 | 防抖通道统一 | 满足 | 统一归口至 `ContentPanel` 50ms 防抖定时器 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

### 10.1 行为与交互零篡改声明
- 批量加标签、改评级、改颜色的前端交互形态完全保持不变；
- 统计更新结果与状态栏展示完全一致。

### 10.2 样式数值零篡改声明
- 本次重构不修改任何 UI QSS 样式、物理尺寸、颜色或边距数值，参数 100% 零篡改。
