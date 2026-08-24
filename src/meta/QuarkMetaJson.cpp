#include "QuarkMetaJson.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
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
    QFile file(toQString(m_filePath));
    if (!file.exists()) {
        m_folder = FolderMeta();
        m_items.clear();
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return false;

    QJsonObject root = doc.object();
    if (root.contains("folder") && root.value("folder").isObject()) {
        m_folder = entryToFolder(root.value("folder").toObject());
    }
    
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
}

bool QuarkMetaJson::save() const {
    QJsonObject root;
    root.insert("version", "2");
    root.insert("folder", folderToEntry(m_folder));

    QJsonObject itemsObj;
    for (const auto& [name, meta] : m_items) {
        if (meta.hasUserOperations()) {
            itemsObj.insert(toQString(name), itemToEntry(meta));
        }
    }
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
    auto it = items.find(oldName.toStdWString());
    if (it != items.end()) {
        items[newName.toStdWString()] = it->second;
        items.erase(it);
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
    std::wstring fileName = info.fileName().toStdWString();

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

bool QuarkMetaJson::roamItemMetadata(const QString& oldFilePath, const QString& newFilePath, bool isMove) {
    if (oldFilePath == newFilePath) return true;

    QFileInfo oldInfo(oldFilePath);
    QFileInfo newInfo(newFilePath);

    QString oldParent = QDir::toNativeSeparators(oldInfo.absolutePath());
    QString newParent = QDir::toNativeSeparators(newInfo.absolutePath());
    std::wstring oldFileName = oldInfo.fileName().toStdWString();
    std::wstring newFileName = newInfo.fileName().toStdWString();

    static std::mutex s_jsonRoamMutex;
    std::lock_guard<std::mutex> lock(s_jsonRoamMutex);

    // 1. 从源目录 .QuarkMeta.json 提取完整元数据包
    QuarkMetaJson oldJson(oldParent.toStdWString());
    if (!oldJson.load()) return false;

    auto& oldItems = oldJson.items();
    auto it = oldItems.find(oldFileName);
    if (it == oldItems.end()) {
        // 源文件无任何用户标记或尺寸，无需转移
        return true;
    }

    // 2. 整包深拷贝（星级、颜色、标签、尺寸、宽高比、备注、链接、失败状态等 100% 完整继承）
    ItemMeta metaPackage = it->second;

    // 3. 如果是【移动 (Move)】，从源 JSON 中抹除该条目并落盘
    if (isMove) {
        oldItems.erase(it);
        oldJson.save();
    }

    // 4. 写入目标目录 .QuarkMeta.json 并物理原子落盘
    QuarkMetaJson newJson(newParent.toStdWString());
    newJson.load();
    newJson.items()[newFileName] = metaPackage;
    return newJson.save();
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
    // 🚨 保持兼容性：磁盘上存储的旧版 JSON 配置文件依然使用 "file_id_128"，内存映射使用统一的 folderId
    obj.insert("file_id_128", QString::fromStdString(meta.folderId));
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
    meta.folderId = obj.value("file_id_128").toString().toStdString();
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
    obj.insert("encrypt_salt", QString::fromStdString(meta.encryptSalt));
    obj.insert("encrypt_iv", QString::fromLatin1(QByteArray::fromStdString(meta.encryptIv).toBase64()));
    obj.insert("encrypt_verify_hash", QString::fromStdString(meta.encryptVerifyHash));
    obj.insert("original_name", toQString(meta.originalName));
    obj.insert("volume", toQString(meta.volume));
    obj.insert("frn", toQString(meta.frn));
    // 🚨 保持兼容性：磁盘上存储的旧版 JSON 配置文件依然使用 "file_id_128"，内存映射使用统一的 folderId
    obj.insert("file_id_128", QString::fromStdString(meta.folderId));
    
    // 2026-07-xx 1:1对等字段写入
    obj.insert("width", meta.width);
    obj.insert("height", meta.height);
    obj.insert("auto_color", toQString(meta.autoColor));
    obj.insert("added_at", meta.addedAt);
    if (meta.thumbStatus > 0) obj.insert("thumb_status", meta.thumbStatus);

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
    meta.encryptSalt = obj.value("encrypt_salt").toString().toStdString();
    meta.encryptIv = QByteArray::fromBase64(obj.value("encrypt_iv").toString().toLatin1()).toStdString();
    meta.encryptVerifyHash = obj.value("encrypt_verify_hash").toString().toStdString();
    meta.originalName = toStdWString(obj.value("original_name").toString());
    meta.volume = toStdWString(obj.value("volume").toString());
    meta.frn = toStdWString(obj.value("frn").toString());
    meta.folderId = obj.value("file_id_128").toString().toStdString();

    // 2026-07-xx 1:1对等字段读取
    meta.width = obj.value("width").toInt(0);
    meta.height = obj.value("height").toInt(0);
    meta.autoColor = toStdWString(obj.value("auto_color").toString());
    meta.addedAt = obj.value("added_at").toVariant().toLongLong();
    meta.thumbStatus = obj.value("thumb_status").toInt(0);

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