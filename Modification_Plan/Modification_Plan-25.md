# DatabaseManager 纯净化与物理 I/O 及路由纠偏逻辑剥离 —— Modification_Plan-25.md

## 1. 任务背景
在《ArchitectureComplianceAudit.md》架构合规性审计报告中，`DatabaseManager`（判定为 **FAIL** 的第 1 项）的核心职责本应仅仅是高并发底层连接池分配、高效率 WAL 并行事务处理以及后台增量持久化异步队列（`SyncTaskToken` / `enqueueSyncTask`）的调度管理。然而，在现有实现中，它深度参与了多项与底层数据事务完全无关的“物理磁盘文件变动”职责：在加载驱动器库时直接调用 Windows 物理文件 API（`SetFileAttributesW`）来强标记隐藏属性、在检测到库盘符漂移时直接通过 `QFile::rename` 重命名物理数据库文件、甚至在数据库发生文件冲突时自行动手去清理、移动并将冗余库重命名为“无效”文件。这严重降低了底层数据库组件的职责单一性与跨平台可用性，给事务高可靠运行引入了无谓的外部 I/O 阻滞和权限崩溃风险。为了使其退化为 100% 纯粹的底层持久化句柄管理器（Pure DB Manager），必须将这些物理操作从 `DatabaseManager` 中彻底剥离并解耦。

## 2. 问题定位
- **定位模块 1（`DatabaseManager::ensureHidden` 直接耦合 Windows API）**：
  在 `src/meta/DatabaseManager.cpp` 中定义并调用了：
  ```cpp
  void DatabaseManager::ensureHidden(const std::wstring& path) {
      SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
  }
  ```
  在 `loadDb` 时会频繁直接调用该方法。这破坏了数据库管理器不涉及任何具体物理操作系统文件标志强改写的原则。
- **定位模块 2（`DatabaseManager::getMemoryDb` 包含重型的盘符重命名纠偏及无效数据库冗余清理）**：
  在 `getMemoryDb`（第 365~455 行）中，当检测到数据库卷序列号与实际传入盘符不符、或者探测到由于系统重新分配盘符导致的文件漂移时，`DatabaseManager` 自行执行了复杂的逻辑：
  * 使用 `QFile::exists` 和 `QFile::rename` 进行物理数据库重命名纠偏；
  * 如果目标文件已存在冲突，通过 `QFile::rename` 将冲突的旧库更名为 `Arcmeta_[序列号]_无效.db` 并在文件夹中无限累加。

  这导致“物理层面的路径自适应与设备变动重对账（Routing & Drift Alignment）”职责被强塞在了分配 DB 句柄的函数中，导致其职责发生严重越权。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 物理隐藏（`ensureHidden`）职责剥离 | 彻底删除 `DatabaseManager::ensureHidden` 以及对 `<windows.h>` 的文件属性级直接调用 | ✅ 一致 |
| 2    | 路由纠偏与物理 rename 剥离 | 移除 `DatabaseManager::getMemoryDb` 中关于盘符漂移物理重命名、冲突过滤、无效垃圾库清理等具体逻辑 | ✅ 一致 |
| 3    | 纯净化 (退化为纯 DB 句柄管理器) | 确保其仅管理 SQLite 内存与磁盘连接的双向备份以及 WAL 模式下的并发事务调度，不再执行物理磁盘文件动作 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 剥离物理隐藏属性（`ensureHidden`）
1. **删除 `ensureHidden` 物理 API 耦合**：
   - 彻底废除 `DatabaseManager::ensureHidden`。
   - 物理文件夹隐藏属性的改写职责，应由其创建者或搬运层（例如 `ImportHelper` 或公用的 `ShellHelper` 搬运核心中专门负责新建文件夹的模块）进行，不属于数据库管理职责。

### 4.2 路由纠偏与盘符漂移物理重命名逻辑上移
1. **上移物理纠偏调度权**：
   - `getMemoryDb` 变为“纯哑连接器”：仅接收已计算、确定正确的 wstring 绝对物理路径，并直接返回已经备份好的 memDb 句柄，不参演任何重命名等文件纠偏动作：
   ```cpp
   sqlite3* DatabaseManager::getMemoryDb(const std::wstring& resolvedDiskPath) {
       std::lock_guard<std::mutex> lock(m_mutex);
       // 纯连接分配与加载，若对应路径的 DbConnection 未就绪则调用 loadDb，不参与物理 rename 盘符修正
   }
   ```
2. **路由纠偏下沉或转移至专职的 Device/Routing 模块**：
   - 盘符漂移检测、冗余数据库清理更名（“无效.db”）等硬件级别生命周期的维护逻辑，统一上移到全局中控（如 `CoreController::handleDeviceChange`）或专职的磁盘配置路由器 `DiskRouteManager` 中。当硬件或盘符重定向事件发生时，由路由器调用 `QFile` 物理整理好文件名，再交由 `DatabaseManager` 专心拉起句柄。

### 4.3 数据库组件纯酸高可靠性设计（Pure DB Manager）
1. **纯粹的持久层定位**：
   - 整个 `DatabaseManager` 实现 100% 的跨平台纯连接池。除了调用 SQLite C-API 进行加载（`loadDb`）、Backup 克隆（`saveDb`）及异步 flush 线程（`workerLoop`）调度外，不再产生任何第三方文件 I/O 动作。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/meta/DatabaseManager.h` / `.cpp` （彻底移除 `ensureHidden` 方法；精简重构 `getMemoryDb` 及加载逻辑，移除物理 rename、垃圾库移动和物理纠偏）
  - `src/util/ShellHelper.h` / `.cpp` 或 `src/core/CoreController.cpp` （承接并封装设置文件物理隐藏、垃圾冗余数据库物理归档清理以及盘符路由纠偏逻辑）

**明确禁止越界修改的范围：**
- [ ] 严禁修改 DatabaseManager 原有的 WAL 并发保护机制、std::shared_mutex 读写分离锁机制以及高效的 SqlTransaction 事务 RAII 守卫。
- [ ] 严禁在剥离纠偏时导致原有的“盘符漂移时能继续正确加载历史元数据”的业务功能受损。

## 6. 实现准则与预警【核心】
1. **重定位精度保障**：盘符漂移纠偏剥离上移后，在挂载新盘符或设备变化时，配置与中控层必须在主线程预热或调用 `DatabaseManager` 加载之前，**先通过物理机制（QFile::rename）将漂移的数据库重定位好**，确保 `DatabaseManager` 拉起句柄时文件确实已各就各位，绝不能导致路径不一致从而加载到空的空白数据库。
2. **多驱动器线程高内聚**：由于 `DatabaseManager` 退化为纯句柄管理器，跨盘加载的线程安全性完全依靠原有已合规的 `shared_lock` 分离锁保护，改动时必须对 `loadDb` 与 `getMemoryDb` 的临界区和锁状态进行仔细审查，确保绝对不发生并发连接溢出或锁死。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-25.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，所有接口风格和注释均与现有 `DatabaseManager` 保持完全一致 |
| 窗口置顶 | 一律使用 Win32 原生 `SetWindowPos` | ✅ 符合，不涉及窗口置顶操作 |
