# Cancel Import & Memory Mode Zombie Code Purge Implementation Plan (cancel_import_purge.md)

## Overview
This implementation plan defines the complete eradication of the legacy `ActionCancelImport` ("取消导入并清除数据") context menu option, its associated `AppCommandType::RemoveBatchSync` command, `MetadataManager::removeMetadataBatchSync` dead SQL execution, obsolete `DELETE FROM metadata` queries on non-root items, and dead `isCapsule` parameters in `BatchRenameCommand`.

In QuarkMeta's standalone pure disk mode:
1. `ActionCancelImport` is a legacy memory-mode action that performed `DELETE FROM metadata WHERE folder_id = ?` on SQLite databases, which is now completely dead because item metadata resides exclusively in per-directory `.QuarkMeta.json` files.
2. `ManagedRole` was repurposed to check for user operations (ratings/tags/notes), causing `ActionCancelImport` to erroneously appear whenever items had metadata.
3. `BatchRenameCommand` retains unused `isCapsule` boolean flags and dead `if (isCapsule)` branches.

Deleting these zombie constructs purges non-functional code and aligns the codebase strictly with the pure disk architecture.

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/core/CoreEngine.h`
- `src/core/CoreEngine.cpp`
- `src/meta/MetadataManager.h`
- `src/meta/MetadataManager.cpp`
- `src/core/BasicCommands.h`
- `src/ui/BatchRenameDialog.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`
Remove `ActionCancelImport` from the `ContextAction` enum.

```
<<<<<<< SEARCH
        ActionCopyName,
        ActionCopyPath,
        ActionAddToCategory,
        ActionAddToFavorites,
        ActionRefresh,
        ActionReextractThumbnail,
        ActionCancelImport,
        ActionBatchCreate
    };
=======
        ActionCopyName,
        ActionCopyPath,
        ActionAddToCategory,
        ActionAddToFavorites,
        ActionRefresh,
        ActionReextractThumbnail,
        ActionBatchCreate
    };
