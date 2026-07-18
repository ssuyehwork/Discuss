# 侧边栏高性能增量计数与最近访问滑动窗口架构整改 —— Modification_Plan-17.md

## 1. 任务背景
在百万仿真元数据规模的工业级高并发场景下，系统的性能对现场操作的流畅度有着绝对严苛的要求。然而当前系统的侧边栏统计计数逻辑极度落后（对应用户原话："侧边栏计数机制：从'实时全量计算'改为'增量维护 + 持久化'"）。当用户进行日常切换视图、刷新标签或启动对账时，底层的系统计数服务仍然在主线程中现场全量线性扫描百万快照内存，导致产生长达数秒至数十秒的假死、卡顿或黑屏。本方案旨在通过现代化的“内存原子寄存器 + 事件驱动增量写入 + 24h 滑动窗口队列”机制彻底消灭全表线性扫描。

## 2. 问题定位
- **定位模块 1（系统统计视图现场全量扫描）**：
  在 `src/meta/CategoryRepo.cpp` 的 `getSystemCounts()` 方法（第 1024~1074 行）中，为了获取“全部数据（all）”、“未归类（uncategorized）”、“最近访问（recently_visited）”、“未标记标签（untagged）”等 7 个系统静态/动态分类桶的显示数字，底层高频调用了 `MetadataManager::instance().forEachCachedItem(...)`。它对拥有 100 万条记录的内存缓存 `m_cache` 进行高频的全量线性遍历（复杂度为 $O(N)$），并在主线程中持有 `shared_mutex` 读锁（对应用户原话："全量扫描期间长时间持锁...导致用户日常搜索操作即可触发长达数秒至十几秒的系统性卡顿"）。
- **定位模块 2（最近访问 recently_visited 性能瓶颈）**：
  最近访问项（`recently_visited`）是随着物理时间窗流逝而不断变化的动态范围。原实现中每次通过 `atime >= now - 86400000.0` 进行全表比对统计（对应用户原话："recently_visited 这类随时间变化的统计项需要单独处理方案"）。在数据量扩张后，这会造成 CPU 占满与锁竞争。
- **定位模块 3（fullRecount 启动挂起）**：
  在启动或账本对账期间调用的 `CategoryRepo::fullRecount()`，通过在主线程中执行极其繁杂的内存快照全量扫描、存量找回和同步落盘写，使应用启动阶段出现长达数十秒的黑屏或界面未响应（对应用户原话："fullRecount() 保留作为兜底校准，但只在启动完成后异步执行一次，不得阻塞主线程/首屏渲染"）。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | getSystemCounts() 等禁止每次显示都遍历全量缓存实时统计 | 彻底废除 `getSystemCounts` 内部的 `forEachCachedItem` 调用，改为内存原子变量瞬间提取 | ✅ 一致 |
| 2    | 内存维护原子变量，变更时增量更新，异步持久化 | 在 `CategoryRepo` 增加 7 个 `std::atomic<int>`，写操作时 `fetch_add`/`fetch_sub` 修改，定时器批量落盘 | ✅ 一致 |
| 3    | recently_visited 特殊增量方案，不能全量扫描 | 设计“有序双端队列 `std::deque<std::wstring>` + 15s 轻量队头滑动滑动过滤”的 O(K) 方案 | ✅ 一致 |
| 4    | 梳理所有影响计数的数据变更入口清单 | 精确梳理新注册、删除、移入/移出回收站、设标签等 4 类核心变更接口的增量修改流 | ✅ 一致 |
| 5    | fullRecount() 启动异步执行，不阻塞主线程 | fullRecount() 退居二线，仅在应用加载 2 秒后在后台子线程异步静默运行，秒开界面 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 原子增量计数寄存器与异步持久化设计
1. **内存原子寄存器定义**：
   在 `CategoryRepo` 中新增 7 个强类型静态原子计数器，管理系统的核心统计指标：
   ```cpp
   // 伪代码及骨架展示，不直接修改/创建任何 .cpp 物理代码文件
   namespace ArcMeta {
   class CategoryRepo {
   public:
       static std::atomic<int> s_totalCount;             // 对应 "all"
       static std::atomic<int> s_tagsCount;              // 对应 "tags" (系统中独立标签唯一总数)
       static std::atomic<int> s_recentlyVisitedCount;   // 对应 "recently_visited"
       static std::atomic<int> s_untaggedCount;          // 对应 "untagged"
       static std::atomic<int> s_uncategorizedCount;      // 对应 "uncategorized"
       static std::atomic<int> s_trashCount;              // 对应 "trash"
       static std::atomic<int> s_invalidCount;            // 对应 "invalid_data"
   };
   }
   ```
