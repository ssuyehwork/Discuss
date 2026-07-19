# CoreController 物理检索职责剥离与控制器纯净化 —— Modification_Plan-30.md

## 1. 任务背景
在《ArchitectureComplianceAudit.md》全量架构合规性审查审计报告中，`CoreController`（判定为 **FAIL** 的第 10 项）被指出在 `performSearch` 内部深度干预了具体的物理磁盘检索实现，直接通过 `QDirIterator` 开展 I/O 级的目录 DFS 递归物理扫描、文件名比对和信号攒批。这导致高维的“全局业务流程管理机制”与“底层物理磁盘文件系统检索细节（对应用户原话：“在 performSearch 内部深度干预了具体的物理磁盘搜索分支、通过 QDirIterator 执行 I/O 级目录递归扫描。这导致业务流程管理与物理磁盘文件系统检索细节高度强耦合”）”发生严重的穿透强耦合，严重违背了“单一职责”与“开闭原则”。在百万条记录级规模的大数据集下，如果用户在库外导航，任何直接对磁盘 DFS 的物理调用都可能由于机械硬盘转速或百万深层目录而将 UI 主线程、后台调度机制死锁或完全挂起。为此，必须将其物理磁盘检索具体实现彻底下沉剥离。

## 2. 问题定位
- **定位模块 1（物理 I/O DFS 检索与过滤强耦合）**：
  在 `src/core/CoreController.cpp` 的 `CoreController::performSearch` 内部（第 142 行至第 188 行）中，其手写了 `QDirIterator`：
  ```cpp
  QDirIterator it(parentPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
  ```
  在 while 循环中逐条判断物理文件是否包含 `keyword` 并进行攒批、去重发射。
- **定位模块 2（控制器非单一职责）**：
  `CoreController` 的核心定位应是“作为全局中控，管理系统就绪、状态栏文本（对应用户原话：“作为全局中控（管理系统就绪、状态栏文本）”）”，协调生命周期的流转与全局动作分流。目前它却直接写物理磁盘 API、硬编码模糊匹配字符串比较、甚至进行攒批（批处理上限为 50 条）限速。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 控制器定位单一化 | 让 `CoreController` 摆脱低维文件系统扫描，仅承担无状态、纯粹的全局业务控制和异步流向调度（对应用户原话：“作为全局中控（管理系统就绪、状态栏文本）”）。 | ✅ 一致 |
| 2    | I/O 检索细节物理剥离 | 彻底废除 `CoreController` 内部硬编码的 `QDirIterator` 及检索过滤，移至专职组件（对应用户原话：“在 performSearch 内部深度干预了具体的物理磁盘搜索分支、通过 QDirIterator 执行 I/O 级目录递归扫描”）。 | ✅ 一致 |
| 3    | 抽象流式查询适配中枢 | 建立独立的物理磁盘查询代理或检索提取器 `PhysicalDiskSearchExtractor`，通过异步回调接收检索出的结果。 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 抽象专职的物理磁盘检索组件（`PhysicalDiskSearchExtractor`）
1. **组件化隔离**：
   在 `src/core/` 目录或持久化层建立专职的物理磁盘检索代理 `PhysicalDiskSearchExtractor` 静态或单例类。
2. **移入并内聚 DFS 物理实现**：
   将 `QDirIterator` 扫描、文件名关键词 `CaseInsensitive` 的包含判定、 seenPaths 重复判定、攒批上限控制（累积 50 条发射一次，防止 UI 信号淹没）等细节算法物理剪切、重写内聚至 `PhysicalDiskSearchExtractor::performDiskSearch` 方法中。

### 4.2 全局中控控制器纯净化（`CoreController`）
1. **哑中控中继流式结果**：
   在 `CoreController::performSearch` 内部，第一阶段（内存快照检索）依旧使用 `MetadataManager::instance().searchInCache(...)` 毫秒级返回并流式发射。
2. **第二阶段异步中转回调**：
   第二阶段（物理磁盘检索）彻底消除 `QDirIterator` 的 physical includes。通过将 `searchResultsAvailable` 信号作为 std::function 回调传递：
   ```cpp
   PhysicalDiskSearchExtractor::performDiskSearch(parentPath, keyword, m_isSearchAborted, searchId, [this](const QStringList& batch) {
       emit searchResultsAvailable(batch, true);
   });
   ```
   如此一来，`CoreController` 无需感知任何关于磁盘迭代器、物理过滤、匹配步进等硬编码物理机制，代码体积极度缩减，编译高度解耦，MVC 边界纯净清晰。

## 5. 修改边界声明【范围】

**本次方案涉及范围：**
- [ ] 模块/file：
  - `src/core/CoreController.h` / `src/core/CoreController.cpp` （彻底废除 `QDirIterator` 以及模糊文件名匹配，使用 PhysicalDiskSearchExtractor 对物理扫描进行无状态适配和中转）
  - 新建 `src/core/PhysicalDiskSearchExtractor.h` / `src/core/PhysicalDiskSearchExtractor.cpp` （封装底层的 `QDirIterator` 磁盘迭代器及文件名正则比对、批上限 50 条控速等具体物理细节）

**明确禁止越界修改的范围：**
- [ ] 明确禁止改动 `abortSearch` 对 `m_currentSearchId` 递增进行超时的判定。
- [ ] 明确禁止改动 `handleDeviceChange` 中关于硬件检测的底层 Win32 COM 回调框架。

## 6. 实现准则与预警【核心】
1. **防止多线程竞态（Race Condition）**：在流式回调时，由于物理扫描是在 `QtConcurrent::run` 后台线程池中开展的，必须保证回调的原子性，且在 `searchId` 超时或被中途中止时，物理 `PhysicalDiskSearchExtractor` 必须能第一时间感知 `abortFlag` 并安全返回。
2. **零编译开销传递**：通过 `std::function` lambda 回调，不引入任何多余的全局信号连接锁阻碍，最大化流式返回性能。
3. **消除头文件编译粘滞**：将 `#include <QDirIterator>` 彻底从 `CoreController.cpp` 移除。重构后，任何对底层扫描算法或攒批数量（如调大 50 至 100）的策略优化将仅编译 `PhysicalDiskSearchExtractor.cpp` 本身，保护了全局控制器不受低维变化牵连。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 `Modification_Plan-30.md` |
| 考古原则 | 重构代码必须基于现有实现保持高度的代码整齐度与风格一致性 | ✅ 符合，采用标准的 QDirIterator 物理接口和 `searchResultsAvailable` 批流式通道进行封装 |
| 异步解耦 | 大批量物理磁盘 I/O 扫描必须完全运行于后台线程池，防止假死 | ✅ 符合，通过异步 `QtConcurrent` Lambda 回调形式隔离在后台线程池 |

## 8. 待确认事项（可选）
- **流式攒批发散上限**：目前的 50 条上限是针对当前主线程 UI 渲染效率的最佳实践。若后续引入更高级的表格虚拟化更新，可在 `PhysicalDiskSearchExtractor` 内进行常量化动态调优。
