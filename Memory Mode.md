# Dual‑mode Version 内存模式（Memory Mode / 托管库模式）代码文件与判断逻辑分析清单

本文档汇总并标记了 `Dual‑mode version` 源码路径中所有涉及**内存模式（Memory Mode / 托管库模式）**的代码文件、核心接口及具体判断逻辑分支。

---

## 一、 UI 模型与视图/逻辑控制层 (UI & Models Layer)

### 1. `Dual‑mode version/src/ui/models/LibraryAssetModel.h` & `LibraryAssetModel.cpp`
* **内存模式角色**：内存托管模式资产数据模型（与纯磁盘模式的 `DiskItemModel` 对应）。
* **判定与关联逻辑**：
  * 只处理内存数据库模式条目及分类节点（`isCategory` 分支及内存库条目）。
  * 包含针对内存库元数据更新、内存记录同步等专属逻辑（如 `updateRecordMetadata`、`migrateCache`、`clearCacheForFolder`）。

### 2. `Dual‑mode version/src/ui/MemoryBatchRenameService.h` & `MemoryBatchRenameService.cpp`
* **内存模式角色**：内存数据库模式条目的批量重命名引擎。
* **判定与关联逻辑**：
  * 在 `MemoryBatchRenameService::execute` 中负责针对 SQLite 内存库/托管库条目的重命名与数据库记录同步及物理撤销回滚机制。

### 3. `Dual‑mode version/src/ui/ContentPanel.h` & `ContentPanel.cpp`
* **内存模式角色**：主内容面板，包含控制显示模式和切换数据源的核心调度器。
* **判定与关联逻辑**：
  * **数据源类型判定 (`dataSourceType`)**：
    ```cpp
    ContentPanel::DataSourceType ContentPanel::dataSourceType() const {
        if (m_isUserCategory) return DataSourceType::UserCategory;
        if (m_isSystemCategory) return DataSourceType::SystemCategory;
        if (m_isPathList) return DataSourceType::PathList;
        return DataSourceType::DiskNav; // 纯磁盘导航模式
    }
    ```
  * **内存/托管模式判定**：`bool isMemory = (dataSourceType() != DataSourceType::DiskNav);` 或 `isMirrorSource()`。
  * **模型绑定**：持有 `LibraryAssetModel* m_libraryModel` 实例。
  * **内存数据库句柄与缓存刷新**：数据加载及批量创建时调用 `DatabaseManager::instance().getActiveMemoryDbs()` 及 `CategoryRepo::refreshMemoryCache()`。

### 4. `Dual‑mode version/src/ui/BatchCreateDialog.h` & `BatchCreateDialog.cpp`
* **内存模式角色**：批量创建文件夹/文件对话框。
* **判定与关联逻辑**：
  * 构造函数及内部成员变量 `m_isMemoryMode`。
  * **分支判断**：
    ```cpp
    if (m_isMemoryMode) {
        // 走内存/托管库模式下的库选择与元数据写入逻辑
    } else {
        // 走纯物理磁盘直接创建逻辑
    }
    ```

### 5. `Dual‑mode version/src/ui/BatchRenameDialog.h` & `BatchRenameDialog.cpp`
* **内存模式角色**：批量重命名对话框。
* **判定与关联逻辑**：
  * 提交重命名请求时引入 `MemoryBatchRenameService.h`，分支调用 `MemoryBatchRenameService::execute(...)`。

### 6. `Dual‑mode version/src/ui/MainWindow.h` & `MainWindow.cpp`
* **内存模式角色**：主窗口调度。
* **判定与关联逻辑**：
  * 包含 `MetadataManager::isInsideManagedLibrary(wPath)` 与 `isMirrorSource()` 的多处分支判断，用于决定切换内容视图类型及顶部导航栏显示逻辑。

---

## 二、 核心业务域与数据结构层 (Core Domain Layer)

### 1. `Dual‑mode version/src/core/ItemRecord.h` & `ItemRecord.cpp`
* **内存模式角色**：通用数据项记录结构体。
* **判定与关联逻辑**：
  * **工厂方法签名**：`static ItemRecord create(const QString& path, const RuntimeMeta* providedMeta = nullptr, bool isFromMemory = false);`
  * **判定分支**：
    ```cpp
    if (isFromMemory) {
        // 标记为内存库条目，填充来自内存/SQLite数据库中的属性
    }
    ```

### 2. `Dual‑mode version/src/core/CategoryDropProcessor.h` & `CategoryDropProcessor.cpp`
* **内存模式角色**：分类拖拽处理器。
* **判定与关联逻辑**：
  * 通过 `MetadataManager::isInsideManagedLibrary(wPath)` 判断拖入的路径节点是否属于托管库内部路径。

