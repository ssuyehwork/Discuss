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
#include "../util/VolumePathResolver.h"
#include "DatabaseManager.h"
#include "QuarkMetaJsonStore.h"
#include "MetaDbRepository.h"
#include "MetaMemoryCache.h"
#include "../core/AppConfig.h"
#include "../util/ColorPaletteEngine.h"
#include "StatisticsService.h"
#include "../core/VolumeOnlineManager.h"
#include "../ui/UiHelper.h"
#include "MediaExtractorPipeline.h"
#include "../util/ShellHelper.h"

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

    MetaDbRepository::instance().initDatabase();

    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_loaded = true;
    }

    notifyUI(RefreshLevel::FullRebuild);
}

void MetadataManager::notifyUI(RefreshLevel level, const QString& path) {
    switch (level) {
        case RefreshLevel::CountsOnly:
            if (m_isInternalOperating) return;
            {
                std::unique_lock<std::shared_mutex> lock(m_mutex);
                m_pendingUiPaths.insert("__RELOAD_COUNT__");
            }
            QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
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
    }
}

void MetadataManager::notifyFullUIRebuild() {
    if (m_isInternalOperating) return;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_pendingUiPaths.insert("__RELOAD_ALL__");
    }
    QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
}

bool MetadataManager::shouldExtractMediaFeatures(const std::wstring& path, long long pSize, long long pMtime) {
    QString qPath = QString::fromStdWString(path);
    QFileInfo info(qPath);
    if (!info.isFile() || !ColorPaletteEngine::isGraphicsFile(info.suffix().toLower())) {
        return false; // 非图片文件，不用提取
    }

    ItemMeta meta;
    if (QuarkMetaJsonStore::instance().readItemMeta(path, meta)) {
        if (!meta.autoColor.empty() && !meta.palettes.empty() && meta.width > 0 && meta.height > 0) {
            if (meta.size == pSize && meta.modificationTime == pMtime) {
                return false; // 已新鲜，跳过
            }
        }
    }
    return true;
}

void MetadataManager::registerItem(const std::wstring& path) {
    std::wstring nPath = normalizePath(path);
    ensureActivated(nPath);

    long long pSize = 0, pMtime = 0;
    fetchWinApiMetadataDirect(nPath, &pSize, nullptr, nullptr, &pMtime, nullptr);

    int pendingCount = 1;
    int skippedFreshCount = 0;
    int enqueuedCount = 0;

    if (!shouldExtractMediaFeatures(nPath, pSize, pMtime)) {
        skippedFreshCount = 1;
    } else {
        enqueuedCount = 1;
        MediaExtractorPipeline::instance().enqueue(nPath);
    }

    qDebug() << "[AutoColor] [MetadataManager] Enqueue check - Pending:" << pendingCount 
             << "SkippedFresh:" << skippedFreshCount << "Enqueued:" << enqueuedCount;
}

void MetadataManager::registerItemsAsync(const QStringList& paths) {
    if (paths.isEmpty()) return;

    (void)QtConcurrent::run([this, paths]() {
        std::vector<std::wstring> enqueuedPaths;
        int pendingCount = paths.size();
        int skippedFreshCount = 0;

        for (const auto& qp : paths) {
            std::wstring nPath = normalizePath(qp.toStdWString());
            ensureActivated(nPath);

            long long pSize = 0, pMtime = 0;
            fetchWinApiMetadataDirect(nPath, &pSize, nullptr, nullptr, &pMtime, nullptr);

            if (shouldExtractMediaFeatures(nPath, pSize, pMtime)) {
                enqueuedPaths.push_back(nPath);
            } else {
                skippedFreshCount++;
            }
        }

        int enqueuedCount = enqueuedPaths.size();
        qDebug() << "[AutoColor] [MetadataManager] Enqueue check - Pending:" << pendingCount 
                 << "SkippedFresh:" << skippedFreshCount << "Enqueued:" << enqueuedCount;

        if (!enqueuedPaths.empty()) {
            MediaExtractorPipeline::instance().enqueueBatch(enqueuedPaths);
        }
    });
}

RuntimeMeta MetadataManager::getMeta(const std::wstring& path) {
    std::wstring nPath = normalizePath(path);
    return MetaMemoryCache::instance().getMeta(nPath);
}

void MetadataManager::ensureActivated(const std::wstring& nPath) {
    if (MetaMemoryCache::instance().contains(nPath)) return;

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
        MetaMemoryCache::instance().put(nPath, rm);
    }
}

