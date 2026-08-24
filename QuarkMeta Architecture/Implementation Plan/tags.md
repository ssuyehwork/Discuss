# Implementation Plan - Tags Display & Case-Insensitive Synchronization

## 1. Overview
### 1.1 问题描述
在 Windows 平台上，当用户为文件打上标签后，虽然标签已成功持久化写入 `.QuarkMeta.json`，但是在主界面选中该项目时，右侧元数据面板（`MetaPanel`）上未展示绑定的标签胶囊。

### 1.2 根本原因
1. **Windows 平台文件名大小写分裂**：
   - 提取文件尺寸/磁盘扫描时使用的是物理磁盘真实文件名（如大写 `HH 花卉_74368.ai`），在 JSON 中建立了大写条目；
   - 打标签时 `MetadataManager::setTags` 将路径全量转为小写（`hh 花卉_74368.ai`），并在 JSON 中写入了小写条目；
   - 底层 `QuarkMetaJson` 内部的 `std::map<std::wstring, ItemMeta>` 严格区分大小写，导致界面用大写查找时命中了 tags 为空的幽灵大写条目，带有标签的小写条目被忽略。
2. **标签变更信号断流未更新 Model**：
   - 标签选择器浮层关闭时触发 `MetaPanel::tagsChanged` 信号，但 `MainWindow` 未建立信号监听，导致内存中的 `DiskItemModel` 未同步更新 `rec.tags`，下次选中时再次被空标签覆盖。

### 1.3 解决方案
1. 在 `QuarkMetaJson.h` 中为 `m_items` 引入大小写不敏感比较器 `CaseInsensitiveWStringLess`，将大写与小写文件名归一化到同一个条目；
2. 在 `QuarkMetaJson.cpp` 的 `load()` 中对同名（不同大小写）条目的有效元数据（tags、rating、color 等）进行自动合并；
3. 在 `MetadataManager.cpp` 中调用 `updateItemMeta` 时传入原始文件路径，保留真实大小写；
4. 在 `MainWindow.cpp` 中连接 `MetaPanel::tagsChanged` 信号，实时刷新内存模型。

---

## 2. Modified Files List
1. `src/meta/QuarkMetaJson.h`
2. `src/meta/QuarkMetaJson.cpp`
3. `src/meta/MetadataManager.cpp`
4. `src/ui/MainWindow.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/meta/QuarkMetaJson.h`
引入大小写不敏感比较器 `CaseInsensitiveWStringLess`，并将 `m_items` 改为 `ItemMap` 类型：

<<<<<<< SEARCH
class QuarkMetaJson {
public:
    /**
     * @brief 物理整体迁移/重命名文件夹缓存接口（历史兼容，在直接保存模式下，重命名会自动由操作系统物理转移子文件）
     */
    static bool migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath);
=======
struct CaseInsensitiveWStringLess {
    bool operator()(const std::wstring& a, const std::wstring& b) const {
#ifdef _WIN32
        return _wcsicmp(a.c_str(), b.c_str()) < 0;
#else
        return wcscasecmp(a.c_str(), b.c_str()) < 0;
#endif
    }
};

class QuarkMetaJson {
public:
    using ItemMap = std::map<std::wstring, ItemMeta, CaseInsensitiveWStringLess>;

    /**
     * @brief 物理整体迁移/重命名文件夹缓存接口（历史兼容，在直接保存模式下，重命名会自动由操作系统物理转移子文件）
     */
    static bool migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath);
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 数据访问接口
    FolderMeta& folder() { return m_folder; }
    const FolderMeta& folder() const { return m_folder; }

    std::map<std::wstring, ItemMeta>& items() { return m_items; }
    const std::map<std::wstring, ItemMeta>& items() const { return m_items; }
=======
    // 数据访问接口
    FolderMeta& folder() { return m_folder; }
    const FolderMeta& folder() const { return m_folder; }

    ItemMap& items() { return m_items; }
    const ItemMap& items() const { return m_items; }
>>>>>>> REPLACE

<<<<<<< SEARCH
    FolderMeta m_folder;
    std::map<std::wstring, ItemMeta> m_items;
=======
    FolderMeta m_folder;
    ItemMap m_items;
>>>>>>> REPLACE

---

### 3.2 `src/meta/QuarkMetaJson.cpp`
在 `load()` 中对大小写同名条目的有效元数据执行合并：

<<<<<<< SEARCH
    m_items.clear();
    if (root.contains("items") && root.value("items").isObject()) {
        QJsonObject itemsObj = root.value("items").toObject();
        for (auto it = itemsObj.begin(); it != itemsObj.end(); ++it) {
            m_items[toStdWString(it.key())] = entryToItem(it.value().toObject());
        }
    }
    return true;
