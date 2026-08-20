# Disk Trash File_ID Isolation and Creation-Time Restore Plan

## 1. Overview
This implementation plan addresses two major defects in QuarkMeta's existing disk trash mechanism:
1. **Physical Renaming & In-Trash Collision Risk**: Previously, `DiskTrashService::moveToDiskTrash` appended timestamps to filenames upon collision, modifying the physical name of files and failing when multiple items with identical names were moved to trash within the same second.
2. **Restore Collision & Name Arbitram**: Previously, restoring a file via `DiskTrashService::restoreFromDiskTrash` directly executed `QFile::rename(trashPath, originalPath)`, causing restore failures if a file with the same name already existed in the target directory.

### Solution Architecture:
- **FILE_ID Container Isolation**: When items (files or directories) are moved to the trash (`<Drive>:\.QuarkMeta\disk_trash\`), each item is placed inside its own dedicated subfolder named after its unique `file_id` (`<Drive>:\.QuarkMeta\disk_trash\{file_id}\`). The item's actual filename/foldername remains 100% untouched.
- **Creation-Time Based Restore Conflict Resolution**: Upon restore, the system queries the item's original creation time ($T_{\text{trash}}$) stored in the `disk_trash` SQLite table and compares it with the creation time ($T_{\text{disk}}$) of any existing item at `original_path`:
  - **No Conflict**: The item is moved back to `original_path`.
  - **$T_{\text{trash}} < T_{\text{disk}}$ (Trash item is older)**: The trash item takes the original name (`A.txt`). The existing disk item is renamed with a hyphenated counter (`A-1.txt`, `A-2.txt`, etc.) to yield the original path.
  - **$T_{\text{disk}} \le T_{\text{trash}}$ (Disk item is older or equal)**: The disk item keeps the original name (`A.txt`). The trash item is restored and renamed with a hyphenated counter (`A-1.txt`, `A-2.txt`, etc.).

---

## 2. Modified Files List
1. `src/meta/DatabaseManager.cpp` - Schema migration to add `file_id` and `created_at` columns to `disk_trash` table.
2. `src/meta/DiskTrashRepo.h` & `src/meta/DiskTrashRepo.cpp` - Update `DiskTrashRawItem` struct and queries to include `file_id` and `created_at`.
3. `src/core/DiskTrashService.h` & `src/core/DiskTrashService.cpp` - Implement `file_id` container creation, physical move without renaming, creation-time comparison, and hyphenated (`-N`) restore conflict resolution.
4. `src/ui/ContentPanel.cpp` - Pass `file_id` and `created_at` when loading trash items into item records.

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/meta/DatabaseManager.cpp`
```
<<<<<<< SEARCH
        -- 物理磁盘回收站独立表 (双轨隔离)
        CREATE TABLE IF NOT EXISTS disk_trash (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            trash_path TEXT NOT NULL,        -- 暂存区物理路径
            original_path TEXT NOT NULL,     -- 原始物理绝对路径
            drive_letter TEXT NOT NULL,      -- 所属盘符
            file_name TEXT NOT NULL,         -- 原始文件名
            is_folder INTEGER DEFAULT 0,     -- 是否为文件夹 (1: 是, 0: 否)
            file_size INTEGER DEFAULT 0,     -- 文件大小
            deleted_at INTEGER DEFAULT 0     -- 删除时间戳 (毫秒)
        );
        CREATE INDEX IF NOT EXISTS idx_disk_trash_drive_letter ON disk_trash(drive_letter);
=======
        -- 物理磁盘回收站独立表 (双轨隔离)
        CREATE TABLE IF NOT EXISTS disk_trash (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id TEXT NOT NULL,           -- 项目自身 File_ID 隔离盒标识
            trash_path TEXT NOT NULL,        -- 暂存区物理路径
            original_path TEXT NOT NULL,     -- 原始物理绝对路径
            drive_letter TEXT NOT NULL,      -- 所属盘符
            file_name TEXT NOT NULL,         -- 原始文件名
            is_folder INTEGER DEFAULT 0,     -- 是否为文件夹 (1: 是, 0: 否)
            file_size INTEGER DEFAULT 0,     -- 文件大小
            created_at INTEGER DEFAULT 0,    -- 原始创建时间戳 (毫秒)
            deleted_at INTEGER DEFAULT 0     -- 删除时间戳 (毫秒)
        );
        CREATE INDEX IF NOT EXISTS idx_disk_trash_drive_letter ON disk_trash(drive_letter);
>>>>>>> REPLACE
```

### 3.2 `src/meta/DiskTrashRepo.h`
```
<<<<<<< SEARCH
struct DiskTrashRawItem {
    int id;
    std::wstring trashPath;
    std::wstring originalPath;
    std::wstring fileName;
    bool isFolder;
    long long fileSize;
    long long deletedAt;
};
=======
struct DiskTrashRawItem {
    int id;
    std::wstring fileId;
    std::wstring trashPath;
    std::wstring originalPath;
    std::wstring fileName;
    bool isFolder;
    long long fileSize;
    long long createdAt;
    long long deletedAt;
};
>>>>>>> REPLACE
```

### 3.3 `src/meta/DiskTrashRepo.cpp`
```
<<<<<<< SEARCH
std::vector<DiskTrashRawItem> DiskTrashRepo::getAllTrashItems() {
    std::vector<DiskTrashRawItem> result;
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();

    for (sqlite3* db : dbs) {
        if (!db) continue;
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, deleted_at FROM disk_trash";
=======
std::vector<DiskTrashRawItem> DiskTrashRepo::getAllTrashItems() {
    std::vector<DiskTrashRawItem> result;
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();

    for (sqlite3* db : dbs) {
        if (!db) continue;
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT id, file_id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, created_at, deleted_at FROM disk_trash";
>>>>>>> REPLACE
```

### 3.4 `src/core/DiskTrashService.cpp` (Move To Trash with File_ID Container)
```
<<<<<<< SEARCH
        QString dest = trashDir + "/" + info.fileName();
        // 冲突处理：如果回收站已有同名文件，增加时间戳后缀
        if (QFile::exists(dest)) {
            dest = trashDir + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_") + info.fileName();
        }

        // 1. 物理同盘位移 (秒级移动)
        if (QFile::rename(p, dest)) {
            // 2. 写入独立的 disk_trash 数据库，不污染 metadata 表
            sqlite3* db = DatabaseManager::instance().getDbForPath(p.toStdWString());
            if (!db) {
                allOk = false;
                continue;
            }

            SqlTransaction trans(db);
            sqlite3_stmt* stmt = nullptr;
            const char* sql = "INSERT INTO disk_trash (trash_path, original_path, drive_letter, file_name, is_folder, file_size, deleted_at) VALUES (?, ?, ?, ?, ?, ?, ?)";
=======
        QString fileId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString itemContainerDir = trashDir + "/" + fileId;
        QDir().mkpath(itemContainerDir);

        QString dest = itemContainerDir + "/" + info.fileName();

        // 1. 物理同盘位移 (原名直接移动至 FILE_ID 隔离盒)
        if (QFile::rename(p, dest)) {
            sqlite3* db = DatabaseManager::instance().getDbForPath(p.toStdWString());
            if (!db) {
                allOk = false;
                continue;
            }

            qint64 createdAt = info.birthTime().isValid() ? info.birthTime().toMSecsSinceEpoch() : info.lastModified().toMSecsSinceEpoch();

            SqlTransaction trans(db);
            sqlite3_stmt* stmt = nullptr;
            const char* sql = "INSERT INTO disk_trash (file_id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, created_at, deleted_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
>>>>>>> REPLACE
```

### 3.5 `src/core/DiskTrashService.cpp` (Restore Conflict Resolution & Creation Time Comparison)
```
<<<<<<< SEARCH
    QString originalPath;
    sqlite3_stmt* stmt = nullptr;
    const char* sqlSel = "SELECT original_path FROM disk_trash WHERE id = ?";
    if (sqlite3_prepare_v2(db, sqlSel, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const wchar_t* wOrig = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
            if (wOrig) {
                originalPath = QString::fromWCharArray(wOrig);
            }
        }
        sqlite3_finalize(stmt);
    }
=======
    QString originalPath;
    qint64 trashCreatedAt = 0;
    sqlite3_stmt* stmt = nullptr;
    const char* sqlSel = "SELECT original_path, created_at FROM disk_trash WHERE id = ?";
    if (sqlite3_prepare_v2(db, sqlSel, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const wchar_t* wOrig = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
            if (wOrig) {
                originalPath = QString::fromWCharArray(wOrig);
            }
            trashCreatedAt = sqlite3_column_int64(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // QFile::rename 移回 original_path
    if (QFile::rename(trashPath, originalPath)) {
        SqlTransaction trans(db);
        sqlite3_stmt* delStmt = nullptr;
        const char* sqlDel = "DELETE FROM disk_trash WHERE id = ?";
=======
    // 检查目标位置是否存在同名文件/文件夹冲突，基于创建时间权威与连字符 -N 递增避让
    QString targetPath = originalPath;
    if (QFile::exists(originalPath)) {
        QFileInfo existingInfo(originalPath);
        qint64 diskCreatedAt = existingInfo.birthTime().isValid() ? existingInfo.birthTime().toMSecsSinceEpoch() : existingInfo.lastModified().toMSecsSinceEpoch();

        if (trashCreatedAt < diskCreatedAt) {
            // 被还原的项目创建时间更早：占用原名 originalPath，将磁盘现有项目自动重命名为 A-1.ext
            QString baseDir = existingInfo.absolutePath();
            QString baseName = existingInfo.completeBaseName();
            QString suffix = existingInfo.suffix();
            QString newDiskPath;
            int counter = 1;
            do {
                QString candidateName = suffix.isEmpty() ? QString("%1-%2").arg(baseName).arg(counter) : QString("%1-%2.%3").arg(baseName).arg(counter).arg(suffix);
                newDiskPath = baseDir + "/" + candidateName;
                counter++;
            } while (QFile::exists(newDiskPath));

            QFile::rename(originalPath, newDiskPath);
            targetPath = originalPath;
        } else {
            // 磁盘项目更早或等于：磁盘项目保留原名，被还原的项目重命名为 A-1.ext 还原移出
            QFileInfo trashInfo(originalPath);
            QString baseDir = trashInfo.absolutePath();
            QString baseName = trashInfo.completeBaseName();
            QString suffix = trashInfo.suffix();
            int counter = 1;
            do {
                QString candidateName = suffix.isEmpty() ? QString("%1-%2").arg(baseName).arg(counter) : QString("%1-%2.%3").arg(baseName).arg(counter).arg(suffix);
                targetPath = baseDir + "/" + candidateName;
                counter++;
            } while (QFile::exists(targetPath));
        }
    }

    if (QFile::rename(trashPath, targetPath)) {
        // 清理空 FILE_ID 隔离盒目录
        QDir(QFileInfo(trashPath).absolutePath()).removeRecursively();

        SqlTransaction trans(db);
        sqlite3_stmt* delStmt = nullptr;
        const char* sqlDel = "DELETE FROM disk_trash WHERE id = ?";
>>>>>>> REPLACE
```

### 3.6 `src/ui/ContentPanel.cpp`
```
<<<<<<< SEARCH
    for (const auto& raw : rawDiskTrash) {
        ItemRecord rec;
        rec.isDiskTrash = true;
        rec.diskTrashId = raw.id;
        rec.path = QString::fromStdWString(raw.trashPath);
=======
    for (const auto& raw : rawDiskTrash) {
        ItemRecord rec;
        rec.isDiskTrash = true;
        rec.diskTrashId = raw.id;
        rec.fileId = QString::fromStdWString(raw.fileId);
        rec.path = QString::fromStdWString(raw.trashPath);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 4.1 Build Instructions
Run the CMake build system to verify proper compilation:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 4.2 Automated & Manual Verification Steps
1. **Batch Move Test**: Move 10 identical files named `document.pdf` from different locations or sequentially to the trash. Verify that 10 separate subdirectories named after their `FILE_ID` are generated in `<Drive>:\.QuarkMeta\disk_trash\`, each containing `document.pdf` without any name alteration.
2. **Restore Collision & Hyphenated Naming Test**:
   - Delete `A.txt` (created at time $T_1$).
   - Re-create a new `A.txt` in the same directory (created at time $T_2$, where $T_2 > T_1$).
   - Trigger restore for the deleted `A.txt`.
   - Verify that the restored `A.txt` (older $T_1$) keeps `A.txt` and the existing file is renamed to `A-1.txt`.
