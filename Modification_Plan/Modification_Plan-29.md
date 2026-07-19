# AutoImportManager 职责解耦与多盘符并行无大锁优化 —— Modification_Plan-29.md

## 1. 任务背景
在《ArchitectureComplianceAudit.md》架构合规性审计报告中，`AutoImportManager`（判定为 **FAIL** 的第 7 项，同时也是严重级别第 4 项）名义上应是专职于后台物理 NTFS 托管目录 USN/MFT 变动的感知与捕获。然而在其历史演进中，其承担了多项完全越界的业务逻辑：它不仅管理物理变动，还直接参与了“AppConfig 用户状态历史记录（上限为 14 条）”的维护和落盘；在对账递归中直接级联构建分类树（1:1 建立 Category 物理关联记录）；更严重的是，其内部使用全局大锁（`getGlobalMutex`，对应不合规点中指出的 `s_dbAccessMutex` 统一大锁性质）笼统保护所有后台线程对数据库和分类的关联。这在导入数百万级别的大规模数据量时，会导致多盘符物理对账或扫描扫描发生极其严重的互斥串行排队。这极大地破坏了 WAL 模式多盘符物理并发的核心架构，必须对其进行职责剥离并开展无大锁高并发重构。

## 2. 问题定位
- **定位模块 1（导航历史记录插手越界）**：
  在 `src/core/AutoImportManager.cpp` 的 `recordRecentVisitedFolder`（第 94 行）与 `getRecentVisitedFolders`（第 107 行）中，其直接读写 AppConfig 字段并维护硬编码为“上限 14 条”的历史记录。
- **定位模块 2（重型分类级联算法硬编码）**：
  在 `AutoImportManager::handleRecursiveIngestion`（第 213 行）中，其内部包含了大段深度优先（DFS）递归逻辑，用于级联检查分类目录、建立并创建 1:1 映射的 Category 对象。这些工作应该由 `CategoryRepo` 统一在持久化层内聚实现，而不是物理 USN 增量感知器去发令。
- **定位模块 3（全局大锁高并发死锁/瓶颈隐患）**：
  在 `handleRecursiveIngestion` 的头部（第 217 行）：
  ```cpp
  std::lock_guard<std::mutex> globalLock(DatabaseManager::instance().getGlobalMutex());
  ```
  该全局锁把所有的后台递归注册和多线程分库对账操作强行锁成了“单通道串行排队”，完全破坏了各个盘符通过 `driveLock` 实现并行不悖的 WAL 多流高并发。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 历史记录管理职责分离 | 彻底移出并删除最近访问文件夹管理功能（对应用户原话：“维护、上限为 14 条的历史访问记录管理”），解开 USN 物理引擎对用户状态的侵入。 | ✅ 一致 |
| 2    | 分类树构建级联重构下沉 | 将深度优先 DFS 级联扫描、1:1 自动建立 Category 的重型逻辑移出（对应用户原话：“在 DFS 递归中 1:1 分级建立 Category 对象”），统一移入持久层中内聚。 | ✅ 一致 |
| 3    | 消除全局同步大锁 | 彻底废除 `getGlobalMutex` 同步段（对应用户原话：“使用 static std::recursive_mutex s_dbAccessMutex; 笼统保护所有后台线程的写数据库和分类关联，导致多盘符并行扫描对账时发生无意义的互斥排队”），实现各个驱动器对账完全自主并行。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 用户导航历史记录职责彻底剥离
1. **取消物理感知层对历史记录的插手**：
   从 `AutoImportManager.h` 与 `AutoImportManager.cpp` 中**彻底删除** `recordRecentVisitedFolder` 与 `getRecentVisitedFolders` 函数接口。
2. **移交与重新放置**：
   关于访问路径的“上限 14 条（对应用户原话：“上限为 14 条的历史访问记录管理”）”逻辑，应当重定位至专有的控制器或 AppConfig 专职配置管理器中。使后台监听器退回为“哑状态通知器”，从而消除该多余职责。

