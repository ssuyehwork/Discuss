# RapidNotes 架构演进时间线与代码修改记录 (Timeline)

## 记录日期：2025-05-18

### 核心演进目标
依据最新架构设计理念，彻底废除 RapidNotes 中的文件级伴随侧车 JSON（`.QuarkMeta.json` 与 `.RapidNotes.json`）存储机制，全面解绑对物理目录中 JSON 文件的读写与缓存维护，所有数据操作与元数据管理彻底移交给本地数据库 `inspiration.db`。

---

### 修改与删除文件清单

#### 1. 物理侧车代码移除（删除）
- **`src/meta/QuarkMetaJson.h`**：[已删除] 废弃原 QuarkMeta 侧车 JSON 抽象定义文件。
- **`src/meta/QuarkMetaJson.cpp`**：[已删除] 废弃原 QuarkMeta 侧车 JSON 解析与序列化实现。

#### 2. 核心与生命周期服务（修改）
- **`src/core/PermanentDeleteService.cpp`**：
  - 移除了对 `QuarkMetaJson.h` 的引用。
  - 删除了文件永久删除流程中对 `RapidNotesJson::removeItemMetadata` 的调用。
- **`src/core/DiskTrashService.cpp`**：
  - 移除了对 `RapidNotesJson.h` 的引用。
  - 删除了回收站恢复与放入时对 `RapidNotesJson::roamItemMetadata` 的调用。

#### 3. 元数据与同步服务（修改）
- **`src/meta/LibrarySyncService.h`** / **`src/meta/LibrarySyncService.cpp`**：
  - 移除了对 `QuarkMetaJsonStore.h` 与 `RapidNotesJsonStore.h` 的依赖。
  - 彻底移除了 `handleJsonChangeEvent` 方法及其关联逻辑，避免在收到元数据变更时回写侧车 JSON。
- **`src/meta/MetadataManager.cpp`**：
  - 移除了对 `RapidNotesJsonStore.h` 的依赖。
  - 清除了所有 `updateItemMeta`、`addTagsBatch`、`setRatingBatch`、`setColorBatch`、`setNoteBatch`、`setURLBatch`、`setPinnedBatch`、`removeTagsBatch`、`migrateFolderCache`、`renameItem` 中向侧车 JSON 同步持久化的调用。
  - 重写了 `getFolderMeta` 与 `setFolderMeta`，解除对目录级侧车 JSON 的读取和落盘。
- **`src/meta/ItemMetaMapper.h`**：
  - 将对 `RapidNotesJson.h` 的依赖重构为对底层通用结构 `MetadataDefs.h` 的依赖。
- **`src/meta/MetaIndexService.cpp`**：
  - 移除了对 `RapidNotesJson.h` 和 `RapidNotesJsonStore.h` 的包含。
  - 清除了扫描物理文件夹时解析与合并侧车 JSON 的代码。
- **`src/meta/DiskNavigatorService.h`** / **`src/meta/DiskNavigatorService.cpp`**：
  - 移除了对 `RapidNotesJson.h` 的引用。
  - 文件夹与条目元数据获取流程全部重构为安全内存/数据库占位逻辑，杜绝解析 `.RapidNotes.json`。
- **`src/meta/MetaCacheDecorator.cpp`**：
  - 移除了对 `RapidNotesJsonStore.h` 的依赖。
  - 解除了 `folderCacheMap` 向侧车 JSON 的读写代理。

#### 4. 工具与辅助模块（修改）
- **`src/util/ShellHelper.cpp`**：
  - 移除了对 `RapidNotesJson.h` 与 `RapidNotesJsonStore.h` 的包含。
  - 移除了 `roamBatchMetadata`（批量漫游）、`flushAllDirtyBuffers`（脏缓冲写盘）以及 `migrateItemMetadata` 对侧车 JSON 的调用。
- **`src/util/DiskMediaExtractor.cpp`**：
  - 移除了对 `RapidNotesJsonStore.h` 的包含。
  - 移除了利用侧车 JSON 缓存标记解析失败缩略图状态的逻辑。
  - 移除了深度解析文件分辨率尺寸后向侧车 JSON 同步写盘的代码。

#### 5. UI 与视图模型层（修改）
- **`src/ui/LibraryPanel.cpp`**：
  - 移除了对 `RapidNotesJsonStore.h` 的无效包含。
- **`src/ui/models/DiskItemModel.h`** / **`src/ui/models/DiskItemModel.cpp`**：
  - 移除了对 `RapidNotesJson.h` 和 `RapidNotesJsonStore.h` 的头文件引用。
  - 移除了异步预加载图片尺寸后向侧车 JSON 同步刷写的逻辑。
