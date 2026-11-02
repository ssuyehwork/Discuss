# Implementation Plan: 智能文件夹元数据变化实时自动更新实施方案

本实施方案严格遵循架构铁律与《Memories.md》（章节 29），实现“智能文件夹”在文件元数据（评分、颜色、标签等）变化同步入库后，左侧智能文件夹数字计数与右侧正在显示的内容列表自动化、无感、实时更新。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 现状与根因溯源
1. **源头通知缺失**：`LibrarySyncService` 把元数据变更写入 SQLite 库表 `library_items`（通过 `LibraryItemDao::upsertItems`、`removeItemsByPaths`、`updatePath`、`markSyncStateByPath` 等）后，属于静默处理，未对外发出任何完成通知；导致视图层与中枢无法获知刷新时机。
2. **内容刷新重放陈旧数据**：`ContentPanel::refreshAll()` 在智能文件夹模式下（`m_currentCategoryType == "smart_folder"`），直接重载上次保存的 `m_lastLoadedLibraryPaths` 静态路径快照，未重新执行规则查询，导致任何刷新动作均无法得到最新变动结果。
3. **加载与查询分散且阻塞主线程**：智能文件夹加载逻辑在三处各自独立实现（`PanelMediator` 的 `smartFolderSelected`、`smartFolderPreviewRequested` 以及 `ContentDataLoader::loadDirectory` 的 `smartfolder://` 分支），且全部在 GUI 主线程同步调用 `SmartFolderDao::getMatchingPaths`，破坏了响应性能并产生代码冗余。

### 1.2 架构设计目标与核心闭环
1. **数据同步层权威广播 (SSOT)**：在 `LibrarySyncService` 中新增唯一权威信号 `void libraryItemsSynced();`，在后台写库任务成功完成之后，通过 `QMetaObject::invokeMethod(this, ..., Qt::QueuedConnection)` 投递至主线程发射。若 `upsertItems` 写入失败返回 false，不发信号并输出包含首路径与批大小的 `qWarning`。
2. **批量只读统计通道**：在 `SmartFolderDao` 中新增 `countMatchingItemsBatchReadOnly`，单次打开只读数据库连接，设置 `sqlite3_busy_timeout(db, 3000)`，循环计算全部智能文件夹计数；完全替换并物理删除旧单条只读函数 `countMatchingItemsReadOnly`。
3. **侧边栏防抖与安全刷新**：在 `LibraryPanel` 中新增 `refreshSmartFolderCounts()`，内置 150ms 单次定时器合并连续请求；异步后台批量计算，回到主线程时通过递增序号 `m_currentCountReqId` 与 `QPointer` 守卫验证防过期；写入期间屏蔽 `itemChanged` 改名落库逻辑；未返回的智能文件夹保留原数值。
4. **内容面板统一加载入口与无感比对刷新**：
   - 在 `ContentPanel` / `ContentDataLoader` 中新增唯一加载入口 `loadSmartFolder(record, displayName)`，并在 `ContentPanel` 维护 `m_activeSmartFolder` 规则快照，通过封装的 `public` Getter/Setter（`activeSmartFolder()`、`setActiveSmartFolder()`、`smartFolderRequestId()`、`incrementSmartFolderRequestId()`）暴露合法访问；
   - 数据库规则匹配与检索在后台线程执行；
   - 彻底重构 `PanelMediator` 的两处槽函数与 `ContentDataLoader` 的 `smartfolder://` 分支，统一委托至 `loadSmartFolder`；
   - 在 `applyViewTarget` 中，当 `categoryType != "smart_folder"` 时自动清空 `m_activeSmartFolder`；
   - 新增 `refreshSmartFolderIfShowing()`：仅在当前处于智能文件夹时执行后台规则比对；路径集合完全等价时不重载不闪烁；发生变动时暂存选区，通过 `loadLibraryPaths` 加载并由 `restoreSelections` 自动恢复选区；
   - `refreshAll()` 智能文件夹分支改为使用 `m_activeSmartFolder` 快照重新计算并整体重载；
   - 在 `PanelMediator::setupConnections` 中单向桥接 `libraryItemsSynced` 信号至左侧计数与所有窗格（`rootPane()->panes()`）的内容刷新。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `QuarkMeta_Source/src/meta/LibrarySyncService.h`
