# QuarkMeta 深度逻辑推理与“顺藤摸瓜”全调用链排查中间记录 (Interim Record-1.md)

## 0. 说明与排查宗旨
本记录为全新排查文档（不修改已有 `Interim Record.md`）。本文档摒弃机械的关键字搜索，完全采用**软件工程代码逻辑推理**方式，从入口文件 `main.cpp` 开始，沿着系统的真实函数调用关系、类实例化拓扑、数据流向（Data Flow）和信号槽（Signals & Slots）流转链条“顺藤摸瓜”，深挖深层逻辑断裂、死代码分支、空转异步任务与无消费数据库写操作。

---

## 1. 深度逻辑推理排查发现 (Call Chain & Data Flow Audit)

### 1.1 顺藤摸瓜链 1：`main.cpp` -> `CoreController::startSystem()` -> `MetadataManager::initFromDatabase()` (Ghost Startup Query)
- **调用流向与逻辑推导**：
  1. `main.cpp` 启动时调用 `CoreController::instance().startSystem()`；
  2. `startSystem()` 开启后台线程提示“正在载入元数据缓存...”，并调用 `MetadataManager::instance().initFromDatabase()`；
  3. `initFromDatabase()` 执行 SQL `SELECT * FROM metadata`，试图将 `global.db` 的 `metadata` 数据表全量调入内存 Shard（`m_shards`）；
  4. **逻辑断裂点**：在 QuarkMeta 纯磁盘直连模式下，非根目录文件的元数据均离散保存在各自目录的 `.QuarkMeta.json` 中，`global.db` 的 `metadata` 表已被废弃且不再写入非根目录数据。因此，`initFromDatabase()` 每次启动都在进行一次**查无此数据的空转 SQL 扫描与无意义的 Shard 锁分配**。

### 1.2 顺藤摸瓜链 2：`MediaExtractorPipeline::dispatchWorkerLoop()` -> `MetadataManager::updateExtractedMediaFeaturesBatch()` -> Ghost SQL Update
- **调用流向与逻辑推导**：
  1. 当用户浏览带有图片的文件夹时，`MediaExtractorPipeline` 异步线程提取图片的 `width`、`height`、`autoColor` 和 `palettes`；
  2. 提取完成后调用 `MetadataManager::instance().updateExtractedMediaFeaturesBatch(results)`；
  3. `updateExtractedMediaFeaturesBatch` 将任务投递给 `DatabaseManager` 异步队列，执行 `UPDATE metadata SET width = ?, height = ?, auto_color = ?, palettes = ? WHERE path = ?`；
  4. **逻辑断裂点**：因非根目录文件根本不保存在 `global.db` 的 `metadata` 表中，该 `UPDATE` 语句在数据库中**永远命中 0 行（幽灵写入）**！而提取出来的尺寸与色彩元数据却没有持久化到真正的 SSOT 来源 `.QuarkMeta.json` 中，导致提图线程消耗了大量 CPU 和 GPU 资源后，提取结果无法落盘持久化。

### 1.3 顺藤摸瓜链 3：`DatabaseManager::loadDb()` -> `:memory:` 镜像库与 Backup 循环 (Legacy In-Memory DB Mirror)
- **调用流向与逻辑推导**：
  1. `DatabaseManager::loadDb()` 尝试打开磁盘 `global.db` 后，紧接着在内存中创建一个 `:memory:` 数据库（`conn.memDb`）；
  2. 使用 SQLite Backup API 将磁盘 `global.db` 数据全量复制进 `conn.memDb`；
  3. 全局 `getGlobalDb()` 返回的全部是内存连接 `conn.memDb`；
  4. 每次写入数据后触发 `WriteGuard` 与 `m_isDirty` 标记，并通过定时器和 `flushAll()` 使用 `sqlite3_backup_init` 将 `:memory:` 数据反向写回磁盘文件；
  5. **逻辑断裂点**：内存镜像数据库是过去“全盘几万张图片频繁 SQL 读写”时代为了避开磁盘 I/O 瓶颈而设计的过度架构。在 QuarkMeta 纯磁盘模式下，`global.db` 仅保存轻量级回收站映射与盘符元数据，根本不需要维持一个在 RAM 中的 `:memory:` 镜像库和复杂的 Backup 线程轮询，增加了锁争抢和数据丢失风险。

