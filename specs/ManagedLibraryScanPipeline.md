# ArcMeta 实施方案：托管库增量扫描准入与特征提取流水线并发优化 (ManagedLibraryScanPipeline)

## 所属大纲章节
`1.1 全局数据与内存管理` (挂靠 1.1.10 / 登记于 0.4.2 索引表)

## 涉及代码文件
1. `src/meta/MetadataManager.h`
2. `src/meta/MetadataManager.cpp`
3. `src/meta/MediaExtractorPipeline.h`
4. `src/meta/MediaExtractorPipeline.cpp`
5. `src/ui/MainWindow.cpp`

---

## 功能描述
在使用“重新扫描托管库”功能时，解决现行架构因将库内全量资产（如 16,000+ 文件）强制重置特征完成状态为 0 (`ingestion_status = 0`) 并塞入低效提取队列（1.5 秒仅处理 16 条单线程串行计算与单条 SQL UPDATE）而导致耗时长达 30+ 分钟、系统严重卡顿的问题。

本方案通过建立**增量指纹准入机制**、**多核并行特征提取**以及**批量事务写盘**，将万级资产重新扫描的耗时从数十分钟大幅压缩至秒级/极短时间内，实现真正的增量比对与高效吞吐。

---

## 技术决策

### 1. 增量扫描准入比对机制 (MetadataManager::markAsRegistered)
在遍历托管库内所有资产文件时：
- 物理读取文件的 `mtime`（修改时间）与 `size`（文件大小）。
- 在内存分片（Shard）或数据库中比对现有的 `RuntimeMeta` 记录。
- **准入规则**：若该文件已记录且状态为完成 (`ingestionStatus == 1`)，并且 `mtime` 与 `size` 均未改变，则**跳过重置与投递**。仅对新增加的文件或修改时间/大小变动的文件执行 `updateIngestionStatus(p, 0)` 并投递至 `MediaExtractorPipeline`。

### 2. 流水线并发与多核 CPU 利用 (MediaExtractorPipeline)
- 废除单定时器 1500ms 慢速单轮询模式。
- 根据硬件核心数设置工作线程池（`QThreadPool` 或 `QtConcurrent::run` 控制最大并发数，如 `QThread::idealThreadCount()`）。
- 队列有任务时由线程池全速并发消费。

### 3. 批量事务写盘与落盘优化 (MetadataManager::updateExtractedMediaFeatures)
- 提取多媒体特征（分辨率、主色调、调色盘）完成后，采用批处理（Chunked Batch）写盘，或使用 SQLite 显式事务 (`SqlTransaction`) 按 200~500 条汇总大事务提交，减少磁盘 I/O 开销与锁竞争。

---

## 已知问题 / 待办
- **状态修复工具**：对历史已被错误洗白为 `ingestion_status = 0` 但实际已有宽高色值数据的资产，可提供一次性数据库纠偏修复脚本/逻辑，恢复其 `ingestion_status = 1`。
- **取消机制响应**：确保多线程并发消费队列时，全局取消标量 (`m_isCanceled`) 能及时响应并安全退出任务。
