# P0级标签持久层重构与扫描器高并发锁粒度细化 —— Modification_Plan-16.md

## 1. 任务背景
在百万级仿真元数据的高并发架构下，系统的职责单一化（Single Responsibility）与多盘符高内聚并行设计是保证整体性能与可用性的核心基石。本方案将聚焦解决本应用最致命的两个 P0 级严重架构设计缺陷（对应用户原话："请针对 P0 优先级中尚未处理的两项问题，创建 Modification_Plan-16.md"）：
1. **`TagManagerView` 硬编码 C 盘分库连接并深度耦合 UI 直接编写和执行原始 SQL 语句**，导致界面逻辑与持久层完全穿透。
2. **`AutoImportManager` 中使用单一全局递归大锁 `s_dbAccessMutex`**，导致不同磁盘盘符的高负荷扫描与对账对同步产生大粒度的物理互斥与排队假死，破坏了 SQLite WAL 模式的高并发优势。

## 2. 问题定位
- **定位模块 1（TagManagerView 职责严重越界）**：
  在 `src/ui/TagManagerView.cpp` 中存在多处原生硬编码 `DatabaseManager::instance().getMemoryDb(L"C")`（对应用户原话："TagManagerView.cpp 中多处硬编码 DatabaseManager::instance().getMemoryDb(L"C")"）以及多处直接使用原生 `sqlite3_prepare_v2`、`sqlite3_step`、`sqlite3_exec` 等系列 C API 进行事务控制、标签和标签组记录的原始增、删、改、查（对应用户原话："直接在 UI 类内部编写并执行 SQL 语句（INSERT/DELETE/事务控制）"），缺乏持久化封装。
- **定位模块 2（AutoImportManager 全局排队）**：
  在 `src/core/AutoImportManager.cpp` 中定义并持有了 `static std::recursive_mutex s_dbAccessMutex;`。它被无差别地置于 MFT 自动导入与 `handleRecursiveIngestion` DFS 级联分类递归建立的超长物理周期中（对应用户原话："static std::recursive_mutex s_dbAccessMutex 笼统保护所有盘符的对账、分类创建、数据库写入"），导致即使不同的物理盘符在进行独立的目录监视与扫描对账，也在这个静态锁上产生了不必要的互斥排队，完全扼杀了 WAL 模式多盘符并行的设计初衷（对应用户原话："导致不同盘符的独立扫描任务互相阻塞排队，废掉了 WAL 并发和多驱动器独立并行的设计初衷"）。

## 3. 强制对照表

| 编号 | 用户原话 / 我的理解 | 方案对应点 | 是否一致 |
|------|---------------------|------------|----------|
| 1    | 建立统一的 TagRepository 持久层，封装标签组、标签项的增删改查 | 在 `src/meta/` 目录下创建专门的 `TagRepository` 静态/单例类，完全接管和封装所有的标签数据库 CRUD | ✅ 一致 |
| 2    | 明确标签数据的权威存储位置，说明理由后再实现 | 明确归属于 `global.db` 全局库。理由：标签与标签组是用户跨盘符通用的全局逻辑属性，并非单个物理磁盘独占的数据（对应用户原话："标签存储位置决策（global.db）：批准。"） | ✅ 一致 |
| 3    | UI 只通过 TagRepository 暴露接口读写，不接触原生 sqlite3 API | 彻底擦除 `TagManagerView.cpp` 中的 `sqlite3_*` 调用，完全重构为对 `TagRepository` 的接口调用 | ✅ 一致 |
| 4    | 提供不丢失老用户数据的平滑迁移方案，废置 C 盘数据，并写入 `tag_migration_completed` 标记 | 采用“静默废置迁移机制”：检查全局库 `system_stats` 里的标志。如有必要，读取 C 盘数据并转移至全局库，之后将 C 盘标签表废置，并在 `system_stats` 中写入 `tag_migration_completed` 强标记 | ✅ 一致 |
| 5    | 按盘符拆分锁粒度，确保不同盘符对账扫描可以真正并行，并详细界定跨盘符共享资源 | 拆除全局大锁。建立按盘符隔离的物理元数据写入锁，同时定义唯一的全局库并发锁，并制定严格的“先全局，后盘符”两阶段锁顺序规范（对应用户原话："锁拆分方案：方向批准，但必须先解决以下两点，否则会引入新的死锁风险"） | ✅ 一致 |
| 6    | 保证动态锁映射表 `m_driveDbMutexMap` 结构本身的线程安全 | 引入独立的轻量级读写锁/自旋互斥锁（如 `std::mutex m_mapMutex`）来严格保护锁映射表的运行时动态插入与检索过程（对应用户原话："m_driveDbMutexMap 这个映射表本身在运行时可能需要动态插入新盘符对应的锁对象，这个插入操作本身需要额外的保护机制"） | ✅ 一致 |

## 4. 详细解决方案

