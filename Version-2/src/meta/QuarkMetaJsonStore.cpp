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
    if (auto* app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this, &QuarkMetaJsonStore::flushAllDirtyBuffers);
    }
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
