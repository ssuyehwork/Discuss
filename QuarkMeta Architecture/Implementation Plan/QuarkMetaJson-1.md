# Implementation Plan - QuarkMetaJson-1.md

## Overview
本实施方案旨在彻底解决物理文件在复制（Copy）、移动（Move）、粘贴（Paste）及“替换”（Replace）冲突处理完成后，由于文件名大小写敏感错配导致元数据无法提取，以及旧条目残留和内存真理源 `MetaMemoryCache` 未被即时刷新的核心缺陷：

1. **统一大小写规范化 (Case Normalization)**：在 `QuarkMetaJson::roamItemMetadata` 源字典查找与目标字典写入时，统一使用 `normalizeItemKey()` 将文件名键值规范化为全小写，杜绝因带大写字母文件名（如 `Folder-sub.svg`）匹配失败导致静默跳过的 Bug。
2. **重构“替换”（Replace）语义**：在漫游写入目标位置前，优先擦除目标 `.QuarkMeta.json` 中已有的同名旧元数据记录（如覆盖冲突场景），防止新旧元数据记录并存或旧记录残留。
3. **0毫秒同步内存真理源 (MetaMemoryCache Sync)**：在落盘保存后，同步将漫游元数据写入 `MetaMemoryCache` 并通知 `MetadataManager` 广播 UI 刷新信号，确保替换/粘贴后的文件 100% 秒级展示继承的星级、颜色标记、标签、备注及尺寸等关联数据。

---

## Modified Files List
1. `src/meta/QuarkMetaJson.cpp`
2. `src/util/ShellHelper.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/QuarkMetaJson.cpp`
在 `QuarkMetaJson::roamItemMetadata` 中对键名实施规范化，补齐替换擦除与内存缓存同步逻辑：

```
<<<<<<< SEARCH
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
=======
    QString oldParent = QDir::toNativeSeparators(oldInfo.absolutePath());
    QString newParent = QDir::toNativeSeparators(newInfo.absolutePath());
    std::wstring oldFileNameKey = normalizeItemKey(oldInfo.fileName().toStdWString());
    std::wstring newFileNameKey = normalizeItemKey(newInfo.fileName().toStdWString());

    static std::mutex s_jsonRoamMutex;
    std::lock_guard<std::mutex> lock(s_jsonRoamMutex);

    // 1. 从源目录 .QuarkMeta.json 提取完整元数据包 (使用小写规范化键)
    QuarkMetaJson oldJson(oldParent.toStdWString());
    if (!oldJson.load()) return false;

    auto& oldItems = oldJson.items();
    auto it = oldItems.find(oldFileNameKey);
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

    // 4. 写入目标目录 .QuarkMeta.json 并物理原子落盘 (覆盖场景先清除旧同名条目)
    QuarkMetaJson newJson(newParent.toStdWString());
    newJson.load();
    newJson.items().erase(newFileNameKey);
    newJson.items()[newFileNameKey] = metaPackage;
    bool saveOk = newJson.save();

    if (saveOk) {
        std::wstring newWPath = MetadataManager::normalizePath(newInfo.absoluteFilePath().toStdWString());
        std::wstring oldWPath = MetadataManager::normalizePath(oldInfo.absoluteFilePath().toStdWString());

        // 0毫秒同步更新 MetaMemoryCache 内存缓存真理源，确保 UI 秒级感知
        MetadataManager::instance().ensureActivated(newWPath);
        MetaMemoryCache::instance().update(newWPath, [&metaPackage](RuntimeMeta& rMeta) {
            rMeta.rating = metaPackage.rating;
            rMeta.manualColor = metaPackage.color;
            rMeta.autoColor = metaPackage.autoColor;
            rMeta.pinned = metaPackage.pinned;
            rMeta.note = metaPackage.note;
            rMeta.url = metaPackage.url;
            rMeta.encrypted = metaPackage.encrypted;
            rMeta.width = metaPackage.width;
            rMeta.height = metaPackage.height;
            rMeta.thumbStatus = metaPackage.thumbStatus;
            rMeta.fileSize = metaPackage.size;
            rMeta.mtime = metaPackage.modificationTime;
            rMeta.added_at = metaPackage.addedAt;

            rMeta.tags.clear();
            for (const auto& t : metaPackage.tags) {
                rMeta.tags.append(QString::fromStdWString(t));
            }

            rMeta.palettes = metaPackage.palettes;
        });

        if (isMove) {
            MetaMemoryCache::instance().remove(oldWPath);
        }

        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, newFilePath);
    }

    return saveOk;
}
>>>>>>> REPLACE
```

---

### 2. `src/util/ShellHelper.cpp`
在 `ShellHelper::copyOrMoveItems` 批量物理拷贝/移动完成后，触发对目标目录的 UI 对账通知：

```
<<<<<<< SEARCH
        bool ok = (SHFileOperationW(&fileOp) == 0 && !fileOp.fAnyOperationsAborted);
        if (ok) {
            QuarkMetaJson::roamItemMetadata(p, destPath, isMove);
            DiskMediaExtractor::roamThumbnailCache(p, destPath, isMove);
        } else {
            overallOk = false;
        }
    }
    return overallOk;
=======
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
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译确认**：
   运行 CMake 编译，确保无语法或未定义错误：
   ```bash
   cmake --build build
   ```

2. **功能复核**：
   - 为包含大写字母的文件（如 `Folder-sub.svg`）设置星级（5星）、色彩标记、标签及备注；
   - 复制并“替换”至目标文件夹，验证替换后的文件 100% 成功提取并呈现继承的星级、色彩标记、标签与备注。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **真理源复用**：复用 `MetaMemoryCache::instance()` 与 `MetadataManager::instance().notifyUI` 通道；
- **防另起炉灶**：在现有 `roamItemMetadata` 逻辑上补全大小写规范化与覆盖清除，不添加任何多余数据结构。

---

## Header API Signature Verification

| 调用的类 / 成员函数 | `.h` 文件源头 | 物理精确签名 |
| :--- | :--- | :--- |
| `MetadataManager::instance()` | `src/meta/MetadataManager.h` | `static MetadataManager& instance();` |
| `MetadataManager::normalizePath(...)` | `src/meta/MetadataManager.h` | `static std::wstring normalizePath(const std::wstring& path);` |
| `MetadataManager::ensureActivated(...)` | `src/meta/MetadataManager.h` | `void ensureActivated(const std::wstring& nPath);` |
| `MetadataManager::notifyUI(...)` | `src/meta/MetadataManager.h` | `void notifyUI(RefreshLevel level, const QString& path = "");` |
| `MetaMemoryCache::instance()` | `src/meta/MetaMemoryCache.h` | `static MetaMemoryCache& instance();` |
| `MetaMemoryCache::update(...)` | `src/meta/MetaMemoryCache.h` | `void update(const std::wstring& normalizedPath, std::function<void(RuntimeMeta&)> modifier);` |
| `MetaMemoryCache::remove(...)` | `src/meta/MetaMemoryCache.h` | `void remove(const std::wstring& normalizedPath);` |

---

## Header Inclusion Chain & Type Completeness Check

- `src/meta/QuarkMetaJson.cpp`:
  - 显式包含 `#include "MetaMemoryCache.h"` 与 `#include "MetadataManager.h"`；
  - 调用的所有类型定义均具备完整头文件包含，不存在不完整类型。
- `src/util/ShellHelper.cpp`:
  - 已包含了 `#include "../meta/MetadataManager.h"`；
  - 类型定义完全闭环。
