#include <QFileInfo>
#include <QRandomGenerator>
#include <QtConcurrent>
#include <QThreadPool>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QCoreApplication>
#include <QUuid>
#ifdef Q_OS_WIN
#include <objbase.h>
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MetadataManager.h"
#include "MetadataDefs.h"
#include "DatabaseManager.h"
#include "DriveMetaDao.h"
#include "../core/AppConfig.h"
#include "../ui/MediaColorExtractor.h"
#include "StatisticsService.h"
#include "../core/VolumeOnlineManager.h"
#include "../ui/UiHelper.h"
#include "MediaExtractorPipeline.h"
#include "../util/ShellHelper.h"
#include "QuarkMetaJson.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <windows.h>
#include <objbase.h>
#include <fileapi.h>
#include <winbase.h>
#include <handleapi.h>
#include <winnt.h>
#include <sddl.h>

#include <cstdio>
#include <cwchar>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <algorithm>

namespace QuarkMeta {

std::wstring MetadataManager::normalizePath(const std::wstring& path) {
    if (path.empty()) return L"";
    QString qp = QDir::toNativeSeparators(QDir::cleanPath(QString::fromStdWString(path))).toLower();
    if (qp.length() == 2 && qp.endsWith(':')) qp += '\\';
    return qp.toStdWString();
}

MetadataManager& MetadataManager::instance() {
    static MetadataManager inst;
    return inst;
}

MetadataManager::MetadataManager(QObject* parent) : QObject(parent) {
    m_uiSignalTimer = new QTimer(this);
    m_uiSignalTimer->setInterval(200);
    m_uiSignalTimer->setSingleShot(true);

    connect(m_uiSignalTimer, &QTimer::timeout, [this]() {
        std::vector<QString> paths;
        bool hasReloadAll = false;
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for (const auto& p : m_pendingUiPaths) {
                if (p == "__RELOAD_ALL__") {
                    hasReloadAll = true;
                }
                paths.push_back(p);
            }
            m_pendingUiPaths.clear();
        }

        if (hasReloadAll || paths.size() > 50) {
            emit metaChanged("__RELOAD_ALL__");
        } else {
            for (const auto& p : paths) {
                emit metaChanged(p);
            }
        }
    });

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [this]() {
        DatabaseManager::instance().shutdown();
        AppConfig::instance().setValue("System/LastCleanShutdown", true);
        AppConfig::instance().sync();
    });
}

void MetadataManager::initFromDatabase() {
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if (m_loaded) return;
    }

    DatabaseManager::instance().init();

    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_loaded = true;
    }

    notifyUI(RefreshLevel::FullRebuild);
}

void MetadataManager::notifyUI(RefreshLevel level, const QString& path) {
    switch (level) {
        case RefreshLevel::CountsOnly:
            notifyCategoryCountChanged();
            break;
        case RefreshLevel::PathUpdate:
            if (!path.isEmpty()) {
                {
                    std::unique_lock<std::shared_mutex> lock(m_mutex);
                    m_pendingUiPaths.insert(path);
                }
                QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
            }
            break;
        case RefreshLevel::FullRebuild:
            notifyFullUIRebuild();
            break;
        case RefreshLevel::CategoryOnly:
            if (m_isInternalOperating) return;
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                m_pendingUiPaths.insert("__RELOAD_CATEGORY_ONLY__");
            }
            QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
            break;
    }
}

void MetadataManager::notifyCategoryCountChanged() {
    if (m_isInternalOperating) return;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_pendingUiPaths.insert("__RELOAD_COUNT__");
    }
    QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
}

void MetadataManager::notifyFullUIRebuild() {
    if (m_isInternalOperating) return;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_pendingUiPaths.insert("__RELOAD_ALL__");
    }
    QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
}

void MetadataManager::registerItem(const std::wstring& path) {
    std::wstring nPath = normalizePath(path);

    long long pSize = 0, pMtime = 0;
    if (fetchWinApiMetadataDirect(nPath, &pSize, nullptr, nullptr, &pMtime, nullptr)) {
        size_t idx = getShardIndex(nPath);
        {
            std::shared_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            auto it = m_shards[idx].items.find(nPath);
            if (it != m_shards[idx].items.end()) {
                bool metadataValid = true;
                QFileInfo info(QString::fromStdWString(nPath));
                if (info.isFile() && MediaColorExtractor::isGraphicsFile(info.suffix().toLower())) {
                    if (it->second.width <= 0 || it->second.height <= 0 || it->second.autoColor.empty()) {
                        metadataValid = false;
                    }
                }
                if (it->second.fileSize == pSize && it->second.mtime == pMtime && metadataValid) {
                    return;
                }
            }
        }
    }

    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        if (m_shards[idx].items.count(nPath)) {
            m_shards[idx].items[nPath].fileSize = pSize;
            m_shards[idx].items[nPath].mtime = pMtime;
        }
    }
    ensureActivated(nPath);

    MediaExtractorPipeline::instance().enqueue(nPath);
}