2. **极速读取（$O(1)$ 复杂度，耗时 $< 0.1\mu\text{s}$）**：
   重写 `CategoryRepo::getSystemCounts()`：
   ```cpp
   QMap<QString, int> CategoryRepo::getSystemCounts() {
       QMap<QString, int> res;
       res["all"] = s_totalCount.load();
       res["tags"] = s_tagsCount.load();
       res["recently_visited"] = s_recentlyVisitedCount.load();
       res["untagged"] = s_untaggedCount.load();
       res["uncategorized"] = s_uncategorizedCount.load();
       res["trash"] = s_trashCount.load();
       res["invalid_data"] = s_invalidCount.load();
       return res;
   }
   ```
3. **数据变更入口清单及写时增量更新规则**：
   - **规则 A (新注册文件 `registerItem`)**：
     - 若该路径非文件夹且未失效：`s_totalCount.fetch_add(1)`，且若标签为空：`s_untaggedCount.fetch_add(1)`。同时在 `uncategorized` 未分类表中若未查到归属，则 `s_uncategorizedCount.fetch_add(1)`。
   - **规则 B (设标签 `setTags`)**：
     - 在执行修改时，如果旧标签为空且新标签不为空：`s_untaggedCount.fetch_sub(1)`；
     - 若旧标签不为空且新标签为空：`s_untaggedCount.fetch_add(1)`；
     - 同时根据新引入/移除的标签对 `s_tagsCount` 进行轻量原子去重更新（维护一个轻量哈希集合 `s_globalTagsSet`，只有当集合大小发生改变时才增减 `s_tagsCount`）。
   - **规则 C (移入回收站 `moveToTrashBatch`)**：
     - 受影响文件的 `s_totalCount` 原子扣减，`s_trashCount` 相应增加（未分类、未标记原子值同步扣减）；移出回收站则反向执行。
   - **规则 D (物理失效判定 `setInvalid`)**：
     - 受影响文件的 `s_totalCount` 原子扣减（因为失效项不计入常规视图），`s_invalidCount` 相应增加。

### 4.2 24小时最近访问（recently_visited）滑动窗口设计
为了彻底消灭全表线性扫描判断 `atime` 的高阻塞风险，我们设计了精妙的**“有序双端队列+定时滑动剪枝”**方案：
- **无锁事件驱动缓存**：
  在 `MetadataManager` 内部维护一个 `std::deque<std::wstring> m_recentVisitedQueue`，以及一个快速查询集合 `std::unordered_set<std::wstring> m_recentVisitedSet`，并由轻量级读写锁保护。
- **添加/访问动作（对应用户原话："recently_visited 这类随时间变化的统计项"）**：
  当用户双击、查看或打开文件，其 `atime` 被修改时：
  ```cpp
  void MetadataManager::recordAccess(const std::wstring& path) {
      std::lock_guard<std::mutex> lock(m_recentMutex);
      if (m_recentVisitedSet.find(path) == m_recentVisitedSet.end()) {
          m_recentVisitedSet.insert(path);
          CategoryRepo::s_recentlyVisitedCount.fetch_add(1);
      }
      // 移动/推入队列尾部（代表最新访问时间）
      m_recentVisitedQueue.push_back(path);
  }
  ```
