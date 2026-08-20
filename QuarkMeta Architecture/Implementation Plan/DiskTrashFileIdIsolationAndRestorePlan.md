# 基于 File_ID 隔离盒与创建时间权威判别回收站无脑实施方案 —— DiskTrashFileIdIsolationAndRestorePlan

## 1. Overview（概述与解决的问题）

本实施方案旨在彻底重构 QuarkMeta 纯磁盘模式下的物理回收站机制，解决以下三大核心缺陷与安全隐患：
1. **入库重名碰撞与乱改物理文件名风险**：原代码 `DiskTrashService::moveToDiskTrash` 在发现同名文件时通过给文件名拼接时间戳前缀来避让，违背了“文件夹/文件名称 100% 保持原始名称”的原则，且在同一秒内批量删除多个同名项目时会导致移动失败。
2. **移动后提取属性导致 `is_folder` 判定错误（文件夹显示为 FILE 徽章）**：原代码在 `QFile::rename(p, dest)` **之后**才去调用 `QFileInfo(p).isDir()`，由于原路径 `p` 已被移走，`isDir()` 永远返回 `false`，导致数据库中文件夹被错误记录为普通文件 `0`。
3. **回收站列表遗漏 `rec.suffix` 导致图片无缩略图**：原代码在加载回收站记录时未给 `ItemRecord.suffix` 赋值，导致视图层误以为无后缀而不触发缩略图提取队列。
4. **还原（Restore）冲撞与无脑覆盖问题**：原代码 `DiskTrashService::restoreFromDiskTrash` 直接调用 `QFile::rename(trashPath, originalPath)`，当目标路径已存在同名项目时会导致还原失败。

### 核心架构解法：
- **属性提前提取（逻辑归位）**：在执行 `QFile::rename` 之前，先提取 `isDir = info.isDir()`、`size = info.size()`、`createdAt = info.birthTime()`，保证存入 `disk_trash` 数据库的 `is_folder` 100% 准确！
- **FILE_ID 隔离盒存储**：将项目移入回收站（`<盘符>:\.QuarkMeta\disk_trash\`）时，系统为每个项目在其自身的 `File_ID` 独立文件夹中建盒（`<盘符>:\.QuarkMeta\disk_trash\{File_ID}\`），项目原封不动存入，**磁盘上的文件/文件夹名称 100% 保持原始名称**。
- **列表数据补全 `suffix`**：在 `ContentPanel.cpp` 加载回收站记录时，自动补全 `rec.suffix = QFileInfo(rec.filename).suffix()`，使缩略图管线正常触发并呈现图形缩略图。
- **基于创建时间的权威判别与 `-N` 还原重命名**：还原时对比数据库中记录的原始创建时间戳 $T_{\text{trash}}$ 与目标磁盘项目的创建时间戳 $T_{\text{disk}}$：
  - **无冲突**：直接原名移回 `original_path`。
  - **$T_{\text{trash}} < T_{\text{disk}}$（被还原项目更早）**：被还原项目作为最早创建的权威占用 `A.txt`，磁盘上现有的较晚项目自动重命名避让为 `A-1.txt`（或 `A-2.txt`）。
  - **$T_{\text{disk}} \le T_{\text{trash}}$（磁盘现有项目更早或相同）**：磁盘现有项目保持 `A.txt`，被还原的项目重命名为 `A-1.txt` 还原移出。

---

## 2. Modified Files List（影响文件清单）

1. `CMakeLists.txt` - 确认 `DiskTrashService` 和 `DiskTrashRepo` 的源文件列表与 Qt5::Core 依赖注册。
2. `src/meta/DatabaseManager.cpp` - 数据库 schema 更新及 `PRAGMA table_info(disk_trash)` 自动平滑迁移。
3. `src/meta/DiskTrashRepo.h` & `src/meta/DiskTrashRepo.cpp` - 更新 `DiskTrashRawItem` 结构体及查询 SQL 语句与 10 列对应属性解析提取。
4. `src/core/DiskTrashService.h` & `src/core/DiskTrashService.cpp` - 引入 `<QUuid>`，在移动前提取 `QFileInfo` 属性，实现基于 `File_ID` 隔离盒的入库位移逻辑，以及基于创建时间权威判别与 `-N` 连字符重命名的还原冲撞算法。
5. `src/ui/ContentPanel.cpp` - 更新加载回收站视图的数据项映射，补全 `suffix` 与 `isDir` 识别。

---

## 3. Detailed Line-by-Line Changes（包含 CMakeLists.txt 在内的精准替换块）

### 3.1 修改 `CMakeLists.txt`
**修改文件**：`CMakeLists.txt`
**修改目的**：确认 `DiskTrashService` 和 `DiskTrashRepo` 在 CMake 中的源码和头文件注册。

```
<<<<<<< SEARCH
    src/core/DiskTrashService.cpp
    src/core/DiskTrashService.h