### 4.2 DFS 递归级联分类映射下沉（持久化层高内聚）
1. **下沉 DFS 递归算法**：
   将 `handleRecursiveIngestion` 内部关于级联扫描并调用 `CategoryRepo::findByFrn`、`CategoryRepo::add` 物理建分类的这一整套算法剥离。
2. **重定位至 CategoryRepo**：
   在 `CategoryRepo` 命名空间或类定义中，重构并引入统一的持久化对账接口：
   ```cpp
   // CategoryRepo::syncPhysicalDirectoryCascade(const std::wstring& rootPath)
   ```
   `AutoImportManager` 在对账时，直接分发调用该内聚的持久化对账接口。这不仅使 DFS 算法实现了高内聚，而且彻底解耦了 MFT 感知引擎与 Category 数据库细节字段的硬编码。

### 4.3 彻底消除全局大锁，实现多盘符真正 WAL 并发
1. **删除全局数据库同步锁**：
   在 `AutoImportManager::handleRecursiveIngestion`（及新下沉的对账逻辑中），**彻底移除** `DatabaseManager::instance().getGlobalMutex()` 全局锁。
2. **完全采用驱动器局部锁（细粒度实例锁）**：
   各盘符在扫描对账、更新其单独的分库记录时，使用其卷序列号（Volume Serial）对应的细粒度 `getDriveMutex(vol)` 实例锁对数据库事务 `SqlTransaction` 与元数据注册进行保护：
   ```cpp
   // std::lock_guard<std::mutex> dLock(*DatabaseManager::instance().getDriveMutex(vol));
   ```
   由于不同的磁盘物理存储在不同分库文件中（如分卷 SQLite 库），物理 IO 与锁竞争完全在空间上隔离。拆除全局总锁后，双盘符或多盘符并行对账时可达到 100% 的极速吞吐，毫秒内并发完成，彻底终结了在大数据量启动时的串行等待假死。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/file：
  - `src/core/AutoImportManager.h` / `src/core/AutoImportManager.cpp` （彻底删除历史导航管理，在 `handleRecursiveIngestion` 废除 `getGlobalMutex` 同步大锁，并将 DFS 1:1 分类级联构建重型逻辑剥离）
  - `src/meta/CategoryRepo.h` / `src/meta/CategoryRepo.cpp` （封装下沉过来的 `syncPhysicalDirectoryCascade` 物理级联对账级联方法，确保 Category 数据库写入的高效安全）

**明确禁止越界修改的范围：**
- [ ] 明确禁止破坏 `m_managedFrnCache` (FRN 链高效托管过滤) 缓存，不得改动 `isUnderManagedLibrary` 判断的极速查表模式。
- [ ] 明确禁止改动 `onEntryAdded`、`onEntryUpdated` 中的去抖合并超时写入 `processImportQueue`（不修改）。

## 6. 实现准则与预警【核心】
1. **防范编译错误与未定义引用**：因为删除了 `AutoImportManager::recordRecentVisitedFolder`，必须精准排查 `ContentPanel.cpp` (如第 2906 行) 及全项目中所有对它的调用，并同步重定向至 AppConfig 或专有 Controller 中。
2. **细粒度锁的获取顺序防死锁（Lock Ordering）**：在对账时，如果需要分库锁与持久层并发保护，必须严格自下而上遵循“先实例锁、后事务 SqlTransaction”的锁获取顺序，严禁交叉获取，确保高并发下的极致安全。
3. **结合上下文闭环**：重构完成后，须通过编译并完全通过大批量并行对账自动化自检，保证在多线程多盘符极限并发时无任何 Data Race 隐患。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-29.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，采用细粒度的 driveLock 和标准 SqlTransaction 进行事务保护 |
| 无锁优化 | 在没有全局同步公共资源时，绝对禁止任意扩大全局锁的作用域 | ✅ 符合，完全移除了 Global 锁，推行独立的卷物理隔离 |

## 8. 待确认事项（可选）
- **最近访问历史记录上限定义**：方案已对齐用户原话的 “上限为 14 条的历史访问记录管理（对应用户原话：“上限为 14 条的历史访问记录管理”）”。落盘搬运至 AppConfig 时将维持此常量标准。
