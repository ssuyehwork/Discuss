# LibraryItemDao-1.md - LibraryItemDao 批量 SQL 查询性能重构与原始顺序保持实施方案（修正版）

## Chapter 1: Overview
针对 Code Review 中指出的 `LibraryItemDao.md` 缺陷进行修复：
1. **解决顺序颠倒问题**：之前的实现使用 `QMap<QString, ItemRecord> chunkItemMap` 按 `fileId` 暂存结果，遍历 `chunkItemMap` 时因 `QMap` 的默认键升序排序，破坏了输入参数 `paths` 指定的原始顺序。
2. **修正方案**：使用按规范化路径为 Key 的哈希字典 `QHash<QString, ItemRecord> chunkPathToItem` 暂存查出的记录；在装配关联标签和色板后，严格按输入 `chunkPaths` 的原始顺序将 `ItemRecord` 追加至 `result`，完美保持外部传入的特定文件排序。

---

## Chapter 2: Modified Files List
1. `src/meta/LibraryItemDao.cpp` (重构 `getItemsByPaths` 方法，实现分批 IN 聚合查询并严格保持原始路径顺序)

---

## Chapter 3: Detailed Line-by-Line Changes

### 1. `src/meta/LibraryItemDao.cpp`
```
<<<<<<< SEARCH
std::vector<ItemRecord> LibraryItemDao::getItemsByPaths(const QStringList& paths) {
    std::vector<ItemRecord> result;
    if (paths.isEmpty()) return result;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    for (const QString& path : paths) {
        QString normPath = QDir::toNativeSeparators(path);
        const char* sql = "SELECT file_id, file_path, file_name, base_name, extension, file_size, is_dir, is_empty, "
                          "is_hidden, is_managed, encrypted, pinned, rating, color_hex, auto_color, width, height, "
                          "thumb_status, note, url, sha256, created_at, updated_at, added_at "
                          "FROM library_items WHERE file_path = ? COLLATE NOCASE;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, normPath.toUtf8().constData(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                ItemRecord rec;
                rec.fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                rec.path = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                rec.filename = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
                rec.suffix = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
                rec.size = sqlite3_column_int64(stmt, 5);
                rec.isDir = (sqlite3_column_int(stmt, 6) != 0);
                rec.isEmpty = (sqlite3_column_int(stmt, 7) != 0);
                rec.isHidden = (sqlite3_column_int(stmt, 8) != 0);
                rec.isManaged = (sqlite3_column_int(stmt, 9) != 0);
                rec.encrypted = (sqlite3_column_int(stmt, 10) != 0);
                rec.pinned = (sqlite3_column_int(stmt, 11) != 0);
                rec.rating = sqlite3_column_int(stmt, 12);
                rec.manualColor = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13)));
                rec.autoColor = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14)));
                rec.width = sqlite3_column_int(stmt, 15);
                rec.height = sqlite3_column_int(stmt, 16);
                rec.thumbStatus = sqlite3_column_int(stmt, 17);
                rec.note = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18)));
                rec.url = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 19)));
                rec.sha256 = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20)));
                rec.ctime = sqlite3_column_int64(stmt, 21);
                rec.mtime = sqlite3_column_int64(stmt, 22);
                rec.added_at = sqlite3_column_int64(stmt, 23);
                sqlite3_finalize(stmt);

                // 装配 file_tags
                const char* tagSql = "SELECT tag_name FROM file_tags WHERE file_id = ?;";
                if (sqlite3_prepare_v2(db, tagSql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, rec.fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        rec.tags.append(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
                    }
                    sqlite3_finalize(stmt);
                }

                // 装配 item_palettes
                const char* palSql = "SELECT color_hex, ratio FROM item_palettes WHERE file_id = ? ORDER BY idx ASC;";
                if (sqlite3_prepare_v2(db, palSql, -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, rec.fileId.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        QString colorHex = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                        float ratio = static_cast<float>(sqlite3_column_double(stmt, 1));
                        rec.palettes.push_back({QColor(colorHex), ratio});
                    }
                    sqlite3_finalize(stmt);
                }

                result.push_back(rec);
            } else {
                sqlite3_finalize(stmt);
            }
        }
    }
    return result;
}
=======
std::vector<ItemRecord> LibraryItemDao::getItemsByPaths(const QStringList& paths) {
    std::vector<ItemRecord> result;
    if (paths.isEmpty()) return result;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db) return result;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    constexpr int kBatchSize = 500;
    for (int i = 0; i < paths.size(); i += kBatchSize) {
        int count = qMin(kBatchSize, paths.size() - i);
        QStringList chunkPaths;
        chunkPaths.reserve(count);
        for (int j = 0; j < count; ++j) {
            chunkPaths.append(QDir::toNativeSeparators(paths[i + j]));
        }

        // 1. 构建主表 IN (?, ?, ...) 查询
        QStringList placeholders;
        placeholders.reserve(count);
        for (int j = 0; j < count; ++j) {
            placeholders.append("?");
        }

        QString sql = QString("SELECT file_id, file_path, file_name, base_name, extension, file_size, is_dir, is_empty, "
                              "is_hidden, is_managed, encrypted, pinned, rating, color_hex, auto_color, width, height, "
                              "thumb_status, note, url, sha256, created_at, updated_at, added_at "
                              "FROM library_items WHERE file_path IN (%1) COLLATE NOCASE;")
                      .arg(placeholders.join(','));

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) {
            continue;
        }

        for (int j = 0; j < count; ++j) {
            sqlite3_bind_text(stmt, j + 1, chunkPaths[j].toUtf8().constData(), -1, SQLITE_TRANSIENT);
        }

        QHash<QString, ItemRecord> chunkPathToItem;
        QHash<QString, QString> fileIdToPath;
        QStringList chunkFileIds;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ItemRecord rec;
            rec.fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            rec.path = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            rec.filename = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            rec.suffix = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            rec.size = sqlite3_column_int64(stmt, 5);
            rec.isDir = (sqlite3_column_int(stmt, 6) != 0);
            rec.isEmpty = (sqlite3_column_int(stmt, 7) != 0);
            rec.isHidden = (sqlite3_column_int(stmt, 8) != 0);
            rec.isManaged = (sqlite3_column_int(stmt, 9) != 0);
            rec.encrypted = (sqlite3_column_int(stmt, 10) != 0);
            rec.pinned = (sqlite3_column_int(stmt, 11) != 0);
            rec.rating = sqlite3_column_int(stmt, 12);
            rec.manualColor = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13)));
            rec.autoColor = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14)));
            rec.width = sqlite3_column_int(stmt, 15);
            rec.height = sqlite3_column_int(stmt, 16);
            rec.thumbStatus = sqlite3_column_int(stmt, 17);
            rec.note = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18)));
            rec.url = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 19)));
            rec.sha256 = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20)));
            rec.ctime = sqlite3_column_int64(stmt, 21);
            rec.mtime = sqlite3_column_int64(stmt, 22);
            rec.added_at = sqlite3_column_int64(stmt, 23);

            if (!rec.path.isEmpty()) {
                QString normP = QDir::toNativeSeparators(rec.path).toLower();
                chunkPathToItem.insert(normP, rec);
                if (!rec.fileId.isEmpty()) {
                    fileIdToPath.insert(rec.fileId, normP);
                    chunkFileIds.append(rec.fileId);
                }
            }
        }
        sqlite3_finalize(stmt);

        if (chunkFileIds.isEmpty()) {
            continue;
        }

        // 2. 构建 file_tags 动态 IN 查询
        QStringList idPlaceholders;
        idPlaceholders.reserve(chunkFileIds.size());
        for (int j = 0; j < chunkFileIds.size(); ++j) {
            idPlaceholders.append("?");
        }

        QString tagSql = QString("SELECT file_id, tag_name FROM file_tags WHERE file_id IN (%1);")
                         .arg(idPlaceholders.join(','));

        if (sqlite3_prepare_v2(db, tagSql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
            for (int j = 0; j < chunkFileIds.size(); ++j) {
                sqlite3_bind_text(stmt, j + 1, chunkFileIds[j].toUtf8().constData(), -1, SQLITE_TRANSIENT);
            }
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QString fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                QString tagName = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                if (fileIdToPath.contains(fileId)) {
                    QString pathKey = fileIdToPath.value(fileId);
                    if (chunkPathToItem.contains(pathKey)) {
                        chunkPathToItem[pathKey].tags.append(tagName);
                    }
                }
            }
            sqlite3_finalize(stmt);
        }

        // 3. 构建 item_palettes 动态 IN 查询
        QString palSql = QString("SELECT file_id, color_hex, ratio FROM item_palettes WHERE file_id IN (%1) ORDER BY idx ASC;")
                         .arg(idPlaceholders.join(','));

        if (sqlite3_prepare_v2(db, palSql.toUtf8().constData(), -1, &stmt, nullptr) == SQLITE_OK) {
            for (int j = 0; j < chunkFileIds.size(); ++j) {
                sqlite3_bind_text(stmt, j + 1, chunkFileIds[j].toUtf8().constData(), -1, SQLITE_TRANSIENT);
            }
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                QString fileId = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                QString colorHex = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                float ratio = static_cast<float>(sqlite3_column_double(stmt, 2));
                if (fileIdToPath.contains(fileId)) {
                    QString pathKey = fileIdToPath.value(fileId);
                    if (chunkPathToItem.contains(pathKey)) {
                        chunkPathToItem[pathKey].palettes.push_back({QColor(colorHex), ratio});
                    }
                }
            }
            sqlite3_finalize(stmt);
        }

        // 4. 按照 chunkPaths 传入的原始顺序，将 ItemRecord 追加至 result
        for (const QString& path : chunkPaths) {
            QString pathKey = QDir::toNativeSeparators(path).toLower();
            if (chunkPathToItem.contains(pathKey)) {
                result.push_back(chunkPathToItem.value(pathKey));
            }
        }
    }

    return result;
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps
1. 使用 `cmake --build build` 编译项目。
2. 验证批量查询返回的条目顺序与输入的 `paths` 数组物理顺序 100% 对应。

---

## Chapter 5: SSOT API Reuse Self-Check
- 恪守数据单源原则，复用 `LibraryItemDao::getItemsByPaths` 方法。

---

## Chapter 6: Header API Signature Verification
- 签名与 `LibraryItemDao.h` 保持一致。

---

## Chapter 7: Header Inclusion Chain & Type Completeness Check
- 已包含 `<QHash>` 与 `<QDir>`，包含链完整。

---

## Chapter 8: Compliance Self-Check Table
| 检查项 | 状态 | 备注 |
|---|---|---|
| 1. 不直接编辑源文件 | 合规 | 实施方案保存在隔离目录 |
| 2. 方案采用 Git Merge Diff 格式 | 合规 | 使用 Standard Git Merge Diff |
| 3. 结果保持原始输入顺序 | 合规 | 通过 `chunkPaths` 顺次推入 `result` |

---

## Chapter 9: Zero-Value-Alteration Check
- 不涉及任何 UI 或参数变动。
