# Implementation Plan - PermanentDeleteService-1.md

## Overview
本实施方案旨在解决物理文件永久删除（Permanent Delete / Shred）完成后，父目录隐藏配置文件 `.QuarkMeta.json` 中残留已删文件孤儿元数据记录的问题：
1. **提供静态物理抹除接口**：在 `QuarkMetaJson` 中新增静态辅助接口 `removeItemMetadata(const QString& filePath)`，负责从文件所在父目录的隐藏配置文件 `.QuarkMeta.json` 中抹除对应小写规范化文件名键的元数据，同时从内存 `MetaMemoryCache` 中清除并广播 UI 刷新；
2. **永久删除链路整合**：在 `PermanentDeleteService::execute` 物理销毁文件成功后，统一调用 `QuarkMetaJson::removeItemMetadata(p)` 清理磁盘配置文件中的孤儿条目，彻底杜绝孤儿元数据残留。

---

## Modified Files List
1. `src/meta/QuarkMetaJson.h`
2. `src/meta/QuarkMetaJson.cpp`
3. `src/core/PermanentDeleteService.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/meta/QuarkMetaJson.h`
在 `QuarkMetaJson` 类中增加静态抹除接口 `removeItemMetadata` 的声明：

```
<<<<<<< SEARCH
    /**
     * @brief 静态辅助方法：修改并落盘指定路径对应文件的 ItemMeta 记录
     */
    static void updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater);
=======
    /**
     * @brief 静态辅助方法：修改并落盘指定路径对应文件的 ItemMeta 记录
     */
    static void updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater);

    /**
     * @brief 静态辅助方法：抹除并落盘指定物理路径对应文件的元数据记录（彻底清除孤儿条目）
     */
    static bool removeItemMetadata(const QString& filePath);
>>>>>>> REPLACE
```

---

### 2. `src/meta/QuarkMetaJson.cpp`
实现 `QuarkMetaJson::removeItemMetadata` 函数：

```
<<<<<<< SEARCH
bool QuarkMetaJson::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
=======
bool QuarkMetaJson::removeItemMetadata(const QString& filePath) {
    if (filePath.isEmpty()) return true;
    QFileInfo info(filePath);
    QString parentDir = QDir::toNativeSeparators(info.absolutePath());
    std::wstring fileNameKey = normalizeItemKey(info.fileName().toStdWString());

    static std::mutex s_jsonRemoveMutex;
    std::lock_guard<std::mutex> lock(s_jsonRemoveMutex);

    QuarkMetaJson json(parentDir.toStdWString());
    if (!json.load()) return false;

    if (json.items().erase(fileNameKey) > 0) {
        json.save();
    }
    std::wstring wPath = MetadataManager::normalizePath(filePath.toStdWString());
    MetaMemoryCache::instance().remove(wPath);
    MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::PathUpdate, filePath);
    return true;
}

bool QuarkMetaJson::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
>>>>>>> REPLACE
```

---

### 3. `src/core/PermanentDeleteService.cpp`
在 `PermanentDeleteService::execute` 物理删除文件成功后，调用 `QuarkMetaJson::removeItemMetadata(p)` 清理配置文件与内存缓存：

```
<<<<<<< SEARCH
#include "../meta/MetadataManager.h"
=======
#include "../meta/MetadataManager.h"
#include "../meta/QuarkMetaJson.h"
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                if (physicalOk) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::DeletePermanently;
                    cmd.targetPaths << QString::fromStdWString(wp);
                    CoreEngine::instance().executeCommand(cmd);
                    UndoManager::instance().removeCommandsForPath(p);
                }
=======
                if (physicalOk) {
                    QuarkMetaJson::removeItemMetadata(p);
                    AppCommand cmd;
                    cmd.type = AppCommandType::DeletePermanently;
                    cmd.targetPaths << QString::fromStdWString(wp);
                    CoreEngine::instance().executeCommand(cmd);
                    UndoManager::instance().removeCommandsForPath(p);
                }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译确认**：
   运行 CMake 编译，确认无语法或符号未定义错误：
   ```bash
   cmake --build build
   ```

2. **孤儿残留清理对账**：
   - 步骤 1：创建一个测试文件 `test.txt` 并添加星级与标签；
   - 步骤 2：对 `test.txt` 执行永久删除；
   - 步骤 3：打开该文件夹下隐藏的 `.QuarkMeta.json`，核查 `"test.txt"` 条目是否已被彻底删除，无孤儿元数据残留；
   - 步骤 4：在同目录下重新新建 `test.txt`，确认新文件不会误继承旧文件的星级与标签。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **管道集中管理**：所有配置文件抹除操作统一下沉至 `QuarkMetaJson::removeItemMetadata`，避免逻辑散落；
- **真理源同步**：同步清除 `MetaMemoryCache` 内存缓存，确保视图与内存数据绝对一致。

---

## Header API Signature Verification

| 调用的类 / 成员函数 | `.h` 文件源头 | 物理精确签名 |
| :--- | :--- | :--- |
| `QuarkMetaJson::removeItemMetadata(...)` | `src/meta/QuarkMetaJson.h` | `static bool removeItemMetadata(const QString& filePath);` |
| `MetadataManager::normalizePath(...)` | `src/meta/MetadataManager.h` | `static std::wstring normalizePath(const std::wstring& path);` |
| `MetaMemoryCache::remove(...)` | `src/meta/MetaMemoryCache.h` | `void remove(const std::wstring& normalizedPath);` |

---

## Header Inclusion Chain & Type Completeness Check

- `src/meta/QuarkMetaJson.cpp`:
  - 已包含了 `MetaMemoryCache.h` 与 `MetadataManager.h`。
- `src/core/PermanentDeleteService.cpp`:
  - 显式包含了 `QuarkMetaJson.h`。