### 4.1 统一标签持久层 `TagRepository` 设计
在 `src/meta/` 路径下新建持久层模块 `TagRepository.h` 与 `TagRepository.cpp`：
- **存储权威决策（对应用户原话："明确标签数据的权威存储位置"）**：统一存储于全局内存库与物理磁盘库中，不依赖、不绑定任何特定物理盘符。
- **接口封装（对应用户原话："建立统一的 TagRepository 持久层"）**：
  ```cpp
  // 伪代码及声明风格：不输出具体 C++ 可执行文件，仅用作技术骨架展示
  namespace ArcMeta {
  class TagRepository {
  public:
      struct TagGroup {
          int id;
          QString name;
          QString color;
          QStringList tags;
      };

      // 标签组核心 CRUD
      static QList<TagGroup> getAllGroups();
      static int createGroup(const QString& name, const QString& color = "#3498db");
      static bool renameGroup(int groupId, const QString& newName);
      static bool deleteGroup(int groupId);

      // 标签组子项（关系映射表）管理
      static bool addTagToGroup(const QString& tagName, int groupId);
      static bool removeTagFromGroup(const QString& tagName, int groupId = -1);
  };
  }
  ```

### 4.2 平滑静默废置数据迁移机制（对应用户原话："说明这次改动是否会影响现有用户数据的迁移"）
为了兼容升级，系统在 `TagRepository` 初始化（如 `getAllGroups` 首次触发）或数据库加载（`DatabaseManager::init` 成功后）时，执行一次优雅的静默迁移逻辑：
1. **优先强标记检查（对应用户原话："优先检查这个标记来判断是否需要迁移"）**：
   查询全局库的 `system_stats` 表中是否存在 `key = 'tag_migration_completed'` 且 `value = 1` 的记录。若存在，直接跳过迁移。
2. **执行条件及平滑复制（对应用户原话："平滑迁移到新存储位置的方案"）**：
   若无标记，且尝试在全局库的 `tag_groups` 表中未发现任何记录，则触发迁移流程：
   - 获取旧 C 盘数据库（即盘符对应的本地数据库）连接（如果存在 C 盘数据库文件）。
   - 通过 `SELECT` 读取 C 盘库中 `tag_groups` 与 `tag_group_items` 的数据。
   - 使用 `SqlTransaction` 开启全局库事务，将这些记录完整 `INSERT` 到全局数据库。
3. **安全废置政策（对应用户原话："明确采用'废置'而非'清除'C盘旧数据"）**：
   迁移成功后，**旧版 C 盘数据库中的标签物理数据不进行任何 DELETE 或 DROP TABLE 清理**，使其原样保留在磁盘上，为可能出现的突发迁移错误预留人工找回数据的余地。
4. **状态写入（对应用户原话："在 system_stats 中写入一个显式的 tag_migration_completed 标记"）**：
   迁移操作成功提交后，在全局数据库的 `system_stats` 表中显式写入 `('tag_migration_completed', 1)` 记录，并写入脏标记确保落盘。下次程序启动即可在 0.1 毫秒内由于命中强标记而直接跳过。

### 4.3 剥离 UI 直接写 SQL（对应用户原话："UI 只通过 TagRepository 暴露的接口读写"）
1. 彻底移除 `TagManagerView.cpp` 中包含的 `#include "sqlite3.h"` 头文件依赖。
2. 将 `addTagToGroup`、`removeTagFromGroup`、`renameGroup`、`deleteGroup`、`createNewGroup` 中的所有物理 SQL 组装、`sqlite3_prepare_v2` 调用、事务 `BEGIN/COMMIT` 逻辑全部废除。
3. 替换为对应的 `QtConcurrent::run` 后台线程中调用 `TagRepository` 的封装接口。
4. 在 `refresh()` 中，原本需要调用 `getMemoryDb(L"C")` 读取标签组的逻辑，直接替换为 `TagRepository::getAllGroups()`，使 UI 拥有极致纯粹的 MVC 视图渲染职责。

### 4.4 高并发锁粒度细化设计
拆除全局大锁 `s_dbAccessMutex`。

#### 1. 锁体系定义与映射表安全（对应用户原话："按盘符/按资源拆分锁粒度"）
- **`m_globalDbMutex` (全局库保护锁)**：由 `DatabaseManager` 管理，专门用于保护全局库中的公共资源写入与读取安全。
- **物理元数据写入隔离锁 (分盘符锁)**：不同物理盘符的数据由于互不相干，使用各自独立的互斥锁保护。
- **`m_driveDbMutexMap` 的线程安全保障（对应用户原话："映射表本身在运行时可能需要动态插入新盘符对应的锁对象"）**：
  为避免动态加载新驱动器时在映射表的操作上产生数据竞争，我们使用独立的自旋锁或轻量级互斥锁 `m_mapMutex` 专门对锁映射结构本身加锁：
  ```cpp
  // 伪代码示意：
  std::shared_ptr<std::mutex> getDriveMutex(const std::wstring& volSerial) {
      std::lock_guard<std::mutex> lock(m_mapMutex); // 强力保护映射表自身操作
      auto it = m_driveDbMutexMap.find(volSerial);
      if (it == m_driveDbMutexMap.end()) {
          // 动态插入新锁对象
          auto mtx = std::make_shared<std::mutex>();
          m_driveDbMutexMap[volSerial] = mtx;
          return mtx;
      }
      return it->second;
  }
  ```

