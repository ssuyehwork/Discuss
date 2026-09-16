# Implementation Plan - DiskTrashService & TrashService Restore Fix

## 1. Overview
修补 `TrashService::restoreItems` 与 `DiskTrashService::restoreFromDiskTrash` 涉及的单项/多项“从回收站还原”逻辑死锁 Bug。
解决因 `trashPath` 传递空字符串以及 SQL SELECT 未读取 `trash_path` 导致的 `QFile::rename` 物理还原失败、数据库记录在 `global.db` 的 `disk_trash` 表中无法删除彻底锁死的严重问题。

## 2. Modified Files List
- `src/core/DiskTrashService.cpp`
- `src/core/TrashService.cpp`

## 3. Detailed Line-by-Line Changes

### File 1: `src/core/DiskTrashService.cpp`
从 SQL 查询中补查 `trash_path` 字段，当传入的 `trashPath` 参数为空时自动以数据库中记录的物理暂存路径 `actualTrashPath` 作为真实还原源路径。

```
<<<<<<< SEARCH
bool DiskTrashService::restoreFromDiskTrash(int id, const QString& trashPath) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

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

    if (originalPath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(originalPath).absolutePath());

    QString targetPath = originalPath;
    if (QFile::exists(originalPath)) {
        QFileInfo existingInfo(originalPath);
        qint64 diskCreatedAt = existingInfo.birthTime().isValid() ? existingInfo.birthTime().toMSecsSinceEpoch() : existingInfo.lastModified().toMSecsSinceEpoch();

        if (trashCreatedAt < diskCreatedAt) {
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
        QDir(QFileInfo(trashPath).absolutePath()).removeRecursively();
=======
bool DiskTrashService::restoreFromDiskTrash(int id, const QString& trashPath) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    QString originalPath;
    QString actualTrashPath = trashPath;
    qint64 trashCreatedAt = 0;
    sqlite3_stmt* stmt = nullptr;
    const char* sqlSel = "SELECT original_path, created_at, trash_path FROM disk_trash WHERE id = ?";
    if (sqlite3_prepare_v2(db, sqlSel, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const wchar_t* wOrig = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
            if (wOrig) {
                originalPath = QString::fromWCharArray(wOrig);
            }
            trashCreatedAt = sqlite3_column_int64(stmt, 1);

            if (actualTrashPath.isEmpty()) {
                const wchar_t* wTrash = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 2));
                if (wTrash) {
                    actualTrashPath = QString::fromWCharArray(wTrash);
                }
            }
        }
        sqlite3_finalize(stmt);
    }

    if (originalPath.isEmpty() || actualTrashPath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(originalPath).absolutePath());

    QString targetPath = originalPath;
    if (QFile::exists(originalPath)) {
        QFileInfo existingInfo(originalPath);
        qint64 diskCreatedAt = existingInfo.birthTime().isValid() ? existingInfo.birthTime().toMSecsSinceEpoch() : existingInfo.lastModified().toMSecsSinceEpoch();

        if (trashCreatedAt < diskCreatedAt) {
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

    if (QFile::rename(actualTrashPath, targetPath)) {
        QDir(QFileInfo(actualTrashPath).absolutePath()).removeRecursively();
>>>>>>> REPLACE
```

---

### File 2: `src/core/TrashService.cpp`
完善 `restoreItems` 提示框与返回值保护。

```
<<<<<<< SEARCH
bool TrashService::restoreItems(const QList<int>& trashIds, QWidget* parentWidget) {
    Q_UNUSED(parentWidget);
    if (trashIds.isEmpty()) return false;

    for (int id : trashIds) {
        DiskTrashService::restoreFromDiskTrash(id, "");
    }
    MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
    emit trashOperationCompleted();
    return true;
}
=======
bool TrashService::restoreItems(const QList<int>& trashIds, QWidget* parentWidget) {
    Q_UNUSED(parentWidget);
    if (trashIds.isEmpty()) return false;

    int successCount = 0;
    for (int id : trashIds) {
        if (DiskTrashService::restoreFromDiskTrash(id, "")) {
            successCount++;
        }
    }
    if (successCount > 0) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("成功还原 %1 个回收站项目").arg(successCount), 1500, QColor("#2ecc71"));
        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
        emit trashOperationCompleted();
    }
    return successCount > 0;
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
```bash
# 1. 配置并构建 CMake 项目
cmake -B build -S .
cmake --build build --config Release

# 2. 验证路径与测试
# - 选中回收站项目点击“还原”
# - 检查被选项目是否正确回到原位置或自动平滑重命名
# - 检查 global.db 数据库中的 disk_trash 表该 id 记录是否被成功物理 DELETE 删除
```

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT 入口复用**：已复用 `MetadataManager::notifyUI` 与 `emit trashOperationCompleted()` 触达 `ContentPanel::refreshAll()` SSOT 刷新通道。
- **无重复实现**：修复直接作用于唯一的 `DiskTrashService` 基础设施层。

## 6. Header API Signature Verification
| 类名 / 模块名 | 调用的成员函数/静态函数 | 物理头文件签名 |
| :--- | :--- | :--- |
| `DiskTrashService` | `restoreFromDiskTrash` | `static bool restoreFromDiskTrash(int id, const QString& trashPath);` |
| `MetadataManager` | `notifyUI` | `void notifyUI(RefreshLevel level);` |
| `ToolTipOverlay` | `showText` | `void showText(const QPoint& globalPos, const QString& text, int timeoutMs = 1500, const QColor& bgColor = QColor("#2ecc71"));` |
