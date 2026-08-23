#include <QFileInfo>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QtConcurrent>
#include <QThreadPool>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QImageReader>
#include <QSvgRenderer>
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
#include "sqlite3.h"
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

// 🚨 全系统权威 22 字段查询 SQL
static const char* kSqlSelectAllMeta = 
    "SELECT folder_id, path, is_folder, rating, color, tags, note, url, "
    "ctime, mtime, atime, file_size, palettes, is_trash, original_path, "
    "width, height, auto_color, base_name, ext, added_at, sha256 "
    "FROM metadata";

// 🚨 全系统权威 22 字段插入/更新 SQL
static const char* kSqlInsertMeta = 
    "INSERT OR REPLACE INTO metadata (folder_id, path, is_folder, rating, color, tags, note, url, "
    "ctime, mtime, atime, file_size, palettes, is_trash, original_path, "
    "width, height, auto_color, base_name, ext, added_at, sha256) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

static void bindMetaHelper(sqlite3_stmt* stmt, const std::wstring& path, const RuntimeMeta& meta) {
    sqlite3_bind_text(stmt, 1, meta.folderId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 2, path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, meta.isFolder ? 1 : 0);
    sqlite3_bind_int(stmt, 4, meta.rating);
    sqlite3_bind_text16(stmt, 5, meta.manualColor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 6, meta.tags.join(",").toStdWString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 7, meta.note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 8, meta.url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 9, meta.ctime);
    sqlite3_bind_int64(stmt, 10, meta.mtime);
    sqlite3_bind_int64(stmt, 11, meta.atime);
    sqlite3_bind_int64(stmt, 12, meta.fileSize);

    QJsonArray arr;
    for (const auto& pe : meta.palettes) {
        QJsonObject obj;
        obj["color"] = pe.color.name();
        obj["ratio"] = (double)pe.ratio;
        arr.append(obj);
    }
    QByteArray ba = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    sqlite3_bind_blob(stmt, 13, ba.constData(), ba.size(), SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 14, meta.isTrash ? 1 : 0);
    sqlite3_bind_text16(stmt, 15, meta.originalPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 16, meta.width);
    sqlite3_bind_int(stmt, 17, meta.height);
    sqlite3_bind_text16(stmt, 18, meta.autoColor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 19, meta.baseName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text16(stmt, 20, meta.ext.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 21, meta.added_at);
    sqlite3_bind_text(stmt, 22, meta.sha256.c_str(), -1, SQLITE_TRANSIENT);
}

// --- Helper Functions ---


std::wstring MetadataManager::normalizePath(const std::wstring& path) {
    if (path.empty()) return L"";
    // 2026-06-xx 物理对账优化：Windows 环境下路径不区分大小写，
    // 统一转换为全小写以确保内存缓存 (std::unordered_map) 的 Key 匹配一致性，彻底消除“幽灵项”。
    QString qp = QDir::toNativeSeparators(QDir::cleanPath(QString::fromStdWString(path))).toLower();
    if (qp.length() == 2 && qp.endsWith(':')) qp += '\\';
    return qp.toStdWString();
}

std::string MetadataManager::generateFallbackFolderId(const std::wstring& vol, const std::wstring& frn) {
    if (vol.empty() || frn.empty()) return "";
    std::string result = "FRN:";
    result.append(QString::fromStdWString(vol).toUpper().toStdString());
    result.append(":");
    result.append(QString::fromStdWString(frn).toUpper().toStdString());
    return result;
}

std::string MetadataManager::generateDeterministicFolderId(const std::wstring& path) {
    if (path.empty()) return "";
    std::wstring nPath = MetadataManager::normalizePath(path);
    std::wstring vol = MetadataManager::getVolumeSerialNumber(nPath);
    
    std::wstring seedW(vol);
    seedW.append(L":");
    seedW.append(nPath);

    QByteArray seed = QString::fromStdWString(seedW).toUtf8();
    QByteArray hash = QCryptographicHash::hash(seed, QCryptographicHash::Sha256);
    
    std::string result = "PATHURL:";
    result.append(hash.left(16).toHex().toUpper().toStdString());
    return result;
}

std::wstring MetadataManager::generateDeterministicFrn(const std::wstring& path) {
    if (path.empty()) return L"VIRTUAL_EMPTY";
    QByteArray hash = QCryptographicHash::hash(QString::fromStdWString(path).toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.left(8).toHex().toUpper()).toStdWString();
}

// --- MetadataManager Implementation ---

MetadataManager& MetadataManager::instance() {
    static MetadataManager inst;
    return inst;
}

MetadataManager::MetadataManager(QObject* parent) : QObject(parent) {
    // [RCU 内存快照初始化]：分配空快照，防空指针异常
    // m_shards auto initialized
    m_uiSignalTimer = new QTimer(this);
    m_uiSignalTimer->setInterval(200); // 200ms 时间窗口
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

    // 2026-06-xx 物理加固：监听程序退出信号
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [this]() {
        // 2026-06-xx 物理切换：强制刷新 SQLite 到磁盘
        DatabaseManager::instance().shutdown();
        AppConfig::instance().setValue("System/LastCleanShutdown", true);
        AppConfig::instance().sync();
    });
}


