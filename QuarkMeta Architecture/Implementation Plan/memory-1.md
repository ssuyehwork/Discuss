# Implementation Plan: Memory Mode & Managed Library Residual Code Purge (memory-1.md)

## Overview
This plan addresses and purges all residual memory mode and managed library legacy code remnants in the `src/` codebase, aligning with the pure disk direct-connect architecture specified in `QuarkMeta-Architecture-Planning.md`.

Specifically:
1. Purge the legacy `isFromMemory` parameter and its internal branching logic in `ItemRecord::create(...)`.
2. Purge the obsolete `meta.isTrash` memory cache scan in `ContentPanel::loadTrashItems` (`src/ui/ContentPanel.cpp`), ensuring trash item loading relies strictly on `DiskTrashRepo`.
3. Clean up legacy comments in `src/ui/ContentPanel.h`, `src/ui/TagManagerDialog.h`, `src/meta/MetadataManager.h`, `src/core/CoreController.cpp`, and `src/core/VolumeOnlineManager.h` that reference obsolete memory mode or managed library concepts.

---

## Modified Files List
1. `src/core/ItemRecord.h`
2. `src/core/ItemRecord.cpp`
3. `src/ui/ContentPanel.h`
4. `src/ui/ContentPanel.cpp`
5. `src/ui/TagManagerDialog.h`
6. `src/meta/MetadataManager.h`
7. `src/core/CoreController.cpp`
8. `src/core/VolumeOnlineManager.h`

---

## Detailed Line-by-Line Changes

### 1. `src/core/ItemRecord.h`
Remove `isFromMemory` parameter from `ItemRecord::create` signature.

<<<<<<< SEARCH
    static ItemRecord create(const QString& path, const RuntimeMeta* providedMeta = nullptr, bool isFromMemory = false);
=======
    static ItemRecord create(const QString& path, const RuntimeMeta* providedMeta = nullptr);
>>>>>>> REPLACE

---

### 2. `src/core/ItemRecord.cpp`
Purge `isFromMemory` parameter and `if (isFromMemory)` branching logic.

