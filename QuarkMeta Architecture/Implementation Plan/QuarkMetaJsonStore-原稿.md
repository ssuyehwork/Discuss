# QuarkMeta 元数据持久化与存储底座治理实施方案

## 1. 目标与范围
- 消除高频写盘震荡：在 `QuarkMetaJsonStore` 中引入“脏目录缓冲合并（Dirty Buffer Merge）”机制，单目录多项连续修改在内存中自动聚合，将 100 次重复写盘降为 1 次原子落盘。
- 消除门面工具代码重复：移除 `MetadataManager` 中与 `VolumePathResolver` 完全重复的 `getVolumeSerialNumber` 代码，统一收敛至 `VolumePathResolver`。
- 巩固三层存储架构：明确划分 `MetadataManager`（调度门面）、`MetaMemoryCache`（0ms 内存读取）、`QuarkMetaJsonStore`（原子磁盘存储）与 `MetaDbRepository`（SQLite 存储）的职责边界。

---

## 2. 核心模块独立实现与改造

### 2.1 `src/meta/QuarkMetaJsonStore.h` (脏缓冲合并引擎)
```cpp
#pragma once

#include "QuarkMetaJson.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <unordered_map>
#include <mutex>
#include <string>
#include <functional>

namespace QuarkMeta {

class QuarkMetaJsonStore : public QObject {
    Q_OBJECT

public:
    static QuarkMetaJsonStore& instance();

    /**
     * @brief 原子修改指定路径对应文件的 ItemMeta 记录 (自动缓冲合并落盘)
     */
    void updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater);

    /**
     * @brief 物理迁移文件夹缓存文件 (.QuarkMeta.json)
     */
    bool migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath);

    /**
     * @brief 单文件重命名时同步修改条目键名
     */
    bool renameItem(const QString& folderPath, const QString& oldName, const QString& newName);

    /**
     * @brief 强制立即将所有未落盘的脏目录 JSON 刷入物理磁盘
     */
    void flushAllDirtyBuffers();

private slots:
    void onFlushTimeout();

private:
    explicit QuarkMetaJsonStore(QObject* parent = nullptr);
    ~QuarkMetaJsonStore() override;
    QuarkMetaJsonStore(const QuarkMetaJsonStore&) = delete;
    QuarkMetaJsonStore& operator=(const QuarkMetaJsonStore&) = delete;

    std::mutex m_storeMutex;
    // 目录路径 -> 内存中待合并的 JSON 对象
    std::unordered_map<std::wstring, QuarkMetaJson> m_dirtyBufferMap;
    QTimer* m_flushTimer = nullptr;
    static constexpr int kFlushDebounceMs = 50; // 50ms 内同目录修改自动合流为 1 次原子写盘
};

} // namespace QuarkMeta
```

