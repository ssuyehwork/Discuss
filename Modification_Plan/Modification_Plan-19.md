# MetadataManager 媒体特征物理抽取职责剥离 —— Modification_Plan-19.md

## 1. 任务背景
根据《ArchitectureComplianceAudit.md》架构合规性审计报告中的重要 FAIL 判定：在面对百万级（1,000,000+）仿真元数据记录的工业场景下，`MetadataManager` 的核心职责应该保持极度纯粹。然而，目前的 `MetadataManager` 依然是一个混合了“内存缓存快速检索”、“数据库 ACID 写入”与“物理图像宽高分析（`tryExtractDimensions`）”、“主色调聚类与代表色解析（`tryExtractColor`）”、“多样本一致性校验”及“异常重试队列维护（`processVisualRetryQueue`）”的多重耦合体。这些物理图像/媒体的 I/O 解码、属性提取以及繁杂的 CPU 显著性色彩量化算法严重违反了单一职责原则，甚至增加了系统在并发 MFT 扫描时的 CPU 与 I/O 交叉抖动。

## 2. 问题定位
- **定位模块 1（尺寸分析职责耦合）**：
  在 `MetadataManager::tryExtractDimensions`（第 1727~1755 行）中，直接执行了 `QImageReader` 物理文件解析、甚至是利用 `QSvgRenderer` 读取 SVG 矢量头大小。这些图像文件的物理 I/O 读取不属于内存元数据管理器的核心范围。
- **定位模块 2（代表色聚类与多样本聚合判定强耦合）**：
  在 `MetadataManager::tryExtractColor`（第 1757~1858 行）中，包含了极其庞大的物理文件夹前 10 个图像样本的提取、Delta E 色差校验以及簇团决策（最强簇需占据 30% 以上权重等），逻辑重且属于计算密集型的物理多媒体分析层，而非纯粹的元数据键值映射（Metadata Mapping）。
- **定位模块 3（重试状态与防阻塞定时器泄露）**：
  在 `MetadataManager` 内部维护了 `m_visualRetryQueue` 队列和 `m_retryTimer` 计时器，每次定时器到期时通过 `QtConcurrent::run` 再次派发批量 5 个重型物理提取任务。此处的线程池管理、重试逻辑和重入限制使 `MetadataManager` 被迫分心做并发调度工作。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 物理抽取与内存元数据管理职责隔离 | 建立专门的 `MediaExtractorPipeline` 后台流水线，完全将 `tryExtractDimensions`、`tryExtractColor` 移出 `MetadataManager` | ✅ 一致 |
| 2    | 定时重试和队列维护下沉 | 将重试队列 `m_visualRetryQueue` 和重试定时器 `m_retryTimer` 剥离，完全交由专门的异步提取流水线管辖 | ✅ 一致 |
| 3    | 内存元数据管理器只做极速键值读写与 SQL 派发 | `MetadataManager` 仅提供 `setItemVisualMetadata`、`setItemDimensions` 等纯内存+底座 SQL 登记接口 | ✅ 一致 |

## 4. 详细解决方案

### 4.1 核心解耦：独立 `MediaExtractorPipeline` (媒体特征分析流水线)
在 `src/meta/` 下新建 `MediaExtractorPipeline.h` 与 `MediaExtractorPipeline.cpp`。
- **职责划分**：
  1. 纯无状态的图像物理读取、SVG 头解析与 `MediaColorExtractor`（感知色差算法）在后台独立运行。
  2. 统筹管理扫描/迁移完成后的物理路径，在独立子线程中依次拉起 `tryExtractDimensions` 和 `tryExtractColor`，完成后回刷内存，不阻塞主线程。
- **异常重试队列下沉**：
  将 `m_visualRetryQueue`（受影响路径队列）和 `m_retryTimer` 定时器全部收拢到 `MediaExtractorPipeline` 的有状态单例或独立后台服务中，从 `MetadataManager` 的核心代码中完全抹去这些与业务缓存无关的调度动作。

