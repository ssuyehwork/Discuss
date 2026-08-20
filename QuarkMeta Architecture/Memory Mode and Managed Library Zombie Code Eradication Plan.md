# 纯磁盘目录模式·内存模式与托管库僵尸代码根除方案 —— Memory Mode and Managed Library Zombie Code Eradication Plan

> **执行总则**：
> 1. 本架构唯二物理持久化载体为：**各文件夹下的 `.QuarkMeta.json`** 与 **根目录下的 `global.db`**。
> 2. 彻底抹除所有 `.arc` 胶囊、Base36 ID、多分盘 `QuarkMeta_*.db`、`categories` / `category_items` 关系表及相关导入打包逻辑。
> 3. 本方案分为 **6 个独立阶段（Stage 1 ~ 6）**，每个阶段均包含明确的代码修改指引与自检断言，支持分段执行与跨会话无缝续接。

---

## 进度追踪清单（执行者自检标记）

- [ ] **Stage 1**：构建系统治理与已废弃物理文件剥离（`CMakeLists.txt`）
- [ ] **Stage 2**：数据库引擎降维（`DatabaseManager.h`, `DatabaseManager.cpp`）
- [ ] **Stage 3**：核心元数据管理器脱耦（`MetadataManager.h`, `MetadataManager.cpp`）
- [ ] **Stage 4**：内容面板与数据模型归一化（`ContentPanel.h/cpp`, `DiskItemModel.cpp`）
- [ ] **Stage 5**：系统辅助服务与查重引擎净化（`ShellHelper.cpp`, `DuplicateDetectorService.cpp`）
- [ ] **Stage 6**：主窗口与对话框协议清理（`MainWindow.cpp`, `TagManagerDialog.cpp`）

---

## Stage 1：构建系统治理（CMakeLists.txt）

### 目标
从编译配置中彻底剔除所有已删除或僵尸托管模块的源文件引用。

### 修改方案（`CMakeLists.txt`）
在 `CMakeLists.txt` 的 `set(SOURCES ...)` 列表中，**删除以下条目**：
```cmake
# 删除以下僵尸文件路径：
src/util/AssetImporter.cpp
src/util/AssetImporter.h
src/util/ImportHelper.cpp
src/util/ImportHelper.h
src/meta/CapsuleMediaExtractor.cpp
src/meta/CapsuleMediaExtractor.h
src/ui/CategoryLockDialog.cpp
src/ui/CategoryLockDialog.h
src/ui/CategoryLockWidget.cpp
src/ui/CategoryLockWidget.h
```

**【阶段自检 Checkpoint 1】**：
执行 CMake 重新配置（`cmake -B build`），确保无“文件未找到”警告。

---

## Stage 2：数据库引擎降维（DatabaseManager）

### 目标
废除多分盘 `QuarkMeta_*.db` 的生命周期管理与分盘路由，降维为仅服务于 `global.db`。

### 修改方案

#### 1. `src/meta/DatabaseManager.h`
- **移除成员与接口**：
  - 移除 `getDriveDb(...)`、`getDbForPath(...)`、`getActiveMemoryDbs()`、`getDiskDb(...)`、`getDriveMutex(...)`。
  - 移除 `m_driveDbs`、`m_driveDbMutexMap`。
  - 移除 `resolveVolumeDrift(...)` 声明。
- **保留并巩固**：
  - 保留 `getGlobalDb()` 作为全系统唯一的 SQLite 句柄获取函数。
  - 保留 `SqlTransaction`、`WriteGuard`、`flushAll()`。

#### 2. `src/meta/DatabaseManager.cpp`
- **重构 `init()`**：仅加载 `global.db` 并初始化全局表（`system_stats`, `tag_groups`, `tag_group_items`, `disk_trash`, `drive_meta`）。
- **删除废弃逻辑**：
  - 彻底删除 `schema` 中的 `categories` 和 `category_items` 建表与字段迁移 SQL。
  - 彻底删除 `resolveVolumeDrift` 全函数。
  - 彻底删除 `getDriveDb` 与分盘 `m_driveDbs` 遍历。
- **重构 `flushAll()`**：仅对 `m_globalDb` 执行 `saveDb(m_globalDb, forceFull)`。

**【阶段自检 Checkpoint 2】**：
全局搜索 `DatabaseManager::instance().getDriveDb`，确保项目中调用数为 0，所有数据库访问统一收敛为 `DatabaseManager::instance().getGlobalDb()`。

---

## Stage 3：元数据管理器脱耦（MetadataManager）

### 目标
消除 `MetadataManager` 对 `.arc` 胶囊、Base36 算法、托管库路径判定及分盘数据库大事务的依赖。

### 修改方案

#### 1. `src/meta/MetadataManager.h`
- **删除以下函数声明**：
  - `registerAsset(...)`
  - `migrateCapsuleToLibrary(...)`
  - `isInsideManagedLibrary(...)`
  - `getManagedLibraryPath(...)`
  - `addCategoryToItemMemory(...)` / `removeCategoryFromItemMemory(...)` / `clearCategoriesFromItemMemory(...)`
- **数据结构精简（`RuntimeMeta`）**：
  - 移除 `std::vector<int> categoryIds;`
  - 移除 `isManaged` 字段。

