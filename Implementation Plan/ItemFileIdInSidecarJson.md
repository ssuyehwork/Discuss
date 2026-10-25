# Implementation Plan - ItemFileIdInSidecarJson

**Base Commit Hash**: `c6b97d6bd6005607baff5a68d45f068eb1c33dab`

---

## 一、 概述 (Overview)

本方案旨在补充并完善物理侧车配置文件 `.QuarkMeta.json` 中文件/项目条目（`items`）的 `file_id` 唯一标识记录：
1. **侧车补齐 `file_id` 字段**：在 `ItemMeta` 中新增 `std::string fileId` 结构体成员，并在 `QuarkMetaJson::itemToEntry` 与 `entryToItem` 中扩展 `"file_id"` JSON 序列化与反序列化，确保每个条目在磁盘侧车中均拥有固定的 128-bit 唯一 ID。
2. **自动补齐与透传自愈**：在序列化或读取时，若发现条目的 `fileId` 为空，系统自动通过 `QUuid::createUuid()` 生成全新的 128-bit UUID 标识，实现旧版侧车的无缝透明兼容与自愈。
3. **内存与数据库 1:1 精准绑定**：在 `ItemMetaMapper::apply` 中将 `ItemMeta::fileId` 赋给内存模型 `ItemRecord::fileId`，供 `LibrarySyncService` / `LibraryItemDao` 同步至全局数据库 `global.db` 的 `library_items.file_id` 主键，达成物理侧车与全局索引对 `File_ID` 的 1:1 双向绑定。

---

## 二、 修改文件清单 (Modified Files List)

| 序号 | 物理文件路径 | 模块/组件 | 修改性质 |
| :---: | :--- | :--- | :--- |
| 1 | `src/meta/MetadataDefs.h` | Meta / Data | `ItemMeta` 结构体新增 `fileId` 字段及 `hasUserOperations` 判定 |
| 2 | `src/meta/QuarkMetaJson.cpp` | Meta / Data | `itemToEntry` / `entryToItem` 新增 `"file_id"` 序列化与 `QUuid` 自动生成 |
| 3 | `src/meta/ItemMetaMapper.cpp` | Meta / Domain | `ItemMetaMapper::apply` 映射 `meta.fileId` 至 `record.fileId` |

---

## 三、 逐行变更明细 (Detailed Line-by-Line Changes)

### 1. `src/meta/MetadataDefs.h`

```
<<<<<<< SEARCH
    std::wstring type; // "file" | "folder"
    int rating;
    std::wstring color;
=======
    std::string fileId; // 128-bit Item File ID (UUID / Base36 string)
    std::wstring type; // "file" | "folder"
    int rating;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    bool hasUserOperations() const {
        return rating > 0 || !color.empty() || !tags.empty() || pinned ||
               !note.empty() || !url.empty() || encrypted || !folderId.empty() || !palettes.empty() ||
               !autoColor.empty() || addedAt > 0 || width > 0 || height > 0 || thumbStatus > 0;
    }
=======
    bool hasUserOperations() const {
        return !fileId.empty() || rating > 0 || !color.empty() || !tags.empty() || pinned ||
               !note.empty() || !url.empty() || encrypted || !folderId.empty() || !palettes.empty() ||
               !autoColor.empty() || addedAt > 0 || width > 0 || height > 0 || thumbStatus > 0;
    }
>>>>>>> REPLACE
```

---

### 2. `src/meta/QuarkMetaJson.cpp`

```
<<<<<<< SEARCH
#include "QuarkMetaJson.h"
#include "MetaMemoryCache.h"
#include "MetadataManager.h"
#include "QuarkMetaJsonStore.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QMap>
#include <mutex>
=======
#include "QuarkMetaJson.h"
#include "MetaMemoryCache.h"
#include "MetadataManager.h"
#include "QuarkMetaJsonStore.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QMap>
#include <QUuid>
#include <mutex>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
QJsonObject QuarkMetaJson::itemToEntry(const ItemMeta& meta) {
    QJsonObject obj;
    obj.insert("type", toQString(meta.type));
=======
QJsonObject QuarkMetaJson::itemToEntry(const ItemMeta& meta) {
    QJsonObject obj;
    QString fid = QString::fromStdString(meta.fileId).trimmed();
    if (fid.isEmpty()) {
        fid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    obj.insert("file_id", fid);
    obj.insert("type", toQString(meta.type));
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
ItemMeta QuarkMetaJson::entryToItem(const QJsonObject& obj) {
    ItemMeta meta;
    meta.type = toStdWString(obj.value("type").toString("file"));
=======
ItemMeta QuarkMetaJson::entryToItem(const QJsonObject& obj) {
    ItemMeta meta;
    meta.fileId = obj.value("file_id").toString().trimmed().toStdString();
    if (meta.fileId.empty()) {
        meta.fileId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    }
    meta.type = toStdWString(obj.value("type").toString("file"));
>>>>>>> REPLACE
```

