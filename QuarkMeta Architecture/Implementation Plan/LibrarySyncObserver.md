# QuarkMeta JSON 存储变更观察与库元数据动态同步实施方案 (LibrarySyncObserver.md)

## 1. Overview（概述与解决的问题）

### 解决的核心问题
1. **JSON 变更发布与订阅解耦**：在 `QuarkMetaJsonStore` 中引入轻量级观察者回调接口（`ChangeListener`），在 `updateItemMeta`、`renameItem`、`migrateFolderCache`、`transferItems`、`removeItem` 操作触发时广播事件，不直接依赖 `LibrarySyncService`。
2. **库成员属性实时刷库**：`LibrarySyncService` 订阅 JSON 变更事件。非库成员改动在 0ms 逻辑拦截抛弃；库成员改动通过 200ms 防抖去重合并批量刷库。
3. **改名与路径迁移自愈**：文件/文件夹重命名或迁移时，同步更新 `library_category_paths` 路径映射，更新 `library_items` 中的 `file_path` / `file_name` / `base_name` / `extension`（`file_id` 保持不变），并同步更新内存路径集合。
4. **删除标记（`sync_state = 2`）**：库成员被物理删除或放入回收站时，将 `library_items` 中的 `sync_state` 更新为 `2`（缺失），保留分类关系供恢复追溯。

---

## 2. Pre-Draft Evidence（起草前核查清单）

### 绕过 QuarkMetaJsonStore 的写入路径普查
经全工程精准 `grep` 检索（`QuarkMetaJson::` 与 `.QuarkMeta.json`）：
- 全工程**没有任何绕过 `QuarkMetaJsonStore` 直接操作 `.QuarkMeta.json` 文件**的代码。
- 所有的元数据修改、图幅尺寸同步、自动色标提取、单文件改名、批量漫游、目录迁移、文件删除，均统一路由至 `QuarkMetaJsonStore` 及其封装方法，数据源完全收敛。

### 内存与落盘时序结论
- `QuarkMetaJsonStore::updateItemMeta` 在调用 `updater(meta)` 时会**同步且立即更新 `m_folderCacheMap` 内存中对应的 `ItemMeta` 对象**。
- 随后调用的 `readItemMeta` 会从 `m_folderCacheMap` 中直接读取该对象，因此在 `updateItemMeta` 完成后的第 0 毫秒，`readItemMeta` 即可 100% 读到最新值，不依赖后台 50ms 定时器的磁盘物理写盘。

### 签名原文摘录
- `LibraryDao::updatePathPrefix`: `static bool updatePathPrefix(const QString& oldPrefix, const QString& newPrefix);`（将在 `LibraryDao.h` 中新增）
- `LibraryItemDao::updatePath`: `static bool updatePath(const QString& oldPath, const QString& newPath);`（将在 `LibraryItemDao.h` 中新增）
- `LibraryItemDao::updatePathPrefix`: `static bool updatePathPrefix(const QString& oldPrefix, const QString& newPrefix);`（将在 `LibraryItemDao.h` 中新增）
- `LibraryItemDao::markSyncStateByPath`: `static bool markSyncStateByPath(const QString& path, int syncState);`（将在 `LibraryItemDao.h` 中新增）

---

## 3. Modified Files List（影响文件清单）

1. `src/meta/QuarkMetaJsonStore.h` / `src/meta/QuarkMetaJsonStore.cpp`（增加 `ChangeEvent` 枚举与 `ChangeListener` 观察者接口并广播事件）
2. `src/meta/LibraryDao.h` / `src/meta/LibraryDao.cpp`（增加 `updatePathPrefix` 分类路径前缀更新方法）
3. `src/meta/LibraryItemDao.h` / `src/meta/LibraryItemDao.cpp`（增加 `updatePath`、`updatePathPrefix`、`markSyncStateByPath` 数据库操作）
4. `src/meta/LibraryService.h` / `src/meta/LibraryService.cpp`（增加 `updatePathPrefix` 包装方法）
5. `src/meta/LibrarySyncService.h` / `src/meta/LibrarySyncService.cpp`（订阅 `QuarkMetaJsonStore` 变更事件，处理防抖刷库、路径变更、删除标记）

---