#### 2. 死锁规避与锁获取顺序规范（对应用户原话："明确规定所有调用点获取这两类锁的固定顺序"）
当对账扫描或导入（例如在 `AutoImportManager::handleRecursiveIngestion` 递归 DFS 目录中）时，可能既需要建立 1:1 的全局分类目录目录树（要写全局库），又需要将文件注册到对应分盘符的分库中（要写盘符库），这就涉及获取两把锁的情况。
为了从理论和物理代码上彻底杜绝死锁，我们在全项目建立一套无条件强制遵守的获取锁规则：
- **“先全局锁 `m_globalDbMutex`，后盘符分库锁 `m_driveMutex`”** 的两阶段两级加锁序列。
- 任何业务流程或调用路径，一旦需要在同一个线程逻辑中同时获取全局保护资源与分盘符私有资源，必须严格按照此加锁方向进行：
  ```cpp
  void someBusinessLogic(const std::wstring& volSerial) {
      // 第一步：无条件先获取全局库锁
      std::lock_guard<std::mutex> globalLock(DatabaseManager::instance().getGlobalMutex());

      // 第二步：再获取特定盘符的分库隔离锁
      auto driveMutex = DatabaseManager::instance().getDriveMutex(volSerial);
      std::lock_guard<std::mutex> driveLock(*driveMutex);

      // 执行具体业务...
  }
  ```
- **现有及新增代码死锁隐患自检（对应用户原话："检查现有及新增代码中是否存在顺序不一致的调用路径"）**：
  - **`AutoImportManager`**：扫描和对账是独立后台线程，无主线程同步获取盘符锁再尝试获取全局锁的情况。
  - **`MetadataManager`**：对元数据的常规读取和写入是通过特定的 Volume 序列号单独寻址特定的物理数据库句柄，并不涉及任何对 `categories` 全局库（即 `global.db`）的并发占用。因此仅需持有特定的盘符锁，完全符合锁层级单向不交叉的设计。
  - 这种严格单向的排序获取锁协议可使系统锁图（Lock Graph）保持无环，从物理层面上杜绝死锁。

## 5. 修改边界声明【红线】

**本次方案涉及范围：**
- [ ] 模块/文件：
  - `src/ui/TagManagerView.h` / `.cpp`
  - `src/core/AutoImportManager.h` / `.cpp`
- [ ] 新增模块/文件：
  - `src/meta/TagRepository.h` / `.cpp`

**明确禁止越界修改的范围：**
- [ ] 严禁修改任何加密加解密物理组件及底层数据流。
- [ ] 严禁在除持久层 (Repository/Repo) 之外的任何 QWidget、Delegates 等 UI 中编写原生 SQL 或加载 `sqlite3` 的 API 函数。
- [ ] 严禁修改未授权的 `UiHelper`、`ContentPanel` 或 `CategoryPanel` 的业务判断逻辑。

## 6. 实现准则与预警【核心】
1. **命名空间与开箱即用（对应用户原话："开箱即用"）**：
   新增的 `TagRepository` 必须处于 `ArcMeta` 命名空间中。其接口使用的所有 Qt 数据结构（如 `QString`, `QStringList`, `QList` 等）必须在头文件中精准引入，杜绝产生任何缺失声明或无法正常编译的情况。
2. **迁移异常安全性**：
   数据迁移操作必须使用 `SqlTransaction` 守护，确保在迁移过程由于不可抗力或软件崩溃时，事务能够安全回滚，保证全局数据库的原子性（Atomicity）与数据不被部分破损污染。

## 7. Memories.md 合规检查

| 组件 / 模式 | Memories.md 规范要求 | 本方案是否符合 |
|-------------|----------------------|----------------|
| 纯分析师模式 | Jules 本 Turn 仅输出方案，绝不修改任何物理代码文件 | ✅ 符合，仅提供 Modification_Plan-16.md 与相关设计 |
| UI 实现规范 | 标签清除按钮一律使用 Qt 原生 `setClearButtonEnabled(true)` | ✅ 符合，本标签管理重构中不涉及不合规的清除自定义按钮 |
| 考古原则 | 样式、间距与命名风格必须对齐现有实现 | ✅ 符合，`TagRepository` 的类结构、事务使用和命名方式完全对齐 `CategoryRepo` |