void MetadataManager::registerItemsAsync(const QStringList& paths) {
    if (paths.isEmpty()) return;
    
    (void)QtConcurrent::run([this, paths]() {
        std::vector<std::wstring> stdPaths;
        for (const auto& qp : paths) {
            std::wstring nPath = normalizePath(qp.toStdWString());
            ensureActivated(nPath);
            stdPaths.push_back(nPath);
        }
        MediaExtractorPipeline::instance().enqueueBatch(stdPaths);
    });
}

RuntimeMeta MetadataManager::getMeta(const std::wstring& path) {
    std::wstring nPath = normalizePath(path);
    size_t idx = getShardIndex(nPath);
    std::shared_lock<std::shared_mutex> lock(m_shards[idx].mutex);
    auto it = m_shards[idx].items.find(nPath);
    if (it != m_shards[idx].items.end()) {
        return it->second;
    }
    return RuntimeMeta();
}

void MetadataManager::ensureActivated(const std::wstring& nPath) {
    {
        size_t idx = getShardIndex(nPath);
        std::shared_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        if (m_shards[idx].items.find(nPath) != m_shards[idx].items.end()) return;
    }

    RuntimeMeta rm;
    std::wstring type;
    
    bool success = fetchWinApiMetadataDirect(nPath, &rm.fileSize, &type, &rm.ctime, &rm.mtime, &rm.atime);
    if (!success) {
        QFileInfo qinfo(QString::fromStdWString(nPath));
        if (qinfo.exists()) {
            rm.fileSize = qinfo.size();
            rm.isFolder = qinfo.isDir();
            rm.ctime = qinfo.birthTime().toMSecsSinceEpoch();
            rm.mtime = qinfo.lastModified().toMSecsSinceEpoch();
            rm.atime = qinfo.lastRead().toMSecsSinceEpoch();
            success = true;
        }
    } else {
        rm.isFolder = (type == L"folder");
    }

    if (success) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        size_t idx = getShardIndex(nPath);
        {
            std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            if (m_shards[idx].items.count(nPath)) return;
            m_shards[idx].items[nPath] = rm;
        }

    }
}

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

    QuarkMetaJson::updateItemMeta(nPath, [rating](ItemMeta& item) {
        item.rating = rating;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setSha256(const std::wstring& path, const std::string& sha256, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].sha256 = sha256;
    }
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::updateExtractedMediaFeaturesBatch(const std::vector<ExtractedFeatureItem>& items) {
    if (items.empty()) return;

    for (const auto& item : items) {
        std::wstring nPath = normalizePath(item.path);
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        if (m_shards[idx].items.count(nPath)) {
            RuntimeMeta& meta = m_shards[idx].items[nPath];
            meta.width = item.width;
            meta.height = item.height;
            if (item.mtime > 0) meta.mtime = item.mtime;
            if (item.fileSize > 0) meta.fileSize = item.fileSize;
            meta.autoColor = item.autoColor;
            meta.palettes.clear();
            for (const auto& p : item.palettes) {
                meta.palettes.emplace_back(p.first, p.second);
            }
        }
    }
}

void MetadataManager::updateExtractedMediaFeatures( 
    const std::wstring& path,  
    int width,  
    int height,  
    const std::wstring& autoColor,  
    const QVector<QPair<QColor, float>>& palettes)  
{ 
    std::wstring nPath = normalizePath(path); 
    { 
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        if (!m_shards[idx].items.count(nPath)) return;

        RuntimeMeta& meta = m_shards[idx].items[nPath]; 
        meta.width = width; 
        meta.height = height; 
        meta.autoColor = autoColor; 
         
        meta.palettes.clear(); 
        for (const auto& p : palettes) { 
            meta.palettes.emplace_back(p.first, p.second); 
        } 
    } 

    notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath)); 
}