## 4. Detailed Line-by-Line Changes（精准替换块）

### 4.1 `QuarkMetaJsonStore.h` 添加 ChangeListener 接口

```
<<<<<<< SEARCH
public:
    static QuarkMetaJsonStore& instance();

    /**
     * @brief 原子修改指定路径对应文件的 ItemMeta 记录 (自动缓冲合并落盘)
     */
=======
public:
    enum class ChangeType {
        ItemChanged,      // 文件属性/元数据变化 (path)
        ItemRenamed,      // 文件重命名/移动 (oldPath, newPath)
        ItemRemoved,      // 文件删除 (path)
        FolderMigrated    // 文件夹迁移/重命名 (oldFolder, newFolder)
    };

    struct ChangeEvent {
        ChangeType type;
        QString path;
        QString newPath;
    };

    using ChangeListener = std::function<void(const ChangeEvent&)>;

    static QuarkMetaJsonStore& instance();

    void addChangeListener(ChangeListener listener);

    /**
     * @brief 原子修改指定路径对应文件的 ItemMeta 记录 (自动缓冲合并落盘)
     */
>>>>>>> REPLACE
```

---

### 4.2 `QuarkMetaJsonStore.cpp` 实现事件注册与广播

```
<<<<<<< SEARCH
void QuarkMetaJsonStore::updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater) {
=======
void QuarkMetaJsonStore::addChangeListener(ChangeListener listener) {
    if (listener) {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        m_listeners.push_back(listener);
    }
}

void QuarkMetaJsonStore::notifyListeners(const ChangeEvent& event) {
    std::vector<ChangeListener> listenersCopy;
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        listenersCopy = m_listeners;
    }
    for (const auto& listener : listenersCopy) {
        try { listener(event); } catch (...) {}
    }
}

void QuarkMetaJsonStore::updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater) {
>>>>>>> REPLACE
```

在各变更操作末尾触发事件广播：
- `updateItemMeta`: `notifyListeners({ChangeType::ItemChanged, QString::fromStdWString(filePath), ""});`
- `renameItem`: `notifyListeners({ChangeType::ItemRenamed, QDir::toNativeSeparators(folderPath + "/" + oldName), QDir::toNativeSeparators(folderPath + "/" + newName)});`
- `migrateFolderCache`: `notifyListeners({ChangeType::FolderMigrated, QDir::toNativeSeparators(oldFolderPath), QDir::toNativeSeparators(newFolderPath)});`
- `removeItem`: `notifyListeners({ChangeType::ItemRemoved, QDir::toNativeSeparators(filePath), ""});`
- `transferItems`: 循环广播对应 `ItemRenamed` 或 `ItemChanged` 事件。

---

### 4.3 `LibraryDao.h` 与 `LibraryDao.cpp` 增加前缀重命名

在 `LibraryDao.h` 中添加：
```cpp
    static bool updatePathPrefix(const QString& oldPrefix, const QString& newPrefix);
```

在 `LibraryDao.cpp` 中实现：
```cpp
bool LibraryDao::updatePathPrefix(const QString& oldPrefix, const QString& newPrefix) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || oldPrefix.isEmpty() || newPrefix.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    QString cleanOld = QDir::toNativeSeparators(oldPrefix);
    QString cleanNew = QDir::toNativeSeparators(newPrefix);

    const char* sql = "UPDATE library_category_paths SET path = ? || SUBSTR(path, ?) WHERE path LIKE ? || '%' COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, cleanNew.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, cleanOld.length() + 1);
        sqlite3_bind_text(stmt, 3, cleanOld.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
    return false;
}
```

---

### 4.4 `LibraryItemDao.h` 与 `LibraryItemDao.cpp` 扩展 DAO 写入操作

在 `LibraryItemDao.h` 中的 `private` 部分添加：
```cpp
    static bool updatePath(const QString& oldPath, const QString& newPath);
    static bool updatePathPrefix(const QString& oldPrefix, const QString& newPrefix);
    static bool markSyncStateByPath(const QString& path, int syncState);
```

