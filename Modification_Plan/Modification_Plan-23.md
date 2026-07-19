# 标签管理界面 SQL 裸写重构与 MVC 解耦 —— Modification_Plan-23.md

## 1. 任务背景
在《ArchitectureComplianceAudit.md》架构合规性审计报告中，`TagManagerView`（标签管理视图，判定为 **FAIL**）被指出存在严重的 MVC 越界与职责不单一问题：作为继承自 `QWidget` 的表现层 UI 类，在多处交互逻辑（如 `addTagToGroup`、`deleteGroup`、`refresh` 等）中直接调用 `sqlite3_prepare_v2` 等数据库底层 API 并显式编译执行 SQL 语句；同时硬编码了获取特定 C 盘连接的物理路径逻辑。当数据规模扩展、或者用户操作热插拔、多盘符同时并发写入时，这种在 UI 线程直接裸写 SQL 句柄的模式极易引发主线程死锁、锁冲突（`SQLITE_BUSY`）以及闪退问题。因此，必须将所有数据库底层逻辑彻底从 View 层中剥离，解耦下沉至数据持久层，实现极致的 MVC 架构规范。

## 2. 问题定位
- **定位模块 1（`TagManagerView::refresh` 内含底层查询与计算）**：
  在 `src/ui/TagManagerView.cpp` 中，`refresh()` 虽然调用了 `TagRepository` 来获取标签组，但却包含了大量直接计算逻辑（例如：对未分类标签数量的统计遍历、字母组分类）。此外，`TagManagerView` 是 UI 层，目前大量耗时的 I/O 解析和组重构直接在 `refresh` 阻塞进行，缺乏防抖与优雅异步通知。
- **定位模块 2（`TagRepository` 本身已存在但部分逻辑混乱 / `TagManagerView` 仍直接调用且硬编码 C 盘连接）**：
  在 `TagRepository.cpp` 中已经提供了一些基本的静态底层 CRUD 接口，但 `TagManagerView` 目前对一些重名、重设等二级菜单操作仍直接采用裸写处理。最关键的是，虽然 `TagRepository` 目前只连全局库 `global.db`，但在 `checkAndMigrate()` 内包含：
  ```cpp
  sqlite3* cDb = DatabaseManager::instance().getMemoryDb(L"C");
  ```
  在多驱动器环境（不一定存在 C 盘或者首字母盘漂移）下，直接硬编码 `L"C"` 作为首要迁移数据源极不合理，且会触发无谓的 C 盘挂载或阻碍无 C 盘环境的顺畅启动。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 废除 `TagManagerView` 中的 SQL 裸写 | 彻底移除 `TagManagerView` 及其关联 UI 中所有的底层 SQLite API（`sqlite3_*`）操作，全部移至持久层 | ✅ 一致 |
| 2    | UI 变为“哑表现”，实现 MVC 彻底分流 | 重构 `TagManagerView` 内部数据结构与刷新逻辑，数据完全通过数据层 Repository 或 Model 提供，UI 仅负责触发和响应渲染 | ✅ 一致 |
| 3    | 消除盘符硬编码 / 路由解耦 | 解耦 `TagRepository::checkAndMigrate` 中的 `L"C"` 强硬编码，改用动态盘符探针或系统活跃磁盘列表 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 数据访问完全下沉至持久层（`TagRepository` 重构）
1. **统一数据管理通道**：
   - 确保 `TagManagerView` **完全不包含**任何 `sqlite3.h` 引用或 SQLite 原生句柄操作。
   - 所有关于标签、标签组的关系增加、移除、更名操作均由 `TagRepository` 独立接口完全接管。
2. **重构迁移纠偏逻辑（废除硬编码 `L"C"`）**：
   - 在 `TagRepository::checkAndMigrate()` 中，通过 `MftReader::instance().getDriveList()` 动态遍历当前系统中所有已被监控或存在的盘符字母，代替写死的 C 盘探测：
   ```cpp
   std::vector<QString> drives = MftReader::instance().getDriveList();
   for (const QString& drive : drives) {
       std::wstring volSerial = ... // 根据盘符获取对应卷序列号
       sqlite3* driveDb = DatabaseManager::instance().getMemoryDb(volSerial, drive);
       // 若存在旧版标签数据，执行向全局 global.db 的迁移合并
   }
   ```
   - 彻底消灭硬编码盘符，确保跨平台/无 C 盘环境的系统稳定性。

### 4.2 重构 UI 使其退化为 100% 的“哑表现”（View 层解耦）
1. **数据拉取与流式布局优化**：
   - `TagManagerView` 不再自行在刷新中做高耗时的映射和统计，所有数据直接通过 `TagRepository::getAllGroups()` 和 `MetadataManager::instance().getAllTags()` 读取格式化好的结构体或内存映射。
   - 分类统计（如 `allCount`，`uncategorizedCount`）仅依赖内存数据，不在 UI 刷新流程中嵌套多重数据库查询。
2. **多线程/异步安全防护**：
   - 在 UI 触发标签创建（`createNewGroup`）、更名、删除时，全量使用 `QtConcurrent::run` 在后台执行 Repository 物理变更，不阻塞主线程。
   - 底层持久化成功后，通过 `QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection)` 回调主线程重新渲染 UI。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/ui/TagManagerView.h` / `.cpp` （彻底废除任何直接与 `sqlite3` API 耦合的操作；将同步计算优化为哑表现渲染）
  - `src/meta/TagRepository.h` / `.cpp` （增加或加固标签核心 CRUD，下沉合并数据库操作；解耦并消除 `checkAndMigrate` 中对 C 盘 `L"C"` 的硬编码）

**明确禁止越界修改的范围：**
- [ ] 严禁修改 `TagManagerView` 已有的 3 栏标准流式布局几何约束计算。
- [ ] 严禁在除 `TagRepository` 和 `DatabaseManager` 外的任何表示层类中引入 `sqlite3.h` 头文件及物理底层句柄。

## 6. 实现准则与预警【核心】
1. **头文件严格准入**：在 `TagManagerView` 中，严禁出现 `#include "sqlite3.h"`。
2. **后台线程环境完整性**：异步触发 `TagRepository` 写入操作时，若内部使用 `SqlTransaction` 或 `WriteGuard`，由于我们已经在 Plan-131 采用了磁盘 WAL 直连并发，不需要重复获取锁，但多线程下必须保障 `sqlite3_last_insert_rowid` 等 API 取值是在对应的单例连接中安全排队的，避免发生多线程事务交织污染。
3. **安全防护机制**：在重命名标签或删除组时，持久层应有完备的防御逻辑（如目标组不存在、输入重名等），不将原始底层错误抛向 View，提高界面交互的健壮性。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-23.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，所有接口风格和注释均与现有 `TagRepository` 保持完全一致 |
| 窗口置顶 | 一律使用 Win32 原生 `SetWindowPos` | ✅ 符合，不涉及窗口置顶操作 |