```cpp
namespace ArcMeta {
class MediaExtractorPipeline : public QObject {
    Q_OBJECT
public:
    static MediaExtractorPipeline& instance();

    // 向队列投递新的物理路径（由 USN 自动入库、或者用户主动点击“解析颜色”触发）
    void enqueue(const std::wstring& path);
    void enqueueBatch(const std::vector<std::wstring>& paths);

private slots:
    void processNextBatch(); // 1.5s/3.0s 防抖定时调度，取代原 MetadataManager 内部定时器

private:
    std::vector<std::wstring> m_queue;
    QTimer* m_timer;
    std::mutex m_queueMutex;
};
}
```

### 4.2 `MetadataManager` 的极速瘦身
1. **接口抹除**：
   - 彻底删除 `MetadataManager::tryExtractDimensions`、`MetadataManager::tryExtractColor` 以及 `MetadataManager::processVisualRetryQueue` 成员函数。
   - 彻底删除 `m_visualRetryQueue` 成员变量和 `m_retryTimer` 成员定时器。
2. **底座接口固化**：
   `MetadataManager` 仅充当存储底座，对外暴露纯粹的元数据状态赋值接口：
   ```cpp
   // 仅做内存和持久层 SQL 派发，不做任何物理文件 I/O
   void setItemDimensions(const std::wstring& path, int width, int height);
   void setItemVisualMetadata(const std::wstring& path, const std::wstring& color, const QVector<QPair<QColor, float>>& palette, bool isRetry);
   ```

### 4.3 物理多样本决策决策与 SVG 读取策略迁移
- 原有的 SVG 矢量图宽/高特殊防护（`QSvgRenderer`）、`QImageReader` 尺寸提取逻辑、以及针对文件夹下前 10 个图像样本的多样本 Delta E 聚类一致性校验规则，**全部迁移并完美封装进 `MediaExtractorPipeline` 内部**，确保物理算法一致、数据安全不破损。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/meta/MetadataManager.h` / `.cpp` （抹除物理分析、定时器、重试队列，保留纯 CRUD 与持久化底座）
- [ ] 新增模块/文件：
  - `src/meta/MediaExtractorPipeline.h` / `.cpp` （有状态重试与物理提取流水线）

**明确禁止越界修改的范围：**
- [ ] 严禁修改加密加解密物理组件及底层数据库 WAL 数据流。
- [ ] 严禁在 `MediaExtractorPipeline` 的提取链路中执行任何阻塞主线程 UI 的同步 I/O 操作。

## 6. 实现准则与预警【核心】
1. **死锁防御**：`MediaExtractorPipeline` 在提取完成后，调用 `MetadataManager::setItemVisualMetadata` 登记数据。由于该登记动作需要持有 `MetadataManager::m_mutex` 的写锁，而物理提取本身是不持锁的（纯无状态）。
   **核心红线**：禁止在物理提取、`extractPalette` 或 QImageReader 加载过程中持有任何 `MetadataManager` 的锁，做到“先提取（无锁），后登记（极速持锁写，<0.1ms）”。
2. **多线程并发安全**：由于异步提取是由后台线程池执行，必须保证 `MediaExtractorPipeline` 在程序退出（`shutdown` 触发）时，能够安全、优雅地打断正在进行的 `QImageReader` 动作并销毁定时器，防止悬挂 lambda。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案说明，绝不提交任何代码修改 | ✅ 符合，仅提供 Modification_Plan-19.md |
| 考古原则 | 新增流水线的结构与调用应当基于已有的 `SyncStatusService` 保持风格一致 | ✅ 符合，采用高内聚、定时节流防抖机制 |
| 输入框清除 | 一律使用 Qt 原生 `setClearButtonEnabled(true)` | ✅ 符合，不涉及清除按钮改动 |
