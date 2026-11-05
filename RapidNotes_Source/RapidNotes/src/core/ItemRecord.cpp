#include "ItemRecord.h"
#include <QFileInfo>
#include <QDir>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

ItemRecord ItemRecord::create(const QString& path, const RuntimeMeta* providedMeta) {
    Q_UNUSED(providedMeta);
    ItemRecord r;
    QFileInfo info(path);

    QString nPath = QDir::toNativeSeparators(info.absoluteFilePath());
    r.path = nPath;
    r.filename = info.fileName();
    r.isDir = info.isDir();
    r.isHidden = info.isHidden();

#ifdef Q_OS_WIN
    HANDLE hFile = CreateFileW(nPath.toStdWString().c_str(), 0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        BY_HANDLE_FILE_INFORMATION basicInfo;
        if (GetFileInformationByHandle(hFile, &basicInfo)) {
            r.size = (static_cast<long long>(basicInfo.nFileSizeHigh) << 32) | basicInfo.nFileSizeLow;
            auto toMS = [](const FILETIME& ft) {
                ULARGE_INTEGER ull; ull.LowPart = ft.dwLowDateTime; ull.HighPart = ft.dwHighDateTime;
                return static_cast<long long>((ull.QuadPart - 116444736000000000ULL) / 10000ULL);
            };
            r.ctime = toMS(basicInfo.ftCreationTime);
            r.mtime = toMS(basicInfo.ftLastWriteTime);
            r.atime = toMS(basicInfo.ftLastAccessTime);
        }
        CloseHandle(hFile);
    } else {
        r.size = info.size();
        r.ctime = info.birthTime().toMSecsSinceEpoch();
        r.mtime = info.lastModified().toMSecsSinceEpoch();
        r.atime = info.lastRead().toMSecsSinceEpoch();
    }
#else
    r.size = info.size();
    r.ctime = info.birthTime().toMSecsSinceEpoch();
    r.mtime = info.lastModified().toMSecsSinceEpoch();
    r.atime = info.lastRead().toMSecsSinceEpoch();
#endif

    if (r.isDir) {
        QDir sub(nPath);
        r.isEmpty = sub.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
        r.suffix = "";
    } else {
        r.suffix = info.suffix();
    }

    r.thumbnailState = ThumbnailState::NotApplicable;
    return r;
}

void ItemRecord::fromMetadata(ItemRecord& r, const RuntimeMeta& meta) {
    Q_UNUSED(r);
    Q_UNUSED(meta);
}

} // namespace QuarkMeta
