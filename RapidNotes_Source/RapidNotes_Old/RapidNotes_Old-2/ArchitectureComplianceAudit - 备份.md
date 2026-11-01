# ArchitectureComplianceAudit.md
# RapidNotes — 架构合规审计与全量重构规划

> **文档定位**：本文档是所有重构工作的唯一规范基准。
> 任何新增或修改的代码，必须先对照本文档核查合规性，不合规一律拒绝合入。
>
> **数据规模前提**：所有设计决策以**数百万条记录**为基准，不接受"数据量小可忽略"的宽松标准。
>
> **最后更新**：2026-07-19（v3，废除数据库图片二进制，配置专有托管文件夹）

---

## 第一章 现状诊断

### 1.1 代码库概况

| 维度 | 数据 |
|------|------|
| 技术栈 | C++20 / Qt 6.10 / MSVC 2022 / SQLite 3（源码直接编译） |
| 构建系统 | CMake 3.16+ |
| 主要模块 | `src/core`（核心）/ `src/models`（数据模型）/ `src/ui`（界面）/ `src/watchdog`（看门狗） |
| 核心文件规模 | `DatabaseManager.cpp` 3,581 行 / `QuickWindow.cpp` ~172 KB |
| **目标数据规模** | **数百万条记录（这是所有设计判断的基准，不得降标）** |

### 1.2 百万级数据规模的具体量化评估

#### 1.2.1 数据库文件体积与图片废除

由于彻底废除了数据库中的图片二进制（`data_blob` 字段从数据库表结构中完全移除），转为使用专有托管文件夹进行外部存储：
- 所有笔记（含原本的图片类笔记）在数据库中仅保留文本元数据。
- 即使在数百万条记录的规模下，纯文本及元数据的数据库体积能得到极大控制。

| 笔记条数 | 单条平均大小（估算） | 预估内存数据库体积 | 磁盘数据库体积（含索引/WAL） |
|----------|----------------------|--------------------|----------------------------|
| 100 万条 | ~500 字节 | ~500 MB | **约 700 MB ~ 1 GB** |
| 500 万条 | ~500 字节 | ~2.5 GB | **约 3.5 GB ~ 5 GB** |

**重大变更决策**：
- **废除二进制字段**：数据库表 `notes` 彻底废除 `data_blob` 字段。
- **专有托管文件夹**：配置并管理一个应用专属的外部托管文件夹（如工作区下的 `attachments/` 或用户配置的专用目录），仅在数据库中保存托管文件相对路径或文件名。

#### 1.2.2 SQLite Backup API 单次全库同步耗时评估（纯文本元数据版）

由于清除了图片大字段，数据库总体积显著缩小，100 万条记录仅需同步约 1 GB 数据。

| 数据库大小 | NVMe SSD（~3 GB/s） | SATA SSD（~500 MB/s） | HDD（~100 MB/s） |
|------------|---------------------|-----------------------|------------------|
| 1 GB | **约 0.3 秒** | **约 2 秒** | **约 10 秒** |
| 5 GB | **约 1.5 秒** | **约 10 秒** | **约 50 秒** |

**关键同步策略**：
- 1 GB 以内时，定时全量 Backup API 对系统影响微乎其微。
- 超过 2 GB 时，在内存 DB 自动启用增量同步（change_log 表 + 触发器），以规避磁盘写入瓶颈，保证百万级大数据量时的退出速度。

---

## 第二章 三条核心原则的落地解释

### 原则 1：只采用 SQLite 内存模式

```
启动时：disk.db --> Backup API (全量) --> :memory: (就绪)
运行期：所有 SQL 读写 --> :memory: (微秒级)
定时落盘：
  [DB < 2 GB] :memory: --> Backup API 全量 --> disk.db (异步，15秒间隔)
  [DB >= 2 GB] change_log --> apply_changes() --> disk.db (增量，15秒间隔)
退出时：最后一次同步 --> PRAGMA wal_checkpoint(FULL) --> 关闭连接
```

**铁律**：
- 严禁任何逐行 SELECT + INSERT/UPDATE 循环同步。
- 严禁在主线程执行 Backup API 同步（退出时除外，须有 10 秒超时保护）。
- 定时同步间隔：15 秒，专用后台工作线程执行。

### 原则 2：必须模块化、职责单一

