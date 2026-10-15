#include "QuarkMetaJson.h"
#include "MetaMemoryCache.h"
#include "MetadataManager.h"
#include "QuarkMetaJsonStore.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QMap>
#include <mutex>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace QuarkMeta {

QuarkMetaJson::QuarkMetaJson(const std::wstring& folderPath)
    : m_folderPath(folderPath) {
    std::wstring path = folderPath;
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/') {
        path += L'\\';
    }
    // 🚨 QuarkMeta 唯一物理离散缓存文件名：.QuarkMeta.json
    m_filePath = path + L".QuarkMeta.json";
}

bool QuarkMetaJson::load() {
    m_loadFailed = false;
    QFile file(toQString(m_filePath));
    if (!file.exists()) {
        m_folder = FolderMeta();
        m_items.clear();
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        m_loadFailed = true;
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        m_loadFailed = true;
        return false;
    }

    QJsonObject root = doc.object();
    if (root.contains("folder") && root.value("folder").isObject()) {
        m_folder = entryToFolder(root.value("folder").toObject());
    }
    
    m_items.clear();
    if (root.contains("items") && root.value("items").isObject()) {
        QJsonObject itemsObj = root.value("items").toObject();
        for (auto it = itemsObj.begin(); it != itemsObj.end(); ++it) {
            std::wstring key = normalizeItemKey(toStdWString(it.key()));
            ItemMeta item = entryToItem(it.value().toObject());
            m_items[key] = item;
        }
    }
    return true;
}

bool QuarkMetaJson::save() const {
    QJsonObject itemsObj;
    for (const auto& [name, meta] : m_items) {
        if (meta.hasUserOperations()) {
            itemsObj.insert(toQString(name), itemToEntry(meta));
        }
    }

    // 零残留物理自愈：当所有子项目的元数据都被移除且文件夹自身为默认配置时，自动物理物理物理擦除 .QuarkMeta.json
    if (itemsObj.isEmpty() && m_folder.isDefault()) {
        QString realPath = toQString(m_filePath);
        if (QFile::exists(realPath)) {
            QFile::remove(realPath);
        }
        return true;
    }

    QJsonObject root;
    root.insert("version", "2");
    root.insert("folder", folderToEntry(m_folder));
    root.insert("items", itemsObj);

    QByteArray jsonData = QJsonDocument(root).toJson(QJsonDocument::Indented);
    QString tmpPath = toQString(m_filePath) + ".tmp";
    
    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly)) return false;
    tmpFile.write(jsonData);
    tmpFile.close();

    if (!MoveFileExW(tmpPath.toStdWString().c_str(), m_filePath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        QFile::remove(tmpPath);
        return false;
    }
    // 赋予 Windows 隐藏文件属性
    SetFileAttributesW(m_filePath.c_str(), FILE_ATTRIBUTE_HIDDEN);
    return true;
}

bool QuarkMetaJson::renameItem(const QString& folderPath, const QString& oldName, const QString& newName) {
    if (oldName == newName) return true;
    QuarkMetaJson meta(folderPath.toStdWString());
    if (!meta.load()) return false;
    auto& items = meta.items();
    std::wstring oldKey = normalizeItemKey(oldName.toStdWString());
    std::wstring newKey = normalizeItemKey(newName.toStdWString());
    auto it = items.find(oldKey);
    if (it != items.end()) {
        ItemMeta itemMeta = it->second;
        items.erase(it);
        items[newKey] = itemMeta;
        return meta.save();
    }
    return true;
}

std::unordered_map<std::wstring, ItemMeta> QuarkMetaJson::readFolderMeta(const std::wstring& folderPath) {
    std::unordered_map<std::wstring, ItemMeta> result;
    QuarkMetaJson meta(folderPath);
    if (meta.load()) {
        const auto& itemsMap = meta.items();
        for (const auto& pair : itemsMap) {
            result[pair.first] = pair.second;
        }
    }
    return result;
}

void QuarkMetaJson::updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater) {
    QFileInfo info(QString::fromStdWString(filePath));
    std::wstring folderPath = info.absolutePath().toStdWString();
    std::wstring fileName = normalizeItemKey(info.fileName().toStdWString());

    QuarkMetaJson parentJson(folderPath);
    parentJson.load();
    ItemMeta& meta = parentJson.items()[fileName];
    meta.type = info.isDir() ? L"folder" : L"file";
    updater(meta);
    parentJson.save();

    if (info.isDir()) {
        QuarkMetaJson selfJson(filePath);
        selfJson.load();
        
        FolderMeta& fMeta = selfJson.folder();
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

        selfJson.save();
    }
}

bool QuarkMetaJson::migrateItemMetadata(const QString& oldFilePath, const QString& newFilePath) { 
    return roamItemMetadata(oldFilePath, newFilePath, true);
}

