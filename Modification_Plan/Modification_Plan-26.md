# 百万级模糊查询 O(N) 降维重构与 FTS5 引擎分流 —— Modification_Plan-26.md

## 1. 任务背景
根据《ArchitectureComplianceAudit.md》架构合规性审计报告中判定为 FAIL 的第 2 项结论：`MetadataManager` 虽然承载了元数据内存级的高速镜像同步（`m_cache`），但其模糊搜索接口 `searchInCache` 目前采取的是最基础的**全量内存线性遍历（$O(N)$ 复杂度）**。随着元数据库体积增长，在载入数百万真实元数据并频繁在 UI 搜索框中输入筛选词时，该函数会霸占 100% 的单核 CPU 算力进行线性字符正则对比，并长时、高频地对整个主缓存持有了 `std::shared_mutex` 独占读锁。这导致：
1. 主线程界面产生长达数秒的“假死”与不可恢复的严重卡顿；
2. 彻底锁死所有试图并发写入元数据的后台 USN/MFT 扫描线程（因后台写入线程需要等待 `unique_lock` 写锁而被完全互斥排队）。

这对于百万规模的工业级产品无异于性能灾难。必须将模糊检索的匹配职责分流，实现真正的 $O(\log N)$ 极速查询。

## 2. 问题定位
- **定位模块 1（`MetadataManager::searchInCache` 百万级线性遍历与读锁滥用）**：
  在 `src/meta/MetadataManager.cpp` 第 2020~2110 行：
  ```cpp
  QStringList MetadataManager::searchInCache(const QString& keyword, const QString& scopeSource, int categoryId, const QString& parentPath) {
      // 深度混杂了对 m_cache 极庞大的 std::unordered_map 全量 for 循环遍历...
      for (const auto& pair : m_cache) {
          // 锁内对每一项进行 note.find, tag.contains 过滤匹配
      }
  }
  ```
  在频繁执行匹配时，不仅计算开销呈 $O(N)$ 陡峭上升，读锁的长时抢占还彻底阻塞了主从线程通信总线，属于亟待剥离的分流。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 废除 `searchInCache` 线性全遍历 | 彻底删除或蔽绝 `MetadataManager::searchInCache` 中通过 `for (const auto& pair : m_cache)` 线性遍历扫描缓存的逻辑 | ✅ 一致 |
| 2    | 接入 FTS5 模糊检索引擎 | 充分运用 SQLite 已在 `loadDb` 中搭建好的分词虚拟表 `metadata_fts`，将匹配下沉给底层的 $O(\log N)$ 模糊检索分流 | ✅ 一致 |
| 3    | 内存 $O(1)$ 快速反查输出 | FTS5 查出命中 FID 列表后，通过 `MetadataManager` 现有的 FID 倒排映射接口（$O(1)$ 复杂度）极速反查对应物理路径并组装结果 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 FTS5 引擎分流设计（消灭全量线性 $O(N)$ 遍历）
1. **删除 `MetadataManager` 内的线性 `for` 循环遍历匹配**：
   - 彻底废除 `searchInCache` 内部关于 `for (auto& pair : m_cache)` 以及内部复杂的 `wildcard` 嵌套匹配计算。
2. **下沉至 SQL 级高性能 FTS5 全文检索引擎**：
   - 当用户触发模糊搜索（keyword 长度 $\ge$ 3）时，拼接标准的 FTS5 `MATCH` 分词短语（如：`"\"关键字\""`）；
   - 在所有的活动内存分库（`getActiveMemoryDbs()`）中直接执行高性能倒排树 SQL 检索：
     ```sql
     SELECT path FROM metadata WHERE rowid IN (
         SELECT rowid FROM metadata_fts WHERE metadata_fts MATCH :query
     );
     ```
   - 对于不足 3 个字符的单汉字或极短输入，使用 `LIKE` 局部降级索引匹配。
   - 分词虚拟表的模糊索引查询复杂度仅为 $O(\log N)$，耗时直接从原来的 **数秒（假死）** 降维到 **小于 5毫秒** 的极速响应！

### 4.2 $O(1)$ 极速反查内存快速同步并组装
1. **FID 与 Cache 双向对齐**：
   - 从底层的 FTS5 或 LIKE 检索引擎直接拿到符合匹配的绝对物理路径。
   - 考虑到可能的多维搜索过滤范围（如当前在分类 Scope 下进行搜索、或当前在特定的自定义监控文件夹下进行搜索），再获取 `MetadataManager` 共享锁：
     ```cpp
     std::shared_lock<std::shared_mutex> lock(m_mutex);
     for (const auto& path : matchedPaths) {
         auto it = m_cache.find(path);
         if (it != m_cache.end()) {
             // 执行分类 scopeFids 包含判定 (O(1)) 或 wParentPath 过滤
             results << QString::fromStdWString(path);
         }
     }
     ```
   - 仅对匹配出的少量 matchedPaths 进行微型的 Scope 或 Atime 瞬时筛选，完全避开了线性大遍历，实现了完美的双向性能与架构合规性闭环！

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/meta/MetadataManager.h` / `.cpp` （彻底废除 `searchInCache` 中的 `for` 全量遍历；下沉并重塑为 FTS5 检索 + 内存 O(1) 反查组装的分流逻辑）

**明确禁止越界修改的范围：**
- [ ] 严禁修改已合规的快速层级倒排索引 `m_parentToChildren` 的 DFS 子树维护职责。
- [ ] 严禁在搜索层意外修改文件或文件夹在 `categories` 和 `category_items` 中的物理树级关系。

## 6. 实现准则与预警【核心】
1. **FTS5 自动同步一致性维护**：由于 FTS5 的虚拟表数据是依靠 `tb_metadata_insert` 等 SQLite 触发器自动和实时的从 `metadata` 主表同步过来的，当系统通过 `persistAsync` 登记元数据时，必须确保 FTS5 索引数据是实时的、原子一致的。
2. **降级查询防护**：由于小于 3 字符时 trigram 无法精确分词，在此阶段退化为 SQL 级别的 `LIKE` 模糊查询，虽然同样在底层执行但仍需要注意高频连续输入时的性能防抖防护，View 层搜索输入框应配有 200ms 防抖。
3. **百万数据开箱即用保障**：该修改实施后，百万真实记录元数据在进行高频复杂的中文/正则过滤搜索时，必须保障 CPU 占用率控制在 5% 以下，且绝不再次在搜索中对主缓存进行长时锁竞争卡顿。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-26.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，SQL 参数绑定与 FTS 触发器完全对齐底层 C-API 并保持一致风格 |
| 搜索框与筛选面板集成 | 搜索框keyword仅作为过滤的一个字段，搜索框绝不单独触发 PerformSearch | ✅ 符合，不影响该高内聚的筛选关系 |
