# QuarkMetaJson-3: roamItemMetadata — Store 缓存失效修复（方案 A）

## 1. Overview（概述与解决的问题）

### 问题描述
将文件从文件夹 A **复制**到文件夹 B 后，文件夹 B 中的该文件不显示星级、标签、颜色标记、备注等绑定关联。**剪切（移动）**场景同理。

### 根因定位

系统中存在两个独立的内存缓存：

| 缓存 | 键 | 使用者 |
|---|---|---|
| `MetaMemoryCache` | 完整文件路径（绝对路径） | `MetadataManager` 系列设置方法 |
| `QuarkMetaJsonStore::m_folderCacheMap` | 父文件夹路径 | `MetaCacheDecorator::decorate()` |

`MetaCacheDecorator::decorate()` 在扫描完目录、准备装配元数据时，**走的是 `QuarkMetaJsonStore::readFolderMeta()`**，该方法命中缓存后直接返回内存快照，**不重读磁盘**。

`QuarkMetaJson::roamItemMetadata` 在复制时：
1. 正确把元数据写入目标文件夹 B 的磁盘 `.QuarkMeta.json` ✅
2. 调用 `MetaMemoryCache::update(newWPath, ...)` 试图同步内存缓存 ❌
   → 但目标文件是**新文件**，根本不在 `MetaMemoryCache` 里
   → `update()` 内部 `find()` 失败后**静默跳过**，什么都没写入
3. 没有对 `QuarkMetaJsonStore::m_folderCacheMap` 做任何处理 ❌
   → 下次 `readFolderMeta("B")` 命中旧快照，返回不含该文件元数据的过期数据

### 修复思路（方案 A — 最小改动，回归 Old-13 简洁设计）

在 `roamItemMetadata` 写盘成功后：
- **移除**已破损的 `MetaMemoryCache::update()` + `ensureActivated()` 调用
- **改为**强制驱逐 `QuarkMetaJsonStore` 缓存中目标文件夹的条目
- 若为移动（isMove），同时驱逐源文件夹缓存
- 保留 `MetadataManager::notifyUI()` 让 UI 感知刷新

驱逐后，下次 `readFolderMeta` 发生 cache miss，自动从磁盘重新读取最新写入的 `.QuarkMeta.json`，元数据正确装配。

---

## 2. Modified Files List（影响文件清单）

| 文件 | 改动类型 |
|---|---|
| `src/meta/QuarkMetaJsonStore.h` | 新增 public 方法签名 `invalidateFolderCache` |
| `src/meta/QuarkMetaJsonStore.cpp` | 新增方法实现 |
| `src/meta/QuarkMetaJson.cpp` | 替换 `roamItemMetadata` 成功回调块；新增 include |

**CMakeLists.txt 无需改动**（均为现有已注册文件内的修改）。

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/meta/QuarkMetaJsonStore.h` — 新增 public 方法

```
<<<<<<< SEARCH
    /**
     * @brief 强制立即将所有未落盘的脏目录 JSON 刷入物理磁盘 (sync 为 true 时在当前线程同步写入)
     */
    void flushAllDirtyBuffers(bool sync = false);
=======
    /**
     * @brief 强制立即将所有未落盘的脏目录 JSON 刷入物理磁盘 (sync 为 true 时在当前线程同步写入)
     */
    void flushAllDirtyBuffers(bool sync = false);

    /**
     * @brief 驱逐指定文件夹的内存缓存快照，使下次 readFolderMeta() 强制从磁盘重新加载
     * @param folderPath 目标物理文件夹路径（原始大小写，内部自动规范化）
     */
    void invalidateFolderCache(const std::wstring& folderPath);
>>>>>>> REPLACE
```

---

### 3.2 `src/meta/QuarkMetaJsonStore.cpp` — 实现 `invalidateFolderCache`

在 `renameItem` 方法之后、`} // namespace QuarkMeta` 之前插入：

