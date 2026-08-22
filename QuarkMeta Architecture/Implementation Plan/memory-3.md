# Memory Mode and Zombie Code Purge Incremental Plan 1

## Overview
This plan targets residual memory mode (托管库) and zombie code remnants identified during the post-sync codebase audit:
1. Purge `TagRepository::checkAndMigrate` and `system_stats` table operations from `TagRepository.cpp` / `TagRepository.h`.
2. Purge `DatabaseMigrator.h` legacy `CREATE TABLE metadata` zombie code.
3. Purge legacy `libraryCounts` and `userCategoryCounts` in `StatisticsService.cpp` / `StatisticsService.h`.
4. Purge category pill UI elements in `MetaPanel.cpp` / `MetaPanel.h`.
5. Remove `folderId` from `DuplicateDetectorService.cpp` / `DuplicateDetectorService.h`.

## Modified Files List
- `src/meta/TagRepository.h`
- `src/meta/TagRepository.cpp`
- `src/meta/DatabaseMigrator.h`
- `src/meta/DatabaseManager.cpp`
- `src/meta/StatisticsService.h`
- `src/meta/StatisticsService.cpp`
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`
- `src/meta/DuplicateDetectorService.h`
- `src/meta/DuplicateDetectorService.cpp`

## Detailed Line-by-Line Changes

### 1. `src/meta/TagRepository.h`
Remove declaration of `checkAndMigrate()`.

```
<<<<<<< SEARCH
    static void checkAndMigrate();
=======
>>>>>>> REPLACE
```

### 2. `src/meta/TagRepository.cpp`
Purge `checkAndMigrate()` implementation and `s_migrateOnce` call.

```
<<<<<<< SEARCH
QList<TagRepository::TagGroup> TagRepository::getAllGroups() {
    // 确保数据已自动检查与迁移
    static std::once_flag s_migrateOnce;
    std::call_once(s_migrateOnce, []() { checkAndMigrate(); });

    QList<TagGroup> results;
=======
QList<TagRepository::TagGroup> TagRepository::getAllGroups() {
    QList<TagGroup> results;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void TagRepository::checkAndMigrate() {
    sqlite3* globalDb = DatabaseManager::instance().getGlobalDb();
    if (!globalDb) return;

    // 1. 优先强标记检查
    bool migrationCompleted = false;
    sqlite3_stmt* checkStmt = nullptr;
    const char* checkSql = "SELECT value FROM system_stats WHERE key = 'tag_migration_completed'";
    if (sqlite3_prepare_v2(globalDb, checkSql, -1, &checkStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(checkStmt) == SQLITE_ROW) {
            migrationCompleted = (sqlite3_column_int(checkStmt, 0) == 1);
        }
        sqlite3_finalize(checkStmt);
    }

    if (migrationCompleted) {
        return; // 已完成迁移，直接跳过
    }

    // 2. 检查全局库中是否已有数据，以防止覆盖
    bool globalHasGroups = false;
    sqlite3_stmt* gGroupStmt = nullptr;
    if (sqlite3_prepare_v2(globalDb, "SELECT 1 FROM tag_groups LIMIT 1", -1, &gGroupStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(gGroupStmt) == SQLITE_ROW) {
            globalHasGroups = true;
        }
        sqlite3_finalize(gGroupStmt);
    }

    (void)globalHasGroups;

    // 4. 写入强标记，即使没有历史数据
    sqlite3_stmt* markerStmt = nullptr;
    const char* markerSql = "INSERT OR REPLACE INTO system_stats (key, value) VALUES ('tag_migration_completed', 1)";
    if (sqlite3_prepare_v2(globalDb, markerSql, -1, &markerStmt, nullptr) == SQLITE_OK) {
        sqlite3_step(markerStmt);
        sqlite3_finalize(markerStmt);
    }
    // 并写入脏标记确保落盘
    DatabaseManager::instance().setDirty(true);
    DatabaseManager::instance().flushAll();
}
=======
>>>>>>> REPLACE
```

### 3. `src/meta/DatabaseMigrator.h`
Remove `ensureActivated` with legacy `metadata` table creation.

```
<<<<<<< SEARCH
class DatabaseMigrator {
public:
    static bool ensureActivated(sqlite3* db) {
        // 专门负责 CREATE TABLE、ALTER TABLE 升级
        const char* sqlCreateMetadata =
            "CREATE TABLE IF NOT EXISTS metadata ("
            "  folder_id TEXT PRIMARY KEY, "
            "  path TEXT UNIQUE, "
            "  rating INTEGER, "
            "  color TEXT, "
            "  pinned INTEGER"
            ");";
        return sqlite3_exec(db, sqlCreateMetadata, nullptr, nullptr, nullptr) == SQLITE_OK;
    }

};
=======
class DatabaseMigrator {
public:
};
>>>>>>> REPLACE
```

### 4. `src/meta/DatabaseManager.cpp`
Remove `#include "DatabaseMigrator.h"` if unused.

```
<<<<<<< SEARCH
#include "DatabaseManager.h"
#include "DatabaseMigrator.h"
=======
#include "DatabaseManager.h"
>>>>>>> REPLACE
```

### 5. `src/meta/StatisticsService.h`
Remove `libraryCounts` and `userCategoryCounts` structs and functions.

```
<<<<<<< SEARCH
    struct LibraryCounts {
        int unassigned = 0;
        int trash = 0;
    };

    LibraryCounts getLibraryCounts();
    QMap<int, int> getUserCategoryCounts();
=======
>>>>>>> REPLACE
```

### 6. `src/meta/StatisticsService.cpp`
Remove implementation of `getLibraryCounts` and `getUserCategoryCounts`.

```
<<<<<<< SEARCH
StatisticsService::LibraryCounts StatisticsService::getLibraryCounts() {
    LibraryCounts counts;
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return counts;

    // Implementation...
    return counts;
}

QMap<int, int> StatisticsService::getUserCategoryCounts() {
    QMap<int, int> counts;
    return counts;
}
=======
>>>>>>> REPLACE
```

### 7. `src/meta/DuplicateDetectorService.h` & `.cpp`
Purge `folderId` string fields from `DuplicateItem` struct.

```
<<<<<<< SEARCH
    struct DuplicateItem {
        QString path;
        QString folderId;
        int64_t fileSize = 0;
=======
    struct DuplicateItem {
        QString path;
        int64_t fileSize = 0;
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Build the project:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
2. Verify pure disk operation:
   - Ensure application launches without SQLite errors or attempts to access `system_stats` or `metadata` tables.
   - Confirm tag management and duplicate detection operate seamlessly.