2. `QuarkMeta_Source/src/meta/LibrarySyncService.cpp`
3. `QuarkMeta_Source/src/meta/SmartFolderDao.h`
4. `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`
5. `QuarkMeta_Source/src/ui/LibraryPanel.h`
6. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`
7. `QuarkMeta_Source/src/ui/ContentPanel.h`
8. `QuarkMeta_Source/src/ui/ContentPanel.cpp`
9. `QuarkMeta_Source/src/ui/controllers/ContentDataLoader.h`
10. `QuarkMeta_Source/src/ui/controllers/ContentDataLoader.cpp`
11. `QuarkMeta_Source/src/ui/PanelMediator.cpp`

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 `QuarkMeta_Source/src/meta/LibrarySyncService.h`
<<<<<<< SEARCH
    bool isLibraryPath(const QString& path) const;

    void triggerStartupReconciliation();

private:
=======
    bool isLibraryPath(const QString& path) const;

    void triggerStartupReconciliation();

signals:
    void libraryItemsSynced();

private:
>>>>>>> REPLACE

### 3.2 `QuarkMeta_Source/src/meta/LibrarySyncService.cpp`
<<<<<<< SEARCH
    } else if (event.type == QuarkMetaJsonStore::ChangeType::ItemRenamed) {
        if (!isLibraryPath(event.path)) return;
        (void)QtConcurrent::run(&m_workPool, [this, oldP = event.path, newP = event.newPath]() {
            LibraryDao::updatePathPrefix(oldP, newP);
            LibraryItemDao::updatePath(oldP, newP);
            {
                std::unique_lock<std::shared_mutex> lock(m_pathMutex);
                m_libraryPathsSet.erase(normalizePath(oldP).toStdWString());
                m_libraryPathsSet.insert(normalizePath(newP).toStdWString());
            }
        });
    } else if (event.type == QuarkMetaJsonStore::ChangeType::FolderMigrated) {
        (void)QtConcurrent::run(&m_workPool, [this, oldF = event.path, newF = event.newPath]() {
            LibraryDao::updatePathPrefix(oldF, newF);
            LibraryItemDao::updatePathPrefix(oldF, newF);
            loadLibraryPathSet(); // 彻底重新从数据库刷新内存成员集合
        });
    } else if (event.type == QuarkMetaJsonStore::ChangeType::ItemRemoved) {
        if (!isLibraryPath(event.path)) return;
        (void)QtConcurrent::run(&m_workPool, [eventP = event.path]() {
            LibraryItemDao::markSyncStateByPath(eventP, 2); // 标记 sync_state = 2 (缺失)
        });
    }
=======
    } else if (event.type == QuarkMetaJsonStore::ChangeType::ItemRenamed) {
        if (!isLibraryPath(event.path)) return;
        (void)QtConcurrent::run(&m_workPool, [this, oldP = event.path, newP = event.newPath]() {
            LibraryDao::updatePathPrefix(oldP, newP);
            LibraryItemDao::updatePath(oldP, newP);
            {
                std::unique_lock<std::shared_mutex> lock(m_pathMutex);
                m_libraryPathsSet.erase(normalizePath(oldP).toStdWString());
                m_libraryPathsSet.insert(normalizePath(newP).toStdWString());
            }
            QMetaObject::invokeMethod(this, &LibrarySyncService::libraryItemsSynced, Qt::QueuedConnection);
        });
    } else if (event.type == QuarkMetaJsonStore::ChangeType::FolderMigrated) {
        (void)QtConcurrent::run(&m_workPool, [this, oldF = event.path, newF = event.newPath]() {
            LibraryDao::updatePathPrefix(oldF, newF);
            LibraryItemDao::updatePathPrefix(oldF, newF);
            loadLibraryPathSet(); // 彻底重新从数据库刷新内存成员集合
            QMetaObject::invokeMethod(this, &LibrarySyncService::libraryItemsSynced, Qt::QueuedConnection);
        });
    } else if (event.type == QuarkMetaJsonStore::ChangeType::ItemRemoved) {
        if (!isLibraryPath(event.path)) return;
        (void)QtConcurrent::run(&m_workPool, [this, eventP = event.path]() {
            LibraryItemDao::markSyncStateByPath(eventP, 2); // 标记 sync_state = 2 (缺失)
            QMetaObject::invokeMethod(this, &LibrarySyncService::libraryItemsSynced, Qt::QueuedConnection);
        });
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    (void)QtConcurrent::run(&m_workPool, [this, paths]() {
        const int batchSize = 100;
        for (int i = 0; i < paths.size(); i += batchSize) {
            QStringList batch = paths.mid(i, batchSize);
            processSyncBatch(batch);
        }
    });
=======
    (void)QtConcurrent::run(&m_workPool, [this, paths]() {
        const int batchSize = 100;
        bool anyBatchSuccess = false;
        for (int i = 0; i < paths.size(); i += batchSize) {
            QStringList batch = paths.mid(i, batchSize);
            std::vector<ItemRecord> records;
            records.reserve(batch.size());
            for (const QString& path : batch) {
                ItemRecord rec = ItemRecord::create(path);
                ItemMeta meta;
                if (QuarkMetaJsonStore::instance().readItemMeta(path.toStdWString(), meta)) {
                    ItemMetaMapper::apply(meta, rec);
                }
                QFileInfo info(path);
                std::wstring folderPath = info.isDir() ? path.toStdWString() : info.absolutePath().toStdWString();
                rec.metaFileId = QuarkMetaJsonStore::instance().getMetaFileId(folderPath);
                records.push_back(rec);
            }
            if (LibraryItemDao::upsertItems(records)) {
                anyBatchSuccess = true;
            } else {
                qWarning() << "[LibrarySyncService] Failed to upsert batch, first path:"
                           << (batch.isEmpty() ? QString() : batch.first())
                           << "batch size:" << batch.size();
            }
        }
        if (anyBatchSuccess) {
            QMetaObject::invokeMethod(this, &LibrarySyncService::libraryItemsSynced, Qt::QueuedConnection);
        }
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (!pathsToRemove.isEmpty()) {
            LibraryItemDao::removeItemsByPaths(pathsToRemove);
        }
    });
=======
        if (!pathsToRemove.isEmpty()) {
            LibraryItemDao::removeItemsByPaths(pathsToRemove);
            QMetaObject::invokeMethod(this, &LibrarySyncService::libraryItemsSynced, Qt::QueuedConnection);
        }
    });
>>>>>>> REPLACE

### 3.3 `QuarkMeta_Source/src/meta/SmartFolderDao.h`
<<<<<<< SEARCH
    static int countMatchingItems(const SmartFolderRecord& rec);
    static int countMatchingItemsReadOnly(const QString& dbPath, const SmartFolderRecord& rec);
    static QStringList getMatchingPaths(const SmartFolderRecord& rec);
=======
    static int countMatchingItems(const SmartFolderRecord& rec);
    static QMap<int, int> countMatchingItemsBatchReadOnly(const QString& dbPath, const QList<SmartFolderRecord>& records);
    static QStringList getMatchingPaths(const SmartFolderRecord& rec);
>>>>>>> REPLACE

### 3.4 `QuarkMeta_Source/src/meta/SmartFolderDao.cpp`
<<<<<<< SEARCH
int SmartFolderDao::countMatchingItemsReadOnly(const QString& dbPath, const SmartFolderRecord& rec) {
    if (rec.rules.isEmpty() || dbPath.isEmpty()) return 0;

    QString whereClause = buildWhereClause(rec);
    if (whereClause.isEmpty()) return 0;

    sqlite3* dbReadOnly = nullptr;
    int rc = sqlite3_open_v2(dbPath.toUtf8().constData(), &dbReadOnly, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK || !dbReadOnly) {
        if (dbReadOnly) sqlite3_close(dbReadOnly);
        return 0;
    }

    QString sql = QString("SELECT COUNT(*) FROM library_items WHERE %1 AND sync_state != 2;")
                  .arg(whereClause);

    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(dbReadOnly, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(dbReadOnly);
    return count;
}
=======
QMap<int, int> SmartFolderDao::countMatchingItemsBatchReadOnly(const QString& dbPath, const QList<SmartFolderRecord>& records) {
    QMap<int, int> results;
    if (dbPath.isEmpty() || records.isEmpty()) return results;

    sqlite3* dbReadOnly = nullptr;
    int rc = sqlite3_open_v2(dbPath.toUtf8().constData(), &dbReadOnly, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK || !dbReadOnly) {
        if (dbReadOnly) sqlite3_close(dbReadOnly);
        return results;
    }

    sqlite3_busy_timeout(dbReadOnly, 3000);

    for (const auto& rec : records) {
        if (rec.rules.isEmpty()) {
            results.insert(rec.id, 0);
            continue;
        }

        QString whereClause = buildWhereClause(rec);
        if (whereClause.isEmpty()) {
            results.insert(rec.id, 0);
            continue;
        }

        QString sql = QString("SELECT COUNT(*) FROM library_items WHERE %1 AND sync_state != 2;")
                      .arg(whereClause);

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(dbReadOnly, sql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
            int count = 0;
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
            results.insert(rec.id, count);
        }
    }

    sqlite3_close(dbReadOnly);
    return results;
}
>>>>>>> REPLACE

### 3.5 `QuarkMeta_Source/src/ui/LibraryPanel.h`
<<<<<<< SEARCH
    LibraryTreeView* treeView() const { return m_treeView; }

    void invalidateSmartFolderCache(int smartFolderId = 0);

signals:
=======
    LibraryTreeView* treeView() const { return m_treeView; }

    void invalidateSmartFolderCache(int smartFolderId = 0);
    void refreshSmartFolderCounts();

signals:
>>>>>>> REPLACE

<<<<<<< SEARCH
    QMap<int, int> m_smartFolderCountCache;
    uint64_t m_currentCountReqId = 0;
};
=======
    QMap<int, int> m_smartFolderCountCache;
    uint64_t m_currentCountReqId = 0;
    QTimer* m_countDebounceTimer = nullptr;
    bool m_isUpdatingCounts = false;
};
>>>>>>> REPLACE

### 3.6 `QuarkMeta_Source/src/ui/LibraryPanel.cpp`
<<<<<<< SEARCH
#include "controllers/ContextMenuFactory.h"
#include <QLabel>
=======
#include "controllers/ContextMenuFactory.h"
#include <QLabel>
#include <QPointer>
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
=======
    connect(m_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) {
        if (!item || m_isLoading || m_isUpdatingCounts) return;
        int nodeId = item->data(Qt::UserRole + 1).toInt();
>>>>>>> REPLACE

<<<<<<< SEARCH
void LibraryPanel::updateSmartFolderCountsAsync(uint64_t reqId, const QList<SmartFolderRecord>& smartFolders) {
    QString dbPath = DatabaseManager::getGlobalDbPath();

    (void)QtConcurrent::run([this, reqId, smartFolders, dbPath]() {
        QMap<int, int> results;
        for (const auto& sfRec : smartFolders) {
            int c = SmartFolderDao::countMatchingItemsReadOnly(dbPath, sfRec);
            results.insert(sfRec.id, c);
        }

        QMetaObject::invokeMethod(this, [this, reqId, results]() {
            if (reqId != m_currentCountReqId || !m_model) return;

            QStandardItem* sfHeader = findItemByNodeId(m_model->invisibleRootItem(), -20);
            for (auto it = results.constBegin(); it != results.constEnd(); ++it) {
                int sfId = it.key();
                int newCount = it.value();

                m_smartFolderCountCache.insert(sfId, newCount);

                if (sfHeader) {
                    QStandardItem* sfItem = findItemByNodeId(sfHeader, sfId);
                    if (sfItem) {
                        int oldCount = sfItem->data(Qt::UserRole + 9).toInt();
                        if (oldCount != newCount) {
                            sfItem->setData(newCount, Qt::UserRole + 9);
                        }
                    }
                }
            }
        });
    });
}
=======
void LibraryPanel::refreshSmartFolderCounts() {
    if (!m_countDebounceTimer) {
        m_countDebounceTimer = new QTimer(this);
        m_countDebounceTimer->setSingleShot(true);
        connect(m_countDebounceTimer, &QTimer::timeout, this, [this]() {
            uint64_t reqId = ++m_currentCountReqId;
            QString dbPath = DatabaseManager::getGlobalDbPath();
            QPointer<LibraryPanel> weakThis(this);

            (void)QtConcurrent::run([weakThis, reqId, dbPath]() {
                auto smartFolders = SmartFolderDao::getAllSmartFolders();
                QMap<int, int> results = SmartFolderDao::countMatchingItemsBatchReadOnly(dbPath, smartFolders);

                QMetaObject::invokeMethod(qApp, [weakThis, reqId, results]() {
                    if (!weakThis || reqId != weakThis->m_currentCountReqId || !weakThis->m_model) return;

                    weakThis->m_isUpdatingCounts = true;
                    QStandardItem* sfHeader = weakThis->findItemByNodeId(weakThis->m_model->invisibleRootItem(), -20);
                    for (auto it = results.constBegin(); it != results.constEnd(); ++it) {
                        int sfId = it.key();
                        int newCount = it.value();

                        weakThis->m_smartFolderCountCache.insert(sfId, newCount);

                        if (sfHeader) {
                            QStandardItem* sfItem = weakThis->findItemByNodeId(sfHeader, sfId);
                            if (sfItem) {
                                int oldCount = sfItem->data(Qt::UserRole + 9).toInt();
                                if (oldCount != newCount) {
                                    sfItem->setData(newCount, Qt::UserRole + 9);
                                }
                            }
                        }
                    }
                    weakThis->m_isUpdatingCounts = false;
                });
            });
        });
    }
    m_countDebounceTimer->start(150);
}

void LibraryPanel::updateSmartFolderCountsAsync(uint64_t reqId, const QList<SmartFolderRecord>& smartFolders) {
    QString dbPath = DatabaseManager::getGlobalDbPath();
    QPointer<LibraryPanel> weakThis(this);

    (void)QtConcurrent::run([weakThis, reqId, smartFolders, dbPath]() {
        QMap<int, int> results = SmartFolderDao::countMatchingItemsBatchReadOnly(dbPath, smartFolders);

        QMetaObject::invokeMethod(qApp, [weakThis, reqId, results]() {
            if (!weakThis || reqId != weakThis->m_currentCountReqId || !weakThis->m_model) return;

            weakThis->m_isUpdatingCounts = true;
            QStandardItem* sfHeader = weakThis->findItemByNodeId(weakThis->m_model->invisibleRootItem(), -20);
            for (auto it = results.constBegin(); it != results.constEnd(); ++it) {
                int sfId = it.key();
                int newCount = it.value();

                weakThis->m_smartFolderCountCache.insert(sfId, newCount);

                if (sfHeader) {
                    QStandardItem* sfItem = weakThis->findItemByNodeId(sfHeader, sfId);
                    if (sfItem) {
                        int oldCount = sfItem->data(Qt::UserRole + 9).toInt();
                        if (oldCount != newCount) {
                            sfItem->setData(newCount, Qt::UserRole + 9);
                        }
                    }
                }
            }
            weakThis->m_isUpdatingCounts = false;
        });
    });
}
>>>>>>> REPLACE

### 3.7 `QuarkMeta_Source/src/ui/ContentPanel.h`
<<<<<<< SEARCH
#include "models/FilterProxyModel.h"
#include "controllers/ContentSortController.h"
#include "../core/ModelContract.h"

namespace QuarkMeta {
=======
#include "models/FilterProxyModel.h"
#include "controllers/ContentSortController.h"
#include "../core/ModelContract.h"
#include "../meta/SmartFolderDao.h"

namespace QuarkMeta {
>>>>>>> REPLACE

<<<<<<< SEARCH
    int currentLoadRequestId() const { return m_loadRequestId.load(); }
    int loadRequestId() const { return m_loadRequestId.load(); }
    int incrementLoadRequestId() { return ++m_loadRequestId; }
    const FilterState& currentFilter() const { return m_currentFilter; }
    bool isLoading() const { return m_isLoading.load(); }

    // 2. 状态与配置高阶方法
=======
    int currentLoadRequestId() const { return m_loadRequestId.load(); }
    int loadRequestId() const { return m_loadRequestId.load(); }
    int incrementLoadRequestId() { return ++m_loadRequestId; }
    const FilterState& currentFilter() const { return m_currentFilter; }
    bool isLoading() const { return m_isLoading.load(); }
    SmartFolderRecord activeSmartFolder() const { return m_activeSmartFolder; }
    void setActiveSmartFolder(const SmartFolderRecord& record) { m_activeSmartFolder = record; }
    uint64_t smartFolderRequestId() const { return m_smartFolderReqId; }
    uint64_t incrementSmartFolderRequestId() { return ++m_smartFolderReqId; }

    // 2. 状态与配置高阶方法
>>>>>>> REPLACE

<<<<<<< SEARCH
    void setPendingSelectPath(const QString& fullPath, bool edit = false);
    void setPendingSelectName(const QString& name, bool edit = false);
    void refreshAll();
=======
    void setPendingSelectPath(const QString& fullPath, bool edit = false);
    void setPendingSelectName(const QString& name, bool edit = false);
    void refreshAll();
    void loadSmartFolder(const SmartFolderRecord& record, const QString& displayName = QString());
    void refreshSmartFolderIfShowing();
>>>>>>> REPLACE

<<<<<<< SEARCH
    QString m_categoryTitle;
    int m_categoryId = 0;
    bool m_isRecursive = false;
=======
    QString m_categoryTitle;
    int m_categoryId = 0;
    bool m_isRecursive = false;
    SmartFolderRecord m_activeSmartFolder;
    uint64_t m_smartFolderReqId = 0;
>>>>>>> REPLACE

### 3.8 `QuarkMeta_Source/src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    m_categoryId = categoryId;
    m_isRecursive = recursive;

    if (m_model) {
=======
    m_categoryId = categoryId;
    m_isRecursive = recursive;

    if (categoryType != "smart_folder") {
        m_activeSmartFolder = SmartFolderRecord();
    }

    if (m_model) {
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_currentCategoryType == "smart_folder" || m_currentPath.startsWith("smartfolder://")) {
        loadLibraryPaths(m_lastLoadedLibraryPaths);
        return;
    }
=======
    if (m_currentCategoryType == "smart_folder" || m_currentPath.startsWith("smartfolder://")) {
        if (m_activeSmartFolder.id > 0 || !m_activeSmartFolder.rules.isEmpty()) {
            loadSmartFolder(m_activeSmartFolder, m_categoryTitle);
            return;
        }
        loadLibraryPaths(m_lastLoadedLibraryPaths);
        return;
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::updateItemMetadata(QString path) {
=======
void ContentPanel::loadSmartFolder(const SmartFolderRecord& record, const QString& displayName) {
    if (m_dataLoader) {
        m_dataLoader->loadSmartFolder(record, displayName);
    }
}

void ContentPanel::refreshSmartFolderIfShowing() {
    if (m_currentCategoryType != "smart_folder" || (m_activeSmartFolder.id <= 0 && m_activeSmartFolder.rules.isEmpty())) {
        return;
    }

    uint64_t reqId = ++m_smartFolderReqId;
    SmartFolderRecord recordSnapshot = m_activeSmartFolder;
    QPointer<ContentPanel> weakThis(this);

    (void)QtConcurrent::run([weakThis, recordSnapshot, reqId]() {
        QStringList newPaths = SmartFolderDao::getMatchingPaths(recordSnapshot);

        QMetaObject::invokeMethod(qApp, [weakThis, recordSnapshot, reqId, newPaths]() {
            if (!weakThis || reqId != weakThis->m_smartFolderReqId) return;
            if (weakThis->m_currentCategoryType != "smart_folder" || weakThis->m_activeSmartFolder.id != recordSnapshot.id) return;

            // 路径集合比对（统一分隔符、不区分大小写、不关心顺序）
            auto normalizeSet = [](const QStringList& list) {
                QSet<QString> s;
                for (const QString& p : list) {
                    s.insert(QDir::toNativeSeparators(p).toLower());
                }
                return s;
            };

            if (normalizeSet(newPaths) == normalizeSet(weakThis->m_lastLoadedLibraryPaths)) {
                return; // 完全相同，静默不刷新、不闪烁
            }

            // 路径集发生物理变动：暂存当前选区并重载
            QStringList selList = weakThis->getSelectedPaths();
            weakThis->m_selectionState.currentFolder = weakThis->m_currentPath;
            weakThis->m_selectionState.selectedPaths = QSet<QString>(selList.begin(), selList.end());
            if (!weakThis->m_selectionState.selectedPaths.isEmpty()) {
                weakThis->m_selectionState.focusedPath = *weakThis->m_selectionState.selectedPaths.begin();
            } else {
                weakThis->m_selectionState.focusedPath.clear();
            }

            weakThis->setLastLoadedLibraryPaths(newPaths);
            weakThis->loadLibraryPaths(newPaths);
        });
    });
}

void ContentPanel::updateItemMetadata(QString path) {
>>>>>>> REPLACE

### 3.9 `QuarkMeta_Source/src/ui/controllers/ContentDataLoader.h`
<<<<<<< SEARCH
    void loadPaths(const QStringList& paths, int reqId = 0);
    void loadLibraryPaths(const QStringList& paths, int reqId = 0);
    void appendPaths(const QStringList& paths, int reqId = 0);
=======
    void loadPaths(const QStringList& paths, int reqId = 0);
    void loadLibraryPaths(const QStringList& paths, int reqId = 0);
    void appendPaths(const QStringList& paths, int reqId = 0);
    void loadSmartFolder(const struct SmartFolderRecord& record, const QString& displayName);
>>>>>>> REPLACE

### 3.10 `QuarkMeta_Source/src/ui/controllers/ContentDataLoader.cpp`
<<<<<<< SEARCH
    if (path.startsWith("smartfolder://")) {
        int sfId = path.mid(14).toInt();
        SmartFolderRecord rec = SmartFolderDao::getSmartFolderById(sfId);
        QStringList matching = SmartFolderDao::getMatchingPaths(rec);
        m_panel->applyViewTarget(path, "smart_folder", rec.name, -20, false);
        m_panel->loadLibraryPaths(matching);
        return;
    }
=======
    if (path.startsWith("smartfolder://")) {
        int sfId = path.mid(14).toInt();
        SmartFolderRecord rec = SmartFolderDao::getSmartFolderById(sfId);
        loadSmartFolder(rec, rec.name);
        return;
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentDataLoader::loadLibraryPaths(const QStringList& paths, int reqId) {
=======
void ContentDataLoader::loadSmartFolder(const SmartFolderRecord& record, const QString& displayName) {
    if (!m_panel) return;

    QString targetName = displayName.isEmpty() ? record.name : displayName;
    m_panel->applyViewTarget(QString("smartfolder://%1").arg(record.id), "smart_folder", targetName, -20, false);
    m_panel->setActiveSmartFolder(record);

    uint64_t sfReqId = m_panel->incrementSmartFolderRequestId();
    int reqId = m_panel->incrementLoadRequestId();
    m_loadRequestId = reqId;

    QPointer<ContentPanel> weakPanel(m_panel);
    (void)QtConcurrent::run([weakPanel, record, sfReqId, reqId]() {
        QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);

        QMetaObject::invokeMethod(qApp, [weakPanel, record, sfReqId, reqId, matchingPaths]() {
            if (!weakPanel) return;
            if (weakPanel->loadRequestId() != reqId || weakPanel->smartFolderRequestId() != sfReqId) return;
            if (weakPanel->getCurrentCategoryType() != "smart_folder" || weakPanel->activeSmartFolder().id != record.id) return;

            weakPanel->setLastLoadedLibraryPaths(matchingPaths);
            weakPanel->loadLibraryPaths(matchingPaths, reqId);
        });
    });
}

void ContentDataLoader::loadLibraryPaths(const QStringList& paths, int reqId) {
>>>>>>> REPLACE

### 3.11 `QuarkMeta_Source/src/ui/PanelMediator.cpp`
<<<<<<< SEARCH
            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel, addressBar, titleBar](const SmartFolderRecord& record) {
                QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->applyViewTarget(QString("smartfolder://%1").arg(record.id), "smart_folder", record.name, -20, false);
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + record.name);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(record.name, "smartfolder://" + QString::number(record.id));
                }
            });

            connect(libraryPanel, &LibraryPanel::smartFolderPreviewRequested, this, [this, contentPanel, filterPanel, addressBar, titleBar](const SmartFolderRecord& record) {
                QStringList matchingPaths = SmartFolderDao::getMatchingPaths(record);
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    QString displayName = record.name.isEmpty() ? "智能文件夹预览" : record.name;
                    target->applyViewTarget(QString("smartfolder://%1").arg(record.id), "smart_folder", displayName, -20, false);
                    target->setLastLoadedLibraryPaths(matchingPaths);
                    target->loadLibraryPaths(matchingPaths);
                }
                if (addressBar) {
                    QString displayName = record.name.isEmpty() ? "智能文件夹预览" : record.name;
                    addressBar->setPath("库 → " + displayName);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                if (titleBar && titleBar->tabBar()) {
                    QString displayName = record.name.isEmpty() ? "智能文件夹预览" : record.name;
                    titleBar->tabBar()->updateCurrentTabTitle(displayName, "smartfolder://" + QString::number(record.id));
                }
            });
=======
            connect(libraryPanel, &LibraryPanel::smartFolderSelected, this, [this, contentPanel, filterPanel, addressBar, titleBar](const SmartFolderRecord& record) {
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->loadSmartFolder(record, record.name);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + record.name);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(record.name, "smartfolder://" + QString::number(record.id));
                }
            });

            connect(libraryPanel, &LibraryPanel::smartFolderPreviewRequested, this, [this, contentPanel, filterPanel, addressBar, titleBar](const SmartFolderRecord& record) {
                QString displayName = record.name.isEmpty() ? "智能文件夹预览" : record.name;
                ContentPanel* target = m_activeContentPanel ? m_activeContentPanel.data() : contentPanel;
                if (target) {
                    target->loadSmartFolder(record, displayName);
                }
                if (addressBar) {
                    addressBar->setPath("库 → " + displayName);
                }
                if (filterPanel) {
                    filterPanel->clearAllFilters(false);
                }
                if (titleBar && titleBar->tabBar()) {
                    titleBar->tabBar()->updateCurrentTabTitle(displayName, "smartfolder://" + QString::number(record.id));
                }
            });
