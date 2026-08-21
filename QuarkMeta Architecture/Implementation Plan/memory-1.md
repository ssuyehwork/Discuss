# Pure Disk Mode Memory & Managed Code Purge Plan (Iterative Version 1)

## Overview
According to `QuarkMeta-Architecture-Planning.md`, QuarkMeta operates exclusively in pure disk directory direct-connect mode. All file and folder metadata (ratings, colors, tags, notes) must strictly be written to discrete `.QuarkMeta.json` files via `QuarkMetaJson`.

This implementation plan completely purges remaining memory-mode remnants, dual-accounting database logic, and dead code structures:
1. Physical deletion of unreferenced zombie class file `src/core/CategoryLockManager.h`.
2. Removal of obsolete `NativeFolderWatcher` (IOCP) references and headless monitor initialization (`src/core/CoreController.cpp`, `src/core/SystemBootstrapper.h`, `src/core/SystemBootstrapper.cpp`).
3. Cleanup of legacy category logic in `StatisticsService` (`userCategoryCounts` map maintenance in `src/meta/StatisticsService.h` and `src/meta/StatisticsService.cpp`).
4. Clean removal of `system_stats` table progress key reads/writes (`PROGRESS:<path>`) in `src/meta/MetadataManager.cpp`.
5. Elimination of dual-writing metadata to `global.db`'s legacy `metadata` table in `src/meta/MetadataManager.cpp` (`kSqlInsertMeta` calls), while preserving `recordsToSync` so that metadata changes pass to `.QuarkMeta.json` via `QuarkMetaJson`.

---

## Modified Files List
- `src/core/CategoryLockManager.h` (Deleted file)
- `src/core/CoreController.cpp`
- `src/core/SystemBootstrapper.h`
- `src/core/SystemBootstrapper.cpp`
- `src/meta/StatisticsService.h`
- `src/meta/StatisticsService.cpp`
- `src/meta/MetadataManager.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/core/CategoryLockManager.h`
Physical deletion of `src/core/CategoryLockManager.h`. (Note: File is unreferenced in `CMakeLists.txt` and all other C++ source files).

---

### 2. `src/core/CoreController.cpp`
Remove obsolete IOCP monitoring comments.

```
<<<<<<< SEARCH
            // 启动原生监控服务 (对应用户原话："采用NativeFolderWatcher (IOCP) 机制的方式")
            // 资源库无需开启 IOCP 监控（已取消）
=======
>>>>>>> REPLACE
```

---

### 3. `src/core/SystemBootstrapper.h`
Update Doxygen comments and header declarations to reflect pure disk mode.

```
<<<<<<< SEARCH
    /**
     * @brief 驱动多盘符资源库并开启底层 NativeFolderWatcher IOCP 监控 (从 MainWindow 移出)
     */
    void bootstrapMonitors();
=======
    /**
     * @brief 驱动多盘符资源库初始化 (从 MainWindow 移出)
     */
    void bootstrapMonitors();
>>>>>>> REPLACE
```

---

### 4. `src/core/SystemBootstrapper.cpp`
Remove legacy IOCP monitoring logs.

```
<<<<<<< SEARCH
void SystemBootstrapper::bootstrapMonitors() {
    qDebug() << "[Boot] SystemBootstrapper 开始点火底层 IOCP 监控...";
=======
void SystemBootstrapper::bootstrapMonitors() {
>>>>>>> REPLACE
```

---

### 5. `src/meta/StatisticsService.h`
Remove legacy category counts container `userCategoryCounts`.

```
<<<<<<< SEARCH
    QMap<int, int> userCategoryCounts;
=======
>>>>>>> REPLACE
```

---

### 6. `src/meta/StatisticsService.cpp`
Remove legacy category counting updates.

```
<<<<<<< SEARCH
        m_cachedSnapshot.userCategoryCounts[targetCatId]++;
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (m_cachedSnapshot.userCategoryCounts.contains(userCatId) && m_cachedSnapshot.userCategoryCounts[userCatId] > 0) {
            m_cachedSnapshot.userCategoryCounts[userCatId]--;
        }
=======
>>>>>>> REPLACE
```

---

### 7. `src/meta/MetadataManager.cpp`
Remove `system_stats` progress persistence and legacy `metadata` table dual-writing while ensuring `.QuarkMeta.json` disk sync pipeline remains intact.

```
<<<<<<< SEARCH
    // 3. 持久化进度到 system_stats 表
    const char* upsertSql = "INSERT OR REPLACE INTO system_stats (key, value) VALUES (?, ?)";
    if (sqlite3_prepare_v2(db, upsertSql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string key = "PROGRESS:" + QString::fromStdWString(nFolder).toUtf8().toStdString();
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 2, progress);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
        }
        sqlite3_finalize(stmt);
    }
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    const char* sql = "SELECT value FROM system_stats WHERE key = ?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        std::string key = "PROGRESS:" + QString::fromStdWString(nFolder).toUtf8().toStdString();
        sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            progress = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
=======
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            sqlite3_stmt* memStmt;
            if (sqlite3_prepare_v2(memDb, kSqlInsertMeta, -1, &memStmt, nullptr) == SQLITE_OK) {
                bindMetaHelper(memStmt, p, rMeta);

                if (sqlite3_step(memStmt) == SQLITE_DONE) {

                    {
                        size_t idx = getShardIndex(p);
                        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
                        m_shards[idx].items[p] = rMeta;
                    }
                    recordsToSync.push_back({p, rMeta});
                }
                sqlite3_finalize(memStmt);
            }
=======
            // Pure disk mode: bypass writing non-root item metadata to global.db's metadata table.
            {
                size_t idx = getShardIndex(p);
                std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
                m_shards[idx].items[p] = rMeta;
            }
            recordsToSync.push_back({p, rMeta});
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. Build Verification
Run CMake and compile the project to verify zero compilation errors:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 2. Verification Checklist
- [x] `src/core/CategoryLockManager.h` is deleted.
- [x] Project compiles cleanly without undefined symbols or MOC issues.
- [x] Category counts maintenance and IOCP monitoring logs are purged.
- [x] Metadata updates bypass SQLite `metadata` tables and pass `recordsToSync` directly to `QuarkMetaJson` for `.QuarkMeta.json` disk persistence.