```
<<<<<<< SEARCH
bool QuarkMetaJsonStore::renameItem(const QString& folderPath, const QString& oldName, const QString& newName) {
    flushAllDirtyBuffers();
    return QuarkMetaJson::renameItem(folderPath, oldName, newName);
}

} // namespace QuarkMeta
=======
bool QuarkMetaJsonStore::renameItem(const QString& folderPath, const QString& oldName, const QString& newName) {
    flushAllDirtyBuffers();
    return QuarkMetaJson::renameItem(folderPath, oldName, newName);
}

void QuarkMetaJsonStore::invalidateFolderCache(const std::wstring& folderPath) {
    std::wstring normFolder = normalizeFolderPath(folderPath);
    std::lock_guard<std::mutex> lock(m_storeMutex);
    m_folderCacheMap.erase(normFolder);
    m_dirtyFolderPaths.erase(normFolder); // 同步清除脏标记，防止用已驱逐的旧快照回写磁盘
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### 3.3 `src/meta/QuarkMetaJson.cpp` — 修复 `roamItemMetadata`

**Step 1：新增 include（在现有 include 列表末尾）**

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

**Step 2：替换 `roamItemMetadata` 成功回调块（完整替换 if(saveOk) 块，当前代码位于第 234–268 行）**

```
<<<<<<< SEARCH
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
=======
    if (saveOk) {
        // 驱逐目标文件夹的 Store 内存缓存快照，确保下次 readFolderMeta() 强制重读磁盘最新 JSON
        QuarkMetaJsonStore::instance().invalidateFolderCache(newParent.toStdWString());
        if (isMove) {
            // 移动时同步驱逐源文件夹缓存（元数据已从源 JSON 物理抹除）
            QuarkMetaJsonStore::instance().invalidateFolderCache(oldParent.toStdWString());
        }
        MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, newFilePath);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

```bash
cmake --build . --config Release --target QuarkMeta
```

### 手动验证步骤
1. 在文件夹 A 中选中一个文件，为其设置星级 ★★★、颜色标记（红色）、标签「测试」、备注「hello」
2. 复制该文件（Ctrl+C）
3. 导航至文件夹 B，粘贴（Ctrl+V）
4. **预期结果**：文件夹 B 中粘贴后的文件立即显示 ★★★、红色标记、标签「测试」、备注「hello」
5. 重新启动程序，再次打开文件夹 B，验证元数据持久化正确
6. 额外验证：剪切（Ctrl+X）场景同样通过；文件夹 A 中原文件的元数据被正确清除

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check

- ✅ 未调用 `loadDirectory` 私自另起炉灶，元数据装配依然走 `MetaCacheDecorator::decorate()` → `QuarkMetaJsonStore::readFolderMeta()` SSOT 通道
- ✅ 磁盘写入仍通过 `QuarkMetaJson::save()` 原有机制，未绕过
- ✅ UI 刷新通知仍通过 `MetadataManager::notifyUI()` SSOT 通道
- ✅ `invalidateFolderCache` 为新增专用接口，不与任何已有接口重叠，无另起炉灶行为

---

## 6. Header API Signature Verification（头文件 API 物理签名核查表）

| 调用点 | 物理签名来源文件 | 精确签名 |
|---|---|---|
| `QuarkMetaJsonStore::instance()` | `QuarkMetaJsonStore.h` | `static QuarkMetaJsonStore& instance();` |
| `QuarkMetaJsonStore::invalidateFolderCache` | **本方案新增** | `void invalidateFolderCache(const std::wstring& folderPath);` |
| `QuarkMetaJsonStore::normalizeFolderPath` | `QuarkMetaJsonStore.cpp` private | `static std::wstring normalizeFolderPath(const std::wstring& path);` |
| `MetadataManager::instance()` | `MetadataManager.h` | `static MetadataManager& instance();` |
| `MetadataManager::notifyUI` | `MetadataManager.h` | `void notifyUI(RefreshLevel level, const QString& path = {});` |

---

## 7. Header Inclusion Chain & Type Completeness Check

### `QuarkMetaJson.cpp` 新增 include 安全性
- 新增 `#include "QuarkMetaJsonStore.h"`
- `QuarkMetaJsonStore.h` 内部已 `#include "QuarkMetaJson.h"`
- 方向为 `.cpp` 包含 `.h`，**无循环依赖**，安全

### 删除的调用路径核查（include 断裂检查）
- 删除了 `MetaMemoryCache::instance().update()` 及 `.remove()` 调用
- 但 `MetaMemoryCache.h` 仍被 `removeItemMetadata`（第289行）使用 → **保留 include，无断裂** ✅
- 删除了 `MetadataManager::instance().ensureActivated()` 调用
- 但 `MetadataManager.h` 仍被 `roamItemMetadata` 的 `notifyUI()` 及文件其他方法使用 → **保留 include，无断裂** ✅

### `QuarkMetaJsonStore.cpp` 无新增 include
- `invalidateFolderCache` 仅使用：
  - `normalizeFolderPath()`（同文件 private 方法）
  - `m_storeMutex`、`m_folderCacheMap`、`m_dirtyFolderPaths`（均为成员变量）
  - `std::lock_guard<std::mutex>` → 已由 `<mutex>` 覆盖（文件顶部已有）
- **全部类型完整，无需新增 include** ✅
