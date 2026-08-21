# Discrete `.QuarkMeta.json` Metadata Synchronization & Persistence Implementation Plan

## 1. Overview (概述与解决的问题)

QuarkMeta 架构规范明确规定：应用运行于**纯磁盘目录直连模式**，彻底取消任何托管库与镜像数据库。
在数据持久化规范中：
- 盘符根节点（如 `C:\`）的元数据持久化在 `global.db` 的 `drive_metadata` 表中；
- 所有普通物理文件与文件夹的元数据（如备注 Note、链接 URL、标签 Tags、评级 Rating、颜色 Color、置顶 Pinned 等）**直接且即时地调用 `QuarkMetaJson::updateItemMeta` 原子化落盘写入物理资产所在目录下的离散隐藏配置文件 `.QuarkMeta.json` 中**。

在之前版本的实现中：
1. `MetadataManager` 中的 `setNote`、`setURL`、`setTags`、`setRating`、`setColor`、`setPinned` 等属性方法均异步提交到 `DatabaseManager` 的队列去调用 `persistAsync`，导致严重的落盘滞后与跨线程开销，且并未直接同步更新对应目录下的 `.QuarkMeta.json`；
2. `ensureActivated` 在首次激活节点时未加载 `.QuarkMeta.json` 离散属性至内存 Shard 中。

本实施方案采用**零延迟直接原子落盘架构**：
1. **0 延迟物理落盘**：在 `MetadataManager` 的各个属性 setter 方法中，直接原子化调用 `QuarkMetaJson::updateItemMeta` 写入对应物理目录下的 `.QuarkMeta.json` 文件，实现即时落盘与无死锁高并发。
2. **激活端双向闭环**：在 `ensureActivated` 激活项目节点时，优先读取该项目对应目录 `.QuarkMeta.json` 里的离散属性填入 Shard 内存中，实现全生命周期无缝同步。

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

**修改点 2：修改 `setRating` 方法（星标评级直连物理落盘）**

```
<<<<<<< SEARCH
void MetadataManager::setRating(const std::wstring& path, int rating, bool notify) {
    std::wstring nPath = normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.rating = rating;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> lock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].rating = rating;
    }
    if (notify) {
        notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
    }
    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
}
=======
void MetadataManager::setRating(const std::wstring& path, int rating, bool notify) {
    std::wstring nPath = normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.rating = rating;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> lock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].rating = rating;
    }

    // 纯磁盘模式：直接原子化写入所在物理目录的 .QuarkMeta.json
    QuarkMetaJson::updateItemMeta(nPath, [rating](ItemMeta& item) {
        item.rating = rating;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}
>>>>>>> REPLACE
```

**修改点 3：修改 `setColor` 方法（颜色标记直连物理落盘）**

```
<<<<<<< SEARCH
void MetadataManager::setColor(const std::wstring& path, const std::wstring& color, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    std::wstring normColor = UiHelper::normalizeColorHex(QString::fromStdWString(color)).toStdWString();
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.color = normColor;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);

    bool changed = false;
    bool isFolder = false;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        size_t idx = getShardIndex(nPath);
        {
            std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            auto it = m_shards[idx].items.find(nPath);
            if (it != m_shards[idx].items.end()) {
                isFolder = it->second.isFolder;
                if (it->second.manualColor != normColor) {
                    it->second.manualColor = normColor;
                    changed = true;
                }
            } else {
                m_shards[idx].items[nPath].manualColor = normColor;
                changed = true;
            }
        }
    }

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));

    if (changed) {
        DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
            persistAsync(nPath);
        });
    }
}
=======
void MetadataManager::setColor(const std::wstring& path, const std::wstring& color, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    std::wstring normColor = UiHelper::normalizeColorHex(QString::fromStdWString(color)).toStdWString();
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.color = normColor;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);

    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].manualColor = normColor;
    }

    // 纯磁盘模式：直接原子化写入所在物理目录的 .QuarkMeta.json
    QuarkMetaJson::updateItemMeta(nPath, [normColor](ItemMeta& item) {
        item.color = normColor;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}
>>>>>>> REPLACE
```

**修改点 4：修改 `setPinned` 方法（置顶保存直连物理落盘）**

```
<<<<<<< SEARCH
void MetadataManager::setPinned(const std::wstring& path, bool pinned, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.pinned = pinned;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].pinned = pinned;
    }
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));

    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
}
=======
void MetadataManager::setPinned(const std::wstring& path, bool pinned, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.pinned = pinned;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].pinned = pinned;
    }

    // 纯磁盘模式：直接原子化写入所在物理目录的 .QuarkMeta.json
    QuarkMetaJson::updateItemMeta(nPath, [pinned](ItemMeta& item) {
        item.pinned = pinned;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}
>>>>>>> REPLACE
```

**修改点 5：修改 `setTags` 方法（标签编辑直连物理落盘）**

```
<<<<<<< SEARCH
void MetadataManager::setTags(const std::wstring& path, const QStringList& tags, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);

    bool oldEmpty = false;
    QStringList oldTags;
    bool isFolder = false;

    {
        size_t idx = getShardIndex(nPath);
        {
            std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            auto it = m_shards[idx].items.find(nPath);
            if (it != m_shards[idx].items.end()) {
                oldEmpty = it->second.tags.isEmpty();
                oldTags = it->second.tags;
                isFolder = it->second.isFolder;
            }
        }
    }

    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].tags = tags;
    }

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));

    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
}
=======
void MetadataManager::setTags(const std::wstring& path, const QStringList& tags, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);

    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].tags = tags;
    }

    // 纯磁盘模式：转换标签并直接原子化写入所在物理目录的 .QuarkMeta.json
    std::vector<std::wstring> wTags;
    for (const QString& t : tags) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty()) {
            wTags.push_back(trimmed.toStdWString());
        }
    }
    QuarkMetaJson::updateItemMeta(nPath, [wTags](ItemMeta& item) {
        item.tags = wTags;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}
>>>>>>> REPLACE
```

**修改点 6：修改 `setNote` 方法（备注保存直连物理落盘）**

```
<<<<<<< SEARCH
void MetadataManager::setNote(const std::wstring& path, const std::wstring& note, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.note = note;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].note = note;
    }
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));

    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
}
=======
void MetadataManager::setNote(const std::wstring& path, const std::wstring& note, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.note = note;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].note = note;
    }

    // 纯磁盘模式：直接原子化写入所在物理目录的 .QuarkMeta.json
    QuarkMetaJson::updateItemMeta(nPath, [note](ItemMeta& item) {
        item.note = note;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}
>>>>>>> REPLACE
```

**修改点 7：修改 `setURL` 方法（链接保存直连物理落盘）**

```
<<<<<<< SEARCH
void MetadataManager::setURL(const std::wstring& path, const std::wstring& url, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.url = url;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].url = url;
    }
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));

    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
}
=======
void MetadataManager::setURL(const std::wstring& path, const std::wstring& url, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = DriveMetaDao::getDriveMeta(nPath);
        rec.url = url;
        DriveMetaDao::saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].url = url;
    }

    // 纯磁盘模式：直接原子化写入所在物理目录的 .QuarkMeta.json
    QuarkMetaJson::updateItemMeta(nPath, [url](ItemMeta& item) {
        item.url = url;
    });

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
2. 在右侧元数据面板（MetaPanel）修改备注（Note）、设置评级（Rating）、颜色标记（Color）、置顶（Pinned）、编辑关联标签（Tags）或链接（URL）。
3. 观察物理目录下的隐藏文件 `.QuarkMeta.json`，验证修改在 **0 毫秒内同步原子化写入**。
4. 切换目录或重启应用，确认填写的元数据 100% 完美呈现，无卡顿无延迟。