void MetadataManager::setRating(const std::wstring& path, int rating, bool notify) {
    std::wstring nPath = normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = MetaDbRepository::instance().getDriveMeta(nPath);
        rec.rating = rating;
        MetaDbRepository::instance().saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);

    MetaMemoryCache::instance().update(nPath, [rating](RuntimeMeta& m) {
        m.rating = rating;
    });

    QuarkMetaJsonStore::instance().updateItemMeta(nPath, [rating](ItemMeta& item) {
        item.rating = rating;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setSha256(const std::wstring& path, const std::string& sha256, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);

    MetaMemoryCache::instance().update(nPath, [sha256](RuntimeMeta& m) {
        m.sha256 = sha256;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::seedMemoryCacheBatch(const std::vector<ItemRecord>& records) {
    for (const auto& rec : records) {
        std::wstring nPath = normalizePath(rec.path.toStdWString());
        QFileInfo info(rec.path);
        if (info.isRoot() || rec.path.endsWith(":\\") || rec.path.endsWith(":/")) {
            continue; // 盘符根目录不参与
        }

        ensureActivated(nPath);

        MetaMemoryCache::instance().update(nPath, [&rec](RuntimeMeta& m) {
            m.rating = rec.rating;
            m.manualColor = rec.manualColor.toStdWString();
            m.pinned = rec.pinned;
            m.tags = rec.tags;
            m.note = rec.note.toStdWString();
            m.url = rec.url.toStdWString();
            m.width = rec.width;
            m.height = rec.height;
            m.autoColor = rec.autoColor.toStdWString();
            m.thumbStatus = (rec.thumbnailState == ItemRecord::ThumbnailState::Failed) ? 1 : 0;
            m.added_at = rec.added_at;
            m.palettes.clear();
            for (const auto& pe : rec.palettes) {
                m.palettes.push_back(PaletteEntry(pe.first, pe.second));
            }
        });
    }
}

void MetadataManager::updateExtractedMediaFeaturesBatch(const std::vector<ExtractedFeatureItem>& items) {
    if (items.empty()) return;

    int actualWritten = 0;
    int skippedSame = 0;

    for (const auto& item : items) {
        std::wstring nPath = normalizePath(item.path);
        
        QFileInfo info(QString::fromStdWString(nPath));
        if (!info.isFile() || !ColorPaletteEngine::isGraphicsFile(info.suffix().toLower())) continue;
        if (item.width <= 0 || item.height <= 0 || item.autoColor.empty() || item.palettes.empty()) continue;

        RuntimeMeta meta = MetaMemoryCache::instance().getMeta(nPath);

        // 判断新值与已有值是否完全相同
        bool sameValue = (meta.width == item.width &&
                          meta.height == item.height &&
                          meta.autoColor == item.autoColor &&
                          meta.fileSize == item.fileSize &&
                          meta.mtime == item.mtime &&
                          meta.palettes.size() == item.palettes.size());
        if (sameValue) {
            bool palettesMatch = true;
            for (size_t i = 0; i < item.palettes.size(); ++i) {
                if (meta.palettes[i].color != item.palettes[i].first ||
                    std::abs(meta.palettes[i].ratio - item.palettes[i].second) > 0.0001f) {
                    palettesMatch = false;
                    break;
                }
            }
            if (palettesMatch) {
                skippedSame++;
                continue;
            }
        }

        // 值发生变化，更新内存缓存
        MetaMemoryCache::instance().update(nPath, [&item](RuntimeMeta& rMeta) {
            rMeta.width = item.width;
            rMeta.height = item.height;
            rMeta.autoColor = item.autoColor;
            if (item.fileSize > 0) rMeta.fileSize = item.fileSize;
            if (item.mtime > 0) rMeta.mtime = item.mtime;
            rMeta.palettes.clear();
            for (const auto& p : item.palettes) {
                rMeta.palettes.push_back(PaletteEntry(p.first, p.second));
            }
        });

        // 仅写这几项落地写入 .QuarkMeta.json 侧车文件：width, height, autoColor, palettes, file_size, file_mtime
        QuarkMetaJsonStore::instance().updateItemMeta(nPath, [&item](ItemMeta& jsonMeta) {
            jsonMeta.width = item.width;
            jsonMeta.height = item.height;
            jsonMeta.autoColor = item.autoColor;
            jsonMeta.size = item.fileSize;
            jsonMeta.modificationTime = item.mtime;
            jsonMeta.palettes.clear();
            for (const auto& p : item.palettes) {
                jsonMeta.palettes.push_back(PaletteEntry(p.first, p.second));
            }
        });

        actualWritten++;
        notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
    }

    qDebug() << "[AutoColor] [MetadataManager] Batch write complete - Written:" << actualWritten << "SkippedSame:" << skippedSame;
}

void MetadataManager::updateExtractedMediaFeatures( 
    const std::wstring& path,  
    int width,  
    int height,  
    const std::wstring& autoColor,  
    const QVector<QPair<QColor, float>>& palettes)  
{ 
    std::wstring nPath = normalizePath(path); 
    MetaMemoryCache::instance().update(nPath, [width, height, &autoColor, &palettes](RuntimeMeta& meta) {
        meta.width = width; 
        meta.height = height; 
        meta.autoColor = autoColor; 
         
        meta.palettes.clear(); 
        for (const auto& p : palettes) { 
            meta.palettes.emplace_back(p.first, p.second); 
        } 
    });

    notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath)); 
}

void MetadataManager::setAddedAt(const std::wstring& path, long long addedAt, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    MetaMemoryCache::instance().update(nPath, [addedAt](RuntimeMeta& m) {
        m.added_at = addedAt;
    });
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::renameTag(const QString& oldName, const QString& newName) {
    if (oldName == newName) return;
    
    MetaMemoryCache::instance().forEachItemMut([&oldName, &newName](const std::wstring&, RuntimeMeta& meta) {
        if (meta.tags.contains(oldName)) {
            meta.tags.removeAll(oldName);
            if (!newName.isEmpty() && !meta.tags.contains(newName)) {
                meta.tags.append(newName);
            }
        }
    });
    
    notifyFullUIRebuild();
}

void MetadataManager::removeTag(const QString& tagName) {
    MetaMemoryCache::instance().forEachItemMut([&tagName](const std::wstring&, RuntimeMeta& meta) {
        if (meta.tags.contains(tagName)) {
            meta.tags.removeAll(tagName);
        }
    });
    
    notifyFullUIRebuild();
}

void MetadataManager::setColor(const std::wstring& path, const std::wstring& color, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    std::wstring normColor = UiHelper::normalizeColorHex(QString::fromStdWString(color)).toStdWString();
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = MetaDbRepository::instance().getDriveMeta(nPath);
        rec.color = normColor;
        MetaDbRepository::instance().saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);

    MetaMemoryCache::instance().update(nPath, [&normColor](RuntimeMeta& m) {
        m.manualColor = normColor;
    });

    QuarkMetaJsonStore::instance().updateItemMeta(nPath, [normColor](ItemMeta& item) {
        item.color = normColor;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setPinned(const std::wstring& path, bool pinned, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = MetaDbRepository::instance().getDriveMeta(nPath);
        rec.pinned = pinned;
        MetaDbRepository::instance().saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);

    MetaMemoryCache::instance().update(nPath, [pinned](RuntimeMeta& m) {
        m.pinned = pinned;
    });

    QuarkMetaJsonStore::instance().updateItemMeta(nPath, [pinned](ItemMeta& item) {
        item.pinned = pinned;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setTags(const std::wstring& path, const QStringList& tags, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);

    MetaMemoryCache::instance().update(nPath, [&tags](RuntimeMeta& m) {
        m.tags = tags;
    });

    std::vector<std::wstring> wTags;
    for (const QString& t : tags) {
        QString trimmed = t.trimmed();
        if (!trimmed.isEmpty()) {
            wTags.push_back(trimmed.toStdWString());
        }
    }
    QuarkMetaJsonStore::instance().updateItemMeta(path, [wTags](ItemMeta& item) {
        item.tags = wTags;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setNote(const std::wstring& path, const std::wstring& note, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = MetaDbRepository::instance().getDriveMeta(nPath);
        rec.note = note;
        MetaDbRepository::instance().saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);

    MetaMemoryCache::instance().update(nPath, [&note](RuntimeMeta& m) {
        m.note = note;
    });

    QuarkMetaJsonStore::instance().updateItemMeta(path, [note](ItemMeta& item) {
        item.note = note;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setURL(const std::wstring& path, const std::wstring& url, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    QFileInfo info(QString::fromStdWString(nPath));
    if (info.isRoot()) {
        auto rec = MetaDbRepository::instance().getDriveMeta(nPath);
        rec.url = url;
        MetaDbRepository::instance().saveDriveMeta(rec);
        if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
        return;
    }
    ensureActivated(nPath);

    MetaMemoryCache::instance().update(nPath, [&url](RuntimeMeta& m) {
        m.url = url;
    });

    QuarkMetaJsonStore::instance().updateItemMeta(path, [url](ItemMeta& item) {
        item.url = url;
    });

    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setEncrypted(const std::wstring& path, bool encrypted, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    MetaMemoryCache::instance().update(nPath, [encrypted](RuntimeMeta& m) {
        m.encrypted = encrypted;
    });
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setPalettes(const std::wstring& path, const QVector<QPair<QColor, float>>& palettes, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    std::vector<PaletteEntry> entries;
    for (int i = 0; i < palettes.size(); ++i) { entries.push_back(PaletteEntry(palettes[i].first, palettes[i].second)); }
    MetaMemoryCache::instance().update(nPath, [&entries](RuntimeMeta& m) {
        m.palettes = entries;
    });
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setItemVisualMetadata(const std::wstring& path, const std::wstring& color, const QVector<QPair<QColor, float>>& palettes, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    std::vector<PaletteEntry> entries;
    for (int i = 0; i < palettes.size(); ++i) { entries.push_back(PaletteEntry(palettes[i].first, palettes[i].second)); }
    
    MetaMemoryCache::instance().update(nPath, [&color, &entries](RuntimeMeta& meta) {
        meta.autoColor = color;
        meta.palettes = entries;
    });
    
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
}

void MetadataManager::setItemDimensions(const std::wstring& path, int width, int height) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    MetaMemoryCache::instance().update(nPath, [width, height](RuntimeMeta& meta) {
        meta.width = width;
        meta.height = height;
    });
}

QVector<QColor> MetadataManager::getPalettes(const std::wstring& path) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    RuntimeMeta meta = MetaMemoryCache::instance().getMeta(nPath);
    if (!meta.palettes.empty()) {
        QVector<QColor> colors;
        for (const auto& entry : meta.palettes) colors << entry.color;
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

                if (!MetaMemoryCache::instance().contains(curOld)) continue;

                RuntimeMeta meta = MetaMemoryCache::instance().getMeta(curOld);
                MetaMemoryCache::instance().remove(curOld);

                bool isFolder = meta.isFolder;

                std::wstring oldName, oldExt;
                parsePathComponents(curOld, isFolder, oldName, oldExt);

                std::wstring newName, newExt;
                parsePathComponents(curNew, isFolder, newName, newExt);
                meta.baseName = newName;
                meta.ext = newExt;

                MetaMemoryCache::instance().put(curNew, meta);

                ioTasks.push_back(pair);
                successCount++;
            }
        }

        for (const auto& pair : ioTasks) {
            QFileInfo oldFileInfo(QString::fromStdWString(pair.first));
            QFileInfo newFileInfo(QString::fromStdWString(pair.second));
            if (oldFileInfo.isDir()) {
                QuarkMetaJsonStore::instance().migrateFolderCache(oldFileInfo.absoluteFilePath(), newFileInfo.absoluteFilePath());
            } else {
                QuarkMetaJsonStore::instance().renameItem(oldFileInfo.absolutePath(), oldFileInfo.fileName(), newFileInfo.fileName());
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

                if (!MetaMemoryCache::instance().contains(curOld)) continue;

                RuntimeMeta meta = MetaMemoryCache::instance().getMeta(curOld);
                MetaMemoryCache::instance().remove(curOld);

                bool isFolder = meta.isFolder;

                std::wstring newName, newExt;
                parsePathComponents(curNew, isFolder, newName, newExt);
                meta.baseName = newName;
                meta.ext = newExt;

                MetaMemoryCache::instance().put(curNew, meta);

                ioTasks.push_back(pair);
            }
        }

        for (const auto& pair : ioTasks) {
            QFileInfo oldFileInfo(QString::fromStdWString(pair.first));
            QFileInfo newFileInfo(QString::fromStdWString(pair.second));
            if (oldFileInfo.isDir()) {
                QuarkMetaJsonStore::instance().migrateFolderCache(oldFileInfo.absoluteFilePath(), newFileInfo.absoluteFilePath());
            } else {
                QuarkMetaJsonStore::instance().renameItem(oldFileInfo.absolutePath(), oldFileInfo.fileName(), newFileInfo.fileName());
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

        for (size_t i = 0; i < MetaMemoryCache::NUM_SHARDS; ++i) {
            std::unique_lock<std::shared_mutex> shardLock(MetaMemoryCache::instance().shards()[i].mutex);
            for (auto it = MetaMemoryCache::instance().shards()[i].items.begin(); it != MetaMemoryCache::instance().shards()[i].items.end(); ) {
                if (it->first == nPath || it->first.find(nPath + L"\\") == 0 || it->first.find(nPath + L"/") == 0) {
                    StatisticsService::instance().purgeAsset(!it->second.tags.isEmpty(), false); 
                    it = MetaMemoryCache::instance().shards()[i].items.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

std::wstring MetadataManager::getVolumeSerialNumber(const std::wstring& path) {
    return VolumePathResolver::getVolumeSerialNumber(path);
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
        MetaMemoryCache::instance().put(p, rMeta);
    }
}

void MetadataManager::persistAsync(const std::wstring& path, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    
    RuntimeMeta rMeta = getMeta(nPath);
    parsePathComponents(nPath, rMeta.isFolder, rMeta.baseName, rMeta.ext);

    MetaMemoryCache::instance().put(nPath, rMeta);
        
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
    MetaMemoryCache::instance().update(nPath, [now](RuntimeMeta& meta) {
        meta.atime = static_cast<long long>(now);
    });
}

double MetadataManager::getCachedAtime(const std::wstring& path) {
    std::wstring nPath = normalizePath(path);
    RuntimeMeta meta = MetaMemoryCache::instance().getMeta(nPath);
    return static_cast<double>(meta.atime);
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