### 3. `Dual‑mode version/src/core/DiskTrashService.h` & `DiskTrashService.cpp`
* **内存模式角色**：回收站物理/逻辑服务。
* **判定与关联逻辑**：
  * 在搜索和清理回收站记录时调用 `DatabaseManager::instance().getActiveMemoryDbs()` 获取活动的内存库数据库连接句柄列表。

### 4. `Dual‑mode version/src/core/LibraryMaintenanceService.h` & `LibraryMaintenanceService.cpp`
* **内存模式角色**：托管库维护服务。
* **判定与关联逻辑**：
  * 定时或手动对托管内存数据库进行整理优化时，调用 `DatabaseManager::instance().getActiveMemoryDbs()` 获取全部数据库句柄。

---

## 三、 元数据管理与数据库抽象层 (Metadata & Data Access Layer)

### 1. `Dual‑mode version/src/meta/DatabaseManager.h` & `DatabaseManager.cpp`
* **内存模式角色**：SQLite 数据库连接与多分片数据库管理器。
* **判定与关联逻辑**：
  * **核心内存库接口**：`std::vector<sqlite3*> getActiveMemoryDbs();` 负责收集并返回所有的内存/托管 SQLite 数据库连接（包含 `global.db` 以及各分片的 `QuarkMeta_*.db`）。

### 2. `Dual‑mode version/src/meta/MetadataManager.h` & `MetadataManager.cpp`
* **内存模式角色**：全局元数据调度中心。
* **判定与关联逻辑**：
  * **托管库路径判定**：`static bool isInsideManagedLibrary(const std::wstring& path);`
  * **内存分类映射**：
    * `addCategoryToItemMemory(path, categoryId)`
    * `removeCategoryFromItemMemory(path, categoryId)`
    * `clearCategoriesFromItemMemory(path)`
  * **内存 Commit 条件分支**：多处 `if (isInsideManagedLibrary(nPath))` 分支决定是否向分片 SQLite 数据库写入元数据或提交异步同步任务。

### 3. `Dual‑mode version/src/meta/CategoryRepo.h` & `CategoryRepo.cpp`
* **内存模式角色**：分类仓储实现。
* **判定与关联逻辑**：
  * **内存缓存刷新**：`static void refreshMemoryCache();`
  * **数据同步**：在全量分类增删改查时广泛调用 `DatabaseManager::instance().getActiveMemoryDbs()`，以及 `MetadataManager::addCategoryToItemMemory` / `removeCategoryFromItemMemory` / `clearCategoriesFromItemMemory`。
  * **托管库根分类枚举**：`CategoryKind::SystemLibrary`（标记系统托管库根分类）。

### 4. `Dual‑mode version/src/meta/StatisticsService.h` & `StatisticsService.cpp`
* **内存模式角色**：统计服务。
* **判定与关联逻辑**：
  * 包含 `m_cachedSnapshot.libraryCounts` 统计逻辑，使用 `DatabaseManager::instance().getActiveMemoryDbs()` 遍历内存库做统计汇总。

### 5. `Dual‑mode version/src/meta/DiskTrashRepo.h` & `DiskTrashRepo.cpp`
* **内存模式角色**：磁盘回收站仓储。
* **判定与关联逻辑**：
  * 调用 `DatabaseManager::instance().getActiveMemoryDbs()` 跨多个活动内存数据库拉取与匹配物理回收站条目。

### 6. `Dual‑mode version/src/meta/TrashRepository.h` & `TrashRepository.cpp`
* **内存模式角色**：物理/托管回收站数据仓储。
* **判定与关联逻辑**：
  * 使用 `DatabaseManager::instance().getActiveMemoryDbs()` 检索和擦除内存模式下的垃圾条目。

---

## 四、 总结：核心判断特征总结 (Core Judgment Keypoints)

1. **底层数据库连接判断**：`DatabaseManager::instance().getActiveMemoryDbs()` —— 用于获取所有活动中的内存数据库连接。
2. **托管库路径范围判断**：`MetadataManager::isInsideManagedLibrary(path)` —— 用于判断某个绝对路径是否落在系统托管库范围内。
3. **UI视图与数据源类型判断**：`ContentPanel::dataSourceType() != ContentPanel::DataSourceType::DiskNav` —— 用于判断是否属于非纯磁盘模式（即分类模式、列表模式、内存托管模式）。
4. **模型层分支判定**：`ItemRecord::create(..., bool isFromMemory)` 与 `LibraryAssetModel` 的使用 —— 区别纯磁盘与内存模式下的 UI 展示模型。