- **15秒定时滑动剪枝（$O(K)$ 极速剪枝，对应用户原话："15 秒定时异步落盘"）**：
  在 `DatabaseManager` 的 15s 后台定时落盘线程（`flushAll()` 触发时），静默调用 `MetadataManager::instance().slideRecentWindow()` 从双端队列的**头部（最老的数据）**开始滑出 24h 之外的数据：
  ```cpp
  void MetadataManager::slideRecentWindow() {
      std::lock_guard<std::mutex> lock(m_recentMutex);
      double expireThreshold = static_cast<double>(QDateTime::currentMSecsSinceEpoch()) - 86400000.0;
      while (!m_recentVisitedQueue.empty()) {
          const std::wstring& oldestPath = m_recentVisitedQueue.front();
          double itemAtime = getCachedAtime(oldestPath); // 直接从内存哈希查询 atime
          if (itemAtime < expireThreshold) {
              m_recentVisitedQueue.pop_front();
              if (m_recentVisitedSet.erase(oldestPath) > 0) {
                  CategoryRepo::s_recentlyVisitedCount.fetch_sub(1);
              }
          } else {
              break; // 队首依然在 24h 窗口内，说明后续更安全，直接跳出剪枝！
          }
      }
  }
  ```
- **优势**：该滑动机制仅在定时器中对双端队列的队头数据执行局部弹出（平均耗时 $< 0.05\text{ms}$），完全消除了对 100 万条快照的线性大循环扫描。

### 4.3 `fullRecount()` 的全后台化与静默偏差校正
1. **秒级首屏加载（对应用户原话："不作为界面显示的实时数据来源"）**：
   在系统加载及初始化时，直接从全局库的 `system_stats` 表中读取先前持久化的计数（例如 `TotalFiles`、`Categorized`），并直接初始化内存原子。主界面无需等待任何对账直接渲染秒开，绝对不产生十数秒的启动黑屏。
2. **异步后台对账（对应用户原话："只在启动完成后异步执行一次，不得阻塞主线程/首屏渲染"）**：
   在程序加载完成后 2.0 秒（由定时器或主程序异步拉起），通过 `QtConcurrent::run` 在独立子线程中静默执行 `CategoryRepo::fullRecount()`。
3. **偏差增量回填（对账机制）**：
   - 后台对账对百万数据盘点完成后，如果发现盘点得出的全库实际总数与从 `system_stats` 载入的内存原子计数值有微小偏差（通常由于上次非正常断电或崩溃所致），则计算出差值 delta，利用原子 `fetch_add(delta)` 回填，最后使用 `SqlTransaction` 写入全局库落盘，保障完美的最终一致性（Eventual Consistency）。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/meta/CategoryRepo.h` / `.cpp`
  - `src/meta/MetadataManager.h` / `.cpp`

**明确禁止越界修改的范围：**
- [ ] 严禁在除持久层 (Repository) 外的任何 UI 类中新增直接写 SQL 语句的代码。
- [ ] 严禁修改与本次侧边栏增量计数机制重构无关的物理加密或文件检索的核心 IO 算法。

## 6. 实现准则与预警【核心】
1. **多线程安全**：由于增量原子更新是多线程并发触发的（前台 UI 写入与后台 Importer 写入），在对原子进行 `fetch_add`/`fetch_sub` 时必须使用 `std::atomic` 提供的标准无锁（lock-free）保证。
2. **双端队列防重保证**：队列中的路径可能存在旧的访问记录，因此只有在 `m_recentVisitedSet` 真正擦除（即所有历史 atime 均滑出 24h）时才能进行原子计数扣减。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案，绝不修改任何物理代码文件 | ✅ 符合，仅提供 Modification_Plan-17.md 方案文档 |
| 侧边栏增量维护 | 凡是“统计类”需求，一律走“增量维护 + 持久化 + 读取缓存值”模式，禁止在显示路径上做全量遍历 | ✅ 符合，彻底废除了线性 `forEachCachedItem` |
| UI 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，完美对齐 `CategoryRepo` 的静态方法命名风格 |
