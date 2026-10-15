# Implementation Plan - BatchDataMigrationOptimization.md

## Overview
本实施方案旨在将 QuarkMeta 中的数据迁移（移动与复制）底层执行链路由现有的“循环逐个单独迁移”重构优化为“一次性批量/批次迁移”。
核心目标：
1. **物理搬运**：将 `ShellHelper::copyOrMoveItems` 重构为单次多目标 `SHFileOperationW` 批处理调用，避免反复创建 `SHFILEOPSTRUCTW`。同时增加 `allocatedDestPaths` 碰撞规避机制，防止不同源目录同名文件撞名。
2. **元数据漫游**：在 `QuarkMetaJson` 中新增 `roamBatchMetadata` 批处理接口，按源/目标物理文件夹分组（Group by folder），使每个文件夹的 `.QuarkMeta.json` 隐藏文件仅执行一次 `load()` 与一次 `save()` 落盘。仅在 `isMove == true` 时才执行 `MetadataManager::renameItem` 避免复制操作损坏源端内存元数据。
3. **缓存驱逐与类型类型核对**：`invalidateFolderCache` 参数接收 `const std::wstring&` 类型，使用 `dirPath.toStdWString()` 转换驱动。
4. **缩略图漫游**：在 `DiskMediaExtractor` 中新增 `roamThumbnailCacheBatch` 批处理接口，批量重命名/复制缩略图缓存文件。

上层调用方（`ContentFileOpsHandler` 和 `DiskIoService`）的函数签名保持完全不变，无缝兼容。

---

## Modified Files List
- `src/util/ShellHelper.h`
- `src/util/ShellHelper.cpp`
- `src/meta/QuarkMetaJson.h`
- `src/meta/QuarkMetaJson.cpp`
- `src/util/DiskMediaExtractor.h`
- `src/util/DiskMediaExtractor.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/util/ShellHelper.h` 与 `src/util/ShellHelper.cpp`

#### `src/util/ShellHelper.h`
保持既有 `copyOrMoveItems` 签名不变：
```
<<<<<<< SEARCH
    static bool copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove,
                                bool overwriteAll = false, bool autoRenameAll = false,
                                const QSet<QString>& overwriteFiles = {}, const QSet<QString>& autoRenameFiles = {});
=======
    static bool copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove,
                                bool overwriteAll = false, bool autoRenameAll = false,
                                const QSet<QString>& overwriteFiles = {}, const QSet<QString>& autoRenameFiles = {});
>>>>>>> REPLACE
```

#### `src/util/ShellHelper.cpp`
重构 `copyOrMoveItems` 物理实现，移除逐文件 `SHFileOperationW` 循环，引入目标路径冲突重分配 `allocatedDestPaths`，组装双 `\0` 宽字符流并单次调用 `SHFileOperationW`：

