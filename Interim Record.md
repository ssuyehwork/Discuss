# QuarkMeta 内存模式遗留孤儿/僵尸代码全盘地毯式排查中间记录 (Interim Record)

## 0. 排查概述与宗旨
为了彻底清除从内存托管库/双模时代（Memory Mode）遗留至今的孤儿代码、过时分支与假死控件，本文档从入口 `main.cpp` 开始，顺着系统的启动流、调度中枢、各个 UI 面板及底层数据/网络/文件服务，逐文件梳理记录所有残留的内存模式僵尸逻辑，作为后续一次性彻底清理的物理依据。

---

## 1. 入口与核心控制层 (Entry & Core Layer)

### 1.1 `src/main.cpp` & `src/core/CoreController.cpp` / `.h`
- **【数据预加载残留】`CoreController::startSystem()` -> `MetadataManager::instance().initFromDatabase()`**
  - **问题描述**：系统启动时调用 `initFromDatabase()` 尝试将 `global.db` 中的 `metadata` 数据表全量读取加载到内存 Shard (`m_shards`) 中。在 QuarkMeta 纯磁盘直连模式下，非根目录文件的元数据由 `.QuarkMeta.json` 直接接管，`global.db` 不再维护非根目录的 `metadata` 记录。预加载此表属于过时内存索引行为。
  - **影响文件**：`src/main.cpp`、`src/core/CoreController.cpp`、`src/meta/MetadataManager.cpp`
- **【分类 ID 遗留参数】`CoreController::performSearch(...)` 中的 `categoryId` 与 `scopeSource == "category"`**
  - **问题描述**：`performSearch` 仍然保留 `int categoryId` 和 `scopeSource == "category"` 参数与处理分支。然而 `CategoryPanel`（侧边栏分类树）早已被物理剔除，此类参数属于过时的分类树遗毒。
  - **影响文件**：`src/core/CoreController.h`、`src/core/CoreController.cpp`

### 1.2 `src/core/OperationSnapshotEngine.cpp`
- **【WinAPI Base36 机制残留】`MetadataManager::instance().getFolderIdSync(...)`**
  - **问题描述**：在快照引擎中依然调用 `getFolderIdSync` 尝试通过 Base36 或物理 FRN 生成 `folder_id`。纯磁盘直连模式下操作以物理路径为准，不需要依赖 Base36 / WinAPI `folder_id`。
  - **影响文件**：`src/core/OperationSnapshotEngine.cpp`、`src/meta/MetadataManager.cpp` / `.h`

---

## 2. 数据库与元数据管理层 (Database & Meta Layer)

### 2.1 `src/meta/DatabaseManager.cpp` / `.h`
- **【内存数据库架构与变量命名遗留】`conn.memDb` / `:memory:` 关联逻辑**
  - **问题描述**：`DatabaseManager` 中仍保留 `memDb` 结构体成员以及与磁盘数据库同步到 `:memory:` 内存 SQLite 数据库的相关代码和命名。在纯磁盘模式下，`global.db` 直接使用磁盘 SQLite 连接，无需内存 DB 双重备份。
  - **影响文件**：`src/meta/DatabaseManager.h`、`src/meta/DatabaseManager.cpp`
- **【`metadata` 与 `metadata_fts` 数据表残留】**
  - **问题描述**：`global.db` 的建表语句中包含全盘索引时代的 `metadata` 表和 FTS5 全文索引 `metadata_fts` 表（含 `category_id`、`folder_id`、`auto_color`、`palettes`、`ingestion_status` 等字段）。在纯磁盘模式下，离散元数据落落于 `.QuarkMeta.json`，`global.db` 仅需保留回收站（`disk_trash`）、盘符元数据及标签配置。
  - **影响文件**：`src/meta/DatabaseManager.cpp`

### 2.2 `src/meta/MetadataManager.cpp` / `.h`
- **【FTS5 全量检索】`MetadataManager::searchInCache(...)`**
  - **问题描述**：`searchInCache` 使用 `SELECT path FROM metadata WHERE rowid IN (SELECT rowid FROM metadata_fts WHERE metadata_fts MATCH ?)` 进行全文搜索。因 `metadata` 表不再记录全盘非根目录项，此查询无法搜到未写入 `global.db` 的普通磁盘文件。
  - **影响文件**：`src/meta/MetadataManager.cpp`
- **【空桩与废弃接口】`registerQuarkMetaFrn(...)`**
  - **问题描述**：`MetadataManager::registerQuarkMetaFrn` 为死存空函数。
  - **影响文件**：`src/meta/MetadataManager.h`、`src/meta/MetadataManager.cpp`

---

## 3. UI 界面与筛选器层 (UI Panel Layer)

### 3.1 `src/ui/FilterPanel.cpp` / `.h`
- **【内存色彩感知三大假死控件】**
  - **`InlineHueSlider`（色相连续条）**：选定 360° 连续 HSV 色彩，在磁盘直连模式下极难精准匹配到具体文件。
  - **`m_accuracySlider`（颜色准确度/容差滑块）**：调节 CIELAB Delta E 计算阈值，对手动离散色标（红/黄/蓝等）完全无意义。
  - **`m_areaSlider`（颜色占比滑块）**：限制图像像素调色板（`palettes`）的最小面积比例，普通磁盘文件缺乏此数据，拖动无效。
  - **影响文件**：`src/ui/FilterPanel.h`、`src/ui/FilterPanel.cpp`

### 3.2 `src/ui/ContentPanel.cpp` / `.h`
- **【过时分类死存桩函数】**
  - `loadCategory(int categoryId)`
  - `loadCategories(const QList<int>& categoryIds)`
  - `categoryClicked(int categoryId)`
  - **影响文件**：`src/ui/ContentPanel.h`、`src/ui/ContentPanel.cpp`
- **【图像调色板 Delta-E 复杂容差匹配逻辑】**
  - `calculateAutoColorMatchedArea` 与 `isColorMatched` 函数中使用 `calculateDeltaE` 动态比较调色板百分比，与离散色标精准过滤的需求脱节。
  - **影响文件**：`src/ui/ContentPanel.cpp`、`src/ui/ColorAlgorithmEngine.cpp` / `.h`、`src/ui/MediaColorExtractor.cpp` / `.h`、`src/ui/UiHelper.h`

### 3.3 `src/ui/MainWindow.cpp`
- **【垃圾桶分类字符串遗留】`m_contentPanel->loadCategory("trash");`**
  - **问题描述**：`MainWindow` 中仍使用字符串分类方式调用 `loadCategory("trash")`，需要改造为直接使用纯磁盘回收站导航逻辑。
  - **影响文件**：`src/ui/MainWindow.cpp`

---

## 4. 后续统一物理清退计划概要 (Purge Roadmap)
在后续优化阶段，将依照此记录进行集中物理清理：
1. **彻底物理剔除** `FilterPanel` 中的 `InlineHueSlider`、`m_accuracySlider` 和 `m_areaSlider` 控件。
2. **清理** `ContentPanel` 中的 `loadCategory`/`loadCategories` 空桩函数及动态 Delta-E 容差累加匹配逻辑。
3. **彻底物理移除** `MetadataManager` 与 `CoreController` 中过时的 `categoryId` 参数和全盘 `metadata` / `metadata_fts` 表 FTS 搜索分支，将搜索统一收拢到纯磁盘文件名称与 `.QuarkMeta.json` 元数据实时匹配逻辑上。
4. **清理** `DatabaseManager` 中过时的 `memDb` 变量命名与 memory 备份残留逻辑。