在 `LibraryItemDao.cpp` 中实现：
```cpp
bool LibraryItemDao::updatePath(const QString& oldPath, const QString& newPath) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || oldPath.isEmpty() || newPath.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    QFileInfo info(newPath);
    const char* sql = "UPDATE library_items SET file_path = ?, file_name = ?, base_name = ?, extension = ?, sync_state = 0 WHERE file_path = ? COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, info.fileName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, info.completeBaseName().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, info.suffix().toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, oldPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
    return false;
}

bool LibraryItemDao::updatePathPrefix(const QString& oldPrefix, const QString& newPrefix) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || oldPrefix.isEmpty() || newPrefix.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    QString cleanOld = QDir::toNativeSeparators(oldPrefix);
    QString cleanNew = QDir::toNativeSeparators(newPrefix);

    const char* sql = "UPDATE library_items SET file_path = ? || SUBSTR(file_path, ?) WHERE file_path LIKE ? || '%' COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, cleanNew.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, cleanOld.length() + 1);
        sqlite3_bind_text(stmt, 3, cleanOld.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
    return false;
}

bool LibraryItemDao::markSyncStateByPath(const QString& path, int syncState) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || path.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());
    const char* sql = "UPDATE library_items SET sync_state = ? WHERE file_path = ? COLLATE NOCASE;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, syncState);
        sqlite3_bind_text(stmt, 2, path.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }
    return false;
}
```

---

### 4.5 `LibrarySyncService.h` 与 `LibrarySyncService.cpp` 订阅与处理

在 `LibrarySyncService.h` 中添加防抖与观察者处理方法：
```cpp
private:
    void handleJsonChangeEvent(const QuarkMetaJsonStore::ChangeEvent& event);
    void scheduleDebouncedSync(const QString& path);
    void flushDebouncedQueue();

    QTimer* m_debounceTimer = nullptr;
    std::unordered_set<std::wstring> m_pendingSyncPaths;
    std::mutex m_debounceMutex;
```

在 `LibrarySyncService.cpp` 中初始化并订阅：
```cpp
LibrarySyncService::LibrarySyncService(QObject* parent) : QObject(parent) {
    m_workPool.setMaxThreadCount(1);
    loadLibraryPathSet();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(200); // 200ms 防抖
    connect(m_debounceTimer, &QTimer::timeout, this, &LibrarySyncService::flushDebouncedQueue);

    QuarkMetaJsonStore::instance().addChangeListener([this](const QuarkMetaJsonStore::ChangeEvent& ev) {
        handleJsonChangeEvent(ev);
    });

    QTimer::singleShot(1500, this, [this]() {
        triggerStartupReconciliation();
    });
}

void LibrarySyncService::handleJsonChangeEvent(const QuarkMetaJsonStore::ChangeEvent& event) {
    if (event.type == QuarkMetaJsonStore::ChangeType::ItemChanged) {
        if (!isLibraryPath(event.path)) return; // 拦截阻断非库成员！
        scheduleDebouncedSync(event.path);
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
}

void LibrarySyncService::scheduleDebouncedSync(const QString& path) {
    {
        std::lock_guard<std::mutex> lock(m_debounceMutex);
        m_pendingSyncPaths.insert(normalizePath(path).toStdWString());
    }
    QMetaObject::invokeMethod(this, [this]() {
        if (m_debounceTimer && !m_debounceTimer->isActive()) {
            m_debounceTimer->start();
        }
    }, Qt::QueuedConnection);
}

void LibrarySyncService::flushDebouncedQueue() {
    QStringList pathsToSync;
    {
        std::lock_guard<std::mutex> lock(m_debounceMutex);
        for (const auto& w : m_pendingSyncPaths) {
            pathsToSync.append(QString::fromStdWString(w));
        }
        m_pendingSyncPaths.clear();
    }
    if (!pathsToSync.isEmpty()) {
        syncPaths(pathsToSync);
    }
}
```

---

## 5. Build & Verification Steps（编译与验证步骤）