```
<<<<<<< SEARCH
bool ShellHelper::copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove,
                                   bool overwriteAll, bool autoRenameAll,
                                   const QSet<QString>& overwriteFiles, const QSet<QString>& autoRenameFiles) {
#ifdef Q_OS_WIN
    if (sourcePaths.isEmpty() || destDir.isEmpty()) return false;

    bool overallOk = true;
    for (const QString& p : sourcePaths) {
        QFileInfo info(p);
        QString destPath = QDir(destDir).filePath(info.fileName());

        bool shouldAutoRename = autoRenameAll || autoRenameFiles.contains(p);
        bool shouldOverwrite = overwriteAll || overwriteFiles.contains(p);

        if (QFile::exists(destPath)) {
            if (shouldAutoRename) {
                QString baseName = info.completeBaseName();
                QString suffix = info.suffix();
                int counter = 1;
                while (QFile::exists(destPath)) {
                    QString newFileName = suffix.isEmpty()
                        ? QString("%1-%2").arg(baseName).arg(counter++)
                        : QString("%1-%2.%3").arg(baseName).arg(counter++).arg(suffix);
                    destPath = QDir(destDir).filePath(newFileName);
                }
            } else if (shouldOverwrite) {
                QFile::remove(destPath);
            }
        }

        std::wstring from = QDir::toNativeSeparators(p).toStdWString() + L'\0' + L'\0';
        std::wstring to = QDir::toNativeSeparators(destPath).toStdWString() + L'\0' + L'\0';

        SHFILEOPSTRUCTW fileOp = { 0 };
        fileOp.wFunc = isMove ? FO_MOVE : FO_COPY;
        fileOp.pFrom = from.c_str();
        fileOp.pTo = to.c_str();
        fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_SILENT;

        bool ok = (SHFileOperationW(&fileOp) == 0 && !fileOp.fAnyOperationsAborted);
        if (ok) {
            QuarkMetaJson::roamItemMetadata(p, destPath, isMove);
            DiskMediaExtractor::roamThumbnailCache(p, destPath, isMove);
        } else {
            overallOk = false;
        }
    }
    if (overallOk) {
        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, destDir);
    }
    return overallOk;
#else
    Q_UNUSED(sourcePaths);
    Q_UNUSED(destDir);
    Q_UNUSED(isMove);
    Q_UNUSED(overwriteAll);
    Q_UNUSED(autoRenameAll);
    Q_UNUSED(overwriteFiles);
    Q_UNUSED(autoRenameFiles);
    return false;
#endif
}
=======
bool ShellHelper::copyOrMoveItems(const QStringList& sourcePaths, const QString& destDir, bool isMove,
                                   bool overwriteAll, bool autoRenameAll,
                                   const QSet<QString>& overwriteFiles, const QSet<QString>& autoRenameFiles) {
#ifdef Q_OS_WIN
    if (sourcePaths.isEmpty() || destDir.isEmpty()) return false;

    QList<QPair<QString, QString>> pathPairs;
    pathPairs.reserve(sourcePaths.size());

    std::wstring fromBuffer;
    std::wstring toBuffer;
    QSet<QString> allocatedDestPaths;

    for (const QString& p : sourcePaths) {
        QFileInfo info(p);
        QString destPath = QDir(destDir).filePath(info.fileName());

        bool shouldAutoRename = autoRenameAll || autoRenameFiles.contains(p);
        bool shouldOverwrite = overwriteAll || overwriteFiles.contains(p);

        if (QFile::exists(destPath) || allocatedDestPaths.contains(destPath.toLower())) {
            if (shouldAutoRename) {
                QString baseName = info.completeBaseName();
                QString suffix = info.suffix();
                int counter = 1;
                while (QFile::exists(destPath) || allocatedDestPaths.contains(destPath.toLower())) {
                    QString newFileName = suffix.isEmpty()
                        ? QString("%1-%2").arg(baseName).arg(counter++)
                        : QString("%1-%2.%3").arg(baseName).arg(counter++).arg(suffix);
                    destPath = QDir(destDir).filePath(newFileName);
                }
            } else if (shouldOverwrite) {
                QFile::remove(destPath);
            }
        }

        allocatedDestPaths.insert(destPath.toLower());
        pathPairs.append(qMakePair(p, destPath));

        std::wstring fromStr = QDir::toNativeSeparators(p).toStdWString();
        std::wstring toStr = QDir::toNativeSeparators(destPath).toStdWString();

        fromBuffer.append(fromStr);
        fromBuffer.push_back(L'\0');

        toBuffer.append(toStr);
        toBuffer.push_back(L'\0');
    }

    fromBuffer.push_back(L'\0');
    toBuffer.push_back(L'\0');

    SHFILEOPSTRUCTW fileOp = { 0 };
    fileOp.wFunc = isMove ? FO_MOVE : FO_COPY;
    fileOp.pFrom = fromBuffer.c_str();
    fileOp.pTo = toBuffer.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION | FOF_SILENT | FOF_MULTIDESTFILES;

    bool ok = (SHFileOperationW(&fileOp) == 0 && !fileOp.fAnyOperationsAborted);
    if (ok && !pathPairs.isEmpty()) {
        QuarkMetaJson::roamBatchMetadata(pathPairs, isMove);
        DiskMediaExtractor::roamThumbnailCacheBatch(pathPairs, isMove);
        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, destDir);
    }
    return ok;
#else
    Q_UNUSED(sourcePaths);
    Q_UNUSED(destDir);
    Q_UNUSED(isMove);
    Q_UNUSED(overwriteAll);
    Q_UNUSED(autoRenameAll);
    Q_UNUSED(overwriteFiles);
    Q_UNUSED(autoRenameFiles);
    return false;
#endif
}
>>>>>>> REPLACE
```

---

### 2. `src/meta/QuarkMetaJson.h` 与 `src/meta/QuarkMetaJson.cpp`

#### `src/meta/QuarkMetaJson.h`
新增 `roamBatchMetadata` 批处理接口与辅助包含：
```
<<<<<<< SEARCH
    /**
     * @brief 物理漫游元数据包：支持文件/文件夹的移动转移与复制克隆
     * @param oldFilePath 源路径
     * @param newFilePath 目标路径
     * @param isMove true 为剪切/移动（抹除源数据）；false 为复制（深克隆副本）
     */
    static bool roamItemMetadata(const QString& oldFilePath, const QString& newFilePath, bool isMove);
=======
    /**
     * @brief 物理漫游元数据包：支持文件/文件夹的移动转移与复制克隆
     * @param oldFilePath 源路径
     * @param newFilePath 目标路径
     * @param isMove true 为剪切/移动（抹除源数据）；false 为复制（深克隆副本）
     */
    static bool roamItemMetadata(const QString& oldFilePath, const QString& newFilePath, bool isMove);

    /**
     * @brief 批量物理漫游元数据包：按源/目标文件夹归类合并（Group by folder），单次 JSON 读写落盘
     * @param pairs 源绝对路径与目标绝对路径映射对
     * @param isMove true 为剪切/移动（抹除源数据）；false 为复制（深克隆副本）
     */
    static bool roamBatchMetadata(const QList<QPair<QString, QString>>& pairs, bool isMove);
>>>>>>> REPLACE
```