<<<<<<< SEARCH
ItemRecord ItemRecord::create(const QString& path, const RuntimeMeta* providedMeta, bool isFromMemory) {
    ItemRecord r;
    std::wstring wPath = MetadataManager::normalizePath(path.toStdWString());
    QString nPath = QString::fromStdWString(wPath);
    bool isArcEnd = nPath.endsWith(".arc", Qt::CaseInsensitive) || nPath.endsWith(".arc/", Qt::CaseInsensitive) || nPath.endsWith(".arc\\", Qt::CaseInsensitive);
    if (isArcEnd && (nPath.endsWith("/") || nPath.endsWith("\\"))) {
        nPath = nPath.left(nPath.length() - 1);
        wPath = nPath.toStdWString();
    }

    RuntimeMeta meta;
    if (providedMeta) {
        meta = *providedMeta;
    } else if (isFromMemory) {
        meta = MetadataManager::instance().getMeta(wPath);
    }

    if (isFromMemory) {
        // 🚨【真·纯内存模式】：100% 从内存 RuntimeMeta 镜像读取，严禁任何物理磁盘 I/O
        r.size = meta.fileSize;
        r.ctime = meta.ctime;
        r.mtime = meta.mtime;
        r.atime = meta.atime;
        r.isDir = meta.isFolder;
        r.isManaged = true;
        r.isEmpty = false;
        r.path = nPath;

        // 直接从内存元数据注入真实素材文件名与后缀
        if (!meta.baseName.empty()) {
            QString baseNameStr = QString::fromStdWString(meta.baseName);
            r.filename = baseNameStr;
            r.suffix = QFileInfo(baseNameStr).suffix();
        } else {
            QFileInfo fi(nPath);
            r.filename = fi.fileName();
            r.suffix = fi.suffix();
        }

        r.isPinned = meta.isPinned;
        r.rating = meta.rating;
        r.color = meta.color;
        r.remarks = QString::fromStdWString(meta.remarks);

        r.tags.clear();
        for (int tid : meta.tags) {
            r.tags.push_back(tid);
        }
        return r;
    }

    QFileInfo fi(nPath);
=======
ItemRecord ItemRecord::create(const QString& path, const RuntimeMeta* providedMeta) {
    ItemRecord r;
    std::wstring wPath = MetadataManager::normalizePath(path.toStdWString());
    QString nPath = QString::fromStdWString(wPath);
    bool isArcEnd = nPath.endsWith(".arc", Qt::CaseInsensitive) || nPath.endsWith(".arc/", Qt::CaseInsensitive) || nPath.endsWith(".arc\\", Qt::CaseInsensitive);
    if (isArcEnd && (nPath.endsWith("/") || nPath.endsWith("\\"))) {
        nPath = nPath.left(nPath.length() - 1);
        wPath = nPath.toStdWString();
    }

    RuntimeMeta meta;
    if (providedMeta) {
        meta = *providedMeta;
    }

    QFileInfo fi(nPath);
>>>>>>> REPLACE

---

### 3. `src/ui/ContentPanel.cpp`
Purge obsolete `meta.isTrash` memory cache scan from `ContentPanel::loadTrashItems`.

<<<<<<< SEARCH
    // 2. 扫描内存缓存中的 isTrash 项
    MetadataManager::instance().forEachCachedItem([&](const std::wstring& wpath, const RuntimeMeta& meta) {
        if (meta.isTrash) {
            ItemRecord rec = ItemRecord::create(QString::fromStdWString(wpath), &meta, true);
            rec.originalPath = QString::fromStdWString(meta.originalPath);
            records.push_back(rec);
        }
    });

    MetaCacheDecorator::decorate(records);
    return records;
=======
    MetaCacheDecorator::decorate(records);
    return records;
>>>>>>> REPLACE

---

### 4. `src/ui/ContentPanel.h`
Clean up obsolete comment referencing memory mode logical subcategories.

<<<<<<< SEARCH
     * @brief 在内存模式下，请求在指定分类下创建 logical 子分类（对应用户原话：“在内存模式下，请求在指定分类下创建逻辑子分类”）
=======
     * @brief 请求在指定分类下创建 logical 子分类
>>>>>>> REPLACE

---

### 5. `src/ui/TagManagerDialog.h`
Clean up comment referencing托管库模式.

<<<<<<< SEARCH
     * @param isMirrorSource 是否处于托管库模式 (true: 托管库, false: 磁盘导航模式)
=======
     * @param isMirrorSource 视图标记源
>>>>>>> REPLACE

---

### 6. `src/meta/MetadataManager.h`
Clean up comment referencing内存模式.

<<<<<<< SEARCH
     * @brief 2026-06-xx：在内存模式下执行多维搜索
=======
     * @brief 在当前引擎下执行多维搜索
>>>>>>> REPLACE

---

### 7. `src/core/CoreController.cpp`
Clean up comment referencing SQLite 内存模式.

<<<<<<< SEARCH
 * 彻底废除分布式文件模式，全面转向 SQLite 内存模式 (One-Drive-One-DB)
=======
 * 运行核心控制逻辑
>>>>>>> REPLACE

---

### 8. `src/core/VolumeOnlineManager.h`
Clean up comment referencing托管库.

<<<<<<< SEARCH
    // 校验特定托管库 (如 "QuarkMeta.library_g" 或 "G:\...") 是否处于在线状态
=======
    // 校验特定盘符/路径是否处于在线状态
>>>>>>> REPLACE

---

## Build & Verification Steps
1. Verify that `memory-1.md` is present under `QuarkMeta Architecture/Implementation Plan/`.
2. Inspect `ItemRecord.h` and `ItemRecord.cpp` to ensure `isFromMemory` parameter and dead branches are completely purged.
3. Build the project using CMake to verify 0 build errors:
   `cmake -B build && cmake --build build`