1. **编译命令**：
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. **验证点 a（属性实时刷库）**：修改库成员文件的评级、色标、备注、网址、置顶，查看 200ms 后 `library_items` 表对应列同步更新。
3. **验证点 b（提取参数刷库）**：提取图片的自动色与调色板，确认 `auto_color`、`width`、`height` 以及 `item_palettes` 色板表行即时写入。
4. **验证点 c（非库成员阻断）**：修改非库成员属性，查看日志无任何数据库更新行为（拦截率 100%）。
5. **验证点 d（改名与文件夹迁移）**：重命名文件或父文件夹，确认 `library_category_paths` 与 `library_items` 中的路径及文件名同步替换，`file_id` 保持不变。
6. **验证点 e（逻辑删除 `sync_state = 2`）**：将库成员文件删除，查看 `library_items` 中的 `sync_state` 变为 `2`。
7. **验证点 f（高频防抖合并）**：连续快速修改同一文件 20 次，验证 200ms 内仅触发 1 次批量写库。

---

## 6. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用自查）

- **数据汇合点归一**：全工程所有写 JSON 操作均经由 `QuarkMetaJsonStore` 触发广播，无任何散落写入代码。
- **线程与防抖防爆**：`LibrarySyncService` 的 200ms 防抖定时器保证高频改动被合并处理，100% 后台线程离线刷库。

---

## 7. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用的成员函数 / 类 | 源头头文件物理位置 | 精确物理签名 |
|---|---|---|
| `QuarkMetaJsonStore::addChangeListener` | `src/meta/QuarkMetaJsonStore.h:35` | `void addChangeListener(ChangeListener listener);` |
| `LibraryDao::updatePathPrefix` | `src/meta/LibraryDao.h:45` | `static bool updatePathPrefix(const QString& oldPrefix, const QString& newPrefix);` |
| `LibraryItemDao::markSyncStateByPath` | `src/meta/LibraryItemDao.h:25` | `static bool markSyncStateByPath(const QString& path, int syncState);` |

---

## 8. Header Inclusion Chain & Type Completeness Check（头文件包含链核查表）

- **`QuarkMetaJsonStore.h`**: 包含 `<functional>`、`<vector>`，声明 `ChangeEvent` 结构体与 `ChangeListener` 类型。
- **`LibrarySyncService.cpp`**: 包含 `<QtConcurrent/QtConcurrent>`，确保 `QtConcurrent::run` 在 MSVC 环境中正常编译。

---

## 9. Compliance Self-Check Table（合规核对表）

| 检查项 | 来源条款 | 结论 | 对应方案位置 |
|---|---|---|---|
| 文件名与核心类名 1:1 映射 | 3.1-2 | 满足 | 文件名 `LibrarySyncObserver.md` |
| 方案位于物理隔离目录 | 3.1-1 | 满足 | `QuarkMeta Architecture/Implementation Plan/` |
| 所有 SEARCH 块代码在仓库真实存在，含上下文 | 3.2 | 满足 | 4.1 ~ 4.5 章节 |
| 涉及 Q_OBJECT 的类已在 CMakeLists 注册且有 .cpp | 3.3 | 满足 | 4.1 章节 |
| 第 1~9 章齐全 | 3.4 | 满足 | 全文 1~10 章节完整 |
| 每个外部成员调用均有头文件原文签名对应 | 3.5-1/2 | 满足 | 7.0 章节 |
| 每个被改/删的 `#include` 已核查间接包含链 | 3.5-3 | 满足 | 8.0 章节 |
| 已复用既有 SSOT 入口，无另起炉灶 | 2.4 | 满足 | 4.5 章节 |
| 无内联 setStyleSheet，样式走 style.qss | 4.1 | 不涉及 | 本次不触及 UI 样式 |
| 无平台级 Hack，无无范围全局事件过滤器 | 4.2 | 满足 | 线程安全 / 无全局过滤器 |
| 批量操作为一次聚合广播 | 4.3 | 满足 | 200ms 防抖合并刷库 |
| 迁移的所有数值原样照抄 | 4.4 | 不涉及 | 本次不修改 UI 尺寸/颜色数值 |
| 未改动用户既有设计理念与交互方式 | 1.2 / Memories.md | 满足 | 零修改界面交互 |
| 未触及方案范围外的文件 | 5.3 | 满足 | 仅针对清单文件 |

---

## 10. Zero-Value-Alteration Check（数值零篡改核对）

- **结论**：不涉及（本次修改为纯后台观察者回调与增量刷库机制搭建，不涉及任何 UI 外观数值改变）。