- **Repository 类**：纯 SQL 映射，只负责存取，不含业务逻辑。
- **Service 类**：实现业务规则，调用 Repository，不直接持有数据库连接。
- **ViewModel 类**：负责 UI 数据源格式化与分页，持有 Service，不直连 SQL。
- **UI 类**：只负责交互呈现，不得直接调用 Repository，禁止引入 SQL。

### 原则 3：记载到 ArchitectureComplianceAudit.md

本文档为项目的唯一重构与合规指导文档。每次迭代后，更新执行进度并在此文档中补充实测性能。

---

## 第三章 目标架构设计

### 3.1 分层架构图

```
+-------------------------------------------------------------+
|                        UI 层 (src/ui/)                      |
|  窗口 / 对话框 / 自定义控件 / Delegate                        |
|  >> 只通过 ViewModel 或 Service 接口获取数据                  |
|  >> 禁止直接调用 Repository / DatabaseConnection             |
+----------------------------+--------------------------------+
                             | 信号槽 / 接口调用
+----------------------------v--------------------------------+
|                    ViewModel 层 (src/viewmodels/)            |
|  NoteViewModel / CategoryViewModel / TagViewModel            |
|  >> 持有 Service 引用，封装 Qt Model 所需数据                 |
|  >> 负责分页（每页 500 条，禁止全量加载）                      |
+----------------------------+--------------------------------+
                             | 调用
+----------------------------v--------------------------------+
|                      Service 层 (src/services/)              |
|  NoteService / CategoryService / TagService                  |
|  TodoService / AuthService / SearchService / StatsService    |
|  >> 实现业务规则，写操作后调用 requestImmediateSync()           |
+----------------------------+--------------------------------+
                             | 调用
+----------------------------v--------------------------------+
|                   Repository 层 (src/repositories/)          |
|  NoteRepository / CategoryRepository / TagRepository         |
|  TodoRepository / AuthRepository / StatsRepository           |
|  >> 封装所有 SQL 语句和 Prepared Statement                    |
+----------------------------+--------------------------------+
                             | 使用
+----------------------------v--------------------------------+
|              Infrastructure 层 (src/infrastructure/)         |
|  DatabaseConnection（内存DB + Backup引擎）                    |
|  AttachmentStorage（专有托管文件夹存储引擎）                    |
|  SchemaManager（建表/版本迁移）                               |
|  FtsIndex（全文检索索引引擎）                                  |
+-------------------------------------------------------------+
```

### 3.2 目录结构规划

与此前规划一致，新建 `infrastructure/`、`repositories/`、`services/`、`viewmodels/` 目录，将上帝类 `DatabaseManager` 彻底解耦。

---

## 第四章 专有托管文件夹详细设计

### 4.1 托管文件夹规则

1. **托管目录定位**：默认路径为应用目录下的 `attachments/`，或从配置文件/配置注册表中读取的可自定义托管路径。
2. **文件名哈希化**：所有导入的图片统一拷贝至托管目录下，文件命名格式为 `img_{sha256}.{ext}`，防止同名冲突，并支持文件级的去重（多条笔记引用相同哈希图片时，托管文件夹内仅保留一份物理文件）。
3. **数据库记录**：`notes` 表中引入 `attachment_path`（TEXT）字段，记录该图片在托管文件夹中的**相对路径**（例如 `img_abc123.png`），而非绝对路径，保证项目文件夹整体移动时链接不失效。
4. **生命周期回收（GC）**：
   - 笔记在被硬删除或清空回收站时，触发 `AttachmentStorage::deleteAttachment()`。
   - 删除前需进行引用计数检查（查询数据库中是否还有其他笔记使用该相对路径），引用计数为 0 时才真正执行物理删除。

### 4.2 AttachmentStorage 接口设计

```cpp
// src/infrastructure/AttachmentStorage.h

class AttachmentStorage {
public:
    static AttachmentStorage& instance();

    // 初始化，确保托管文件夹存在，并校验读写权限
    bool initialize(const QString& managedDirPath);

    // 导入外部文件至托管文件夹，返回相对路径。若已存在相同哈希文件则直接返回已有文件名
    QString importFile(const QString& sourceFilePath);

    // 获取托管附件的绝对物理路径，用于 UI 渲染加载
    QString getAbsoluteFilePath(const QString& relativePath) const;

    // 释放/删除附件（带引用计数检查）
    bool releaseFile(const QString& relativePath);

    // 获取当前的托管文件夹绝对路径
    QString managedPath() const;

private:
    QString m_managedDirPath;
    mutable QMutex m_mutex;
};
```

---

## 第五章 百万级专项性能设计

