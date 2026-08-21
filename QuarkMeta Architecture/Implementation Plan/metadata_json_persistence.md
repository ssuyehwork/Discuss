# Discrete `.QuarkMeta.json` Metadata Synchronization & Persistence Implementation Plan

## 1. Overview (概述与解决的问题)

QuarkMeta 架构规范要求在纯磁盘直连模式下，用户操作产生的属性元数据（如备注 Note、链接 URL、评级 Rating、颜色 Color、标签 Tags 等）必须直接持久化写入物理资产所在目录下的离散隐藏配置文件 **`.QuarkMeta.json`** 中。

然而在之前版本的实现中，`MetadataManager::persistAsync` 与 `ensureActivated` 存在离散元数据文件同步脱节的问题：
1. **写入端断层**：用户在元数据面板中编辑备注、链接、标签等数据时，`MetadataManager::persistAsync` 仅将元数据保存到了 SQLite 内存/数据库中，并没有调用 `QuarkMetaJson::updateItemMeta` 实时同步落盘写回 `.QuarkMeta.json` 文件。
2. **激活端缺漏**：在项目首次激活（`ensureActivated`）时，系统未实时读取磁盘对应物理目录下的 `.QuarkMeta.json` 文件，导致已有的离散 JSON 属性无法自动还原到 Shard 内存中。

本方案旨在修补此双向同步链路，确保修改属性时实时持久化写入 `.QuarkMeta.json`，并在项目激活时优先自动加载 `.QuarkMeta.json` 里的离散属性。

---

## 2. Modified Files List (影响文件清单)

1. `src/meta/MetadataManager.cpp`

---

## 3. Detailed Line-by-Line Changes (精准替换块)

### 3.1 `src/meta/MetadataManager.cpp`

**修改点 1：在 `ensureActivated` 激活流程中，自动从所在目录的 `.QuarkMeta.json` 恢复项目元数据**

```
<<<<<<< SEARCH
        // 共享元数据逻辑 (FID 关联)
        if (!rm.folderId.empty() && m_folderIdToPath.count(rm.folderId)) {
=======
        // 尝试从磁盘所在目录的 .QuarkMeta.json 中恢复离散元数据
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

**修改点 2：在 `persistAsync` 异步持久化流程中，直接同步写入所在目录的 `.QuarkMeta.json` 文件**

```
<<<<<<< SEARCH
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}
=======
    // 2. 离散 .QuarkMeta.json 文件持久化
    QFileInfo info(QString::fromStdWString(nPath));
    if (!info.isRoot()) {
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
3. 使用文件资源管理器或文本编辑器打开该物理文件所在目录下的 `.QuarkMeta.json` 隐藏文件。
4. 确认 `.QuarkMeta.json` 文件中包含对应文件名的键，且其 `note`、`rating`、`color`、`tags` 等字段均已实时、正确更新落盘。