>>>>>>> REPLACE
```

### 2. `src/ui/ContentPanel.cpp`
Remove right-click context menu construction for `ActionCancelImport` and its switch case handling.

```
<<<<<<< SEARCH
        // 仅在选中普通文件时展示“重新提取缩略图”
        if (onItem && !isFolder) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#3498db"), 18), "重新提取缩略图")->setData(ActionReextractThumbnail);
        }


        // 2026-07-27 按照 Plan-107：仅对已在资源库中登记的文件夹，增加“取消导入并清除数据”菜单项
        if (currentIndex.data(TypeRole).toString() == "folder" && currentIndex.data(ManagedRole).toBool()) {
            menu.addAction(UiHelper::getIcon("close", QColor("#e81123"), 18), "取消导入并清除数据")->setData(ActionCancelImport);
        }

        if (!isFolder) {
=======
        // 仅在选中普通文件时展示“重新提取缩略图”
        if (onItem && !isFolder) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#3498db"), 18), "重新提取缩略图")->setData(ActionReextractThumbnail);
        }

        if (!isFolder) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        case ActionRename: view->edit(currentIndex); break;
        case ActionCopy: performCopy(false); break;
        case ActionCut: performCopy(true); break;
        case ActionPaste: performPaste(); break;
        case ActionCancelImport: {
            auto indexes = view->selectionModel()->selectedIndexes();
            QStringList targetPaths;
            for (const auto& idx : indexes) {
                if (idx.column() == 0) {
                    QString p = idx.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths << p;
                }
            }
            if (targetPaths.isEmpty() && !path.isEmpty()) targetPaths << path;

            if (!targetPaths.isEmpty()) {
                std::vector<std::wstring> stdPaths;
                for (const QString& tp : targetPaths) {
                    stdPaths.push_back(tp.toStdWString());
                    // 物理清退内容面板缩略图与宽高比缓存
                    clearFolderCache(tp);
                }

                // 1. 中止并取消队列中以及正在提取的高级多媒体任务
                MediaExtractorPipeline::instance().cancelBatch(stdPaths);

                AppCommand cmd;
                cmd.type = AppCommandType::RemoveBatchSync;
                cmd.targetPaths = targetPaths;
                CoreEngine::instance().executeCommand(cmd);

                ToolTipOverlay::instance()->showText(QCursor::pos(), "已彻底擦除相关元数据", 2000, QColor("#e81123"));
                refreshAll();
            }
            break;
        }
        case ActionBatchCreate: {
=======
        case ActionRename: view->edit(currentIndex); break;
        case ActionCopy: performCopy(false); break;
        case ActionCut: performCopy(true); break;
        case ActionPaste: performPaste(); break;
        case ActionBatchCreate: {
>>>>>>> REPLACE
```

### 3. `src/core/CoreEngine.h`
Remove `RemoveBatchSync` enum value from `AppCommandType`.

```
<<<<<<< SEARCH
    DeletePermanently, // 物理粉碎擦除
    RemoveBatchSync,   // 批量移除元数据
    RecordAccess       // 访问时间记录
=======
    DeletePermanently, // 物理粉碎擦除
    RecordAccess       // 访问时间记录
>>>>>>> REPLACE
```

### 4. `src/core/CoreEngine.cpp`
Remove command handler for `AppCommandType::RemoveBatchSync`.

```
<<<<<<< SEARCH
    case AppCommandType::DeletePermanently: {
        for (const QString& path : cmd.targetPaths) {
            MetadataManager::instance().deletePermanently(path.toStdWString());
        }
        AppEvent ev;
        ev.type = AppEventType::ItemsDeleted;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RemoveBatchSync: {
        MetadataManager::instance().removeMetadataBatchSync(cmd.targetPaths);
        AppEvent ev;
        ev.type = AppEventType::MetadataUpdated;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RecordAccess: {
=======
    case AppCommandType::DeletePermanently: {
        for (const QString& path : cmd.targetPaths) {
            MetadataManager::instance().deletePermanently(path.toStdWString());
        }
        AppEvent ev;
        ev.type = AppEventType::ItemsDeleted;
        ev.paths = cmd.targetPaths;
        CentralEventHub::instance().publishEvent(ev);
        break;
    }
    case AppCommandType::RecordAccess: {
>>>>>>> REPLACE
```

### 5. `src/meta/MetadataManager.h`
Remove declaration of `removeMetadataBatchSync`.

```
<<<<<<< SEARCH
    void deletePermanently(const std::wstring& filePath);
    void removeMetadataBatchSync(const QStringList& paths);

    std::wstring getVolumeSerialNumberForPath(const std::wstring& path);
=======
    void deletePermanently(const std::wstring& filePath);

    std::wstring getVolumeSerialNumberForPath(const std::wstring& path);
>>>>>>> REPLACE
```

### 6. `src/meta/MetadataManager.cpp`
Remove dead `DELETE FROM metadata WHERE folder_id = ?` query block in `deletePermanently` and remove the `removeMetadataBatchSync` function definition entirely.

```
<<<<<<< SEARCH
    // 2026-06-xx 物理级根除：基于 File ID (FRN) 批量清理
    if (db && !fids.empty()) {
        const char* sql = "DELETE FROM metadata WHERE folder_id = ?";
        // [Plan-131 方案 A] 直连模式，取消冗余异步任务
        SqlTransaction trans(db);
        sqlite3_stmt* memStmt;
        if (sqlite3_prepare_v2(db, sql, -1, &memStmt, nullptr) == SQLITE_OK) {
            for (const auto& fid : fids) {
                sqlite3_bind_text(memStmt, 1, fid.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(memStmt);
                sqlite3_reset(memStmt);
            }
            sqlite3_finalize(memStmt);
        }
        trans.commit();
    }

    notifyUI(RefreshLevel::FullRebuild);
}

void MetadataManager::removeMetadataBatchSync(const QStringList& paths) {
    if (paths.isEmpty()) return;

    // 1. 按数据库分组以支持大事务
    std::map<sqlite3*, std::vector<std::string>> groupedFids;
    std::vector<std::string> allFids;
    int totalDelta = 0;

    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        for (const QString& qp : paths) {
            std::wstring nPath = normalizePath(qp.toStdWString());

            std::vector<std::wstring> toRemove;
            forEachCachedItem([&](const std::wstring& p, const RuntimeMeta&) {
                if (p == nPath || p.find(nPath + L"\\") == 0 || p.find(nPath + L"/") == 0) {
                    toRemove.push_back(p);
                }
            });

            for (const auto& p : toRemove) {
                size_t idx = getShardIndex(p);
                RuntimeMeta meta;
                bool found = false;
                {
                    std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
                    auto it = m_shards[idx].items.find(p);
                    if (it != m_shards[idx].items.end()) {
                        meta = it->second;
                        m_shards[idx].items.erase(it);
                        found = true;
                    }
                }
                if (!found) continue;

                if (!meta.isTrash) {
                    totalDelta--;
                }

                std::string fid = meta.folderId;
                if (!fid.empty()) {
                    allFids.push_back(fid);
                    m_folderIdToPath.erase(fid);

                    std::wstring volSerial = getVolumeSerialNumber(p);
                    QString letter = (p.length() >= 2 && p[1] == L':') ? QString::fromWCharArray(&p[0], 1) : "";
                    sqlite3* db = DatabaseManager::instance().getGlobalDb();
                    if (db) groupedFids[db].push_back(fid);

                    std::wstring name, ext;
                    parsePathComponents(p, meta.isFolder, name, ext);
                    if (!name.empty()) {
                        if (meta.isFolder) {
                            auto& v = m_subFolderNameToFolderIds[name];
                            v.erase(std::remove(v.begin(), v.end(), fid), v.end());
                            if (v.empty()) m_subFolderNameToFolderIds.erase(name);
                        } else {
                            auto& v = m_assetNameToFolderIds[name];
                            v.erase(std::remove(v.begin(), v.end(), fid), v.end());
                            if (v.empty()) m_assetNameToFolderIds.erase(name);
                            if (!ext.empty()) {
                                auto& ve = m_extensionToFolderIds[ext];
                                ve.erase(std::remove(ve.begin(), ve.end(), fid), ve.end());
                                if (ve.empty()) m_extensionToFolderIds.erase(ext);
                            }
                        }
                    }
                    m_parentToChildren.erase(p);
                    m_folderProgressCache.erase(p);
                }
            }
        }
    }

    // 2. 数据库执行
    const char* sql = "DELETE FROM metadata WHERE folder_id = ?";
    for (auto& entry : groupedFids) {
        sqlite3* db = entry.first;
        const auto& fids = entry.second;

        // [Plan-131 方案 A] 直连模式，废除冗余异步分发
        SqlTransaction trans(db);
        sqlite3_stmt* memStmt;
        if (sqlite3_prepare_v2(db, sql, -1, &memStmt, nullptr) == SQLITE_OK) {
            for (const auto& fid : fids) {
                sqlite3_bind_text(memStmt, 1, fid.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(memStmt);
                sqlite3_reset(memStmt);
            }
            sqlite3_finalize(memStmt);
        }
        trans.commit();
    }

    notifyUI(RefreshLevel::FullRebuild);
}
=======
    notifyUI(RefreshLevel::FullRebuild);
}
>>>>>>> REPLACE
```

### 7. `src/core/BasicCommands.h`
Purge dead `isCapsule` boolean flag and branches from `BatchRenameCommand`.

```
<<<<<<< SEARCH
class BatchRenameCommand : public ActionCommand {
public:
    BatchRenameCommand(bool isCapsule,
                       DiskOperationMode mode,
                       const QStringList& oldPaths,
                       const QStringList& newPaths)
        : m_isCapsule(isCapsule), m_mode(mode), m_oldPaths(oldPaths), m_newPaths(newPaths) {}

    void execute() override {
        if (m_oldPaths.size() != m_newPaths.size() || m_oldPaths.isEmpty()) return;

        QStringList oldPaths = m_oldPaths;
        QStringList newPaths = m_newPaths;
        DiskOperationMode mode = m_mode;
        bool isCapsule = m_isCapsule;

        (void)QtConcurrent::run([oldPaths, newPaths, isCapsule, mode]() {
            for (int i = 0; i < oldPaths.size(); ++i) {
                const QString& src = oldPaths[i];
                const QString& dst = newPaths[i];
                if (src == dst) continue;

                if (isCapsule) {
                    QFile::rename(src, dst);
                } else {
                    DiskIoContext ctx;
                    ctx.sources = {src};
                    ctx.destination = QFileInfo(dst).absolutePath();
                    ctx.isMove = (mode == DiskOperationMode::Move);
                    DiskIoService::instance().executeAsync(ctx, nullptr);
                }
            }
        });
    }

    void undo() override {
        if (m_oldPaths.size() != m_newPaths.size() || m_oldPaths.isEmpty()) return;

        QStringList oldPaths = m_oldPaths;
        QStringList newPaths = m_newPaths;
        DiskOperationMode mode = m_mode;
        bool isCapsule = m_isCapsule;

        (void)QtConcurrent::run([oldPaths, newPaths, isCapsule, mode]() {
            for (int i = 0; i < oldPaths.size(); ++i) {
                const QString& src = newPaths[i];
                const QString& dst = oldPaths[i];
                if (src == dst) continue;

                if (isCapsule) {
                    QFile::rename(src, dst);
                } else {
                    DiskIoContext ctx;
                    ctx.sources = {src};
                    ctx.destination = QFileInfo(dst).absolutePath();
                    ctx.isMove = (mode == DiskOperationMode::Move);
                    DiskIoService::instance().executeAsync(ctx, nullptr);
                }
            }
        });
    }

    void redo() override {
        execute();
    }

    QString title() const override {
        if (m_isCapsule) return "批量重命名 (胶囊)";
        return "批量重命名";
    }

private:
    bool m_isCapsule;
    DiskOperationMode m_mode;
    QStringList m_oldPaths;
    QStringList m_newPaths;
};
=======
class BatchRenameCommand : public ActionCommand {
public:
    BatchRenameCommand(DiskOperationMode mode,
                       const QStringList& oldPaths,
                       const QStringList& newPaths)
        : m_mode(mode), m_oldPaths(oldPaths), m_newPaths(newPaths) {}

    void execute() override {
        if (m_oldPaths.size() != m_newPaths.size() || m_oldPaths.isEmpty()) return;

        QStringList oldPaths = m_oldPaths;
        QStringList newPaths = m_newPaths;
        DiskOperationMode mode = m_mode;

        (void)QtConcurrent::run([oldPaths, newPaths, mode]() {
            for (int i = 0; i < oldPaths.size(); ++i) {
                const QString& src = oldPaths[i];
                const QString& dst = newPaths[i];
                if (src == dst) continue;

                DiskIoContext ctx;
                ctx.sources = {src};
                ctx.destination = QFileInfo(dst).absolutePath();
                ctx.isMove = (mode == DiskOperationMode::Move);
                DiskIoService::instance().executeAsync(ctx, nullptr);
            }
        });
    }

    void undo() override {
        if (m_oldPaths.size() != m_newPaths.size() || m_oldPaths.isEmpty()) return;

        QStringList oldPaths = m_oldPaths;
        QStringList newPaths = m_newPaths;
        DiskOperationMode mode = m_mode;

        (void)QtConcurrent::run([oldPaths, newPaths, mode]() {
            for (int i = 0; i < oldPaths.size(); ++i) {
                const QString& src = newPaths[i];
                const QString& dst = oldPaths[i];
                if (src == dst) continue;

                DiskIoContext ctx;
                ctx.sources = {src};
                ctx.destination = QFileInfo(dst).absolutePath();
                ctx.isMove = (mode == DiskOperationMode::Move);
                DiskIoService::instance().executeAsync(ctx, nullptr);
            }
        });
    }

    void redo() override {
        execute();
    }

    QString title() const override {
        return "批量重命名";
    }

private:
    DiskOperationMode m_mode;
    QStringList m_oldPaths;
    QStringList m_newPaths;
};
>>>>>>> REPLACE
```

### 8. `src/ui/BatchRenameDialog.cpp`
Update `BatchRenameCommand` constructor call site.

```
<<<<<<< SEARCH
        if (mode == DiskOperationMode::Rename) {
            DiskBatchRenameService::executeBatchRenameAsync(oldPathsSnap, newPathsSnap, onCompletedCallback);
        } else {
            UndoManager::instance().pushCommand(std::make_unique<BatchRenameCommand>(isCapsule, mode, oldPathsSnap, newPathsSnap));
        }
=======
        if (mode == DiskOperationMode::Rename) {
            DiskBatchRenameService::executeBatchRenameAsync(oldPathsSnap, newPathsSnap, onCompletedCallback);
        } else {
            UndoManager::instance().pushCommand(std::make_unique<BatchRenameCommand>(mode, oldPathsSnap, newPathsSnap));
        }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile project:
   ```bash
   cmake --build build --config Debug
   ```
2. Verify that `ActionCancelImport` no longer exists in context menus.
3. Verify that `BatchRenameCommand` builds without warnings or errors.
