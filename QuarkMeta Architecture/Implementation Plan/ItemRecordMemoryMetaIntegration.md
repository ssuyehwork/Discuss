# ItemRecordMemoryMetaIntegration Implementation Plan

This implementation plan details the precise changes required to ensure all created `ItemRecord` instances automatically populate their metadata directly from `MetadataManager` (SSOT in-memory cache) upon creation.

## Overview
When `DiskScanService` or `ContentDataLoader` scans a directory, it creates `ItemRecord` instances using `ItemRecord::create(path, nullptr)`. Previously, `ItemRecord::create` only fetched Win32 API file system attributes (size, times), leaving metadata fields (`rating`, `manualColor`, `tags`, `note`, `url`, `pinned`, `encrypted`) blank.

For Column View (Miller Columns), which loads directory records using `DiskScanService::scanDirectory` into isolated private models, this resulted in all items displaying with empty metadata (no rating stars, no color tags).

This plan modifies `ItemRecord::create` in `ItemRecord.cpp` to automatically query `MetadataManager::instance().getMeta(wPath)` and invoke `fromMetadata` to populate all in-memory metadata fields.

---

## Modified Files List
- `src/core/ItemRecord.cpp`

---

## Detailed Line-by-Line Changes

### File: `src/core/ItemRecord.cpp`

```git
<<<<<<< SEARCH
ItemRecord ItemRecord::create(const QString& path, const RuntimeMeta* providedMeta) {
    ItemRecord r;
    QFileInfo info(path);

    QString nPath = QDir::toNativeSeparators(info.absoluteFilePath());
    std::wstring wPath = nPath.toStdWString();

    RuntimeMeta meta;
    if (providedMeta) {
        meta = *providedMeta;
    }

    long long size = 0, ctime = 0, mtime = 0, atime = 0;
    MetadataManager::fetchWinApiMetadataDirect(wPath, &size, nullptr, &ctime, &mtime, &atime);
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
=======
ItemRecord ItemRecord::create(const QString& path, const RuntimeMeta* providedMeta) {
    ItemRecord r;
    QFileInfo info(path);

    QString nPath = QDir::toNativeSeparators(info.absoluteFilePath());
    std::wstring wPath = nPath.toStdWString();

    RuntimeMeta meta;
    if (providedMeta) {
        meta = *providedMeta;
    } else {
        meta = MetadataManager::instance().getMeta(wPath);
    }

    long long size = 0, ctime = 0, mtime = 0, atime = 0;
    MetadataManager::fetchWinApiMetadataDirect(wPath, &size, nullptr, &ctime, &mtime, &atime);
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

    fromMetadata(r, meta);

    return r;
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **CMake Build Verification**:
   ```bash
   cmake --build build --config Debug
   ```
2. **Functional Verification**:
   - Switch application to Column View mode (Miller Columns).
   - Navigate to a directory containing files with existing metadata (e.g. rating stars, color tags, or tags).
   - Verify that:
     - All column view items display their rating stars and color tags immediately upon opening the column, populated directly from `MetadataManager` memory SSOT.
     - Rating stars and color tags render consistently across Grid View, List View, Justified View, and Column View without any missing data.
