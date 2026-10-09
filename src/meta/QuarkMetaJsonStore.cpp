#include "QuarkMetaJsonStore.h"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

namespace QuarkMeta {

QuarkMetaJsonStore& QuarkMetaJsonStore::instance() {
    static QuarkMetaJsonStore s_instance;
    return s_instance;
}

QuarkMetaJsonStore::QuarkMetaJsonStore(QObject* parent)
    : QObject(parent) {
    if (auto* app = QCoreApplication::instance()) {
        this->moveToThread(app->thread());
        connect(app, &QCoreApplication::aboutToQuit, this, [this]() {
            flushAllDirtyBuffers(true);
        });
    }

    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(kFlushDebounceMs);
    connect(m_flushTimer, &QTimer::timeout, this, &QuarkMetaJsonStore::onFlushTimeout);
}

QuarkMetaJsonStore::~QuarkMetaJsonStore() {
    flushAllDirtyBuffers(true);
}

std::wstring QuarkMetaJsonStore::normalizeFolderPath(const std::wstring& path) {
    if (path.empty()) return L"";
    QString qp = QDir::toNativeSeparators(QDir::cleanPath(QString::fromStdWString(path))).toLower();
    if (qp.length() == 2 && qp.endsWith(':')) qp += '\\';
    return qp.toStdWString();
}

void QuarkMetaJsonStore::updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater) {
    if (!updater) return;

    QFileInfo info(QString::fromStdWString(filePath));
    std::wstring folderPath = info.absolutePath().toStdWString();
    std::wstring fileName = info.fileName().toLower().toStdWString();
    std::wstring normFolder = normalizeFolderPath(folderPath);

    {
        std::lock_guard<std::mutex> lock(m_storeMutex);

        auto it = m_folderCacheMap.find(normFolder);
        if (it == m_folderCacheMap.end()) {
            auto jsonPtr = std::make_shared<QuarkMetaJson>(folderPath);
            jsonPtr->load();
            it = m_folderCacheMap.emplace(normFolder, jsonPtr).first;
        }

        ItemMeta& meta = it->second->items()[fileName];
        meta.type = info.isDir() ? L"folder" : L"file";
        updater(meta);

        m_dirtyFolderPaths.insert(normFolder);

        // 如果是文件夹，同步更新自身目录内部的 .QuarkMeta.json 镜像
        if (info.isDir()) {
            std::wstring selfDirPath = info.absoluteFilePath().toStdWString();
            std::wstring normSelfDir = normalizeFolderPath(selfDirPath);

            auto selfIt = m_folderCacheMap.find(normSelfDir);
            if (selfIt == m_folderCacheMap.end()) {
                auto selfJsonPtr = std::make_shared<QuarkMetaJson>(selfDirPath);
                selfJsonPtr->load();
                selfIt = m_folderCacheMap.emplace(normSelfDir, selfJsonPtr).first;
            }

            FolderMeta& fMeta = selfIt->second->folder();
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

            m_dirtyFolderPaths.insert(normSelfDir);
        }
    }

    // 🚀【50ms 自动防抖】：安全跨线程通知 UI 主线程启动定时器
    QMetaObject::invokeMethod(this, [this]() {
        if (m_flushTimer && !m_flushTimer->isActive()) {
            m_flushTimer->start();
        }
    }, Qt::QueuedConnection);
}

QuarkMetaJson::ItemMap QuarkMetaJsonStore::readFolderMeta(const std::wstring& folderPath) {
    std::wstring normFolder = normalizeFolderPath(folderPath);
    std::lock_guard<std::mutex> lock(m_storeMutex);

    auto it = m_folderCacheMap.find(normFolder);
    if (it != m_folderCacheMap.end()) {
        return it->second->items();
    }

    auto jsonPtr = std::make_shared<QuarkMetaJson>(folderPath);
    jsonPtr->load();
    m_folderCacheMap[normFolder] = jsonPtr;

    return jsonPtr->items();
}

bool QuarkMetaJsonStore::readItemMeta(const std::wstring& filePath, ItemMeta& outMeta) {
    QFileInfo info(QString::fromStdWString(filePath));
    std::wstring folderPath = info.absolutePath().toStdWString();
    std::wstring fileName = info.fileName().toLower().toStdWString();

    auto folderMetaMap = readFolderMeta(folderPath);
    auto it = folderMetaMap.find(fileName);
    if (it != folderMetaMap.end()) {
        outMeta = it->second;
        return true;
    }
    return false;
}

void QuarkMetaJsonStore::onFlushTimeout() {
    flushAllDirtyBuffers();
}

void QuarkMetaJsonStore::flushAllDirtyBuffers(bool sync) {
    std::vector<QuarkMetaJson> snapshotsToSave;
    {
        std::lock_guard<std::mutex> lock(m_storeMutex);
        if (m_dirtyFolderPaths.empty()) return;

        for (const auto& normFolder : m_dirtyFolderPaths) {
            auto it = m_folderCacheMap.find(normFolder);
            if (it != m_folderCacheMap.end() && it->second) {
                snapshotsToSave.push_back(*(it->second)); // 锁保护下深拷贝快照
            }
        }
        m_dirtyFolderPaths.clear();
    }

    if (snapshotsToSave.empty()) return;

    auto doSave = [](std::vector<QuarkMetaJson> snapshots) {
        for (auto& jsonSnapshot : snapshots) {
            if (jsonSnapshot.isLoadFailed()) {
                qDebug() << "[AutoColor] [QuarkMetaJsonStore] Skipped flushing for folder due to load failure";
                continue;
            }

            bool ok = jsonSnapshot.save();
            qDebug() << "[AutoColor] [QuarkMetaJsonStore] Flushed dirty buffer for folder, items count:"
                     << jsonSnapshot.items().size() << "status:" << ok;
        }
    };

    if (sync) {
        doSave(std::move(snapshotsToSave));
    } else {
        (void)QtConcurrent::run([snapshotsToSave = std::move(snapshotsToSave), doSave]() mutable {
            doSave(std::move(snapshotsToSave));
        });
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
