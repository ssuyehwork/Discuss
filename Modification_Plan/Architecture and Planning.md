# ArcMeta 架构与设计理念规范 (Architecture and Planning)

## 1. 全局数据引擎与内存模式架构规范

### 1.1 全局数据/内存管理面板 (Global Data & Memory Engine)

#### 1.1.1 500万+ 海量数据高性能架构规范

##### A. 内存模式下：
1. **废除读写写时复制 (COW) 全量拷贝机制**
   - **架构要求**：在 500 万+ 数据规模下（内存占用约 1.5GB - 2GB），严禁在单个元数据（如评分、标签、颜色、分类）更新时使用 `std::unordered_map` 的全量深拷贝（`make_shared<map>(*oldMap)`）。
   - **重构设计**：内存元数据镜像由单一整块哈希表升级为 **分片并发哈希容器（Sharded Concurrent Map）** 或 **按块划分的增量内存存储结构**。单节点更新仅对所在桶（Bucket）加粒度锁，时间复杂度严格限制为 $O(1)$，分配内存 $O(1)$，彻底根除 OOM 与垃圾回收（GC）引起的界面冻结。

2. **纯增量计数与状态引擎（彻底废除 O(N) 全量扫描遍历）**
   - **架构要求**：严禁在 `StatisticsService` 侧边栏计数、`getAllTags()` 标签统计或任何高频事件中使用 `for` 循环全量遍历 500 万项节点。
   - **重构设计**：`StatisticsService` 必须重构成**事件驱动型纯增量计数器（Incremental Counter Engine）**。当资产被添加、删除、打标签、归类或放入回收站时，通过原子变量（Atomic Counter）同步增减对应的分类与系统计数。系统启动（Bootstrapper）时仅进行一次并行/分块汇汇总，后续生命周期内零全量扫描。

3. **内存级倒排索引（Inverted Indexing Engine）**
   - **架构要求**：分类筛选与标签查询禁止使用内存线性扫描过滤。
   - **重构设计**：建立并实时维护内存倒排索引：
     - `TagIndex: tag_name -> std::vector<folderId>`
     - `CategoryIndex: category_id -> std::vector<folderId>`
   - 执行分类与标签筛选时，直接进行 $O(1)$ 的集合取交/并集操作，检索响应时间控制在 5ms 以内。

4. **零计算 UI 视图筛选代理（Zero-Calculation Row Filter）**
   - **架构要求**：`FilterProxyModel::filterAcceptsRow` 属于 UI 高频渲染热点函数，严禁在其中执行 CIELAB Delta E 色差浮点计算、调色盘循环遍历或高频字符串解析。
   - **重构设计**：耗时的色差判定与多维属性筛选全部移至资产导入或元数据修改时的**预烘焙阶段（Metadata Baking Stage）**，生成按位存储的特征码（Bitmask）或预判标识。`filterAcceptsRow` 仅执行纯布尔/位运算，确保 500 万规模下列表滚动保持 60 FPS 流畅度。

5. **SSOT 资产关系与单一职责控制（拒绝补丁式代码）**
   - **架构要求**：彻底解耦资产元数据与分类关系，禁止在 `MetadataManager` 与 `CategoryRepo` 间出现交叉同步与两套账问题。
   - **重构设计**：建立专职的资产关系引擎，明确单一事实源（SSOT）。资产与分类的绑定、迁移与解除必须通过统一的数据流管线进行，彻底消除补丁代码与僵尸逻辑。

##### B. 磁盘目录模式下：
> *（注：磁盘目录模式 下的 内存模式500万+数据量高性能全局架构重构 逻辑架构尚未进行专题探讨与定义，暂时留空。）*