void MetadataManager::setAddedAt(const std::wstring& path, long long addedAt, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].added_at = addedAt;
    }
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::renameTag(const QString& oldName, const QString& newName) {
    if (oldName == newName) return;
    
    std::vector<std::wstring> affectedPaths;
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
        std::unique_lock<std::shared_mutex> lock(m_shards[i].mutex);
        for (auto& pair : m_shards[i].items) {
            if (pair.second.tags.contains(oldName)) {
                pair.second.tags.removeAll(oldName);
                if (!newName.isEmpty() && !pair.second.tags.contains(newName)) {
                    pair.second.tags.append(newName);
                }
                affectedPaths.push_back(pair.first);
            }
        }
    }
    
    notifyFullUIRebuild();
}

void MetadataManager::removeTag(const QString& tagName) {
    std::vector<std::wstring> affectedPaths;
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
        std::unique_lock<std::shared_mutex> lock(m_shards[i].mutex);
        for (auto& pair : m_shards[i].items) {
            if (pair.second.tags.contains(tagName)) {
                pair.second.tags.removeAll(tagName);
                affectedPaths.push_back(pair.first);
            }
        }
    }
    
    notifyFullUIRebuild();
}

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

    QuarkMetaJson::updateItemMeta(nPath, [normColor](ItemMeta& item) {
        item.color = normColor;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

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

    QuarkMetaJson::updateItemMeta(nPath, [pinned](ItemMeta& item) {
        item.pinned = pinned;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setTags(const std::wstring& path, const QStringList& tags, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);

    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].tags = tags;
    }

    std::vector<std::wstring> wTags;
    for (const QString& t : tags) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty()) {
            wTags.push_back(trimmed.toStdWString());
        }
    }
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

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setEncrypted(const std::wstring& path, bool encrypted, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].encrypted = encrypted;
    }
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setPalettes(const std::wstring& path, const QVector<QPair<QColor, float>>& palettes, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    std::vector<PaletteEntry> entries;
    for (int i = 0; i < palettes.size(); ++i) { entries.push_back(PaletteEntry(palettes[i].first, palettes[i].second)); }
    size_t idx = getShardIndex(nPath);
    {
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath].palettes = entries;
    }
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setItemVisualMetadata(const std::wstring& path, const std::wstring& color, const QVector<QPair<QColor, float>>& palettes, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    std::vector<PaletteEntry> entries;
    for (int i = 0; i < palettes.size(); ++i) { entries.push_back(PaletteEntry(palettes[i].first, palettes[i].second)); }
    
    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        RuntimeMeta& meta = m_shards[idx].items[nPath];
        meta.autoColor = color;
        meta.palettes = entries;
    }
    
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setItemDimensions(const std::wstring& path, int width, int height) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        RuntimeMeta& meta = m_shards[idx].items[nPath];
        meta.width = width;
        meta.height = height;
    }
}

QVector<QColor> MetadataManager::getPalettes(const std::wstring& path) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    size_t idx = getShardIndex(nPath);
    std::shared_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
    auto it = m_shards[idx].items.find(nPath);
    if (it != m_shards[idx].items.end() && !it->second.palettes.empty()) {
        QVector<QColor> colors;
        for (const auto& entry : it->second.palettes) colors << entry.color;
        return colors;
    }
    return {};
}