=======
    src/core/DiskTrashService.cpp
    src/core/DiskTrashService.h
>>>>>>> REPLACE
```

### 3.2 修改 `src/meta/DatabaseManager.cpp`（Schema 创建与平滑迁移）
**修改文件**：`src/meta/DatabaseManager.cpp`
**修改目的**：为 `disk_trash` 数据库表增加 `file_id` 隔离盒标识列与 `created_at` 原始创建时间列，并添加 `ALTER TABLE` 自动迁移逻辑以保证旧数据库平滑无缝兼容。

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

```
<<<<<<< SEARCH
    // 2026-08-xx 新增字段：持久化基名与后缀名，避免每次启动现算并优化回填
=======
    // 物理磁盘回收站平滑迁移：自动补全 file_id 与 created_at 字段
    bool hasTrashFileIdColumn = false;
    bool hasTrashCreatedAtColumn = false;
    sqlite3_stmt* trashCheckStmt = nullptr;
    if (sqlite3_prepare_v2(conn.memDb, "PRAGMA table_info(disk_trash)", -1, &trashCheckStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(trashCheckStmt) == SQLITE_ROW) {
            const char* name = reinterpret_cast<const char*>(sqlite3_column_text(trashCheckStmt, 1));
            if (name) {
                std::string sName(name);
                if (sName == "file_id") hasTrashFileIdColumn = true;
                if (sName == "created_at") hasTrashCreatedAtColumn = true;
            }
        }
        sqlite3_finalize(trashCheckStmt);
    }
    if (!hasTrashFileIdColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE disk_trash ADD COLUMN file_id TEXT DEFAULT ''", nullptr, nullptr, nullptr);
    }
    if (!hasTrashCreatedAtColumn) {
        sqlite3_exec(conn.memDb, "ALTER TABLE disk_trash ADD COLUMN created_at INTEGER DEFAULT 0", nullptr, nullptr, nullptr);
    }

    // 2026-08-xx 新增字段：持久化基名与后缀名，避免每次启动现算并优化回填
