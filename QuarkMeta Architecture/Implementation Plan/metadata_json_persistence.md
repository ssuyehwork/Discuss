# Discrete `.QuarkMeta.json` Metadata Synchronization & Persistence Implementation Plan

## 1. Overview (概述与解决的问题)

QuarkMeta 架构规范明确规定：应用运行于**纯磁盘目录直连模式**，取消任何镜像数据库与托管库。
在数据持久化规范中：
- 盘符根节点（如 `C:\`）的元数据持久化在 `global.db` 的 `drive_metadata` 表中；
- 所有普通物理文件与文件夹的元数据（如备注 Note、链接 URL、评级 Rating、颜色 Color、标签 Tags 等）**唯一且直接持久化写入物理资产所在目录下的离散隐藏配置文件 `.QuarkMeta.json` 中**。

在之前版本的实现中，`MetadataManager::persistAsync` 遗留了向 SQLite `global.db` 的 `metadata` 表写入的历史代码（即镜像数据库残余），而未调用 `QuarkMetaJson::updateItemMeta` 写入离散 `.QuarkMeta.json` 文件；同时 `ensureActivated` 在激活节点时未加载 `.QuarkMeta.json`。

本实施方案将彻底清退 `persistAsync` 中的 SQLite 历史冗余写入逻辑，建立单轨、高效、纯洁的离散 `.QuarkMeta.json` 持久化与加载机制：
1. **彻底物理擦除写入端的历史双写/镜像库冗余**：`persistAsync` 中普通文件与文件夹直接调用 `QuarkMetaJson::updateItemMeta` 写入物理目录 `.QuarkMeta.json`，不再往 `global.db` 写入多余元数据表。
2. **激活端离散恢复**：在 `ensureActivated` 首次激活项目节点时，优先读取该项目对应目录 `.QuarkMeta.json` 里的属性填入内存 Shard，完成双向闭环。

---

## 2. Modified Files List (影响文件清单)

1. `src/meta/MetadataManager.cpp`

---

## 3. Detailed Line-by-Line Changes (精准替换块)

### 3.1 `src/meta/MetadataManager.cpp`

**修改点 1：在 `ensureActivated` 激活流程中，优先从所在目录的 `.QuarkMeta.json` 加载还原属性**

```
<<<<<<< SEARCH
        // 共享元数据逻辑 (FID 关联)
        if (!rm.folderId.empty() && m_folderIdToPath.count(rm.folderId)) {
=======
        // 优先从磁盘所在目录的 .QuarkMeta.json 中恢复离散元数据
        QFileInfo qinfo(QString::fromStdWString(nPath));
        if (!qinfo.isRoot()) {
            std::wstring folderPath = qinfo.absolutePath().toStdWString();
            std::wstring fileName = qinfo.fileName().toStdWString();
            auto itemsMap = QuarkMetaJson::readFolderMeta(folderPath);
            auto it = itemsMap.find(fileName);
            if (it != itemsMap.end()) {
                const auto& itemMeta = it->second;
                rm.rating = itemMeta.rating;
                rm.manualColor = itemMeta.color;
                rm.autoColor = itemMeta.autoColor;
                rm.tags.clear();
                for (const auto& tag : itemMeta.tags) {
                    rm.tags.append(QString::fromStdWString(tag));
                }
                rm.pinned = itemMeta.pinned;
                rm.note = itemMeta.note;
                rm.url = itemMeta.url;
                rm.encrypted = itemMeta.encrypted;
                if (!itemMeta.folderId.empty()) rm.folderId = itemMeta.folderId;
                rm.width = itemMeta.width;
                rm.height = itemMeta.height;
                rm.palettes = itemMeta.palettes;
            }
        }

        // 共享元数据逻辑 (FID 关联)
        if (!rm.folderId.empty() && m_folderIdToPath.count(rm.folderId)) {
>>>>>>> REPLACE
```

**修改点 2：在 `persistAsync` 中彻底清退历史镜像库 SQLite 写入，纯粹收口至 `.QuarkMeta.json`**

```
<<<<<<< SEARCH
    sqlite3* memDb = DatabaseManager::instance().getGlobalDb();
    if (!memDb) {
        return;
    }

    // 1. 内存库操作 (Memory Commit)
    bool isNew = true;
    {
        sqlite3_stmt* checkStmt;
        if (sqlite3_prepare_v2(memDb, "SELECT 1 FROM metadata WHERE folder_id = ?", -1, &checkStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(checkStmt, 1, rMeta.folderId.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(checkStmt) == SQLITE_ROW) isNew = false;
            sqlite3_finalize(checkStmt);
        }
    }

    if (isNew && !authorized) {

    }

    sqlite3_stmt* memStmt;
    if (sqlite3_prepare_v2(memDb, kSqlInsertMeta, -1, &memStmt, nullptr) == SQLITE_OK) {
        bindMetaHelper(memStmt, nPath, rMeta);
        if (sqlite3_step(memStmt) == SQLITE_DONE) {
            {

                size_t idx = getShardIndex(nPath);
                std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
                m_shards[idx].items[nPath] = rMeta;
            }
        }
        sqlite3_finalize(memStmt);
    }

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}
=======
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        // 盘符根节点（如 C:\）：写入 global.db 的 drive_metadata 表
        DriveMetaRec rec;
        rec.drivePath = nPath;
        rec.rating = rMeta.rating;
        rec.color = rMeta.manualColor;
        rec.pinned = rMeta.pinned;
        rec.note = rMeta.note;
        rec.url = rMeta.url;
        DriveMetaDao::saveDriveMeta(rec);
    } else {
        // 普通物理文件与文件夹：纯粹且原子化地直接落盘写入所在物理目录下的 .QuarkMeta.json
        QuarkMetaJson::updateItemMeta(nPath, [&rMeta](ItemMeta& item) {
            item.type = rMeta.isFolder ? L"folder" : L"file";
            item.rating = rMeta.rating;
            item.color = rMeta.manualColor;
            item.autoColor = rMeta.autoColor;

            std::vector<std::wstring> wTags;
            for (const QString& t : rMeta.tags) {
                wTags.push_back(t.toStdWString());
            }
            item.tags = wTags;

            item.pinned = rMeta.pinned;
            item.note = rMeta.note;
            item.url = rMeta.url;
            item.encrypted = rMeta.encrypted;
            if (!rMeta.folderId.empty()) item.folderId = rMeta.folderId;
            item.ingestionStatus = rMeta.ingestionStatus;
            item.size = rMeta.fileSize;
            item.creationTime = rMeta.ctime;
            item.modificationTime = rMeta.mtime;
            item.accessTime = rMeta.atime;
            item.addedAt = rMeta.added_at;
            item.width = rMeta.width;
            item.height = rMeta.height;
            item.palettes = rMeta.palettes;
        });
    }

    // 内存 Shard 同步更新
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath] = rMeta;
    }

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps (编译命令与验证方法)

### 4.1 构建与编译验证
在项目根目录下使用 CMake 进行配置与构建：
```bash
cmake -B build -S .
cmake --build build --config Release
```

### 4.2 功能验证
1. 启动应用，在内容视图选中任意物理文件。
2. 在右侧元数据面板（MetaPanel）为该文件修改备注（Note）、设置评级（Rating）、颜色标记（Color）以及编辑关联标签（Tags）。
3. 打开对应物理文件所在的磁盘文件夹，检查隐藏的 `.QuarkMeta.json` 文件。
4. 验证：
   - 物理文件所在目录下的 `.QuarkMeta.json` 包含该文件的属性，修改被即时原子化保存。
   - `global.db` 数据库中不再产生冗余的 `metadata` 数据，架构完全恢复单轨纯直连模式。
