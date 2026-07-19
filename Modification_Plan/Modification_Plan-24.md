# 账本核对审计（fullRecount）增量化重构与 MVC 分离 —— Modification_Plan-24.md

## 1. 任务背景
根据《ArchitectureComplianceAudit.md》架构合规性审计报告中判定为 FAIL 的第 3 项结论：`CategoryRepo` 不仅承载了分类的持久化核心逻辑，还深度混合了“全账本核对审计（`fullRecount`）”、“全局静态计数器维护（`s_totalFileCount`）”等重型业务。在数百万记录级别的高吞吐场景下，每次分类树刷新或启动初始化，系统会高频触发 $O(N)$ 复杂度的全缓存线性扫描进行对账盘点，从而在主线程产生严重的、无法自动恢复的死锁与 UI 长时间假死。对此，必须将 `CategoryRepo` 的全量阻塞式账本核对解耦下沉为**高效的内存增量对账模型**，彻底斩断启动假死源。

## 2. 问题定位
- **定位模块 1（`CategoryRepo::fullRecount` $O(N)$ 全缓存对账瓶颈）**：
  在 `src/meta/CategoryRepo.cpp` 第 814 行：
  ```cpp
  void CategoryRepo::fullRecount() {
      // ... 遍历内存中数百万记录
      MetadataManager::instance().forEachCachedItem([&](...){ ... });
      // ... 阻塞主线程执行 7 组原子计数更新
  }
  ```
  在系统启动及后台对账加载（由 `CoreController` 启动信号拉起）时，这一阻塞调用将直接霸占主线程大量 CPU 时间。
- **定位模块 2（全局静态指标与分类持久层的过度耦合）**：
  在 `src/meta/CategoryRepo.h` 第 139~155 行中，直接定义了 `s_totalFileCount` 等大批纯业务指标原子，并在增删关联项时强行耦合调用 `updatePersistentStat`。持久层与内存统计指标的高耦合限制了数据库 WAL 模式下的纯粹并行能力。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 废除 `fullRecount` 全量线性对账 | 彻底删除或废除启动与高频刷新对 `fullRecount()` 的全量 $O(N)$ 缓存遍历，改为非阻塞和增量模式 | ✅ 一致 |
| 2    | 构建“内存增量对账模型” | 运用分类树变更与持久化写入的事务信号，增量增减分类数量，在 global.db 和内存间形成原子状态闭环 | ✅ 一致 |
| 3    | 剥离全局静态计数器职责 | 将非纯粹分类关系的计数维护从 `CategoryRepo` 类中剥离或迁移至专职统计层中，降低其职责耦合度 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 引入增量对账模型（Incremental Recount）与锁解耦
1. **废除 `fullRecount` 同步阻塞**：
   - 彻底废除 `CategoryRepo::fullRecount` 在主初始化链路（`CoreController::startSystem` 后续）中的高危害全量线性对账阻塞。
   - `fullRecount` 仅保留在后台守护线程中作为辅助性的异常恢复（或完全替换），且执行时不对主缓存加独占锁。
2. **增量内存统计引擎的设计**：
   - 在添加分类项 `addItemToCategory` 时，不调用 `syncCategorizedCountForFid` 重新扫描全表；而是精确更新对应分类 ID 的局部增量：
   ```cpp
   // 添加分类项时
   s_uncategorizedCount.fetch_sub(1); // 未分类计数 -1
   s_categorizedCount.fetch_add(1);   // 已分类计数 +1
   updatePersistentStat(STAT_CATEGORIZED, 1);
   ```
   - 在移除分类项 `removeItemFromCategory` 时：
   ```cpp
   // 移除分类项时
   s_uncategorizedCount.fetch_add(1); // 未分类计数 +1
   s_categorizedCount.fetch_sub(1);   // 已分类计数 -1
   updatePersistentStat(STAT_CATEGORIZED, -1);
   ```
   - 完美的原子状态转移（Transaction-Level Delta Tracking），从机制上将计数器计算复杂度由 $O(N)$ 剧降为 $O(1)$。

### 4.2 剥离全局静态计数器职责（MVC 及架构解耦）
1. **建立隔离的统计指标层**：
   - 废除 `CategoryRepo` 中维护 `total_file_count` 等全局统计的冗余职责。
   - 分类计数（`getSystemCounts`）直接从专职的 `system_stats` 数据库中读取最新的持久化数值并缓存，实现真正的职责下沉。
2. **异步 UI 局部通知（消灭 UI 颠簸）**：
   - 更新计数后，通过标准的 Qt 局部信号通知，利用 `updateStatistics` 增量通知 UI 组件刷新角标，不调用 `beginResetModel`，从而彻底杜绝全量展开折叠重新计算。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/meta/CategoryRepo.h` / `.cpp` （重构 `fullRecount` 与分类项操作，下沉增量计数逻辑；剥离静态计数器的非持久化关联）
  - `src/core/CoreController.cpp` （调整启动链条，移除对 `CategoryRepo::fullRecount()` 的同步加载调用，确保开箱即秒级加载）

**明确禁止越界修改的范围：**
- [ ] 严禁修改物理分类文件的 FRN 映射（`findByFrn`）及受管逻辑树的父子层级重排结构。
- [ ] 严禁修改回收站高级状态转换的批量事务。

## 6. 实现准则与预警【核心】
1. **数据库一致性同步**：大批量导入时，增量增减的累积原子差值必须通过 WAL 直连事务定时或原子地同步持久化回 `system_stats` 数据库，防止因断电等异常退出导致计数对账发生偏移。
2. **线程隔离防御**：增量对账模型处于后台，其局部 `fetch_add` / `fetch_sub` 操作必须是线程安全的（采用 `std::atomic`），防止主界面点击与 USN 热插拔线程引发并发数据交织。
3. **开箱即用度保障**：对账优化后，必须确保在百万量级数据载入时，从点击盘符到侧边栏更新响应在一毫秒内完成，绝对没有任何 $O(N)$ 扫描带来的毫秒级卡顿。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-24.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，新设计的增量更新接口和机制完全遵循 `CategoryRepo` 原有的风格规范 |
| 侧边栏分类树 | 侧边栏分类树驱动必须基于 DB 驱动和 1:1 分级构建 | ✅ 符合，不影响其关系持久化内核 |
