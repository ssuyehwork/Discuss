#include "DuplicateDetectorService.h"
#include "MetadataManager.h"
#include "../util/DiskMediaExtractor.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QCryptographicHash>
#include <QImageReader>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace QuarkMeta {

static QString computeFastHash(const QString& filePath, qint64 fileSize) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QString();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    constexpr qint64 chunkSize = 64 * 1024; // 首尾 64KB

    if (fileSize <= chunkSize * 2) {
        if (!hash.addData(&file)) return QString();
    } else {
        QByteArray head = file.read(chunkSize);
        hash.addData(head);

        if (file.seek(fileSize - chunkSize)) {
            QByteArray tail = file.read(chunkSize);
            hash.addData(tail);
        }
    }
    file.close();
    return QString(hash.result().toHex()).toLower();
}

static QString computeFullSha256(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) return QString();
    file.close();
    return QString(hash.result().toHex()).toLower();
}

std::vector<DuplicateConflictGroup> DuplicateDetectorService::detectDuplicates(const QStringList& newImportedPaths) {
    std::vector<DuplicateConflictGroup> conflicts;
    if (newImportedPaths.isEmpty()) return conflicts;

    std::unordered_map<long long, std::vector<std::pair<std::wstring, RuntimeMeta>>> sizeIndexMap;
    std::unordered_map<std::wstring, std::vector<std::pair<std::wstring, RuntimeMeta>>> nameIndexMap;

    MetadataManager::instance().forEachCachedItem([&](const std::wstring& existPathW, const RuntimeMeta& meta) {
        if (meta.isFolder) return;

        sizeIndexMap[meta.fileSize].push_back({existPathW, meta});

        QFileInfo info(QString::fromStdWString(existPathW));
        std::wstring lowerName = info.fileName().toStdWString();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        nameIndexMap[lowerName].push_back({existPathW, meta});
    });

    for (const QString& newPath : newImportedPaths) {
        QFileInfo newInfo(newPath);
        if (!newInfo.exists() || newInfo.isDir()) continue;

        qint64 size = newInfo.size();
        QString fileName = newInfo.fileName();
        std::wstring lowerNewName = fileName.toStdWString();
        std::transform(lowerNewName.begin(), lowerNewName.end(), lowerNewName.begin(), ::tolower);

        QString newFastHash;
        QString newFullSha256;

        QImageReader reader(newPath);
        QSize newImgSize = reader.size();
        int newWidth = newImgSize.width();
        int newHeight = newImgSize.height();

        bool alreadyMatched = false;

        // 一重判定：文件大小相同且 FastHash & Full SHA-256 相同
        auto sizeIt = sizeIndexMap.find(size);
        if (sizeIt != sizeIndexMap.end()) {
            for (const auto& pair : sizeIt->second) {
                const std::wstring& existPathW = pair.first;
                const RuntimeMeta& meta = pair.second;
                QString existPathStr = QString::fromStdWString(existPathW);
                if (existPathStr == newPath) continue;

                if (newFastHash.isEmpty()) {
                    newFastHash = computeFastHash(newPath, size);
                }
                if (newFastHash.isEmpty()) break;

                QString existFastHash = computeFastHash(existPathStr, meta.fileSize);
                if (existFastHash.isEmpty() || existFastHash != newFastHash) {
                    continue; // FastHash 不匹配，直接跳过
                }

                if (newFullSha256.isEmpty()) {
                    newFullSha256 = computeFullSha256(newPath);
                }

                QString existSha;
                if (!meta.sha256.empty()) {
                    existSha = QString::fromStdString(meta.sha256).toLower();
                } else {
                    existSha = computeFullSha256(existPathStr);
                }

                if (!existSha.isEmpty() && existSha == newFullSha256) {
                    DuplicateConflictGroup group;
                    group.existingItem.path = existPathStr;
                    group.existingItem.filename = QFileInfo(existPathStr).fileName();
                    group.existingItem.width = meta.width;
                    group.existingItem.height = meta.height;
                    group.existingItem.size = meta.fileSize;
                    group.existingItem.tagHint = meta.tags.isEmpty() ? "" : meta.tags.first();
                    group.existingItem.thumbnail = DiskMediaExtractor::getCapsuleThumbnailReadOnly(existPathStr);

                    group.newItem.path = newPath;
                    group.newItem.filename = fileName;
                    group.newItem.width = newWidth; 
                    group.newItem.height = newHeight;
                    group.newItem.size = size;
                    group.newItem.thumbnail = DiskMediaExtractor::getCapsuleThumbnail(newPath, 512);

                    conflicts.push_back(group);
                    alreadyMatched = true;
                    break;
                }
            }
        }

        // 二重判定：只有一重判定完全没找到匹配时才执行
        if (!alreadyMatched) {
            auto nameIt = nameIndexMap.find(lowerNewName);
            if (nameIt != nameIndexMap.end()) {
                for (const auto& pair : nameIt->second) {
                    const std::wstring& existPathW = pair.first;
                    const RuntimeMeta& meta = pair.second;
                    QString existPathStr = QString::fromStdWString(existPathW);
                    if (existPathStr == newPath) continue;

                    if (meta.width > 0 && meta.height > 0 && meta.width == newWidth && meta.height == newHeight) {
                        DuplicateConflictGroup group;
                        
                        group.existingItem.path = existPathStr;
                        group.existingItem.filename = QFileInfo(existPathStr).fileName();
                        group.existingItem.width = meta.width;
                        group.existingItem.height = meta.height;
                        group.existingItem.size = meta.fileSize;
                        group.existingItem.tagHint = meta.tags.isEmpty() ? "" : meta.tags.first();
                        group.existingItem.thumbnail = DiskMediaExtractor::getCapsuleThumbnailReadOnly(existPathStr);

                        group.newItem.path = newPath;
                        group.newItem.filename = fileName;
                        group.newItem.width = newWidth; 
                        group.newItem.height = newHeight;
                        group.newItem.size = size;
                        group.newItem.thumbnail = DiskMediaExtractor::getCapsuleThumbnail(newPath, 512);

                        conflicts.push_back(group);
                        break;
                    }
                }
            }
        }
    }

    return conflicts;
}

} // namespace QuarkMeta