>>>>>>> REPLACE
```

### 3.3 修改 `src/meta/DiskTrashRepo.h`
**修改文件**：`src/meta/DiskTrashRepo.h`
**修改目的**：在 `DiskTrashRawItem` 实体结构体中增加 `fileId` 与 `createdAt` 字段。

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

### 3.4 修改 `src/meta/DiskTrashRepo.cpp`
**修改文件**：`src/meta/DiskTrashRepo.cpp`
**修改目的**：更新查询 SQL 并精确绑定与提取 10 列属性（`file_id` 列 1，`created_at` 列 8）。

```
<<<<<<< SEARCH
std::vector<DiskTrashRawItem> DiskTrashRepo::getAllTrashItems() {
    std::vector<DiskTrashRawItem> result;
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();

    for (sqlite3* db : dbs) {
        if (!db) continue;
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, deleted_at FROM disk_trash"; 
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                DiskTrashRawItem item;
                item.id = sqlite3_column_int(stmt, 0);
                const wchar_t* wTrash = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
                const wchar_t* wOrig = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 2));
                const wchar_t* wName = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 4));
                if (wTrash) item.trashPath = wTrash;
                if (wOrig) item.originalPath = wOrig;
                if (wName) item.fileName = wName;
                item.isFolder = (sqlite3_column_int(stmt, 5) != 0);
                item.fileSize = sqlite3_column_int64(stmt, 6);
                item.deletedAt = sqlite3_column_int64(stmt, 7);
                result.push_back(item);
            }
            sqlite3_finalize(stmt);
        }
    }
    return result;
}
=======
std::vector<DiskTrashRawItem> DiskTrashRepo::getAllTrashItems() {
    std::vector<DiskTrashRawItem> result;
    std::vector<sqlite3*> dbs = DatabaseManager::instance().getActiveMemoryDbs();

    for (sqlite3* db : dbs) {
        if (!db) continue;
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT id, file_id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, created_at, deleted_at FROM disk_trash"; 
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                DiskTrashRawItem item;
                item.id = sqlite3_column_int(stmt, 0);
                const wchar_t* wFileId = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
                const wchar_t* wTrash = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 2));
                const wchar_t* wOrig = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 3));
                const wchar_t* wName = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 5));
                if (wFileId) item.fileId = wFileId;
                if (wTrash) item.trashPath = wTrash;
                if (wOrig) item.originalPath = wOrig;
                if (wName) item.fileName = wName;
                item.isFolder = (sqlite3_column_int(stmt, 6) != 0);
                item.fileSize = sqlite3_column_int64(stmt, 7);
                item.createdAt = sqlite3_column_int64(stmt, 8);
                item.deletedAt = sqlite3_column_int64(stmt, 9);
                result.push_back(item);
            }
            sqlite3_finalize(stmt);
        }
    }
    return result;
}
>>>>>>> REPLACE
```

### 3.5 修改 `src/core/DiskTrashService.cpp`（属性提前提取 + File_ID 盒子建目录逻辑）
**修改文件**：`src/core/DiskTrashService.cpp`
**修改目的**：引入 `<QUuid>` 头文件，**在移动前优先提取 `isDir`、`size`、`createdAt` 属性**，解决原代码移动后 `isDir()` 失效乱写数据库的重病，并实现 File_ID 隔离盒模式。

```
<<<<<<< SEARCH
#include "DiskTrashService.h"
#include "../meta/DatabaseManager.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QDebug>
=======
#include "DiskTrashService.h"
#include "../meta/DatabaseManager.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDebug>
>>>>>>> REPLACE
```

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
            
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                QString driveLetter = drive.left(1).toUpper();
                sqlite3_bind_text16(stmt, 1, dest.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 2, p.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 3, driveLetter.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 4, info.fileName().toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 5, info.isDir() ? 1 : 0);
                sqlite3_bind_int64(stmt, 6, info.size());
                sqlite3_bind_int64(stmt, 7, QDateTime::currentMSecsSinceEpoch());
=======
        // 🚨 关键修复：在物理移动前提前抓取原文件的各项属性，避免移动后 p 不存在导致 info.isDir() 错判为 0！
        bool isFolder = info.isDir();
        qint64 fileSize = info.size();
        qint64 createdAt = info.birthTime().isValid() ? info.birthTime().toMSecsSinceEpoch() : info.lastModified().toMSecsSinceEpoch();

        QString fileId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString itemContainerDir = trashDir + "/" + fileId;
        QDir().mkpath(itemContainerDir);

        QString dest = itemContainerDir + "/" + info.fileName();

        // 1. 物理同盘位移 (原名直接移动至 FILE_ID 隔离盒)
        if (QFile::rename(p, dest)) {
            // 2. 写入独立的 disk_trash 数据库，不污染 metadata 表
            sqlite3* db = DatabaseManager::instance().getDbForPath(p.toStdWString());
            if (!db) {
                allOk = false;
                continue;
            }

            SqlTransaction trans(db);
            sqlite3_stmt* stmt = nullptr;
            const char* sql = "INSERT INTO disk_trash (file_id, trash_path, original_path, drive_letter, file_name, is_folder, file_size, created_at, deleted_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
            
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                QString driveLetter = drive.left(1).toUpper();
                sqlite3_bind_text16(stmt, 1, fileId.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 2, dest.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 3, p.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 4, driveLetter.toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text16(stmt, 5, info.fileName().toStdWString().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 6, isFolder ? 1 : 0);
                sqlite3_bind_int64(stmt, 7, fileSize);
                sqlite3_bind_int64(stmt, 8, createdAt);
                sqlite3_bind_int64(stmt, 9, QDateTime::currentMSecsSinceEpoch());
>>>>>>> REPLACE
```