bool QuarkMetaJson::roamBatchMetadata(const QList<QPair<QString, QString>>& pairs, bool isMove) {
    if (pairs.isEmpty()) return false;

    struct TransferItem {
        QString oldPath;
        QString newPath;
        std::wstring oldName;
        std::wstring newName;
        ItemMeta meta;
        bool hasMeta = false;
    };

    // 1. 按源文件夹归类 Group by source folder
    QMap<QString, QList<TransferItem>> srcFolderMap;
    for (const auto& pair : pairs) {
        QFileInfo oldFi(pair.first);
        QFileInfo newFi(pair.second);

        TransferItem item;
        item.oldPath = pair.first;
        item.newPath = pair.second;
        item.oldName = oldFi.fileName().toStdWString();
        item.newName = newFi.fileName().toStdWString();

        QString srcFolder = oldFi.absolutePath();
        srcFolderMap[srcFolder].append(item);
    }

    QList<TransferItem> allTransfers;

    // 2. 读取并在移动场景下抹除源端元数据 (仅对每一个源文件夹加载一次和保存一次)
    for (auto it = srcFolderMap.begin(); it != srcFolderMap.end(); ++it) {
        QString srcDir = it.key();
        QList<TransferItem>& itemsInDir = it.value();

        QuarkMetaJson srcMeta(srcDir.toStdWString());
        bool srcLoaded = srcMeta.load();

        for (auto& item : itemsInDir) {
            std::wstring normKey = normalizeItemKey(item.oldName);
            if (srcLoaded) {
                auto mIt = srcMeta.items().find(normKey);
                if (mIt == srcMeta.items().end()) {
                    // 大小写敏感降级匹配
                    for (auto entry = srcMeta.items().begin(); entry != srcMeta.items().end(); ++entry) {
                        if (QString::fromStdWString(entry->first).compare(QString::fromStdWString(item.oldName), Qt::CaseInsensitive) == 0) {
                            mIt = entry;
                            break;
                        }
                    }
                }

                if (mIt != srcMeta.items().end()) {
                    item.meta = mIt->second;
                    item.hasMeta = true;
                    if (isMove) {
                        srcMeta.items().erase(mIt);
                    }
                }
            }

            // 仅在移动操作时更新数据库与内存映射，避免复制操作损坏源端元数据
            if (isMove) {
                MetadataManager::instance().renameItem(item.oldPath.toStdWString(), item.newPath.toStdWString());
            }
            allTransfers.append(item);
        }

        if (isMove && srcLoaded) {
            srcMeta.save();
            QuarkMetaJsonStore::instance().invalidateFolderCache(srcDir.toStdWString());
        }
    }

    // 3. 按目标文件夹归类 Group by destination folder 并在目标端保存
    QMap<QString, QList<TransferItem>> destFolderMap;
    for (const auto& item : allTransfers) {
        QFileInfo newFi(item.newPath);
        QString destFolder = newFi.absolutePath();
        destFolderMap[destFolder].append(item);
    }

    for (auto it = destFolderMap.begin(); it != destFolderMap.end(); ++it) {
        QString destDir = it.key();
        const QList<TransferItem>& itemsInDir = it.value();

        QuarkMetaJson destMeta(destDir.toStdWString());
        destMeta.load();

        bool destModified = false;
        for (const auto& item : itemsInDir) {
            if (item.hasMeta) {
                std::wstring destKey = normalizeItemKey(item.newName);
                destMeta.items()[destKey] = item.meta;
                destModified = true;
            }
        }

        if (destModified) {
            destMeta.save();
            QuarkMetaJsonStore::instance().invalidateFolderCache(destDir.toStdWString());
        }
    }

    return true;
}

