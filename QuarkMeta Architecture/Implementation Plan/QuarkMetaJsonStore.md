# QuarkMetaJsonStore 元数据持久化与脏缓冲合并落盘实施方案

## 1. Overview（概述与解决的问题）
本实施方案旨在彻底解决元数据修改过程中的高频磁盘写盘震荡，并消除门面工具重复代码：
1. **脏缓冲合并与 50ms 防抖落盘**：在 `QuarkMetaJsonStore` 中引入 `m_dirtyBufferMap`（目录路径 -> QuarkMetaJson 对象）与 50ms 防抖定时器 (`kFlushDebounceMs = 50`)。同目录内的连续元数据修改自动在内存中合流，到期后仅执行 1 次物理原子落盘。
2. **应用退出原子刷盘**：挂载 `QCoreApplication::aboutToQuit` 信号并于 `~QuarkMetaJsonStore()` 析构函数中显式强制刷盘，确保零数据丢失。
3. **消除门面代码重复**：删除 `MetadataManager` 中重复的 `getVolumeSerialNumber` Win32 底层实现，统一收敛委托给 `VolumePathResolver`。

---

## 2. Modified Files List（影响文件清单）
- `src/meta/QuarkMetaJsonStore.h`
- `src/meta/QuarkMetaJsonStore.cpp`
- `src/meta/MetadataManager.cpp`
- `CMakeLists.txt`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/QuarkMetaJsonStore.h`
<<<<<<< SEARCH
#pragma once

#include <string>
#include <QStringList>
#include "QuarkMetaJson.h"

namespace QuarkMeta {

class QuarkMetaJsonStore {
public:
    static QuarkMetaJsonStore& instance();

    void updateItemMeta(const std::wstring& path, std::function<void(ItemMeta&)> modifier);
    bool renameItem(const QString& folderPath, const QString& oldName, const QString& newName);
    bool migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath);

private:
    QuarkMetaJsonStore() = default;
    ~QuarkMetaJsonStore() = default;
};

} // namespace QuarkMeta
=======
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
>>>>>>> REPLACE

### 3.2 `src/meta/QuarkMetaJsonStore.cpp`
<<<<<<< SEARCH
#include "QuarkMetaJsonStore.h"
#include "QuarkMetaJson.h"
#include <QFileInfo>

namespace QuarkMeta {

QuarkMetaJsonStore& QuarkMetaJsonStore::instance() {
    static QuarkMetaJsonStore inst;
    return inst;
}

void QuarkMetaJsonStore::updateItemMeta(const std::wstring& path, std::function<void(ItemMeta&)> modifier) {
    if (!modifier) return;
    QFileInfo info(QString::fromStdWString(path));
    QString folder = info.absolutePath();
    QuarkMetaJson store(folder.toStdWString());
    store.load();
    std::wstring fileName = info.fileName().toStdWString();
    
    ItemMeta& meta = store.items()[fileName];
    meta.type = info.isDir() ? L"folder" : L"file";
    modifier(meta);
    
    if (info.isDir()) {
        QuarkMetaJson selfStore(info.absoluteFilePath().toStdWString());
        selfStore.load();
        FolderMeta& fMeta = selfStore.folder();
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

        modifier(dummyItem);

        fMeta.rating = dummyItem.rating;
        fMeta.color = dummyItem.color;
        fMeta.pinned = dummyItem.pinned;
        fMeta.note = dummyItem.note;
        fMeta.url = dummyItem.url;
        fMeta.encrypted = dummyItem.encrypted;
        fMeta.folderId = dummyItem.folderId;
        fMeta.tags = dummyItem.tags;
        fMeta.palettes = dummyItem.palettes;
        selfStore.save();
    }
    
    store.save();
}

bool QuarkMetaJsonStore::renameItem(const QString& folderPath, const QString& oldName, const QString& newName) {
    return QuarkMetaJson::renameItem(folderPath, oldName, newName);
}

bool QuarkMetaJsonStore::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
    return QuarkMetaJson::migrateFolderCache(oldFolderPath, newFolderPath);
}

} // namespace QuarkMeta
=======
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

    // 🚀【50ms 自动防抖】：安全跨线程通知 UI 主线程启动定时器
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
>>>>>>> REPLACE

### 3.3 `src/meta/MetadataManager.cpp`
<<<<<<< SEARCH
#include "MetadataManager.h"
#include "MetadataDefs.h"
=======
#include "MetadataManager.h"
#include "MetadataDefs.h"
#include "../util/VolumePathResolver.h"
>>>>>>> REPLACE

<<<<<<< SEARCH
std::wstring MetadataManager::getVolumeSerialNumber(const std::wstring& path) {
    if (path.length() < 2 || path[1] != L':') return L"UNKNOWN";
    wchar_t root[4] = { static_cast<wchar_t>(towupper(path[0])), L':', L'\\', L'\0' };
    DWORD serial = 0;
    if (GetVolumeInformationW(root, nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) {
        wchar_t buf[16]; swprintf(buf, 16, L"%08X", serial); return buf;
    }
    return L"UNKNOWN";
}
=======
std::wstring MetadataManager::getVolumeSerialNumber(const std::wstring& path) {
    return VolumePathResolver::getVolumeSerialNumber(path);
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps（编译命令与验证方法）
1. 检查 CMake 配置：确保 `CMakeLists.txt` 中包含 `src/meta/QuarkMetaJsonStore.h` 与 `src/meta/QuarkMetaJsonStore.cpp`。
2. 校验单例与线程模型：确认 `QuarkMetaJsonStore` 继承自 `QObject` 且 MOC 正常处理 `Q_OBJECT` 宏。
3. 动态逻辑验证：在 50ms 窗口内连续发起多项元数据更新，验证仅在到期后触发 1 次原子磁盘写盘。