### 1.4 顺藤摸瓜链 4：`MainWindow` -> `NavPanel::requestOpenTrash` -> `ContentPanel::loadCategory("trash")` (Legacy Category Navigation)
- **调用流向与逻辑推导**：
  1. 用户在导航栏点击回收站，触发 `m_navPanel` 的 `requestOpenTrash` 信号；
  2. `MainWindow` 响应函数中调用 `m_contentPanel->loadCategory("trash")`；
  3. `loadCategory` 内部设置 `m_currentCategoryType = "trash"` 并调用 `loadCategory(int categoryId)` 死桩函数；
  4. **逻辑断裂点**：`loadCategory(int)` 和 `loadCategories(QList<int>)` 是已废弃的分类树（CategoryPanel）遗留方法。回收站导航应当统一走 `trash://` 协议路径调度，不应继续混用旧时代的 `categoryType` 字符串死分支。

### 1.5 顺藤摸瓜链 5：`ItemRecord` & `DiskItemModel` (Memory Mode Legacy Fields & Roles)
- **调用流向与逻辑推导**：
  1. `ItemRecord` 结构体中保留了：
     - `double registrationProgress = -1.0;`（内存托管库导入/解析进度）；
     - `bool isManaged = false;`（内存托管库受控状态标记）；
     - `QString frn;`（WinAPI 内存索引文件参考号）；
  2. `DiskItemModel::data()` 中继续保留响应 `RegistrationProgressRole` 和 `ManagedRole`（硬编码返回 `false`）的处理分支；
  3. **逻辑断裂点**：纯磁盘模式下所有文件均非内存托管，`registrationProgress` 与 `isManaged` 字段在数据流向中只被赋值或默认化，没有任何 UI 或业务逻辑消费这些数据，属于纯粹的内存占用垃圾。

### 1.6 顺藤摸瓜链 6：`FilterPanel` -> `ContentPanel::filterAcceptsRow` -> CIELAB Delta-E (Decoupled Color Filtering)
- **调用流向与逻辑推导**：
  1. `FilterPanel` 提供 `InlineHueSlider`（色相条）、`m_accuracySlider`（准确度/容差）、`m_areaSlider`（占比）；
  2. 当用户拖动滑块时，`ContentPanel` 触发 `filterAcceptsRow`；
  3. 在 `filterAcceptsRow` 中，调用 `calculateAutoColorMatchedArea` 与 `isColorMatched`，逐行计算 `calculateDeltaE`；
  4. **逻辑断裂点**：普通磁盘文件夹中的文件只有用户手动打上的离散色标（红/黄/蓝等）。由于缺乏全盘调色板索引，连续 Delta-E 算法在此处不仅拖慢了表格绘制效率，而且导致拖动滑块后完全无法筛选出结果，逻辑完全脱节。

---

## 2. 深度逻辑重构清理建议 (Action Plan)
1. **修正提图落盘路径**：重构 `MediaExtractorPipeline`，将提取出的 `width`、`height`、`autoColor`、`palettes` 直接通过 `QuarkMetaJson` 保存至对应目录下的 `.QuarkMeta.json` 中，物理注销对 `global.db` `metadata` 表的空转 SQL `UPDATE`。
2. **简化数据库架构**：清理 `DatabaseManager` 中 `:memory:` 镜像库 (`memDb`) 和 SQLite Backup 复杂逻辑，让 `global.db` 直接使用轻量直连 SQLite 连接。
3. **消除空转预加载**：取消 `CoreController::startSystem()` 对 `MetadataManager::initFromDatabase()` 的调用，启动时只预热盘符元数据与标签配置。
4. **清理过时字段与 Roles**：从 `ItemRecord` 中清退 `registrationProgress`、`isManaged` 和 `frn` 字段，清理 `DiskItemModel` 中对应的冗余 Roles。
5. **收拢回收站导航**：将 `MainWindow` 和 `NavPanel` 中对回收站的响应彻底重构为纯 `trash://` 协议导航，清理 `loadCategory("trash")` 及死存桩函数。