void MetadataManager::renameBatchAsync(
    const std::vector<std::pair<std::wstring, std::wstring>>& rawPathPairs,
    std::function<void(int successCount)> onCompleted) 
{
    if (rawPathPairs.empty()) {
        if (onCompleted) onCompleted(0);
        return;
    }

    setInternalOperating(true);

    (void)QtConcurrent::run([this, rawPathPairs, onCompleted]() {
        int successCount = 0;

        std::vector<std::pair<std::wstring, std::wstring>> normalizedPairs;
        normalizedPairs.reserve(rawPathPairs.size());
        for (const auto& pair : rawPathPairs) {
            std::wstring nOld = normalizePath(pair.first);
            std::wstring nNew = normalizePath(pair.second);
            if (!nOld.empty() && !nNew.empty() && nOld != nNew) {
                normalizedPairs.emplace_back(nOld, nNew);
            }
        }

        if (normalizedPairs.empty()) {
            setInternalOperating(false);
            if (onCompleted) {
                QMetaObject::invokeMethod(qApp, [onCompleted]() { onCompleted(0); }, Qt::QueuedConnection);
            }
            return;
        }

        std::vector<std::pair<std::wstring, std::wstring>> ioTasks;
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            for (const auto& pair : normalizedPairs) {
                const std::wstring& curOld = pair.first;
                const std::wstring& curNew = pair.second;

                size_t oldIdx = getShardIndex(curOld);
                RuntimeMeta meta;
                bool found = false;
                {
                    std::unique_lock<std::shared_mutex> shardLock(m_shards[oldIdx].mutex);
                    auto it = m_shards[oldIdx].items.find(curOld);
                    if (it != m_shards[oldIdx].items.end()) {
                        meta = it->second;
                        m_shards[oldIdx].items.erase(it);
                        found = true;
                    }
                }

                if (!found) continue;

                bool isFolder = meta.isFolder;

                std::wstring oldName, oldExt;
                parsePathComponents(curOld, isFolder, oldName, oldExt);

                std::wstring newName, newExt;
                parsePathComponents(curNew, isFolder, newName, newExt);
                meta.baseName = newName;
                meta.ext = newExt;

                size_t newIdx = getShardIndex(curNew);
                {
                    std::unique_lock<std::shared_mutex> shardLock(m_shards[newIdx].mutex);
                    m_shards[newIdx].items[curNew] = meta;
                }

                ioTasks.push_back(pair);
                successCount++;
            }
        }

        for (const auto& pair : ioTasks) {
            QFileInfo oldFileInfo(QString::fromStdWString(pair.first));
            QFileInfo newFileInfo(QString::fromStdWString(pair.second));
            if (oldFileInfo.isDir()) {
                QuarkMetaJson::migrateFolderCache(oldFileInfo.absoluteFilePath(), newFileInfo.absoluteFilePath());
            } else {
                QuarkMetaJson::renameItem(oldFileInfo.absolutePath(), oldFileInfo.fileName(), newFileInfo.fileName());
            }
        }

        setInternalOperating(false);

        QMetaObject::invokeMethod(qApp, [this, successCount, onCompleted]() {
            notifyFullUIRebuild();
            if (onCompleted) onCompleted(successCount);
        }, Qt::QueuedConnection);
    });
}

void MetadataManager::renameItem(const std::wstring& oldPath, const std::wstring& newPath) {
    std::wstring nOld = normalizePath(oldPath);
    std::wstring nNew = normalizePath(newPath);
    if (nOld == nNew) return;

    (void)QtConcurrent::run([this, nOld, nNew]() {
        std::vector<std::pair<std::wstring, std::wstring>> itemsToRename;
        std::vector<std::pair<std::wstring, std::wstring>> ioTasks;
        
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            forEachCachedItem([&](const std::wstring& p, const RuntimeMeta&) {
                if (p == nOld) {
                    itemsToRename.push_back({p, nNew});
                } else if (p.find(nOld + L"\\") == 0 || p.find(nOld + L"/") == 0) {
                    std::wstring relative = p.substr(nOld.length());
                    itemsToRename.push_back({p, nNew + relative});
                }
            });

            if (itemsToRename.empty()) return;

            for (const auto& pair : itemsToRename) {
                const std::wstring& curOld = pair.first;
                const std::wstring& curNew = pair.second;

                size_t oldIdx = getShardIndex(curOld);
                RuntimeMeta meta;
                bool found = false;
                {
                    std::unique_lock<std::shared_mutex> shardLock(m_shards[oldIdx].mutex);
                    auto it = m_shards[oldIdx].items.find(curOld);
                    if (it != m_shards[oldIdx].items.end()) {
                        meta = it->second;
                        m_shards[oldIdx].items.erase(it);
                        found = true;
                    }
                }

                if (!found) continue;

                bool isFolder = meta.isFolder;

                std::wstring newName, newExt;
                parsePathComponents(curNew, isFolder, newName, newExt);
                meta.baseName = newName;
                meta.ext = newExt;

                size_t newIdx = getShardIndex(curNew);
                {
                    std::unique_lock<std::shared_mutex> shardLock(m_shards[newIdx].mutex);
                    m_shards[newIdx].items[curNew] = meta;
                }

                ioTasks.push_back(pair);
            }
        }

        for (const auto& pair : ioTasks) {
            QFileInfo oldFileInfo(QString::fromStdWString(pair.first));
            QFileInfo newFileInfo(QString::fromStdWString(pair.second));
            if (oldFileInfo.isDir()) {
                QuarkMetaJson::migrateFolderCache(oldFileInfo.absoluteFilePath(), newFileInfo.absoluteFilePath());
            } else {
                QuarkMetaJson::renameItem(oldFileInfo.absolutePath(), oldFileInfo.fileName(), newFileInfo.fileName());
            }
        }

        notifyFullUIRebuild();
    });
}