1. **全文检索 (FTS5)**：废弃 `LIKE '%keyword%'`，所有查询全部通过 FTS5 虚拟表匹配。
2. **侧边栏计数增量化**：创建 `system_stats` 计数表，使用内存 `std::atomic` 维护计数，首屏刷新零 SQL。
3. **Qt Model 分页**：限制单页 500 条，滑动到底部时增量加载，避免全量 reset 导致 UI 挂起。
4. **索引优化**：对所有频繁用于 `WHERE` 或 `ORDER BY` 的字段（如 `category_id`、`is_deleted`、`is_pinned`、`created_at`）建立精准索引。

---

## 第六章 执行路线图

### P0 阶段（立即执行，可并行）

| 编号 | 任务 | 百万级验收标准 | 状态 |
|------|------|--------------|------|
| P0-A | 建立 `DatabaseConnection`（内存模式核心 + Backup 引擎） | 启动时 Backup 耗时 < 3 秒（1 GB DB / SATA SSD） | ⬜ |
| P0-B | 建立 `AttachmentStorage`（专有托管文件夹存储引擎） | 导入及检索毫秒级响应，生命周期回收逻辑合规 | ⬜ |
| P0-C | 建立 `SchemaManager`（建表 + 所有必要索引，废除 `data_blob`） | 确认数据库表结构中不包含任何 blob 字段 | ⬜ |
| P0-D | 建立 `FtsIndex`（FTS5 虚拟表 + 中文分词） | 百万条记录关键词搜索耗时 < 100ms | ⬜ |
| P0-E | 建立 5 个 Repository 骨架（空接口，强类型结构体） | 新增文件，不破坏现有逻辑 | ⬜ |

### P1 阶段（P0 完成后，可并行）

| 编号 | 任务 | 百万级验收标准 | 状态 |
|------|------|--------------|------|
| P1-A | `NoteRepository` SQL 迁移（含 FTS5 搜索，废除 blob 读写） | 分页查询 500 条条目耗时 < 20ms | ⬜ |
| P1-B | `CategoryRepository` SQL 迁移 | 递归父级查询（`findRootOf`）耗时 < 5ms | ⬜ |
| P1-C | `TagRepository` SQL 迁移 | 全量标签列表加载（含计数）耗时 < 50ms | ⬜ |
| P1-D | `TodoRepository` SQL 迁移 | 按日期查询待办耗时 < 10ms | ⬜ |
| P1-E | `StatsRepository` + `StatsService` 增量计数机制 | 侧边栏计数刷新无 SQL 查询（纯内存读取） | ⬜ |

### P2 阶段（P1 完成后，可并行）

| 编号 | 任务 | 百万级验收标准 | 状态 |
|------|------|--------------|------|
| P2-A | 建立 Service 层（6 个 Service），迁移业务逻辑 | 无 SQL 直接调用在 Service 层出现 | ⬜ |
| P2-B | 修复 `TagManagerWindow` 直接 sqlite3 调用 | `TagManagerWindow` 中无 `sqlite3_*` 调用 | ⬜ |
| P2-C | 修复 `NoteModel` 直接调用 `DatabaseManager`，改用新托管图片路径加载 | `NoteModel` 中无 `DatabaseManager` 依赖，大图加载正常 | ⬜ |
| P2-D | 实现 ViewModel 分页（500条/页，增量加载） | 滚动 100 万条数据无 UI 冻结（< 16ms/帧） | ⬜ |
| P2-E | `DatabaseManager` 清理（移除已迁移内容） | `DatabaseManager.cpp` 行数 < 200 行（仅兼容外壳） | ⬜ |

### P3 阶段（P2 完成后）

| 编号 | 任务 | 百万级验收标准 | 状态 |
|------|------|--------------|------|
| P3-A | `DatabaseManager` 完全退役 | 工程中无 `DatabaseManager` 引用 | ⬜ |
| P3-B | 定时同步实测验收 | 1 GB DB 下 15 秒落盘耗时 < 3 秒（后台线程，不阻塞 UI） | ⬜ |
| P3-C | CMakeLists.txt 更新（加入所有新目录源文件） | `cmake --build` 零错误 | ⬜ |
| P3-D | 全量压测与托管文件夹清理测试 | 100 万条数据下冷启动时间 < 10 秒，被删笔记图片物理删除成功 | ⬜ |

---

*本文档由 Antigravity 于 2026-07-19 依据代码库现状生成，v3 版本确定废除数据库二进制，采用应用托管附件文件夹方案。*