#### `src/meta/QuarkMetaJson.cpp`
实现 `roamBatchMetadata`，按源目录与目标目录归类聚合，单次 `load()`，批量合并操作，仅在 `isMove == true` 时更新内存与抹除源数据，单次 `save()`：

```
<<<<<<< SEARCH
bool QuarkMetaJson::roamItemMetadata(const QString& oldFilePath, const QString& newFilePath, bool isMove) {
=======
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
            ShellHelper::ensureHidden(srcDir.toStdWString() + L"/.QuarkMeta.json");
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
            ShellHelper::ensureHidden(destDir.toStdWString() + L"/.QuarkMeta.json");
            QuarkMetaJsonStore::instance().invalidateFolderCache(destDir.toStdWString());
        }
    }

    return true;
}

bool QuarkMetaJson::roamItemMetadata(const QString& oldFilePath, const QString& newFilePath, bool isMove) {
>>>>>>> REPLACE
```

---

### 3. `src/util/DiskMediaExtractor.h` 与 `src/util/DiskMediaExtractor.cpp`

#### `src/util/DiskMediaExtractor.h`
新增 `roamThumbnailCacheBatch` 批处理接口：
```
<<<<<<< SEARCH
    static void roamThumbnailCache(const QString& oldFilePath, const QString& newFilePath, bool isMove);
=======
    static void roamThumbnailCache(const QString& oldFilePath, const QString& newFilePath, bool isMove);
    static void roamThumbnailCacheBatch(const QList<QPair<QString, QString>>& pairs, bool isMove);
>>>>>>> REPLACE
```

#### `src/util/DiskMediaExtractor.cpp`
实现 `roamThumbnailCacheBatch`，高效批量迁移/复制缩略图：

```
<<<<<<< SEARCH
void DiskMediaExtractor::roamThumbnailCache(const QString& oldFilePath, const QString& newFilePath, bool isMove) {
=======
void DiskMediaExtractor::roamThumbnailCacheBatch(const QList<QPair<QString, QString>>& pairs, bool isMove) {
    if (pairs.isEmpty()) return;

    for (const auto& pair : pairs) {
        roamThumbnailCache(pair.first, pair.second, isMove);
    }
}

void DiskMediaExtractor::roamThumbnailCache(const QString& oldFilePath, const QString& newFilePath, bool isMove) {
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 编译步骤
```bash
cmake -B build
cmake --build build --config Release
```

### 验证方法
1. **多文件拖拽批量移动测试**：选中 10 个具有标签/颜色/星级元数据的文件，拖拽移动至目标文件夹。验证 `SHFileOperationW` 单次弹出操作系统平滑搬运进度条，迁移完成后 10 个文件的元数据、色彩与图标 100% 完美保留。
2. **I/O 放大自查**：对同一个源文件夹迁移 50 个文件时，源文件夹与目标文件夹下的 `.QuarkMeta.json` 文件分别仅被写入（`save()`）1 次，而非 50 次。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **物理 I/O**：通过 `SHFileOperationW` 批量宽字符 `FOF_MULTIDESTFILES` 既有 Windows API，消除了对循环 `copyOrMoveItems` 的重复滥用。
- **元数据落盘**：复用既有 `QuarkMetaJson::load()` 与 `QuarkMetaJson::save()` API，仅改动内存层面的聚合策略。

---

## Header API Signature Verification
| 类名 / 接口 | 物理签名核查 (.h 源头) | 结论 |
| :--- | :--- | :--- |
| `QuarkMetaJson::roamBatchMetadata` | `static bool roamBatchMetadata(const QList<QPair<QString, QString>>& pairs, bool isMove);` | 新增批处理签名 |
| `DiskMediaExtractor::roamThumbnailCacheBatch` | `static void roamThumbnailCacheBatch(const QList<QPair<QString, QString>>& pairs, bool isMove);` | 新增批处理签名 |
| `ShellHelper::copyOrMoveItems` | 保持原有公开签名无缝兼容 | 100% 向后兼容 |

---

## Header Inclusion Chain & Type Completeness Check
- `ShellHelper.cpp` 包含 `#include "../meta/QuarkMetaJson.h"`，使用 `QPair`、`QList` 对应 Qt 头文件完整。
- `QuarkMetaJson.cpp` 包含 `#include "../meta/MetadataManager.h"`、`#include "../util/ShellHelper.h"` 与 `#include "QuarkMetaJsonStore.h"`，类型完整，无头文件断裂风险。