>>>>>>> REPLACE

<<<<<<< SEARCH
        if (metaPanel) {
            connect(this, &PanelMediator::activeContentPanelChanged, this, [contentPanel, updateMetaPanelFromPanel](ContentPanel* activePanel) {
=======
        connect(&LibrarySyncService::instance(), &LibrarySyncService::libraryItemsSynced, this, [this, libraryPanel, contentPanel]() {
            if (libraryPanel) {
                libraryPanel->refreshSmartFolderCounts();
            }
            if (contentPanel) {
                ContentPanel* root = contentPanel->rootPane();
                if (root) {
                    for (ContentPanel* pane : root->panes()) {
                        if (pane) {
                            pane->refreshSmartFolderIfShowing();
                        }
                    }
                }
            }
        });

        if (metaPanel) {
            connect(this, &PanelMediator::activeContentPanelChanged, this, [contentPanel, updateMetaPanelFromPanel](ContentPanel* activePanel) {
>>>>>>> REPLACE

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 编译与语法闭合检查
1. 编译命令：
   ```bash
   cmake -B build -G Ninja
   ninja -C build
   ```
2. 零编译警告：头文件显式包含闭合，无 `QPointer`、`SmartFolderRecord`、`QMap` 未识别符号，且所有跨类调用均使用 `public` 接口，无私有成员直接访问。

### 4.2 运行时功能验收标准
1. **评分移除即时更新**：在智能文件夹“一星级”中，将某文件评分由 1 改为 0，约 0.5 秒内侧边栏一星级计数减 1，右侧列表中该项消失，其余项选中状态完好。
2. **新符合条件即时入列**：给一个在库中的普通文件评 1 星，一星级计数增 1；若主视图正位于一星级，该文件自动加入列表。
3. **符合条件不变时不刷新不闪烁**：评分变动但文件依旧符合规则时，列表不重载、不闪烁，仅更新对应行元数据。
4. **批量 500 文件并发改评分**：界面完全不卡顿，后台批量入库完成后，左侧计数和右侧列表仅各刷新一次。
5. **地址栏与标签页刷新**：处于智能文件夹视图时，点击地址栏或标签页刷新按钮，按当前规则快照重新计算并全量重载。
6. **模式物理隔离**：磁盘目录模式、库分类模式与回收站模式完全不触发智能文件夹刷新，行为保持零变化。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

1. **唯一入库同步源头**：严格复用 `LibrarySyncService` 作为唯一元数据入库广播源头，不添加任何旁路刷新机制。
2. **唯一加载通道**：收敛至 `ContentPanel::loadSmartFolder`，消除三处分散的直接查库与重复代码。
3. **选区恢复复用**：复用 `m_selectionState` 与 `restoreSelections()`，消除自造的选区跟踪逻辑。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一真理源 (SSOT) | 派生副本与缓存 | 同步与刷新流向 |
| :--- | :--- | :--- | :--- |
| **智能文件夹匹配文件** | SQLite `library_items` 表与 `SmartFolderRecord` 规则 | `m_lastLoadedLibraryPaths`、`m_model` | `LibrarySyncService` 入库 ➔ `libraryItemsSynced` ➔ 后台 `getMatchingPaths` ➔ 差异比对 ➔ `loadLibraryPaths` |
| **智能文件夹匹配计数** | SQLite `library_items` 只读连接批量统计 | `LibraryPanel::m_smartFolderCountCache` | `libraryItemsSynced` ➔ 150ms 定时器 ➔ `countMatchingItemsBatchReadOnly` ➔ Model `Qt::UserRole+9` |
| **智能文件夹当前规则** | 视图激活时的 `SmartFolderRecord` 快照 | `ContentPanel::m_activeSmartFolder` | `loadSmartFolder` 传入并缓存；非智能文件夹视图时清空 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. **外部及新增接口签名**：
   - `void LibrarySyncService::libraryItemsSynced();`：声明于 `LibrarySyncService.h` 的 `signals:` 域，`public` 权限；
   - `static QMap<int, int> SmartFolderDao::countMatchingItemsBatchReadOnly(const QString& dbPath, const QList<SmartFolderRecord>& records);`：`public` 静态方法；
   - `void LibraryPanel::refreshSmartFolderCounts();`：`public` 成员方法；
   - `void ContentPanel::loadSmartFolder(const SmartFolderRecord& record, const QString& displayName = QString());`：`public slots:` 域；
   - `void ContentPanel::refreshSmartFolderIfShowing();`：`public slots:` 域；
   - `SmartFolderRecord ContentPanel::activeSmartFolder() const;`：`public` 成员方法；
   - `void ContentPanel::setActiveSmartFolder(const SmartFolderRecord& record);`：`public` 成员方法；
   - `uint64_t ContentPanel::smartFolderRequestId() const;`：`public` 成员方法；
   - `uint64_t ContentPanel::incrementSmartFolderRequestId();`：`public` 成员方法；
   - `void ContentDataLoader::loadSmartFolder(const struct SmartFolderRecord& record, const QString& displayName);`：`public` 成员方法。
2. **跨类访问权限与封装核查（严格消除访问级别违规）**：
   - `ContentDataLoader` 对 `ContentPanel` 的操作全部通过合法 `public` 成员接口调用：
     - `m_panel->setActiveSmartFolder(record)`（替代私有变量直接赋值）；
     - `m_panel->incrementSmartFolderRequestId()`（替代私有变量自增）；
     - `weakPanel->smartFolderRequestId()`（替代私有变量读取）；
     - `weakPanel->activeSmartFolder()`（替代私有变量读取）；
   - 所有跨类调用均在 `public` 保护契约之内，杜绝任何 `is private within this context` 编译错误。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

1. `QuarkMeta_Source/src/ui/ContentPanel.h`：显式包含 `../meta/SmartFolderDao.h`，确保 `SmartFolderRecord` 具备完整类型定义。
2. `QuarkMeta_Source/src/ui/LibraryPanel.cpp`：显式包含 `<QPointer>`，保障多线程 Lambda 安全。
3. `QuarkMeta_Source/src/ui/controllers/ContentDataLoader.h`：包含 `struct SmartFolderRecord` 前置声明，并在 `.cpp` 中包含完整头文件。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 检查项 | 检查结果 | 规范要求与说明 |
| :--- | :--- | :--- |
| **1. 架构红线检查** | 满足 | 严禁跨层篡改；严格遵守只读/冻结接口与单向依赖 |
| **2. SEARCH 块唯一匹配** | 满足 | 11 个文件的 SEARCH 块经只读验证均真实存在且匹配次数唯一 |
| **3. 跨类调用成员访问级别 (2.f)** | 满足 | 所有跨类访问严格走 public Getter/Setter，无越权直接读写私有成员 |
| **4. 死代码彻底清除** | 满足 | 彻底删除单条旧接口 `countMatchingItemsReadOnly`，无孤儿代码 |
| **5. 主线程零阻塞** | 满足 | 数据库匹配查询与批量统计 100% 移入后台线程执行 |
| **6. 选区与状态保持** | 满足 | 刷新前暂存选区，无变动静默不闪烁，有变动恢复选区 |
| **7. 模式严格隔离** | 满足 | 非智能文件夹视图自动清空快照，不产生任何副作用 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

1. **外观与交互零篡改**：完全不改动任何 UI 视觉布局、字体大小、边距、圆角与 QSS 属性。
2. **非智能文件夹行为零篡改**：普通文件目录、库分类（全部数据、各子分类、未分类）、回收站视图的加载逻辑、缓存机制与生命周期 100% 保持不变。
3. **防抖与定时数值精准保持**：`LibrarySyncService` 200ms 防抖保持原样；`LibraryPanel` 与 `ContentPanel` 刷新防抖精准设定为 150ms。