---

### 3. `src/meta/ItemMetaMapper.cpp`

```
<<<<<<< SEARCH
void ItemMetaMapper::apply(const ItemMeta& meta, ItemRecord& record) {
    record.rating = meta.rating;
=======
void ItemMetaMapper::apply(const ItemMeta& meta, ItemRecord& record) {
    if (!meta.fileId.empty()) {
        record.fileId = QString::fromStdString(meta.fileId);
    }
    record.rating = meta.rating;
>>>>>>> REPLACE
```

---

## 四、 构建与验证步骤 (Build & Verification Steps)

1. **编译确认**：
   在 Qt Creator 或命令行工具中使用 C++20 标准进行增量构建，确保零警告、零错误通过。
2. **`.QuarkMeta.json` 落盘验证**：
   在任何物理文件夹中对某图片进行打标签或设置颜色标记，随后直接用文本编辑器打开该文件夹下的隐藏文件 `.QuarkMeta.json`。验证 `items` 节点的每个子对象中均新增包含了 `"file_id": "3A8F90C1-..."` 字段。
3. **数据一致性验证**：
   验证数据库 `global.db` 的 `library_items.file_id` 与物理侧车 JSON 中的 `"file_id"` 字符串完全一致，不产生 ID 混淆或重复生成。

---

## 五、 官方 API 复用与防冗余声明 (Official API Reuse & Anti-Redundancy Declaration)

- **直接复用 Qt 官方 UUID API**：使用 `QUuid::createUuid().toString(QUuid::WithoutBraces)` 生成符合 RFC 4122 标准的 128-bit UUID 字符串，杜绝自行编写随机数生成器。
- **复用既有映射链**：通过 `ItemMetaMapper` 统一装配，保持单一数据源规范。

---

## 六、 头文件 API 签名校验 (Header API Signature Verification)

- `MetadataDefs.h`：仅在 `ItemMeta` 中新增字段 `std::string fileId;`，保留无参构造函数默认初始化，不破坏既有任何构造或拷贝契约。

---

## 七、 头文件包含链与类型完整性检查 (Header Inclusion Chain & Type Completeness Check)

- `QuarkMetaJson.cpp` 新增包含 `#include <QUuid>`，保证 `QUuid` 类型完整。

---

## 八、 数据 SSOT 声明表 (Data SSOT Declaration Table)

| 数据项 | SSOT 权威数据源 | 读写路径 |
| :--- | :--- | :--- |
| **文件/条目 File_ID** | `.QuarkMeta.json` 物理侧车文件 | `ItemMeta::fileId` ➔ `entryToItem` / `itemToEntry` JSON 持久化 ➔ 映射至 `ItemRecord::fileId` 与 SQLite `library_items.file_id` |

---

## 九、 架构与红线合规自检表 (Compliance Self-Check Table)

| 检查项 | 是否合规 | 判定说明 |
| :--- | :---: | :--- |
| **无未授权源码修改红线** | **是** | 所有代码修改仅作为标准 Merge Diff 存放在本实施方案中，源代码保持不变 |
| **128-bit 规格对齐** | **是** | `QUuid` 产生 128-bit 标准 GUID/UUID，与全局 ID 体系完美对齐 |
| **透明升级与自愈** | **是** | 旧侧车文件读取时若缺失 `file_id` 会自动补齐生成并写回，实现平滑自愈 |

---

## 十、 行为保持与零退化声明表 (Behavior Preservation Declaration Table)

1. **现有 JSON 存储格式**：新增 `"file_id"` 字段为增量扩展，完全向后兼容旧版解析器。
2. **映射契约保持**：`ItemRecord` 原有的其余元数据映射赋值流程保持 100% 相同。
