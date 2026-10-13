# Implementation Plan - MetadataRoamingAndCacheSync.md

## Overview
在物理文件的复制（Copy）、移动（Move）和粘贴（Paste）操作中，现有的 `QuarkMetaJson::roamItemMetadata` 虽然将元数据（星级、颜色、标签、备注、关联网址、长宽尺寸、色彩调色板等）写入了目标位置隐藏的 `.QuarkMeta.json` 侧车配置文件中，但由于未同步更新全局内存缓存（`MetaMemoryCache`）及驱动 `MetadataManager` 通知 UI，导致粘贴/覆盖操作完成后，界面（及右侧 `MetaPanel`）无法即时感知漫游过来的元数据，造成“粘贴后关联关联丢失”的假象。

本实施方案旨在修补该元数据漫游与内存缓存脱节的缺陷，确保在文件复制/移动/粘贴/覆盖后，源文件的所有扩展元数据 100% 被深克隆/迁移并即时刷新呈现于界面。

---

## Modified Files List
1. `src/meta/QuarkMetaJson.cpp`
2. `src/util/ShellHelper.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/QuarkMetaJson.cpp`
在 `QuarkMetaJson::roamItemMetadata` 函数中：
- 当写入目标目录 `.QuarkMeta.json` 后，同步将漫游后的 `ItemMeta` 转换并更新至 `MetaMemoryCache` 内存缓存中；
- 若为移动操作 (`isMove == true`)，同步从源路径的 `MetaMemoryCache` 中移除旧条目；
- 触发 `MetadataManager::instance().notifyUI(RefreshLevel::PathUpdate, newFilePath)`，通知界面与关联 Controller 刷新。

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

同时，补齐 `#include "MetaMemoryCache.h"` 与 `#include "MetadataManager.h"` 头文件包含，防止类型不完整或缺失定义。

```
<<<<<<< SEARCH
#include "QuarkMetaJson.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <mutex>
=======
#include "QuarkMetaJson.h"
#include "MetaMemoryCache.h"
#include "MetadataManager.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <mutex>
>>>>>>> REPLACE
```

---

### 2. `src/util/ShellHelper.cpp`
在 `ShellHelper::copyOrMoveItems` 物理复制/移动完成后，增加对目标目录 `.QuarkMeta.json` 与 `MetadataManager` 的广播通知，确保批量漫游后 UI 能实时全量对账与重绘：

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
   在终端运行 CMake 编译命令确认无 MOC / 符号未定义或不完整类型（C2027/C2039）编译错误：
   ```bash
   cmake --build build
   ```

2. **功能测试验证**：
   - 步骤 1：在一个文件夹中选中某个文件，设置星级（如 5 星）、红色标记、添加标签“测试标签”及备注；
   - 步骤 2：按下 `Ctrl + C` 复制该文件，然后进入另一个文件夹按下 `Ctrl + V` 粘贴；
   - 步骤 3：核查粘贴后的新文件是否立即展示 5 星、红色标记与“测试标签”，且右侧 `MetaPanel` 同步呈现；
   - 步骤 4：在同名冲突场景下选择“替换”，核查覆盖后的文件是否精准继承了新粘贴源文件的所有元数据关联。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **真理源复用**：完整复用了统一的全局内存真理源 `MetaMemoryCache::instance()` 与唯一更新管道 `MetadataManager::instance().notifyUI`；
- **无另起炉灶**：没有创建任何冗余的私有元数据数据结构或独立的映射字典，完全在既有的 `QuarkMetaJson::roamItemMetadata` 与 `MetaMemoryCache` 通道内闭环。

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
  - 显式增加 `#include "MetaMemoryCache.h"`
  - 显式增加 `#include "MetadataManager.h"`
  - 核查：`RuntimeMeta`、`ItemMeta`、`MetaMemoryCache`、`MetadataManager` 类型的完整定义均已由各自的 `.h` 头文件显式提供，不存在不完整类型或依赖隐式间接包含的情况。