void MetadataManager::syncAfterMove(const std::wstring& oldPath, const std::wstring& newPath) {
    std::wstring nOld = normalizePath(oldPath);
    std::wstring nNew = normalizePath(newPath);
    if (nOld == nNew) return;
    renameItem(nOld, nNew);
}

void MetadataManager::removeMetadataSync(const std::wstring& path) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        for (size_t i = 0; i < NUM_SHARDS; ++i) {
            std::unique_lock<std::shared_mutex> shardLock(m_shards[i].mutex);
            for (auto it = m_shards[i].items.begin(); it != m_shards[i].items.end(); ) {
                if (it->first == nPath || it->first.find(nPath + L"\\") == 0 || it->first.find(nPath + L"/") == 0) {
                    StatisticsService::instance().purgeAsset(!it->second.tags.isEmpty(), false);
                    it = m_shards[i].items.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

std::wstring MetadataManager::getVolumeSerialNumber(const std::wstring& path) {
    if (path.length() < 2 || path[1] != L':') return L"UNKNOWN";
    wchar_t root[4] = { static_cast<wchar_t>(towupper(path[0])), L':', L'\\', L'\0' };
    DWORD serial = 0;
    if (GetVolumeInformationW(root, nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) {
        wchar_t buf[16]; swprintf(buf, 16, L"%08X", serial); return buf;
    }
    return L"UNKNOWN";
}

bool MetadataManager::fetchWinApiMetadataDirect(const std::wstring& path, long long* outSize, std::wstring* outType, long long* outCtime, long long* outMtime, long long* outAtime) {
    HANDLE hFile = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    BY_HANDLE_FILE_INFORMATION basicInfo;
    if (GetFileInformationByHandle(hFile, &basicInfo)) {
        if (outSize) *outSize = (static_cast<long long>(basicInfo.nFileSizeHigh) << 32) | basicInfo.nFileSizeLow;
        if (outType) *outType = (basicInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? L"folder" : L"file";
        auto toMS = [](const FILETIME& ft) {
            ULARGE_INTEGER ull; ull.LowPart = ft.dwLowDateTime; ull.HighPart = ft.dwHighDateTime;
            return static_cast<long long>((ull.QuadPart - 116444736000000000ULL) / 10000ULL);
        };
        if (outCtime) *outCtime = toMS(basicInfo.ftCreationTime);
        if (outMtime) *outMtime = toMS(basicInfo.ftLastWriteTime);
        if (outAtime) *outAtime = toMS(basicInfo.ftLastAccessTime);
        CloseHandle(hFile);
        return true;
    }
    CloseHandle(hFile);
    return false;
}

void MetadataManager::syncPhysicalMetadata(const std::wstring& path, bool notify) { persistAsync(path, notify); }

void MetadataManager::activateItem(const std::wstring& path) {
    instance().registerItem(path);
}

void MetadataManager::persistBatchAsync(const std::vector<std::wstring>& paths) {
    if (paths.empty()) return;

    for (const auto& p : paths) {
        RuntimeMeta rMeta = getMeta(p);
        parsePathComponents(p, rMeta.isFolder, rMeta.baseName, rMeta.ext);

        size_t idx = getShardIndex(p);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[p] = rMeta;
    }
}

void MetadataManager::persistAsync(const std::wstring& path, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    
    RuntimeMeta rMeta = getMeta(nPath);
    parsePathComponents(nPath, rMeta.isFolder, rMeta.baseName, rMeta.ext);

    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        m_shards[idx].items[nPath] = rMeta;
    }
        
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::parsePathComponents(const std::wstring& normalizedPath, bool isFolder, std::wstring& outName, std::wstring& outExt) {
    size_t lastSlash = normalizedPath.find_last_of(L"\\/");
    std::wstring fullName = (lastSlash == std::wstring::npos) ? normalizedPath : normalizedPath.substr(lastSlash + 1);

    if (isFolder) {
        outName = fullName;
        outExt = L"";
    } else {
        outName = fullName;
        size_t lastDot = fullName.find_last_of(L'.');
        if (lastDot != std::wstring::npos && lastDot > 0) {
            outExt = fullName.substr(lastDot + 1);
            std::transform(outExt.begin(), outExt.end(), outExt.begin(), ::towlower);
        } else {
            outExt = L"";
        }
    }
}

QStringList MetadataManager::searchInCache(const QString& keyword, const QString& scopeSource, int categoryId, const QString& parentPath) {
    Q_UNUSED(categoryId);
    QStringList results;
    if (keyword.isEmpty()) return results;
    
    std::wstring wParentPath = (scopeSource == "nav" && !parentPath.isEmpty()) ? normalizePath(parentPath.toStdWString()) : L"";
    if (!wParentPath.empty()) {
        bool endsWithSlash = (wParentPath.back() == L'\\' || wParentPath.back() == L'/');
        if (!endsWithSlash) {
            wParentPath += L'\\';
        }
    }

    std::wstring lowerKeyword = keyword.toLower().toStdWString();

    forEachCachedItem([&](const std::wstring& path, const RuntimeMeta& meta) {
        if (!wParentPath.empty() && path.find(wParentPath) != 0) {
            return;
        }

        if (meta.baseName.find(lowerKeyword) != std::wstring::npos ||
            meta.note.find(lowerKeyword) != std::wstring::npos ||
            meta.tags.contains(keyword, Qt::CaseInsensitive)) {
            results << QString::fromStdWString(path);
        }
    });

    return results;
}

QMap<QString, int> MetadataManager::getAllTags() const {
    QMap<QString, int> tagCounts;
    forEachCachedItem([&](const std::wstring&, const RuntimeMeta& meta) {
        for (const QString& tag : meta.tags) {
            tagCounts[tag]++;
        }
    });
    return tagCounts;
}

QList<QPair<QString, int>> MetadataManager::getTopTags(int limit) const {
    QMap<QString, int> counts = getAllTags();
    QList<QPair<QString, int>> list;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        list.append({it.key(), it.value()});
    }

    std::sort(list.begin(), list.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });

    if (list.size() > limit) {
        return list.mid(0, limit);
    }
    return list;
}

void MetadataManager::recordAccess(const std::wstring& path) {
    std::wstring nPath = normalizePath(path);
    {
        std::lock_guard<std::mutex> lock(m_recentMutex);
        auto it = std::find(m_recentVisitedQueue.begin(), m_recentVisitedQueue.end(), nPath);
        if (it != m_recentVisitedQueue.end()) {
            m_recentVisitedQueue.erase(it);
        } else {
            m_recentVisitedSet.insert(nPath);
        }
        m_recentVisitedQueue.push_back(nPath);
    }
    
    double now = static_cast<double>(QDateTime::currentMSecsSinceEpoch());
    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        if (m_shards[idx].items.count(nPath)) {
            m_shards[idx].items[nPath].atime = static_cast<long long>(now);
        }
    }
}

double MetadataManager::getCachedAtime(const std::wstring& path) {
    size_t idx = getShardIndex(path);
    std::shared_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
    auto it = m_shards[idx].items.find(path);
    if (it != m_shards[idx].items.end()) {
        return static_cast<double>(it->second.atime);
    }
    return 0.0;
}

void MetadataManager::slideRecentWindow() {
    double expireThreshold = static_cast<double>(QDateTime::currentMSecsSinceEpoch()) - 86400000.0;

    std::deque<std::wstring> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_recentMutex);
        snapshot = m_recentVisitedQueue;
    }

    std::vector<std::wstring> toErase;
    for (const auto& path : snapshot) {
        double itemAtime = getCachedAtime(path);
        if (itemAtime < expireThreshold) {
            toErase.push_back(path);
        } else {
            break;
        }
    }

    if (toErase.empty()) return;

    std::lock_guard<std::mutex> lock(m_recentMutex);
    for (const auto& path : toErase) {
        if (!m_recentVisitedQueue.empty() && m_recentVisitedQueue.front() == path) {
            m_recentVisitedQueue.pop_front();
            m_recentVisitedSet.erase(path);
        }
    }
}

std::vector<LightMeta> MetadataManager::getLightweightCacheSnapshot() const {
    std::vector<LightMeta> result;
    forEachCachedItem([&](const std::wstring& path, const RuntimeMeta& meta) {
        result.push_back({
            path,
            meta.isFolder,
            meta.tags.isEmpty(),
            static_cast<double>(meta.atime),
            meta.tags
        });
    });
    return result;
}

} // namespace QuarkMeta