#### 2. `src/meta/MetadataManager.cpp`
- **删除辅助逻辑**：
  - 删除 `isManagedAsset` 与 `extractBase36Id` 静态解析器。
  - 删除 `initFromDatabase()` 中对 `category_items` 表的读取与 `QuarkMeta_*.db` 分盘扫描代码。
- **删除全函数实现**：
  - 删除 `registerAsset`、`migrateCapsuleToLibrary`、`isInsideManagedLibrary`、`getManagedLibraryPath`。
- **重构 `persistAsync` / `persistBatchAsync`**：
  - 移除对分盘 `getDriveDb` 的获取，所有元数据落盘一律直连 `DatabaseManager::instance().getGlobalDb()`（或在纯磁盘模式下直接交由 `QuarkMetaJson` 维护，取消无意义的单文件数据库持久化）。

**【阶段自检 Checkpoint 3】**：
全局搜索 `isInsideManagedLibrary` 与 `registerAsset`，确保无任何代码残留。

---

## Stage 4：内容面板与数据模型归一化（ContentPanel & DiskItemModel）

### 目标
清除 `ContentPanel` 中的镜像源/受控库分流逻辑，统一底层文件操作。

### 修改方案

#### 1. `src/ui/ContentPanel.h`
- 移除 `isMirrorSource()` 与 `isManagedContext()`。
- 移除 `CategoryLockWidget* m_lockWidget` 成员。
- 移除 `loadCategory(int)` 与 `loadCategories(...)` 空占位声明。

#### 2. `src/ui/ContentPanel.cpp`
- **清理构造函数与 UI**：
  - 从 `m_viewStack` 中移除 `m_lockWidget`。
- **重构 `performPaste()` 与 `onPathsDropped()`**：
  - 彻底删除 `else` 分支中的 `AssetImporter::importAssets` 调用。
  - 无论是粘贴还是拖拽投放，100% 走 `DiskIoService::instance().executeAsync` 物理文件操作。
- **重构 `createNewItem(const QString& type)`**：
  - 彻底删除分流 B（`m_currentCategoryId`、`requestCreateSubCategory`、`.arc` 胶囊生成）。
  - 仅保留在 `m_currentPath` 下基于 `QDir::mkdir` / `QFile` 的常规文件创建。
- **重构右键菜单（`onCustomContextMenuRequested`）**：
  - 删除 `ActionCategorize`、`ActionAddToCategory`、`ActionCancelImport` 相关菜单项与 `case` 执行块。

#### 3. `src/ui/models/DiskItemModel.cpp`
- 移除 `loadThumbnailsForRows` 中硬编码的 `pathsToLoad.size() >= 2` 限流补丁，恢复标准视口批量加载。
- 确保 `setData` 时直接调用 `QuarkMetaJson` 进行当前目录 JSON 的原子读写。

**【阶段自检 Checkpoint 4】**：
全局搜索 `AssetImporter` 与 `CategoryLockWidget`，确保编译单元完全脱钩。

---

## Stage 5：系统辅助服务净化（ShellHelper & DuplicateDetectorService）

### 目标
清除辅助工具中残留的旧版 `.arc` 胶囊与回收站逻辑。

### 修改方案

#### 1. `src/util/ShellHelper.cpp`
- **重构 `moveToTrash`**：
  - 移除原先向 `.QuarkMeta/trash` 移动并生成时间戳的旧逻辑，统一交由 `DiskTrashService::moveToDiskTrash` 调度。

#### 2. `src/meta/DuplicateDetectorService.cpp`
- **清理缩略图提取器**：
  - 彻底删除 `#include "CapsuleMediaExtractor.h"`。
  - 将所有 `CapsuleMediaExtractor::getCapsuleThumbnail` 替换为 `DiskMediaExtractor` 或系统级缩略图提取接口。
- **优化查重哈希算法**：
  - 引入文件头尾各 64KB 的 FastHash 进行第一级预筛，杜绝大文件全量计算 SHA-256 导致的 I/O 阻塞。

**【阶段自检 Checkpoint 5】**：
全局搜索 `CapsuleMediaExtractor`，确保没有任何代码调用该类。

---

## Stage 6：主窗口与对话框协议清理（MainWindow & TagManagerDialog）

### 目标
清理主窗口导航协议分流与标签对话框中的历史分支。

### 修改方案

#### 1. `src/ui/MainWindow.cpp`
- **重构 `unifiedNavigateTo`**：
  - 彻底删除 `if (url.startsWith(kProtocolCategory))` 分支及 `kProtocolCategory` 常量。
  - 所有导航请求统一作为物理路径（`computer://` 或本地文件路径）处理。
- **清理无用头文件**：
  - 移除 `AssetImporter.h`、`ImportHelper.h`、`CapsuleMediaExtractor.h` 等引用。

#### 2. `src/ui/TagManagerDialog.cpp`
- **重构 `createTag` 与 `refreshTags`**：
  - 彻底删除 `if (m_isMirrorSource)` 分支。
  - 新建标签：**第一步**写入 `global.db`（`TagRepository::addTagToGroup` / 标签主表），**第二步**若有选中物理文件则写入该文件所在目录的 `.QuarkMeta.json`。
  - 读取标签：一律从 `global.db` 读取全局标签词典。

**【阶段自检 Checkpoint 6】**：
全工程重新编译（Rebuild All），确保 0 编译错误、0 链接错误，并验证标签创建与文件操作流畅无假死。