void MetadataManager::initFromDatabase() {
    // 2026-06-xx 物理加固：防止重复初始化
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if (m_loaded) return;
    }

    DatabaseManager::instance().init();
    
    std::unordered_map<std::wstring, RuntimeMeta> tempCache;
    std::unordered_map<std::string, std::wstring> tempFidToPath;
    std::unordered_map<std::wstring, std::vector<std::wstring>> tempParentToChildren;
    std::unordered_map<std::wstring, double> tempFolderProgressCache;

    auto loadFromDb = [&](sqlite3* db) {
        if (!db) return;
        sqlite3_stmt* stmt = nullptr;

        // 1. 读取主元数据表
        if (sqlite3_prepare_v2(db, kSqlSelectAllMeta, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                RuntimeMeta rm;
                const char* fid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (fid) rm.folderId = fid;

                const wchar_t* wpath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 1));
                std::wstring path = normalizePath(wpath ? wpath : L"");

                rm.isFolder = sqlite3_column_int(stmt, 2) != 0;
                rm.rating = sqlite3_column_int(stmt, 3);
                
                const wchar_t* color = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 4));
                if (color) rm.manualColor = color;

                // 🚨 Column 5: 标签字段提取与物理路径数据清洗 (Data Sanitizer)
                const wchar_t* wtags = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 5));
                QString tagsStr = wtags ? QString::fromWCharArray(wtags) : "";
                QStringList rawTags = tagsStr.split(",", Qt::SkipEmptyParts);
                QStringList cleanTags;
                bool isDirtyData = false;

                for (const QString& t : rawTags) {
                    QString trimmed = t.trimmed();
                    // 物理清洗拦截：过滤包含盘符冒号(:)、路径斜杠(\ or /) 或 .arc 胶囊后缀的错误路径数据
                    if (trimmed.contains(":\\") || trimmed.contains(":/") || trimmed.contains(".arc", Qt::CaseInsensitive)) {
                        isDirtyData = true;
                        continue; // 强行丢弃垃圾标签
                    }
                    cleanTags.append(trimmed);
                }
                rm.tags = cleanTags;

                // Column 6 ~ 21 严格按 kSqlSelectAllMeta 顺序列提取
                const wchar_t* note = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 6));
                if (note) rm.note = note;
                
                const wchar_t* url = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 7));
                if (url) rm.url = url;

                rm.ctime = sqlite3_column_int64(stmt, 8);
                rm.mtime = sqlite3_column_int64(stmt, 9);
                rm.atime = sqlite3_column_int64(stmt, 10);
                rm.fileSize = sqlite3_column_int64(stmt, 11);

                const void* paletteBlob = sqlite3_column_blob(stmt, 12);
                int paletteSize = sqlite3_column_bytes(stmt, 12);
                if (paletteBlob && paletteSize > 0) {
                    QByteArray ba(reinterpret_cast<const char*>(paletteBlob), paletteSize);
                    QJsonDocument doc = QJsonDocument::fromJson(ba);
                    QJsonArray arr = doc.array();
                    for (const auto& v : arr) {
                        QJsonObject obj = v.toObject();
                        PaletteEntry pe;
                        pe.color = QColor(obj["color"].toString());
                        pe.ratio = (float)obj["ratio"].toDouble();
                        rm.palettes.push_back(pe);
                    }
                }

                rm.isTrash = sqlite3_column_int(stmt, 13) != 0;
                const wchar_t* wOrigPath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 14));
                if (wOrigPath) rm.originalPath = wOrigPath;

                rm.width = sqlite3_column_int(stmt, 15);
                rm.height = sqlite3_column_int(stmt, 16);

                const wchar_t* autoColor = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 17));
                if (autoColor) rm.autoColor = autoColor;

                const wchar_t* wBaseName = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 18));
                if (wBaseName) rm.baseName = wBaseName;

                const wchar_t* wExt = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 19));
                if (wExt) rm.ext = wExt;

                rm.added_at = sqlite3_column_int64(stmt, 20);

                const char* hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21));
                if (hash) rm.sha256 = hash;

                tempCache[path] = rm;
                if (!rm.folderId.empty()) tempFidToPath[rm.folderId] = path;

                // 维护树级索引...
                std::wstring parentPath = QDir::toNativeSeparators(QFileInfo(QString::fromStdWString(path)).absolutePath()).toStdWString();
                parentPath = normalizePath(parentPath);
                if (parentPath != path) {
                    tempParentToChildren[parentPath].push_back(path);
                }
            }
            sqlite3_finalize(stmt);
        }

    }; // 🚨 正确：loadFromDb 闭包在这里统一结束！

    // 加载全局库
    loadFromDb(DatabaseManager::instance().getGlobalDb());

    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_folderIdToPath = tempFidToPath;
        m_parentToChildren = tempParentToChildren;

        for (auto& entry : m_parentToChildren) {
            std::sort(entry.second.begin(), entry.second.end());
            entry.second.erase(std::unique(entry.second.begin(), entry.second.end()), entry.second.end());
        }

        m_loaded = true;
        for (const auto& pair : tempCache) {
            size_t idx = getShardIndex(pair.first);
            std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            m_shards[idx].items[pair.first] = pair.second;
        }
    }

    notifyUI(RefreshLevel::FullRebuild);
}

