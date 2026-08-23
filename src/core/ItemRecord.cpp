#include "ItemRecord.h"
#include "../meta/MetadataManager.h"
#include <QFileInfo>
#include <QDir>
#include <mutex>
#include <unordered_map>

namespace QuarkMeta {

void ItemRecord::fromMetadata(ItemRecord& r, const RuntimeMeta& meta) {
    r.rating = meta.rating;
    r.manualColor = QString::fromStdWString(meta.manualColor);
    r.autoColor = QString::fromStdWString(meta.autoColor);
    r.tags = meta.tags;
    r.pinned = meta.pinned;
    r.encrypted = meta.encrypted;
    r.url = QString::fromStdWString(meta.url);
    r.note = QString::fromStdWString(meta.note);
    r.sha256 = QString::fromStdString(meta.sha256);
    r.width = meta.width;
    r.height = meta.height;
    r.added_at = meta.added_at;
    r.thumbStatus = meta.thumbStatus;
    r.isManaged = meta.hasUserOperations();
    r.palettes.clear();
    for (const auto& pe : meta.palettes) {
        r.palettes.push_back({pe.color, pe.ratio});
    }
}

ItemRecord ItemRecord::create(const QString& path, const RuntimeMeta* providedMeta) {
    ItemRecord r;
    QFileInfo info(path);

    QString nPath = QDir::toNativeSeparators(info.absoluteFilePath());
    std::wstring wPath = nPath.toStdWString();

    bool isArcEnd = nPath.endsWith(".arc", Qt::CaseInsensitive) || nPath.endsWith(".arc/", Qt::CaseInsensitive) || nPath.endsWith(".arc\\", Qt::CaseInsensitive);
    if (isArcEnd && (nPath.endsWith("/") || nPath.endsWith("\\"))) {
        nPath = nPath.left(nPath.length() - 1);
        wPath = nPath.toStdWString();
    }

    RuntimeMeta meta;
    if (providedMeta) {
        meta = *providedMeta;
    }

    // 磁盘模式分支（保持原有 Win32 探测）
    std::string fid;
    long long size = 0, ctime = 0, mtime = 0, atime = 0;
    MetadataManager::fetchWinApiMetadataDirect(wPath, fid, nullptr, &size, nullptr, &ctime, &mtime, &atime);
    r.size = size;
    r.ctime = ctime;
    r.mtime = mtime;
    r.atime = atime;
    r.isDir = info.isDir();
    r.path = nPath;
    r.filename = info.fileName();
    r.isHidden = info.isHidden();

    if (r.isDir) {
        QDir sub(nPath);
        r.isEmpty = sub.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
        r.suffix = "";
    } else {
        r.suffix = info.suffix();
    }

    return r;
}

} // namespace QuarkMeta
