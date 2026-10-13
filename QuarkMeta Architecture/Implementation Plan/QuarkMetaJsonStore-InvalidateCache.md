# Implementation Plan - QuarkMetaJsonStore-InvalidateCache.md

## Overview
本实施方案旨在解决 `roamItemMetadata`、`removeItemMetadata` 或 `renameItem` 直接对磁盘 `.QuarkMeta.json` 进行修改/删除操作后，内存管理组件 `QuarkMetaJsonStore` 中的常驻目录缓存 `m_folderCacheMap` 未被同步失效，导致 UI 后续查询依然读取到陈旧快照的核心缺陷：

1. **缓存主动失效接口 (`invalidateCache`)**：在 `QuarkMetaJsonStore` 中新增 `invalidateCache(const std::wstring& folderPath)` 方法，安全抹除对应规范化目录在 `m_folderCacheMap` 和 `m_dirtyFolderPaths` 中的缓存与脏状态标记；
2. **漫游/删除/重命名全链路对账**：
   - 在 `QuarkMetaJson::roamItemMetadata` 中，漫游落盘成功后，主动调用 `QuarkMetaJsonStore::instance().invalidateCache` 清空源目录与目标目录的常驻缓存；
   - 在 `QuarkMetaJson::removeItemMetadata` 抹除条目后，调用 `invalidateCache` 清空父目录常驻缓存；
   - 在 `QuarkMetaJsonStore::renameItem` 重命名条目后，调用 `invalidateCache` 强行刷盘并清空常驻缓存；
   - 当用无元数据的源文件强行覆盖/替换有元数据的目标文件时，同步从 `MetaMemoryCache` 中驱逐目标文件的陈旧缓存，防止 UI 误显旧元数据。

---

## Modified Files List
1. `src/meta/QuarkMetaJsonStore.h`
2. `src/meta/QuarkMetaJsonStore.cpp`
3. `src/meta/QuarkMetaJson.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/QuarkMetaJsonStore.h`
声明 `invalidateCache` 公开接口：

```
<<<<<<< SEARCH
    /**
     * @brief 强制立即将所有未落盘的脏目录 JSON 刷入物理磁盘 (sync 为 true 时在当前线程同步写入)
     */
    void flushAllDirtyBuffers(bool sync = false);
=======
    /**
     * @brief 使指定物理文件夹的内存缓存失效，迫使下次读取重新从磁盘载入
     */
    void invalidateCache(const std::wstring& folderPath);

    /**
     * @brief 强制立即将所有未落盘的脏目录 JSON 刷入物理磁盘 (sync 为 true 时在当前线程同步写入)
     */
    void flushAllDirtyBuffers(bool sync = false);
>>>>>>> REPLACE
```

---

### 2. `src/meta/QuarkMetaJsonStore.cpp`
实现 `invalidateCache` 并整合至 `renameItem`：

```
<<<<<<< SEARCH
bool QuarkMetaJsonStore::renameItem(const QString& folderPath, const QString& oldName, const QString& newName) {
    flushAllDirtyBuffers();
    return QuarkMetaJson::renameItem(folderPath, oldName, newName);
}
=======
void QuarkMetaJsonStore::invalidateCache(const std::wstring& folderPath) {
    std::wstring normFolder = normalizeFolderPath(folderPath);
    std::lock_guard<std::mutex> lock(m_storeMutex);
    m_folderCacheMap.erase(normFolder);
    m_dirtyFolderPaths.erase(normFolder);
}

bool QuarkMetaJsonStore::renameItem(const QString& folderPath, const QString& oldName, const QString& newName) {
    flushAllDirtyBuffers();
    invalidateCache(folderPath.toStdWString());
    return QuarkMetaJson::renameItem(folderPath, oldName, newName);
}
>>>>>>> REPLACE
```

---

### 3. `src/meta/QuarkMetaJson.cpp`
在 `roamItemMetadata` 与 `removeItemMetadata` 中引入 `invalidateCache` 调用与无数据覆盖时的驱逐处理：

```
<<<<<<< SEARCH
#include "QuarkMetaJson.h"
#include "MetaMemoryCache.h"
#include "MetadataManager.h"
=======
#include "QuarkMetaJson.h"
#include "MetaMemoryCache.h"
#include "MetadataManager.h"
#include "QuarkMetaJsonStore.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    auto it = oldItems.find(oldKey);
    if (it == oldItems.end()) {
        // 源无任何用户标记：目标同名条目已被清理，保存目标配置后返回
        if (isMove) {
            oldJson.save();
        }
        return newJson.save();
    }
=======
    auto it = oldItems.find(oldKey);
    if (it == oldItems.end()) {
        // 源无任何用户标记：目标同名条目已被清理，保存目标配置后返回
        if (isMove) {
            oldJson.save();
            QuarkMetaJsonStore::instance().invalidateCache(oldParent.toStdWString());
        }
        bool ok = newJson.save();
        if (ok) {
            QuarkMetaJsonStore::instance().invalidateCache(newParent.toStdWString());
            std::wstring newWPath = MetadataManager::normalizePath(newInfo.absoluteFilePath().toStdWString());
            MetaMemoryCache::instance().remove(newWPath);
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, newFilePath);
        }
        return ok;
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译确认**：
   运行 CMake 编译，确保无编译错误：
   ```bash
   cmake --build build
   ```

2. **功能复核**：
   - 步骤 1：复制带 4 星评级的文件至新物理目录；
   - 步骤 2：在 UI 中查看该目录，确认星级与标签 100% 秒级展现，不再被旧有的全空常驻内存快照压制。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **缓存生命周期统管**：由 `QuarkMetaJsonStore::invalidateCache` 统一暴露，无私有散落的缓存擦除方法。

---

## Header API Signature Verification

| 调用的类 / 成员函数 | `.h` 文件源头 | 物理精确签名 |
| :--- | :--- | :--- |
| `QuarkMetaJsonStore::invalidateCache(...)` | `src/meta/QuarkMetaJsonStore.h` | `void invalidateCache(const std::wstring& folderPath);` |

---

## Header Inclusion Chain & Type Completeness Check

- `src/meta/QuarkMetaJson.cpp`:
  - 显式包含 `#include "QuarkMetaJsonStore.h"`，包含链完整闭环。