void MetadataManager::notifyUI(RefreshLevel level, const QString& path) {
    switch (level) {
        case RefreshLevel::CountsOnly:
        case RefreshLevel::FullRebuild:
            notifyFullUIRebuild();
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

void MetadataManager::notifyFullUIRebuild() {
    if (m_isInternalOperating) return; // 2026-xx-xx 按照 Plan-105：操作期间拦截冗余刷新信号
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_pendingUiPaths.insert("__RELOAD_ALL__");
    }
    QMetaObject::invokeMethod(this, "triggerUiSignalTimer", Qt::QueuedConnection);
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

std::wstring MetadataManager::getPathByFolderId(const std::string& fid) {
    if (fid.empty()) return L"";
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_folderIdToPath.find(fid);
    return (it != m_folderIdToPath.end()) ? it->second : L"";
}

void MetadataManager::ensureActivated(const std::wstring& nPath) {
    // 1. 读锁检查 (快速路径)
    {
        size_t idx = getShardIndex(nPath);
        std::shared_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        if (m_shards[idx].items.find(nPath) != m_shards[idx].items.end()) return;
    }

    // 2. 锁外同步获取物理属性 (耗时 I/O 操作)
    // 2026-07-xx 按照 Plan-88：杜绝在 unique_lock 期间执行 Win32 API 访问
    RuntimeMeta rm;
    std::wstring frn;
    std::wstring type;
    
    // 自愈与健壮性改造：若 Win32 原生 API 失败（如 Linux Sandbox、共享访问冲突等），提供 QFileInfo 完美兜底，确保激活成功 
    bool success = fetchWinApiMetadataDirect(nPath, rm.folderId, &frn, &rm.fileSize, &type, &rm.ctime, &rm.mtime, &rm.atime);
    if (!success) {
        QFileInfo qinfo(QString::fromStdWString(nPath));
        if (qinfo.exists()) {
            rm.fileSize = qinfo.size();
            rm.isFolder = qinfo.isDir();
            rm.ctime = qinfo.birthTime().toMSecsSinceEpoch();
            rm.mtime = qinfo.lastModified().toMSecsSinceEpoch();
            rm.atime = qinfo.lastRead().toMSecsSinceEpoch();
            rm.folderId = generateDeterministicFolderId(nPath);
            success = true;
        }
    } else {
        rm.isFolder = (type == L"folder");
    }

    if (success) {
        // 3. 写锁写入缓存
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        size_t idx = getShardIndex(nPath);
        {
            std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            if (m_shards[idx].items.count(nPath)) return; // 二次检查防止竞态
        }

        // 共享元数据逻辑 (FID 关联)
        if (!rm.folderId.empty() && m_folderIdToPath.count(rm.folderId)) {
            std::wstring existPath = m_folderIdToPath[rm.folderId];
            size_t existIdx = getShardIndex(existPath);
            std::shared_lock<std::shared_mutex> shardLock(m_shards[existIdx].mutex);
            auto existingIt = m_shards[existIdx].items.find(existPath);
            if (existingIt != m_shards[existIdx].items.end()) {
                const RuntimeMeta& existing = existingIt->second;
                rm.rating    = existing.rating;
                rm.manualColor = existing.manualColor;
                rm.autoColor = existing.autoColor;
                rm.tags      = existing.tags;
                rm.note      = existing.note;
                rm.url       = existing.url;
                rm.width     = existing.width;
                rm.height    = existing.height;
                rm.palettes  = existing.palettes;
                
            }
        }

        {
            std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            m_shards[idx].items[nPath] = rm;
        }
        if (!rm.folderId.empty()) {
            m_folderIdToPath[rm.folderId] = nPath;

            // Plan-124: 维护树级索引
            std::wstring parentPath = QDir::toNativeSeparators(QFileInfo(QString::fromStdWString(nPath)).absolutePath()).toStdWString();
            parentPath = normalizePath(parentPath);
            if (parentPath != nPath) {
                auto& children = m_parentToChildren[parentPath];
                if (std::find(children.begin(), children.end(), nPath) == children.end()) {
                    children.push_back(nPath);
                }
            }

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

    // 🚨 纯磁盘模式：直接原子写入所在物理目录的 .QuarkMeta.json
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
    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
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
    RuntimeMeta metaCopy; 
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
        metaCopy = meta; 
    } 

    // 仅发起 1 次数据库事务 
    DatabaseManager::instance().enqueueSyncTask([this, nPath, metaCopy]() { 
        sqlite3* db = DatabaseManager::instance().getGlobalDb(); 
        if (!db) return; 

        SqlTransaction trans(db); 
        const char* sql = "UPDATE metadata SET width = ?, height = ?, auto_color = ?, palettes = ? WHERE folder_id = ?"; 
        sqlite3_stmt* stmt = nullptr; 
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) { 
            sqlite3_bind_int(stmt, 1, metaCopy.width); 
            sqlite3_bind_int(stmt, 2, metaCopy.height); 
            sqlite3_bind_text16(stmt, 3, metaCopy.autoColor.c_str(), -1, SQLITE_TRANSIENT); 
             
            // 序列化调色盘 JSON 
            QJsonArray arr; 
            for (const auto& pe : metaCopy.palettes) { 
                QJsonObject obj; obj["color"] = pe.color.name(); obj["ratio"] = (double)pe.ratio; arr.append(obj); 
            } 
            QByteArray ba = QJsonDocument(arr).toJson(QJsonDocument::Compact); 
            sqlite3_bind_blob(stmt, 4, ba.constData(), ba.size(), SQLITE_TRANSIENT); 
            sqlite3_bind_text(stmt, 5, metaCopy.folderId.c_str(), -1, SQLITE_TRANSIENT); 

            sqlite3_step(stmt); 
            sqlite3_finalize(stmt); 
        } 
        trans.commit(); 
    }); 

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
    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
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
    
    persistBatchAsync(affectedPaths);
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
    
    persistBatchAsync(affectedPaths);
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

    // 🚨 纯磁盘模式：直接原子写入所在物理目录的 .QuarkMeta.json
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

    // 🚨 纯磁盘模式：直接原子写入所在物理目录的 .QuarkMeta.json
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

    // 🚨 纯磁盘模式：转换标签并直接原子写入所在物理目录的 .QuarkMeta.json
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

    // 🚨 纯磁盘模式：直接原子写入所在物理目录的 .QuarkMeta.json
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

    // 🚨 纯磁盘模式：直接原子写入所在物理目录的 .QuarkMeta.json
    QuarkMetaJson::updateItemMeta(nPath, [url](ItemMeta& item) {
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
    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
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
    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
}

void MetadataManager::setItemVisualMetadata(const std::wstring& path, const std::wstring& color, const QVector<QPair<QColor, float>>& palettes, bool notify) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    ensureActivated(nPath);
    std::vector<PaletteEntry> entries;
    for (int i = 0; i < palettes.size(); ++i) { entries.push_back(PaletteEntry(palettes[i].first, palettes[i].second)); }
    
    bool isFolder = false;
    {
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        RuntimeMeta& meta = m_shards[idx].items[nPath];
        meta.autoColor = color;
        meta.palettes = entries;
        isFolder = meta.isFolder;
    }
    
    
    if (notify) notifyUI(RefreshLevel::PathUpdate, QString::fromStdWString(nPath));
    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath);
    });
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
    DatabaseManager::instance().enqueueSyncTask([this, nPath]() {
        persistAsync(nPath, false);
    });
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

    // 1. 全局开启信号锁，挂起并发 UI 刷新
    setInternalOperating(true);

    // 2. 将全量改名合并为【唯一 1 个后台异步线程】
    (void)QtConcurrent::run([this, rawPathPairs, onCompleted]() {
        int successCount = 0;

        // A. 路径归一化预处理（解决 Windows 斜杠/盘符大小写造成的幽灵数据）
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

        // B. 内存分片节点批量替换
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

                std::string fid = meta.folderId;
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
                if (!fid.empty()) m_folderIdToPath[fid] = curNew;

                ioTasks.push_back(pair);
                successCount++;
            }
        }

        // B2. 在锁外执行磁盘元数据文件的重命名与迁移
        for (const auto& pair : ioTasks) {
            QFileInfo oldFileInfo(QString::fromStdWString(pair.first));
            QFileInfo newFileInfo(QString::fromStdWString(pair.second));
            if (oldFileInfo.isDir()) {
                QuarkMetaJson::migrateFolderCache(oldFileInfo.absoluteFilePath(), newFileInfo.absoluteFilePath());
            } else {
                QuarkMetaJson::renameItem(oldFileInfo.absolutePath(), oldFileInfo.fileName(), newFileInfo.fileName());
            }
        }

        // C. SQLite 数据库分库批量提交大事务（彻底消除 SQLITE_BUSY 报错）
        struct DbBatchRenameTask {
            std::string fid;
            std::wstring newPath;
            std::wstring newName;
            std::wstring newExt;
        };
        std::map<sqlite3*, std::vector<DbBatchRenameTask>> groupedTasks;
        for (const auto& pair : normalizedPairs) {
            const std::wstring& curNew = pair.second;
            std::string fid;
            std::wstring newName, newExt;
            {
                size_t idx = getShardIndex(curNew);
                std::shared_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
                auto it = m_shards[idx].items.find(curNew);
                if (it != m_shards[idx].items.end()) {
                    fid = it->second.folderId;
                    newName = it->second.baseName;
                    newExt = it->second.ext;
                }
            }
            if (fid.empty()) continue;

            std::wstring volSerial = getVolumeSerialNumber(curNew);
            QString letter = (curNew.length() >= 2 && curNew[1] == L':') ? QString::fromWCharArray(&curNew[0], 1) : "";
            sqlite3* db = DatabaseManager::instance().getGlobalDb();
            if (db) {
                groupedTasks[db].push_back({fid, curNew, newName, newExt});
            }
        }

        const char* updSql = "UPDATE metadata SET path = ?, base_name = ?, ext = ? WHERE folder_id = ?";
        for (auto& entry : groupedTasks) {
            sqlite3* targetDb = entry.first;
            const auto& tasks = entry.second;

            SqlTransaction trans(targetDb); // 【仅开启 1 次事务】
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(targetDb, updSql, -1, &stmt, nullptr) == SQLITE_OK) {
                for (const auto& task : tasks) {
                    sqlite3_bind_text16(stmt, 1, task.newPath.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text16(stmt, 2, task.newName.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text16(stmt, 3, task.newExt.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(stmt, 4, task.fid.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_step(stmt);
                    sqlite3_reset(stmt);
                }
                sqlite3_finalize(stmt);
            }
            trans.commit(); // 【仅提交 1 次事务】
        }


        // E. 清理操作标志位，安全回到 UI 主线程通知
        setInternalOperating(false);

        QMetaObject::invokeMethod(qApp, [this, successCount, onCompleted]() {
            notifyFullUIRebuild(); // 【仅发射 1 次全量 UI 刷新信号】
            if (onCompleted) onCompleted(successCount);
        }, Qt::QueuedConnection);
    });
}


void MetadataManager::renameItem(const std::wstring& oldPath, const std::wstring& newPath) {
    std::wstring nOld = normalizePath(oldPath);
    std::wstring nNew = normalizePath(newPath);
    if (nOld == nNew) return;

    // 2026-08-xx 按照性能优化要求：将级联更名逻辑移至后台线程，杜绝大目录重命名阻塞主线程 (Plan-128)
    (void)QtConcurrent::run([this, nOld, nNew]() {
        std::vector<std::pair<std::wstring, std::wstring>> itemsToRename;
        std::vector<std::pair<std::wstring, std::wstring>> ioTasks;
        
        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            // 1. 深度收集所有子孙路径
            forEachCachedItem([&](const std::wstring& p, const RuntimeMeta&) {
                if (p == nOld) {
                    itemsToRename.push_back({p, nNew});
                } else if (p.find(nOld + L"\\") == 0 || p.find(nOld + L"/") == 0) {
                    std::wstring relative = p.substr(nOld.length());
                    itemsToRename.push_back({p, nNew + relative});
                }
            });

            if (itemsToRename.empty()) return;

            // 2. 优化：先一次性切断根级树索引关系，防止循环内 O(K^2) 的 std::remove 开销
            std::wstring rootOldParent = normalizePath(QDir::toNativeSeparators(QFileInfo(QString::fromStdWString(nOld)).absolutePath()).toStdWString());
            if (m_parentToChildren.count(rootOldParent)) {
                auto& children = m_parentToChildren[rootOldParent];
                children.erase(std::remove(children.begin(), children.end(), nOld), children.end());
                if (children.empty()) m_parentToChildren.erase(rootOldParent);
            }

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

                std::string fid = meta.folderId;
                bool isFolder = meta.isFolder;

                // [树级索引维护] - 内部项仅移除
                if (curOld != nOld) {
                    m_parentToChildren.erase(curOld); 
                }

                // 3. 缓存迁移
                std::wstring newName, newExt;
                parsePathComponents(curNew, isFolder, newName, newExt);
                meta.baseName = newName;
                meta.ext = newExt;

                size_t newIdx = getShardIndex(curNew);
                {
                    std::unique_lock<std::shared_mutex> shardLock(m_shards[newIdx].mutex);
                    m_shards[newIdx].items[curNew] = meta;
                }
                if (!fid.empty()) m_folderIdToPath[fid] = curNew;

                std::wstring curNewParent = normalizePath(QDir::toNativeSeparators(QFileInfo(QString::fromStdWString(curNew)).absolutePath()).toStdWString());
                if (curNewParent != curNew) {
                    auto& children = m_parentToChildren[curNewParent];
                    if (std::find(children.begin(), children.end(), curNew) == children.end()) {
                        children.push_back(curNew);
                    }
                }


                ioTasks.push_back(pair);
            }
        }

        // 在全局锁外安全执行磁盘 JSON 重命名与迁移
        for (const auto& pair : ioTasks) {
            QFileInfo oldFileInfo(QString::fromStdWString(pair.first));
            QFileInfo newFileInfo(QString::fromStdWString(pair.second));
            if (oldFileInfo.isDir()) {
                QuarkMetaJson::migrateFolderCache(oldFileInfo.absoluteFilePath(), newFileInfo.absoluteFilePath());
            } else {
                QuarkMetaJson::renameItem(oldFileInfo.absolutePath(), oldFileInfo.fileName(), newFileInfo.fileName());
            }
        }

        // 4. 物理数据库批量同步 (Plan-128: 引入事务保护)
        // 极致优化：预取根路径的卷信息，避免在循环中重复执行耗时的 Win32 磁盘查询
        std::wstring volSerial = getVolumeSerialNumber(nNew);
        QString letter = (nNew.length() >= 2 && nNew[1] == L':') ? QString::fromWCharArray(&nNew[0], 1) : "";
        sqlite3* memDb = DatabaseManager::instance().getGlobalDb();
        
        struct DbItemRenameTask {
            std::string fid;
            std::wstring newPath;
            std::wstring newName;
            std::wstring newExt;
        };
        std::map<sqlite3*, std::vector<DbItemRenameTask>> groupedSyncTasks;
        for (const auto& pair : itemsToRename) {
            const std::wstring& curNew = pair.second;
            std::string fid;
            std::wstring newName, newExt;
            {
                size_t idx = getShardIndex(curNew);
                std::shared_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
                auto it = m_shards[idx].items.find(curNew);
                if (it != m_shards[idx].items.end()) {
                    fid = it->second.folderId;
                    newName = it->second.baseName;
                    newExt = it->second.ext;
                }
            }
            if (fid.empty()) continue;

            if (memDb) {
                groupedSyncTasks[memDb].push_back({fid, curNew, newName, newExt});
            }
        }

        const char* updSql = "UPDATE metadata SET path = ?, base_name = ?, ext = ? WHERE folder_id = ?";
        for (auto& entry : groupedSyncTasks) {
            sqlite3* targetDb = entry.first;
            auto& tasks = entry.second;

            // [Plan-131 方案 A] 直连磁盘模式，无需重复异步分发
            SqlTransaction trans(targetDb);
            sqlite3_stmt* memStmt;
            if (sqlite3_prepare_v2(targetDb, updSql, -1, &memStmt, nullptr) == SQLITE_OK) {
                for (const auto& task : tasks) {
                    sqlite3_bind_text16(memStmt, 1, task.newPath.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text16(memStmt, 2, task.newName.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text16(memStmt, 3, task.newExt.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(memStmt, 4, task.fid.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_step(memStmt);
                    sqlite3_reset(memStmt);
                }
                sqlite3_finalize(memStmt);
            }
            trans.commit();
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
    std::wstring volSerial = getVolumeSerialNumber(nPath);
    QString letter = (nPath.length() >= 2 && nPath[1] == L':') ? QString::fromWCharArray(&nPath[0], 1) : "";
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    
    int totalDelta = 0;
    std::vector<std::string> fids;
    
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        // 1. 优化：先从父级索引中一次性移除根路径，避免循环内 O(K^2)
        std::wstring rootParent = normalizePath(QDir::toNativeSeparators(QFileInfo(QString::fromStdWString(nPath)).absolutePath()).toStdWString());
        if (m_parentToChildren.count(rootParent)) {
            auto& children = m_parentToChildren[rootParent];
            children.erase(std::remove(children.begin(), children.end(), nPath), children.end());
            if (children.empty()) m_parentToChildren.erase(rootParent);
        }

        for (size_t i = 0; i < NUM_SHARDS; ++i) {
            std::unique_lock<std::shared_mutex> shardLock(m_shards[i].mutex);
            for (auto it = m_shards[i].items.begin(); it != m_shards[i].items.end(); ) {
                if (it->first == nPath || it->first.find(nPath + L"\\") == 0 || it->first.find(nPath + L"/") == 0) {
                    std::wstring curPath = it->first;

                    if (!it->second.isTrash) {
                        totalDelta--;
                        StatisticsService::instance().purgeAsset({}, !it->second.tags.isEmpty(), it->second.isTrash); 
                    }
                    if (!it->second.folderId.empty()) {
                        std::string fid = it->second.folderId;
                        bool isFolder = it->second.isFolder;
                        fids.push_back(fid);
                        m_folderIdToPath.erase(fid);

                        m_parentToChildren.erase(curPath);
                    }
                    it = m_shards[i].items.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

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

}


void MetadataManager::markAsTrash(const std::wstring& path, bool isTrash, const std::wstring& origPath) {
    std::wstring nPath = MetadataManager::normalizePath(path);
    std::string fid;
    fetchWinApiMetadataDirect(nPath, fid);

    bool changed = false;
        {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        if (!fid.empty() && m_folderIdToPath.count(fid)) {
            std::wstring oldPath = m_folderIdToPath[fid];
            if (oldPath != nPath) {
                size_t oldIdx = getShardIndex(oldPath);
                std::unique_lock<std::shared_mutex> shardLock(m_shards[oldIdx].mutex);
                auto itOld = m_shards[oldIdx].items.find(oldPath);
                if (itOld != m_shards[oldIdx].items.end()) {

                    std::wstring oldParent = QDir::toNativeSeparators(QFileInfo(QString::fromStdWString(oldPath)).absolutePath()).toStdWString();
                    oldParent = normalizePath(oldParent);
                    if (m_parentToChildren.count(oldParent)) {
                        auto& children = m_parentToChildren[oldParent];
                        children.erase(std::remove(children.begin(), children.end(), oldPath), children.end());
                        if (children.empty()) m_parentToChildren.erase(oldParent);
                    }

                    m_shards[oldIdx].items.erase(itOld);
                }
            }
        }
    }
    
    ensureActivated(nPath); 

    bool isFolder = false;
    bool oldEmpty = false;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        size_t idx = getShardIndex(nPath);
        std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        auto it = m_shards[idx].items.find(nPath);
        if (it != m_shards[idx].items.end()) {
            if (it->second.isTrash != isTrash) {
                it->second.isTrash = isTrash;
                if (isTrash && !origPath.empty()) it->second.originalPath = origPath;
                changed = true;
                                isFolder = it->second.isFolder;
                oldEmpty = it->second.tags.isEmpty();
            }
        }
        if (!fid.empty()) m_folderIdToPath[fid] = nPath;
    }
    
    if (changed) {

        // 实时通知统计服务回收站状态变更 
        StatisticsService::instance().notifyAssetTrashChanged(isTrash, oldEmpty); 

        persistAsync(nPath);
        
        // 2026-06-xx 物理修复：状态变更后必须强制发射信号，驱动侧边栏重数一遍
        notifyUI(RefreshLevel::FullRebuild);
    }
}

void MetadataManager::setTrash(const std::wstring& path, bool isTrash) {
    std::wstring nPath = normalizePath(path);
    bool changed = false;
    bool isFolder = false;
    bool oldEmpty = false;
    std::string fid;
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        size_t idx = getShardIndex(nPath);
        {
            std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
            auto it = m_shards[idx].items.find(nPath);
            if (it != m_shards[idx].items.end()) {
                if (it->second.isTrash != isTrash) {
                    it->second.isTrash = isTrash;
                    if (!isTrash) {
                        it->second.originalPath = L""; // Clear on restore
                    }
                    changed = true;
                    isFolder = it->second.isFolder;
                    oldEmpty = it->second.tags.isEmpty();
                    fid = it->second.folderId;
                }
            }
        }
    }
    persistAsync(nPath);
}

void MetadataManager::deletePermanently(const std::wstring& path) {
    std::wstring nPath = MetadataManager::normalizePath(path);

    // 执行彻底根除 (removeMetadataSync 会级联擦除 SQLite metadata 与 category_items)
    removeMetadataSync(nPath);

    // 广播 UI 全量刷新信号
    notifyUI(RefreshLevel::FullRebuild);
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



bool MetadataManager::fetchWinApiMetadataDirect(const std::wstring& path, std::string& outId128, std::wstring* outFrn, long long* outSize, std::wstring* outType, long long* outCtime, long long* outMtime, long long* outAtime) {
    HANDLE hFile = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    std::wstring vol = getVolumeSerialNumber(path);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (outFrn) *outFrn = MetadataManager::generateDeterministicFrn(path);
        outId128 = MetadataManager::generateDeterministicFolderId(path);
        return false;
    }
    BY_HANDLE_FILE_INFORMATION basicInfo;
    if (GetFileInformationByHandle(hFile, &basicInfo)) {
        wchar_t frnBuf[17];
        unsigned long long fullFrn = (static_cast<unsigned long long>(basicInfo.nFileIndexHigh) << 32) | basicInfo.nFileIndexLow;
        swprintf(frnBuf, 17, L"%016llX", fullFrn);
        if (outFrn) *outFrn = frnBuf;
        outId128 = MetadataManager::generateFallbackFolderId(vol, frnBuf);
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
    instance().ensureActivated(path);
}

std::string MetadataManager::getFolderIdSync(const std::wstring& path) {
    std::string fid;
    if (!fetchWinApiMetadataDirect(path, fid, nullptr)) fid = MetadataManager::generateDeterministicFolderId(path);
    return fid;
}

void MetadataManager::persistBatchAsync(const std::vector<std::wstring>& paths, bool authorized) {
    WriteGuard guard;
    if (paths.empty()) return;

    // 1. 按数据库对路径进行分组，以支持大事务写入
    struct BatchTask {
        sqlite3* memDb;
        std::vector<std::wstring> groupPaths;
    };
    std::map<sqlite3*, std::vector<std::wstring>> groups;

    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (db) groups[db] = paths;

    for (auto& entry : groups) {
        sqlite3* memDb = entry.first;
        const auto& groupPaths = entry.second;

        // 2. 内存库批量提交 (使用 SqlTransaction 确保原子性与速度)
        SqlTransaction trans(memDb);
        std::vector<std::pair<std::wstring, RuntimeMeta>> recordsToSync;

        for (const auto& p : groupPaths) {
            RuntimeMeta rMeta = getMeta(p);
            if (rMeta.folderId.empty()) continue;

            // 准入检查
            bool isNew = true;
            sqlite3_stmt* checkStmt;
            if (sqlite3_prepare_v2(memDb, "SELECT 1 FROM metadata WHERE folder_id = ?", -1, &checkStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(checkStmt, 1, rMeta.folderId.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(checkStmt) == SQLITE_ROW) isNew = false;
                sqlite3_finalize(checkStmt);
            }

            if (isNew && !authorized) {
                
            }

            // 重新解析出最新基名与后缀塞入
            parsePathComponents(p, rMeta.isFolder, rMeta.baseName, rMeta.ext);

            // Pure disk mode: bypass writing non-root item metadata to global.db's metadata table.
            {
                size_t idx = getShardIndex(p);
                std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
                m_shards[idx].items[p] = rMeta;
            }
            recordsToSync.push_back({p, rMeta});
        }
        trans.commit();
    }

    // 关键操作后即时异步落盘
    DatabaseManager::instance().enqueueSyncTask([]() {
        DatabaseManager::instance().flushAll();
    });
}

void MetadataManager::persistAsync(const std::wstring& path, bool notify, bool authorized) {
    WriteGuard guard;
    std::wstring nPath = MetadataManager::normalizePath(path);
    
    RuntimeMeta rMeta = getMeta(nPath);
    // 写入前现算一次持久化基名与后缀
    parsePathComponents(nPath, rMeta.isFolder, rMeta.baseName, rMeta.ext);
    
    sqlite3* memDb = DatabaseManager::instance().getGlobalDb();
    if (!memDb) {
        return;
    }

    // 1. 数据库记录操作
    bool isNew = true;
    {
        sqlite3_stmt* checkStmt;
        if (sqlite3_prepare_v2(memDb, "SELECT 1 FROM metadata WHERE folder_id = ?", -1, &checkStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(checkStmt, 1, rMeta.folderId.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(checkStmt) == SQLITE_ROW) isNew = false;
            sqlite3_finalize(checkStmt);
        }
    }

    if (isNew && !authorized) {
        
    }

    sqlite3_stmt* memStmt;
    if (sqlite3_prepare_v2(memDb, kSqlInsertMeta, -1, &memStmt, nullptr) == SQLITE_OK) {
        bindMetaHelper(memStmt, nPath, rMeta);
        if (sqlite3_step(memStmt) == SQLITE_DONE) {
            {
                
                size_t idx = getShardIndex(nPath);
                std::unique_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
                m_shards[idx].items[nPath] = rMeta;
            }
        }
        sqlite3_finalize(memStmt);
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
            // 统一转换为小写
            std::transform(outExt.begin(), outExt.end(), outExt.begin(), ::towlower);
        } else {
            outExt = L"";
        }
    }
}

std::wstring MetadataManager::getVolumeFromFolderId(const std::string& fid) {
    if (fid.empty()) return L"UNKNOWN";
    if (fid.find("FRN:") == 0) {
        size_t secondColon = fid.find(':', 4);
        if (secondColon != std::string::npos) {
            std::string vol = fid.substr(4, secondColon - 4);
            return QString::fromStdString(vol).toStdWString();
        }
    }
    return L"UNKNOWN";
}


QStringList MetadataManager::searchInCache(const QString& keyword, const QString& scopeSource, int categoryId, const QString& parentPath) {
    Q_UNUSED(categoryId);
    // [Plan-26] 彻底废除 O(N) 全量内存线性遍历，全面拥抱 FTS5 trigram 模糊检索引擎 + 内存 O(1) 快速反查
    QStringList results; if (keyword.isEmpty()) return results;
    
    // 2026-07-xx 按照方案计划：实现范围感知搜索
    std::unordered_set<std::string> scopeFids;
    bool hasScope = false;


    // 2026-07-xx 物理对账：规范化父路径前缀用于导航范围搜索
    std::wstring wParentPath = (scopeSource == "nav" && !parentPath.isEmpty()) ? normalizePath(parentPath.toStdWString()) : L"";
    if (!wParentPath.empty()) {
        bool endsWithSlash = false;
        if (wParentPath.back() == L'\\' || wParentPath.back() == L'/') endsWithSlash = true;
        if (!endsWithSlash) {
            wParentPath += L'\\';
        }
    }

    // 2. 区分检索词长度获取匹配路径，避开 O(N) 扫描
    std::vector<std::wstring> matchedPaths;
    std::vector<sqlite3*> dbs = { DatabaseManager::instance().getGlobalDb() };

    if (keyword.length() >= 3) {
        // [Plan-26] FTS5 trigram 快速 Match 路径：通过倒排索引实现 O(log N) 模糊检索分流，彻底释解读写锁
        QString cleanKeyword = keyword;
        cleanKeyword.replace("\"", "");
        QString ftsQuery = "\"" + cleanKeyword + "\"";
        std::string utf8Query = ftsQuery.toUtf8().toStdString();

        const char* sql = "SELECT path FROM metadata WHERE rowid IN (SELECT rowid FROM metadata_fts WHERE metadata_fts MATCH ?)";
        for (sqlite3* db : dbs) {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, utf8Query.c_str(), -1, SQLITE_TRANSIENT);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const wchar_t* wpath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(stmt, 0));
                    if (wpath) {
                        matchedPaths.push_back(normalizePath(wpath));
                    }
                }
                sqlite3_finalize(stmt);
            }
        }
    } else {
        // [Plan-26] 退化路径：LIKE 模糊匹配降级路径 (使用高性能 UTF-8 绑定以避免 SQLite 内部编码转换开销)
        QString likeQueryStr = "%" + keyword + "%";
        std::string utf8LikeQuery = likeQueryStr.toUtf8().toStdString();

        const char* sql = "SELECT path FROM metadata WHERE path LIKE ? OR note LIKE ? OR tags LIKE ?";
        for (sqlite3* db : dbs) {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, utf8LikeQuery.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, utf8LikeQuery.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 3, utf8LikeQuery.c_str(), -1, SQLITE_TRANSIENT);
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* utf8Path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    if (utf8Path) {
                        matchedPaths.push_back(normalizePath(QString::fromUtf8(utf8Path).toStdWString()));
                    }
                }
                sqlite3_finalize(stmt);
            }
        }
    }

    // 去重
    std::sort(matchedPaths.begin(), matchedPaths.end());
    matchedPaths.erase(std::unique(matchedPaths.begin(), matchedPaths.end()), matchedPaths.end());

    // 3. 关联内存缓存并执行 Scope 过滤
    for (const auto& path : matchedPaths) {
        size_t idx = getShardIndex(path);
        std::shared_lock<std::shared_mutex> shardLock(m_shards[idx].mutex);
        auto it = m_shards[idx].items.find(path);
        if (it != m_shards[idx].items.end()) {
            const RuntimeMeta& meta = it->second;

            // Scope check
            if (hasScope) {
                if (scopeFids.find(meta.folderId) == scopeFids.end()) continue;
            } else if (!wParentPath.empty()) {
                if (path.find(wParentPath) != 0) continue;
            }

            results << QString::fromStdWString(path);
        }
    }

    return results;
}

QMap<QString, int> MetadataManager::getAllTags() const {
    QMap<QString, int> tagCounts;
    forEachCachedItem([&](const std::wstring&, const RuntimeMeta& meta) {
        if (!meta.isTrash) {
            for (const QString& tag : meta.tags) {
                tagCounts[tag]++;
            }
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
    
    // 2. 纯内存更新访问时间，取消隐式数据库持久化写库任务
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

    // 🚨 核心修复：第一步，只在 m_recentMutex 保护下做"取快照"，不在这把锁里做任何可能耗时或
    // 需要嵌套加锁的操作（如 getCachedAtime 需要另一把 m_mutex）。
    // 这样即使这一步稍有延迟，也绝不会阻塞 UI 线程里 recordAccess() 抢 m_recentMutex 的时间超过微秒级。
    std::deque<std::wstring> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_recentMutex);
        snapshot = m_recentVisitedQueue; // 拷贝一份，立即释放锁
    }

    // 第二步：在锁外计算哪些需要过期剔除（getCachedAtime 内部自己的 m_mutex 与本函数无嵌套关系）
    std::vector<std::wstring> toErase;
    for (const auto& path : snapshot) {
        double itemAtime = getCachedAtime(path);
        if (itemAtime < expireThreshold) {
            toErase.push_back(path);
        } else {
            break; // 队首往后都在窗口内，跳出
        }
    }

    if (toErase.empty()) return;

    // 第三步：只在真正要修改队列时，再次短暂加锁做剔除，持锁时间同样是微秒级
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
            meta.folderId,
            meta.isFolder,
            meta.isTrash,
            meta.tags.isEmpty(),
            static_cast<double>(meta.atime),
            meta.tags
        });
    });
    return result;
}

} // namespace QuarkMeta