bool QuarkMetaJson::roamItemMetadata(const QString& oldFilePath, const QString& newFilePath, bool isMove) {
    if (oldFilePath == newFilePath) return true;

    QFileInfo oldInfo(oldFilePath);
    QFileInfo newInfo(newFilePath);

    QString oldParent = QDir::toNativeSeparators(oldInfo.absolutePath());
    QString newParent = QDir::toNativeSeparators(newInfo.absolutePath());
    std::wstring oldKey = normalizeItemKey(oldInfo.fileName().toStdWString());
    std::wstring newKey = normalizeItemKey(newInfo.fileName().toStdWString());

    static std::mutex s_jsonRoamMutex;
    std::lock_guard<std::mutex> lock(s_jsonRoamMutex);

    // 1. 从源目录 .QuarkMeta.json 提取完整元数据包
    QuarkMetaJson oldJson(oldParent.toStdWString());
    if (!oldJson.load()) return false;

    // 2. 加载目标目录 .QuarkMeta.json 准备覆盖/替换
    QuarkMetaJson newJson(newParent.toStdWString());
    newJson.load();

    auto& oldItems = oldJson.items();
    auto& newItems = newJson.items();

    // “替换”语义：先强行抹除目标位置同名的旧条目
    newItems.erase(newKey);

    auto it = oldItems.find(oldKey);
    if (it == oldItems.end()) {
        // 源无任何用户标记：目标同名条目已被清理，保存目标配置后返回
        if (isMove) {
            oldJson.save();
        }
        return newJson.save();
    }

    // 整包深拷贝（星级、颜色、标签、尺寸、宽高比、备注、链接、失败状态等 100% 完整继承）
    ItemMeta metaPackage = it->second;

    // 如果是【移动 (Move)】，从源 JSON 中抹除该条目并落盘
    if (isMove) {
        oldItems.erase(it);
        oldJson.save();
    }

    // 写入目标目录 .QuarkMeta.json (使用统一规范化的键名) 并物理原子落盘
    newItems[newKey] = metaPackage;
    bool saveOk = newJson.save();

    if (saveOk) {
        // 驱逐目标文件夹的 Store 内存缓存快照，确保下次 readFolderMeta() 强制重读磁盘最新 JSON
        QuarkMetaJsonStore::instance().invalidateFolderCache(newParent.toStdWString());
        if (isMove) {
            // 移动时同步驱逐源文件夹缓存（元数据已从源 JSON 物理抹除）
            QuarkMetaJsonStore::instance().invalidateFolderCache(oldParent.toStdWString());
        }
        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, newFilePath);
    }

    return saveOk;
}

bool QuarkMetaJson::removeItemMetadata(const QString& filePath) {
    if (filePath.isEmpty()) return true;
    QFileInfo info(filePath);
    QString parentDir = QDir::toNativeSeparators(info.absolutePath());
    std::wstring fileNameKey = normalizeItemKey(info.fileName().toStdWString());

    static std::mutex s_jsonRemoveMutex;
    std::lock_guard<std::mutex> lock(s_jsonRemoveMutex);

    QuarkMetaJson json(parentDir.toStdWString());
    if (!json.load()) return false;

    if (json.items().erase(fileNameKey) > 0) {
        json.save();
    }
    std::wstring wPath = MetadataManager::normalizePath(filePath.toStdWString());
    MetaMemoryCache::instance().remove(wPath);
    MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, filePath);
    return true;
}

bool QuarkMetaJson::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
    if (oldFolderPath == newFolderPath) return true;
    
    // 物理自愈：当发生物理文件夹重命名时，自动原子迁移其目录内部隐藏的 .QuarkMeta.json 配置
    QString oldMetaFile = oldFolderPath + "/.QuarkMeta.json";
    QString newMetaFile = newFolderPath + "/.QuarkMeta.json";

    if (QFile::exists(oldMetaFile)) {
        // 创建新物理目录（如果不存在）
        QDir().mkpath(newFolderPath);
        
        if (QFile::exists(newMetaFile)) {
            QFile::remove(newMetaFile);
        }
        if (QFile::copy(oldMetaFile, newMetaFile)) {
            QFile::remove(oldMetaFile);
            
            // 赋予 Windows 环境隐藏文件属性
            SetFileAttributesW(newMetaFile.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
            return true;
        }
    }
    return true;
}

// --- 内部 JSON 结构转换实现 ---

QJsonObject QuarkMetaJson::folderToEntry(const FolderMeta& meta) {
    QJsonObject obj;
    obj.insert("sort_by", toQString(meta.sortBy));
    obj.insert("sort_order", toQString(meta.sortOrder));
    obj.insert("rating", meta.rating);
    obj.insert("color", toQString(meta.color));
    obj.insert("pinned", meta.pinned);
    obj.insert("note", toQString(meta.note));
    obj.insert("url", toQString(meta.url));
    obj.insert("encrypted", meta.encrypted);
    QJsonArray tagsArr; for (const auto& t : meta.tags) tagsArr.append(toQString(t));
    obj.insert("tags", tagsArr);
    if (!meta.palettes.empty()) {
        QJsonArray palArr;
        for (const auto& p : meta.palettes) {
            QJsonObject pObj;
            QJsonArray cArr;
            cArr.append(p.color.red()); cArr.append(p.color.green()); cArr.append(p.color.blue());
            pObj.insert("color", cArr);
            pObj.insert("ratio", (double)p.ratio);
            palArr.append(pObj);
        }
        obj.insert("palettes", palArr);
    }
    return obj;
}

