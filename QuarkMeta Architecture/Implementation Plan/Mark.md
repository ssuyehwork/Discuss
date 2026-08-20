# QuarkMeta 代码库遗留僵尸架构与全维度缺陷标记清单 (Mark.md)

本文档由排查系统自动与人工核查生成，详细记录了当前 QuarkMeta 代码库中所有残留的**内存模式历史遗毒**、**分区数据库 (QuarkMeta_*.db) 僵尸代码**以及**回收站机制中的潜在缺陷与重构标记**。后续将基于本文档逐项进行清理与修正。

---

## 一、 僵尸分库 (`QuarkMeta_<VSN>_<Drive>.db`) 遗留代码标记清单

### 1. `src/meta/DatabaseManager.h` & `DatabaseManager.cpp`
* **【标记 1.1】`getDbForPath(const std::wstring& path)` 函数逻辑过时**：
  - **位置**：`DatabaseManager.cpp:870-891`
  - **缺陷说明**：仍尝试调用 `VolumePathResolver::getVolumeSerialNumber` 并触发 `getDriveDb()` 为不同盘符分配单独的数据库句柄。
  - **重构目标**：收归逻辑，**统一直接返回 `getGlobalDb()`**（即全系统唯一数据库 `global.db`）。

* **【标记 1.2】`getDriveDb(const std::wstring& volumeSerial, const QString& driveLetter)` 僵尸函数**：
  - **位置**：`DatabaseManager.cpp:644-689`，`DatabaseManager.h:76`
  - **缺陷说明**：负责分库的创建、检查与加载，且内含盘符漂移修复逻辑，完全违背“单全局库 + 离散 JSON”架构。
  - **重构目标**：废除或标记弃用，内部统一桥接到 `getGlobalDb()`，不再产生/操作盘符分库。

* **【标记 1.3】`resolveVolumeDrift(...)` 漂移处理僵尸函数**：
  - **位置**：`DatabaseManager.cpp:789-868`，`DatabaseManager.h:190`
  - **缺陷说明**：专门用于在 `.QuarkMeta` 目录下生成 `QuarkMeta_<VSN>_<Letter>.db` 并在盘符变化时重命名，是污染用户磁盘根目录的罪魁祸首。
  - **重构目标**：彻底从代码库中物理剔除。

* **【标记 1.4】`getActiveMemoryDbs()` 与 `m_driveDbs` 映射表**：
  - **位置**：`DatabaseManager.cpp:700-708`，`DatabaseManager.h:182`
  - **缺陷说明**：维护了分库容器 `std::unordered_map<std::wstring, DbConnection> m_driveDbs`，导致内存中管理多个分库连接。
  - **重构目标**：简化容器，全局仅保留 `m_globalDb`。

---

### 2. `src/meta/MetadataManager.h` & `MetadataManager.cpp`
* **【标记 2.1】`QuarkMeta_*.db` 文件扫描与加载遗毒**：
  - **位置**：`MetadataManager.cpp:389-415`
  - **缺陷说明**：在初始化或扫描盘符时，使用 `QRegularExpression("^QuarkMeta_([0-9A-F]{8})(?:_([A-Z]))?\\.db$")` 去查找硬盘根目录下的分库文件并加载。
  - **重构目标**：清理正则表达式扫描与分库加载分支。

* **【标记 2.2】多处散落的 `getDriveDb` 调用**：
  - **位置**：`MetadataManager.cpp:723, 842, 911, 1769, 1949, 2031, 2158, 2491, 2565` 以及 `TagRepository.cpp:202`
  - **缺陷说明**：多处业务查询在写数据库前先通过 `getDriveDb` 获取句柄。
  - **重构目标**：全部收归使用 `getGlobalDb()`。

---

## 二、 回收站 (Disk Trash) 机制潜在缺陷与改进标记

### 1. `src/core/DiskTrashService.cpp`
* **【标记 3.1】多数据库遍历问题 (`getActiveMemoryDbs()`)**：
  - **位置**：`DiskTrashService.cpp:266, 299`
  - **缺陷说明**：在 `restoreAllDiskTrash()` 和 `emptyDiskTrash()` 中遍历 `getActiveMemoryDbs()` 来清空/还原回收站。由于历史分库存在，会导致多次重复查询。
  - **重构目标**：直接面向 `getGlobalDb()` 查 `disk_trash` 全量表。

* **【标记 3.2】还原时父目录重建防护验证**：
  - **位置**：`DiskTrashService.cpp:115`
  - **现状**：已包含 `QDir().mkpath(QFileInfo(originalPath).absolutePath())`。
  - **重构目标**：需确保在跨驱动器或者磁盘未挂载时有更友好、稳健的错误提示与失败捕获。

* **【标记 3.3】硬编码 `.QuarkMeta/disk_trash` 字符串收归统一**：
  - **位置**：`DiskTrashService.cpp:23`，`ContentPanel.cpp:2386`
  - **缺陷说明**：多处手工硬编码路径字符串，容易因拼写或大小写不一致导致逻辑失效。
  - **重构目标**：定义全局常量 `QUARKMETA_TRASH_RELATIVE_PATH` 集中管理。

---

## 三、 UI 与数据同步层缺陷标记

### 1. `src/ui/ContentPanel.cpp`
* **【标记 4.1】回收站数据加载字段完整性（图片缩略图与 File_ID 关联）**：
  - **位置**：`ContentPanel.cpp:2939-2980`
  - **现状**：已补全 `rec.suffix` 和 `rec.fileId`。
  - **重构目标**：进一步推演 UI 上对 `disk_trash` 项目的快捷操作（如右键彻底删除、右键还原），确保其关联的 `File_ID` 隔离盒文件夹被干净物理剔除。

---

## 四、 总结与后续处理路线图

1. **第一阶段**：更新 `QuarkMeta-Architecture-Planning.md`，确立全局唯一数据库 `global.db` 铁律，彻底废除分库理念。
2. **第二阶段**：清理 `DatabaseManager` 和 `MetadataManager` 中的 `getDriveDb` / `resolveVolumeDrift` 等分库僵尸代码。
3. **第三阶段**：规范回收站对 `global.db` 的统一读写与全链路异常防护。
