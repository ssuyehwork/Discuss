# Implementation Plan - DiskTrashService-1.md

## Overview
本实施方案旨在解决物理文件移入回收站及从回收站还原过程中的扩展元数据（星级、颜色标记、标签、备注、关联网址及尺寸）丢失问题：
1. **移入回收站漫游**：在 `DiskTrashService::moveToDiskTrash` 将物理文件/文件夹移动至回收站专属 UUID 容器后，调用 `QuarkMetaJson::roamItemMetadata(p, dest, true)`，将原文件在源目录 `.QuarkMeta.json` 中的元数据记录深克隆迁移至回收站容器下的 `.QuarkMeta.json` 中。
2. **从回收站还原漫游**：在 `restoreFromDiskTrash` 与 `restoreToDirectory` 中，物理文件还原成功后，反向调用 `QuarkMetaJson::roamItemMetadata(actualTrashPath, targetPath, true)`，将回收站配置文件中的元数据深克隆还原并回写至目标物理目录的 `.QuarkMeta.json` 中，并在 `MetaMemoryCache` 中同步刷新。

---

## Modified Files List
1. `src/core/DiskTrashService.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/core/DiskTrashService.cpp`
补齐 `#include "../meta/QuarkMetaJson.h"`，并在移入回收站与还原逻辑成功点注入 `QuarkMetaJson::roamItemMetadata` 漫游函数：

```
<<<<<<< SEARCH
#include "DiskTrashService.h"
#include "../meta/DatabaseManager.h"
#include <QFileInfo>
=======
#include "DiskTrashService.h"
#include "../meta/DatabaseManager.h"
#include "../meta/QuarkMetaJson.h"
#include <QFileInfo>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (QFile::rename(p, dest)) {
            sqlite3* db = DatabaseManager::instance().getGlobalDb();
=======
        if (QFile::rename(p, dest)) {
            // 物理移动成功后，漫游元数据至回收站专属 .QuarkMeta.json
            QuarkMetaJson::roamItemMetadata(p, dest, true);

            sqlite3* db = DatabaseManager::instance().getGlobalDb();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (QFile::rename(actualTrashPath, targetPath)) {
        QDir(QFileInfo(actualTrashPath).absolutePath()).removeRecursively();
=======
    if (QFile::rename(actualTrashPath, targetPath)) {
        // 从回收站反向漫游元数据回真实目标物理目录
        QuarkMetaJson::roamItemMetadata(actualTrashPath, targetPath, true);

        QDir(QFileInfo(actualTrashPath).absolutePath()).removeRecursively();
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (moved) {
        QDir(QFileInfo(trashPath).absolutePath()).removeRecursively();
=======
    if (moved) {
        // 还原至指定目录时反向漫游元数据
        QuarkMetaJson::roamItemMetadata(trashPath, dest, true);

        QDir(QFileInfo(trashPath).absolutePath()).removeRecursively();
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. **编译确认**：
   运行 CMake 编译，确保无语法或关联头文件缺失错误：
   ```bash
   cmake --build build
   ```

2. **功能复核**：
   - 步骤 1：为某个文件打上 5 星评级、红色标记并添加备注“还原测试”；
   - 步骤 2：右键将该文件“移入回收站”；
   - 步骤 3：进入回收站面板，选择该文件点击“还原”；
   - 步骤 4：确认文件原位还原后，其 5 星评级、红色标记与“还原测试”备注 100% 完整保留。

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **元数据管道复用**：复用统一的 `QuarkMetaJson::roamItemMetadata` 漫游管道，无私有数据结构与副本；
- **防另起炉灶**：漫游逻辑完全融入现有的 `moveToDiskTrash` / `restoreFromDiskTrash` 处理链路。

---

## Header API Signature Verification

| 调用的类 / 成员函数 | `.h` 文件源头 | 物理精确签名 |
| :--- | :--- | :--- |
| `QuarkMetaJson::roamItemMetadata(...)` | `src/meta/QuarkMetaJson.h` | `static bool roamItemMetadata(const QString& oldFilePath, const QString& newFilePath, bool isMove);` |

---

## Header Inclusion Chain & Type Completeness Check

- `src/core/DiskTrashService.cpp`:
  - 显式引入 `#include "../meta/QuarkMetaJson.h"`；
  - 调用的所有类型定义闭环完整。