=======
    m_items.clear();
    if (root.contains("items") && root.value("items").isObject()) {
        QJsonObject itemsObj = root.value("items").toObject();
        for (auto it = itemsObj.begin(); it != itemsObj.end(); ++it) {
            std::wstring key = toStdWString(it.key());
            ItemMeta item = entryToItem(it.value().toObject());
            auto existingIt = m_items.find(key);
            if (existingIt != m_items.end()) {
                if (item.rating > 0) existingIt->second.rating = item.rating;
                if (!item.color.empty()) existingIt->second.color = item.color;
                if (!item.autoColor.empty()) existingIt->second.autoColor = item.autoColor;
                if (!item.tags.empty()) existingIt->second.tags = item.tags;
                if (item.pinned) existingIt->second.pinned = item.pinned;
                if (!item.note.empty()) existingIt->second.note = item.note;
                if (!item.url.empty()) existingIt->second.url = item.url;
                if (item.encrypted) existingIt->second.encrypted = item.encrypted;
                if (item.width > 0) existingIt->second.width = item.width;
                if (item.height > 0) existingIt->second.height = item.height;
                if (item.thumbStatus > 0) existingIt->second.thumbStatus = item.thumbStatus;
                if (item.addedAt > 0) existingIt->second.addedAt = item.addedAt;
                if (!item.palettes.empty()) existingIt->second.palettes = item.palettes;
            } else {
                m_items[key] = item;
            }
        }
    }
    return true;
>>>>>>> REPLACE

---

### 3.3 `src/meta/MetadataManager.cpp`
保存元数据时传递原始物理文件路径：

<<<<<<< SEARCH
    QuarkMetaJson::updateItemMeta(nPath, [wTags](ItemMeta& item) {
        item.tags = wTags;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

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

    QuarkMetaJson::updateItemMeta(nPath, [note](ItemMeta& item) {
        item.note = note;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

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

    QuarkMetaJson::updateItemMeta(nPath, [url](ItemMeta& item) {
        item.url = url;
    });
=======
    QuarkMetaJson::updateItemMeta(path, [wTags](ItemMeta& item) {
        item.tags = wTags;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

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

    QuarkMetaJson::updateItemMeta(path, [note](ItemMeta& item) {
        item.note = note;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

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

    QuarkMetaJson::updateItemMeta(path, [url](ItemMeta& item) {
        item.url = url;
    });
>>>>>>> REPLACE

---

### 3.4 `src/ui/MainWindow.cpp`
连接 `tagsChanged` 信号：

<<<<<<< SEARCH
    // 删除标签管网 
    connect(m_metaPanel, &MetaPanel::tagRemoveRequested, this, [this](const QStringList& paths, const QString& removeTag) { 
        if (!paths.isEmpty() && !removeTag.isEmpty()) {
            AppCommand cmd;
            cmd.type = AppCommandType::RemoveTag;
            cmd.targetPaths = paths;
            cmd.params["tag"] = removeTag;
            CoreEngine::instance().executeCommand(cmd);
            for (const QString& p : paths) {
                m_contentPanel->updateItemMetadata(p);
            }
        }
    }); 

    // 2026-06-xx调色盘搜索联动：将颜色喂给筛选器，由筛选器驱动过滤
=======
    // 删除标签管网 
    connect(m_metaPanel, &MetaPanel::tagRemoveRequested, this, [this](const QStringList& paths, const QString& removeTag) { 
        if (!paths.isEmpty() && !removeTag.isEmpty()) {
            AppCommand cmd;
            cmd.type = AppCommandType::RemoveTag;
            cmd.targetPaths = paths;
            cmd.params["tag"] = removeTag;
            CoreEngine::instance().executeCommand(cmd);
            for (const QString& p : paths) {
                m_contentPanel->updateItemMetadata(p);
            }
        }
    }); 

    // 标签批量更新（TagSelectorOverlay 关闭时）：同步刷新 Model 内存镜像，防止 rec.tags 过期导致下次选中时被空值覆盖
    connect(m_metaPanel, &MetaPanel::tagsChanged, this, [this](const QStringList& paths, const QStringList&) {
        for (const QString& p : paths) {
            m_contentPanel->updateItemMetadata(p);
        }
    });

    // 2026-06-xx调色盘搜索联动：将颜色喂给筛选器，由筛选器驱动过滤
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. 编译命令：
   ```powershell
   cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release -j 8
   ```
2. 验证方法：
   - 运行程序，打开包含已绑定标签文件的目录（如 `HH 花卉_74368.ai`）；
   - 鼠标单击选中该文件；
   - 检查右侧元数据面板，所有绑定的标签（如 `"测试"`、`"333"`、`"222"`、`"111"`、`"444"`）能够完整显示为胶囊控件，且流式布局高度自适应正常；
   - 打开标签浮层增删标签后关闭，再次选中文件或切换其他文件，标签保持实时同步与持久化。