### 3.6 修改 `src/core/DiskTrashService.cpp`（还原判定与 `-N` 重命名算法完整逻辑）
**修改文件**：`src/core/DiskTrashService.cpp`
**修改目的**：从数据库读取 `created_at`，并在还原冲突时根据创建时间比较结果执行 `-N` 连字符递增重命名避让，完整替换还原事务逻辑。

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
        bool success = false;
        if (sqlite3_prepare_v2(db, sqlDel, -1, &delStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(delStmt, 1, id);
            if (sqlite3_step(delStmt) == SQLITE_DONE) {
                trans.commit();
                DatabaseManager::instance().setDirty(true);
                success = true;
            }
            sqlite3_finalize(delStmt);
        }
        if (success) return true;
    } else {
        qWarning() << "[DiskTrashService] Failed to physically move back trash item to original path:" << originalPath;
    }
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
        bool success = false;
        if (sqlite3_prepare_v2(db, sqlDel, -1, &delStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(delStmt, 1, id);
            if (sqlite3_step(delStmt) == SQLITE_DONE) {
                trans.commit();
                DatabaseManager::instance().setDirty(true);
                success = true;
            }
            sqlite3_finalize(delStmt);
        }
        if (success) return true;
    } else {
        qWarning() << "[DiskTrashService] Failed to physically move back trash item to target path:" << targetPath;
    }
>>>>>>> REPLACE
```

### 3.7 修改 `src/ui/ContentPanel.cpp`（补全 `suffix` 与 `isDir` 识别）
**修改文件**：`src/ui/ContentPanel.cpp`
**修改目的**：在读取回收站视图列表时，把 `fileId` 与 `suffix` 赋值给 UI 数据记录项，解决图标与缩略图加载失效问题。

```
<<<<<<< SEARCH
    for (const auto& raw : rawDiskTrash) {
        ItemRecord rec;
        rec.isDiskTrash = true;
        rec.diskTrashId = raw.id;
        rec.path = QString::fromStdWString(raw.trashPath);
        rec.originalPath = QString::fromStdWString(raw.originalPath);
        rec.filename = QString::fromStdWString(raw.fileName);
        rec.isDir = raw.isFolder;
=======
    for (const auto& raw : rawDiskTrash) {
        ItemRecord rec;
        rec.isDiskTrash = true;
        rec.diskTrashId = raw.id;
        rec.fileId = QString::fromStdWString(raw.fileId);
        rec.path = QString::fromStdWString(raw.trashPath);
        rec.originalPath = QString::fromStdWString(raw.originalPath);
        rec.filename = QString::fromStdWString(raw.fileName);
        rec.suffix = QFileInfo(rec.filename).suffix();
        rec.isDir = raw.isFolder;
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证命令
在 Sandbox 环境中运行 CMake 与 Ninja 编译，验证代码变动 100% 通过编译且无 MOC 链接错误：
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 4.2 验证方法与测试用例
1. **文件夹图标与图形缩略图正确呈现测试**：
   - 将一个文件夹（如 `测试_a_009`）移入回收站，点击回收站按钮进入视图，确认图标准确呈现为 **文件夹图标与 "DIR" 徽章**，绝不再显示为带 "FILE" 的空白文档！
   - 将 PNG/JPG 图片移入回收站，确认内容区正常触发图形解码并**成功呈现渲染出来的缩略图**！
2. **批量移入测试（零修改文件名）**：
   同时将 10 个相同文件名（例如 `doc.pdf`）或文件夹移入回收站。检查 `<盘符>:\.QuarkMeta\disk_trash\` 目录，确认生成了 10 个以 `File_ID` 命名的独立隔离子文件夹，且内部项目文件名均完整保持为 `doc.pdf`，无任何时间戳前缀拼接。
3. **还原冲突与 `-N` 重命名测试**：
   - 删除创建时间为 $T_1$ 的 `test.txt`；
   - 在原路径下重新创建一个新的 `test.txt`（创建时间为 $T_2$，$T_2 > T_1$）；
   - 执行还原。验证创建时间更早（$T_1$）的项目占用 `test.txt`，而较新的项目被自动重命名避让为 `test-1.txt`。