FolderMeta QuarkMetaJson::entryToFolder(const QJsonObject& obj) {
    FolderMeta meta;
    meta.sortBy = toStdWString(obj.value("sort_by").toString("name"));
    meta.sortOrder = toStdWString(obj.value("sort_order").toString("asc"));
    meta.rating = obj.value("rating").toInt();
    meta.color = toStdWString(obj.value("color").toString());
    meta.pinned = obj.value("pinned").toBool();
    meta.note = toStdWString(obj.value("note").toString());
    meta.url = toStdWString(obj.value("url").toString());
    meta.encrypted = obj.value("encrypted").toBool();
    if (obj.contains("tags") && obj.value("tags").isArray()) {
        for (const auto& v : obj.value("tags").toArray()) meta.tags.push_back(toStdWString(v.toString()));
    }
    if (obj.contains("palettes") && obj.value("palettes").isArray()) {
        for (const auto& v : obj.value("palettes").toArray()) {
            QJsonObject pObj = v.toObject();
            QJsonArray cArr = pObj.value("color").toArray();
            if (cArr.size() >= 3) {
                meta.palettes.push_back({QColor(cArr.at(0).toInt(), cArr.at(1).toInt(), cArr.at(2).toInt()), (float)pObj.value("ratio").toDouble()});
            }
        }
    }
    return meta;
}

QJsonObject QuarkMetaJson::itemToEntry(const ItemMeta& meta) {
    QJsonObject obj;
    obj.insert("type", toQString(meta.type));
    obj.insert("rating", meta.rating);
    obj.insert("color", toQString(meta.color));
    obj.insert("pinned", meta.pinned);
    obj.insert("note", toQString(meta.note));
    obj.insert("url", toQString(meta.url));
    obj.insert("encrypted", meta.encrypted);
    obj.insert("original_name", toQString(meta.originalName));
    obj.insert("volume", toQString(meta.volume));
    obj.insert("frn", toQString(meta.frn));

    // 2026-07-xx 1:1对等字段写入
    obj.insert("width", meta.width);
    obj.insert("height", meta.height);
    obj.insert("auto_color", toQString(meta.autoColor));
    obj.insert("added_at", meta.addedAt);
    if (meta.thumbStatus > 0) obj.insert("thumb_status", meta.thumbStatus);
    if (meta.size > 0) obj.insert("file_size", meta.size);
    if (meta.modificationTime > 0) obj.insert("file_mtime", meta.modificationTime);

    QJsonArray tagsArr; for (const auto& t : meta.tags) tagsArr.append(toQString(t));
    obj.insert("tags", tagsArr);
    if (!meta.palettes.empty()) {
        QJsonArray palArr;
        for (const auto& p : meta.palettes) {
            QJsonObject pObj;
            QJsonArray cArr;
            cArr.append(p.color.red()); cArr.append(p.color.green()); cArr.append(p.color.blue());
            pObj.insert("color", cArr);
            pObj.insert("ratio", (double)p.ratio);
            palArr.append(pObj);
        }
        obj.insert("palettes", palArr);
    }
    return obj;
}

ItemMeta QuarkMetaJson::entryToItem(const QJsonObject& obj) {
    ItemMeta meta;
    meta.type = toStdWString(obj.value("type").toString("file"));
    meta.rating = obj.value("rating").toInt();
    meta.color = toStdWString(obj.value("color").toString());
    meta.pinned = obj.value("pinned").toBool();
    meta.note = toStdWString(obj.value("note").toString());
    meta.url = toStdWString(obj.value("url").toString());
    meta.encrypted = obj.value("encrypted").toBool();
    meta.originalName = toStdWString(obj.value("original_name").toString());
    meta.volume = toStdWString(obj.value("volume").toString());
    meta.frn = toStdWString(obj.value("frn").toString());

    // 2026-07-xx 1:1对等字段读取
    meta.width = obj.value("width").toInt(0);
    meta.height = obj.value("height").toInt(0);
    meta.autoColor = toStdWString(obj.value("auto_color").toString());
    meta.addedAt = obj.value("added_at").toVariant().toLongLong();
    meta.thumbStatus = obj.value("thumb_status").toInt(0);
    meta.size = obj.value("file_size").toVariant().toLongLong();
    meta.modificationTime = obj.value("file_mtime").toVariant().toLongLong();

    if (obj.contains("tags") && obj.value("tags").isArray()) {
        for (const auto& v : obj.value("tags").toArray()) meta.tags.push_back(toStdWString(v.toString()));
    }
    if (obj.contains("palettes") && obj.value("palettes").isArray()) {
        for (const auto& v : obj.value("palettes").toArray()) {
            QJsonObject pObj = v.toObject();
            QJsonArray cArr = pObj.value("color").toArray();
            if (cArr.size() >= 3) {
                meta.palettes.push_back({QColor(cArr.at(0).toInt(), cArr.at(1).toInt(), cArr.at(2).toInt()), (float)pObj.value("ratio").toDouble()});
            }
        }
    }
    return meta;
}

} // namespace QuarkMeta