### 2.2 `src/meta/QuarkMetaJsonStore.cpp`
```cpp
#include "QuarkMetaJsonStore.h"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

namespace QuarkMeta {

QuarkMetaJsonStore& QuarkMetaJsonStore::instance() {
    static QuarkMetaJsonStore s_instance;
    return s_instance;
}

QuarkMetaJsonStore::QuarkMetaJsonStore(QObject* parent)
    : QObject(parent) {
    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(kFlushDebounceMs);
    connect(m_flushTimer, &QTimer::timeout, this, &QuarkMetaJsonStore::onFlushTimeout);

    // 应用程序退出时强制刷盘
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QuarkMetaJsonStore::flushAllDirtyBuffers);
}

QuarkMetaJsonStore::~QuarkMetaJsonStore() {
    flushAllDirtyBuffers();
}

void QuarkMetaJsonStore::updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater) {
    if (!updater) return;

    QFileInfo info(QString::fromStdWString(filePath));
    std::wstring folderPath = info.absolutePath().toStdWString();
    std::wstring fileName = info.fileName().toStdWString();

    std::lock_guard<std::mutex> lock(m_storeMutex);

    // 1. 如果该目录已经在脏缓冲中，直接在内存修改；否则载入内存
    auto it = m_dirtyBufferMap.find(folderPath);
    if (it == m_dirtyBufferMap.end()) {
        QuarkMetaJson json(folderPath);
        json.load();
        it = m_dirtyBufferMap.emplace(folderPath, std::move(json)).first;
    }

    ItemMeta& meta = it->second.items()[fileName];
    meta.type = info.isDir() ? L"folder" : L"file";
    updater(meta);

    // 2. 如果是文件夹，同步更新自身目录内部的 .QuarkMeta.json 镜像
    if (info.isDir()) {
        std::wstring selfDirPath = info.absoluteFilePath().toStdWString();
        auto selfIt = m_dirtyBufferMap.find(selfDirPath);
        if (selfIt == m_dirtyBufferMap.end()) {
            QuarkMetaJson selfJson(selfDirPath);
            selfJson.load();
            selfIt = m_dirtyBufferMap.emplace(selfDirPath, std::move(selfJson)).first;
        }

        FolderMeta& fMeta = selfIt->second.folder();
        ItemMeta dummyItem;
        dummyItem.rating = fMeta.rating;
        dummyItem.color = fMeta.color;
        dummyItem.pinned = fMeta.pinned;
        dummyItem.note = fMeta.note;
        dummyItem.url = fMeta.url;
        dummyItem.encrypted = fMeta.encrypted;
        dummyItem.folderId = fMeta.folderId;
        dummyItem.tags = fMeta.tags;
        dummyItem.palettes = fMeta.palettes;

        updater(dummyItem);

        fMeta.rating = dummyItem.rating;
        fMeta.color = dummyItem.color;
        fMeta.pinned = dummyItem.pinned;
        fMeta.note = dummyItem.note;
        fMeta.url = dummyItem.url;
        fMeta.encrypted = dummyItem.encrypted;
        fMeta.folderId = dummyItem.folderId;
        fMeta.tags = dummyItem.tags;
        fMeta.palettes = dummyItem.palettes;
    }

    // 🚀【50ms 自动防抖】：同目录连续 100 次修改只重置定时器，到期后仅执行 1 次物理原子落盘！
    QMetaObject::invokeMethod(this, [this]() {
        if (m_flushTimer && !m_flushTimer->isActive()) {
            m_flushTimer->start();
        }
    }, Qt::QueuedConnection);
}

void QuarkMetaJsonStore::onFlushTimeout() {
    flushAllDirtyBuffers();
}

void QuarkMetaJsonStore::flushAllDirtyBuffers() {
    std::unordered_map<std::wstring, QuarkMetaJson> toFlush;
    {
        std::lock_guard<std::mutex> lock(m_storeMutex);
        if (m_dirtyBufferMap.empty()) return;
        toFlush = std::move(m_dirtyBufferMap);
        m_dirtyBufferMap.clear();
    }

    // 在无锁环境下执行磁盘临时文件写入与 MoveFileExW 原子替换
    for (const auto& [folderPath, json] : toFlush) {
        json.save();
    }
}

bool QuarkMetaJsonStore::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
    flushAllDirtyBuffers();
    return QuarkMetaJson::migrateFolderCache(oldFolderPath, newFolderPath);
}

bool QuarkMetaJsonStore::renameItem(const QString& folderPath, const QString& oldName, const QString& newName) {
    flushAllDirtyBuffers();
    return QuarkMetaJson::renameItem(folderPath, oldName, newName);
}

} // namespace QuarkMeta
```

---

### 2.3 `src/meta/MetadataManager.cpp` 消除重复代码
```cpp
#include "MetadataManager.h"
#include "VolumePathResolver.h" // 👈 统一引入唯一的卷路径解析服务
// ...

namespace QuarkMeta {

// 🚀【彻底消除重复】：删除原有的 MetadataManager::getVolumeSerialNumber，直接委托给权威类！
std::wstring MetadataManager::getVolumeSerialNumber(const std::wstring& path) {
    return VolumePathResolver::getVolumeSerialNumber(path);
}

// ... [其余门面调度逻辑保持不变] ...

} // namespace QuarkMeta
```

---

## 3. 构建配置注册
确保相关源文件在构建系统中已正规注册：
```cmake
set(META_SOURCES
    # ...
    src/meta/QuarkMetaJson.h
    src/meta/QuarkMetaJson.cpp
    src/meta/QuarkMetaJsonStore.h
    src/meta/QuarkMetaJsonStore.cpp
    src/meta/MetadataManager.h
    src/meta/MetadataManager.cpp
    src/util/VolumePathResolver.h
    src/util/VolumePathResolver.cpp
)
```