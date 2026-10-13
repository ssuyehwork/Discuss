# Implementation Plan - QuarkMetaJson.md

## Overview
本实施方案旨在解决物理文件在复制（Copy）、移动（Move）、粘贴（Paste）及“替换”（Replace）冲突处理完成后，虽然文件已落盘且隐藏配置文件 `.QuarkMeta.json` 已更新，但由于内存真理源 `MetaMemoryCache` 未被即时刷新，导致界面（与右侧 `MetaPanel`）无法秒级呈现漫游后元数据（星级、颜色标记、标签、备注、关联网址及尺寸）的问题。

---

## Modified Files List
1. `src/meta/QuarkMetaJson.cpp`
2. `src/util/ShellHelper.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/QuarkMetaJson.cpp`
在 `QuarkMetaJson::roamItemMetadata` 中，保存目标文件 `.QuarkMeta.json` 后，增加将 `metaPackage` 深克隆数据 0 毫秒同步更新至 `MetaMemoryCache` 内存缓存并向 `MetadataManager` 广播 UI 刷新的逻辑：

```
<<<<<<< SEARCH
    // 4. 写入目标目录 .QuarkMeta.json 并物理原子落盘
    QuarkMetaJson newJson(newParent.toStdWString());
    newJson.load();
    newJson.items()[newFileName] = metaPackage;
    return newJson.save();
}
=======
    // 4. 写入目标目录 .QuarkMeta.json 并物理原子落盘
    QuarkMetaJson newJson(newParent.toStdWString());
    newJson.load();
    newJson.items()[newFileName] = metaPackage;
    bool saveOk = newJson.save();

    if (saveOk) {
        std::wstring newWPath = MetadataManager::normalizePath(newInfo.absoluteFilePath().toStdWString());
        std::wstring oldWPath = MetadataManager::normalizePath(oldInfo.absoluteFilePath().toStdWString());

        // 同步更新 MetaMemoryCache 内存缓存真理源，确保 UI 秒级感知
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
   运行 CMake 编译，确保无类型未定义或语法错误：
   ```bash
   cmake --build build
   ```

2. **功能复核**：
   - 为源文件设置星级（5星）、色彩标记、标签及备注；
   - 复制/粘贴或“替换”至目标文件夹，验证粘贴/替换后的文件 100% 呈现继承的星级、色彩标记、标签与备注。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **真理源复用**：复用 `MetaMemoryCache::instance()` 与 `MetadataManager::instance().notifyUI` 通道；
- **防另起炉灶**：在现有漫游逻辑上实现补全，不增加多余全局变量。

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
  - 已显式包含 `#include "MetaMemoryCache.h"` 与 `#include "MetadataManager.h"`；
  - 调用的所有类型定义均具备完整头文件包含，不存在不完整类型。
