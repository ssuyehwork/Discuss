# 中央神经调度中枢（Central Dispatcher）与底层洗髓重构无脑实施方案

## 一、 架构理念与设计目标

为了彻底解决全系统消息网状乱牵线、UI层直接越权修改底层数据库与内存缓存、缺乏统一并发锁与取消机制以及残留历史垃圾代码（.arc胶囊容器等）的问题，本方案采用**“双核中央中枢 + 顶层三条铁律 + 底层数据纯净化与分库锁”**的合体重构架构。

---

## 二、 架构核心设计

### 1. 双核中央神经调度中枢 (Core Dispatcher)

中枢由两个职责单一、彻底解耦的类组成：

```
                    ┌───────────────────────────┐
                    │  UI 视图层 (五栏布局/弹窗) │
                    └─────────────┬─────────────┘
                                  │ 1. 发送 Command (意图/命令)
                                  ▼
 ┌─────────────────────────────────────────────────────────────────┐
 │                   中央神经调度中枢 (Core Dispatcher)             │
 │                                                                 │
 │  ┌──────────────────────────────┐  ┌─────────────────────────┐  │
 │  │      1. 传声筒               │  │      2. 中央大脑        │  │
 │  │    (CentralEventHub)         │  │     (CoreEngine)        │  │
 │  │                              │  │                         │  │
 │  │  · 纯消息/事件广播分发       │  │  · 校验业务合法性       │  │
 │  │  · 不含任何数据与磁盘/DB逻辑 │  │  · 协调各个 Service 执行│  │
 │  │  · 保证线程安全与防抖削峰   │  │  · 管理 CancellationToken│  │
 │  └──────────────┬───────────────┘  └────────────┬────────────┘  │
 └─────────────────┼───────────────────────────────┼───────────────┘
                   │ 3. 广播 Event (状态改动)       │ 2b. 调用后端 Service 执行
                   ▼                               ▼
 ┌─────────────────────────────────┐  ┌────────────────────────────┐
 │  UI 订阅者 (增量更新/局部刷新)   │  │ 后端服务 (Metadata/Disk/DB)│
 └─────────────────────────────────┘  └────────────────────────────┘
```

#### (1) 传声筒：`CentralEventHub` (`src/core/CentralEventHub.h/.cpp`)
- **单一职责**：纯事件总线（Event Bus），负责“收信”和“广播”，**绝不包含任何业务数据处理、数据库读写或磁盘/内存缓存逻辑**。
- **线程安全**：提供基于 `QCoreApplication::postEvent` 或 Qt 信号槽的跨线程安全分发，支持对高频事件（如滚动、输入）的防抖与削峰。
- **核心事件定义**：
  - `E_VolumeStateChanged` (驱动器盘符挂载/卸载)
  - `E_PathNavigated` (目录导航切换)
  - `E_SelectionChanged` (选中项变更)
  - `E_MetadataUpdated` (元数据变更：星级/颜色/标签/备注/置顶等)
  - `E_ItemsDeleted` / `E_ItemsRenamed` (文件物理删除/重命名)
  - `E_FilterStateChanged` (条件筛选状态变更)

#### (2) 中央大脑：`CoreEngine` (`src/core/CoreEngine.h/.cpp`)
- **单一职责**：业务逻辑编排与指令决策中心。
- **职责范围**：
  - 接收 UI 发出的 Command（如 `CmdSetRating`, `CmdDeleteItems`, `CmdBatchRename`）。
  - 进行业务合法性校验与并发锁/取消令牌管理（`CancellationToken`）。
  - 调用对应的后端 Service（如 `MetadataManager`, `DiskIoService`, `DatabaseManager`）执行实际操作。
  - 操作成功后，向 `CentralEventHub` 提交对应的 `AppEvent` 进行广播。

---

### 2. 不可逾越的“三条铁律”

1. **铁律一（UI 禁调底层）**：`src/ui/` 下的所有 UI 视图、代理、窗口，**严格禁止直接调用** `MetadataManager`、`DatabaseManager`、`DiskIoService` 或任何底层 DAO！
2. **铁律二（UI 只能发 Command）**：UI 发生任何用户交互（如点击打星、添加标签、重命名、彻底删除），必须且只能封装为 Command 提交给 `CoreEngine`。
3. **铁律三（UI 只能等 Event 局部刷）**：UI 提交 Command 后不直接修改本地 Model 数据，必须等待 `CentralEventHub` 广播增量 Event，由 UI 订阅者进行精准局部刷新。严禁无脑调用 `notifyFullUIRebuild()` 强刷新全屏！

---

### 3. 底层洗髓与并发加固规范

1. **底层数据纯净化（根除 Dual-mode 历史残留毒瘤代码）**：
   - 由于当前 QuarkMeta 是从原双轨架构（`Dual-mode version`）中强行剥离与独立出来的纯磁盘直连模式应用，代码库中残留了大量属于原 ArcMeta 托管库模式的僵尸与补丁代码。
   - **必须彻底根除**：
     - `.arc` 胶囊容器解析与打包逻辑；
     - `Base36 ID` 资产路径编码/转换遗留代码；
     - 托管库复制、导入及 `isInsideManagedLibrary` 的硬编码/打补丁逻辑；
     - 内存索引与托管库镜像同步器（`DatabaseSynchronizer`）的僵尸代码。
2. **独立分库递归互斥锁**：
   - 在 `DatabaseManager` 中建立严格的 `QRecursiveMutex`，将读写操作与事务彻底锁定，杜绝 UI 线程与后台刷新线程在 `sqlite3*` 上的锁争抢。
3. **异步取消令牌机制 (`CancellationToken`)**：
   - 为后台耗时任务（如目录深层搜索、媒体特征提取流水线）引入可原子化标记的 `CancellationToken`，确保用户切换目录或取消搜索时，正在运行的 `QThreadPool` 线程能瞬间中断响应，杜绝线程雪崩与界面卡顿。

---

## 三、 详细实施步骤规范

### 步骤一：创建中枢核心代码 (`src/core/CentralEventHub` & `src/core/CoreEngine`)
- 在 `src/core/` 建立 `CentralEventHub.h/.cpp` 和 `CoreEngine.h/.cpp`。
- 定义统一事件结构体 `AppEvent` 与命令枚举/结构体 `AppCommand`。

### 步骤二：清理 `MainWindow.cpp` 中的网状连接
- 移除 `MainWindow.cpp` 中所有 UI 控件之间的直连 connect（如 `ContentPanel` -> `FilterPanel` 的直连）。
- 改为各控件在初始化时向 `CentralEventHub` 订阅感兴趣的 Event。

### 步骤三：替换 UI 控件中的底层越权调用
- 排查并替换 `ContentPanel.cpp`、`MetaPanel.cpp`、`CategoryPanel.cpp`、`FavoritePanel.cpp`、`TagManagerView.cpp` 中出现的 100 多处 `MetadataManager::instance().xxx()` 逻辑，统一改为向 `CoreEngine::instance().executeCommand(...)` 提交。

### 步骤四：加固 `DatabaseManager` 与异步任务取消
- 为 `DatabaseManager` 加上全局/分库递归互斥锁保护。
- 在 `DiskScanService` 与 `MediaExtractorPipeline` 中植入 `CancellationToken` 机制。

---

## 四、 确认与授权

请审阅上述实施方案。在获得你的明确批准后，我将：
1. 将本设计理念更新写入 `Modification_Plan/QuarkMeta-Architecture-Planning.md`（或新建的架构文档中）。
2. 在评审面板中完整预览展示。
