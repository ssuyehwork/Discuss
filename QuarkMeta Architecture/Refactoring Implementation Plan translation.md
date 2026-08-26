### 审查报告（基于 `MainWindow.h` 与 `MainWindow.cpp`）

---

#### 一、 职责违背标记（SRP 违规，规则 5）
1. **视图聚合与底层业务深度耦合**：在 `selectionChanged` 中，`MainWindow` 直接向下转型 `DiskItemModel` 并读取底层记录数组 `allRecords()` 提取属性。元数据解析与提取职责应由 `ContentPanel` 或独立 Presenter 封装，主窗口不应触碰底层模型数据结构。
2. **UI 逻辑越界承载计时与算法**：`m_elapsedTimer` 内部计算扫描百分比、倒计时与剩余时间算法，属于后台扫描/同步控制器的职责。
3. **气泡渲染与屏幕几何计算越界**：在 `ratingRequested` 与 `colorRequested` 中，主窗口直接拼接 HTML、通过 `QScreen` 计算全局居中坐标并驱动 `ToolTipOverlay`，属于展示层侵入。
4. **无边框边缘缩放逻辑冗余**：已挂载 `ResizeEventFilter`，但 `MainWindow` 内部仍自行实现了 `ResizeDirection`、`mousePressEvent`、`mouseMoveEvent` 等完整拖拽缩放逻辑，存在双重实现与职责重叠。

---

#### 二、 孤儿 / 僵尸 / 幽灵代码通报（规则 14）
1. **幽灵状态标志**：`m_isTagManagerMode` 在头文件中声明，在 `resetSplitterLayout` 和 `savePanelVisibility` 中被读取/重置，但全工程没有任何业务逻辑将其设为 `true`。
2. **孤儿定时器与进度条驱动**：`m_topProgressBar`、`m_elapsedTimer`、`m_syncStartTime`、`m_totalBatchCount` 存在初始化和定时器计算代码，但没有外部信号或方法对其进行启动、赋值与显示。
3. **重复安装事件过滤器**：`initToolbar()` 内创建按钮时通过 Lambda 安装了 `this` 作为过滤器，紧接着又对 `m_btnBack` / `m_btnForward` / `m_btnUp` 重复安装了 `m_hoverFilter`。
4. **未使用的协议常量**：`MainWindow.h` 中定义的 `kProtocolSystem = "system://"` 在实现文件中无任何引用。
5. **空事件槽**：`onDriveBarContextMenu(const QPoint& pos)` 为空桩函数。

---

#### 三、 架构与设计缺陷通报（规则 15）
1. **状态栏更新冲突**：`CoreController::statusTextChanged`、`onStatusBarStatsUpdated` 与 `updateStatusBar()` 三者同时直接向 `m_statusLeft` 写入不同格式的文本，存在竞态覆盖隐患。
2. **硬编码强依赖**：多处依赖硬编码索引（如 `m_mainSplitter` 的 0~4 索引与拉伸系数、ItemRecord 兄弟列索引 0/4/5/6），一旦面板增减或模型列结构变动将引发越界或展示错位。

---

### 下一步操作

请明确当前具体需要执行的调整目标（如：清理僵尸代码、重构状态栏流向、解耦元数据同步、消除无边框冗余逻辑等）。

若需对关联模块协同审查，请提供以下文件最新源码（规则 7 格式）：
`ContentPanel.h, ContentPanel.cpp, MetaPanel.h, MetaPanel.cpp, DiskItemModel.h, DiskItemModel.cpp, TaskProgressToolBar.h, TaskProgressToolBar.cpp`

// ===================|===================

### 审查报告（基于全量审查 `MainWindow`, `ContentPanel`, `MetaPanel`, `DiskItemModel`, `TaskProgressToolBar`）

---

#### 一、 职责违背标记（SRP 违规，规则 5）

1. **`DiskItemModel::setData` 越界执行底层 I/O 与文件系统迁移**：
   - 模型在 `Qt::EditRole` 中直接调用 `FileOperationHelper::safeRename` 执行物理更名，并就地重命名缩略图磁盘缓存与修改 `MetadataManager`。数据模型变成了“文件系统执行引擎”，破坏了 MVC 分层。
   - 模型在修改 `RatingRole`/`ColorRole`/`PinnedRole` 时，直接加载、解析、序列化并同步写入 `.QuarkMeta.json` 或 `global.db`，绕过了命令总线（`CoreEngine`）。
2. **`MetaPanel` 混杂业务修改与数据总线调度**：
   - `MetaPanel` 的 `setRating`/`setColor`/`setTags` 内部直接调用 `MetadataManager::instance().set*` 进行磁盘数据持久化，同时又向上抛出 `metadataChanged` 信号让 `MainWindow` 调用 `CoreEngine::executeCommand`，导致双重写入。
   - `MetaPanel` 自行承载了 `ShellHelper::openInExplorer`、`QDesktopServices::openUrl` 和系统剪贴板操作。
3. **`ContentPanel` 越界承载底层加密与抹除流水线**：
   - 右键菜单动作 `ActionEncrypt`、`ActionSecureDelete` 内部直接启动 `QtConcurrent::run` 与 `QThreadPool`，并在 UI 视图内手写递归删除与文件粉碎逻辑。
4. **`ContentPanel::eventFilter` 越界计算视图 Delegate 的 Hitbox**：
   - `ContentPanel` 在鼠标按下事件中，手动硬编码计算 `banRect`、5 个星级的坐标（如 `totalW = banW + banGap + 5 * starSize + 4 * starSpacing`）来处理星级点击，严重侵入了 `ThumbnailDelegate` 与 `TreeItemDelegate` 的渲染/交互职责。

---

#### 二、 孤儿 / 僵尸 / 幽灵代码通报（规则 14）

1. **越界隐藏不存在的列**：
   - `ContentPanel.cpp` 中的 `initListView()` 调用了 `header->setSectionHidden(7, true);`，但 `DiskItemModel::columnCount()` 返回值硬编码为 7（即仅有 0~6 列），索引 7 为越界无效调用。
2. **`m_isPendingEdit` 状态未复位**：
   - `ContentPanel::restoreSelections()` 中使用 `if (m_isPendingEdit) view->edit(lastProxyIdx);` 后，未重置 `m_isPendingEdit = false;`，导致后续偶发意外触发编辑态。
3. **模型层与视图层双重缓存冗余**：
   - `DiskItemModel` 维护了 `m_iconCache`（`QCache<QString, QIcon>`），而 `ShellIconManager` 内部同时维护了系统图标缓存；在提取图形缩略图时又存在独立的磁盘缓存机制，造成内存占用冗余与多头缓存失效问题。

---

#### 三、 架构、并发与死锁/冲突缺陷通报（规则 6 & 规则 15）

1. **元数据双写风暴与死循环链路（严重）**：
   - 链路：`QuickLookWindow` 触发评级 -> `MainWindow` 执行 `CoreEngine::executeCommand(SetRating)` 并调用 `m_metaPanel->setRating` -> `MetaPanel::setRating` 内部调用 `MetadataManager::setRating` 并发出 `metadataChanged` -> `MainWindow` 收到 `metadataChanged` 再次执行 `CoreEngine::executeCommand(SetRating)`。
   - 结果：一次操作触发两次全链路持久化广播，产生冗余磁盘 I/O 与事件中枢风暴。
2. **工作线程锁竞争与主线程假死隐患**：
   - `DiskItemModel::preloadDimensionsAsync` 在后台线程中锁住 `DiskMediaExtractor::s_jsonSaveMutex`，全量执行 `QuarkMetaJson::load()` 与 `save()`。如果用户此时在主线程快速切换目录或编辑元数据，主线程访问该互斥锁会被迫阻塞等待，造成界面卡顿。
3. **批量更新时的同步 I/O 级联阻塞**：
   - `MetaPanel::openTagSelectorOverlay` 在关闭时调用 `MetadataManager::instance().setTags(...)`，随后抛出 `tagsChanged`；`MainWindow` 监听到后对每个选中路径循环调用 `m_contentPanel->updateItemMetadata(p)`，导致在主线程上**同步连续执行 N 次磁盘 JSON 文件读取与解析**。

---

### 下一步目标与所需文件

为从根本上治理上述架构缺陷与双写问题，请明确本次首要解决目标：
1. **重构元数据修改与通知链路**（彻底消除 `MetaPanel`、`DiskItemModel`、`CoreEngine`、`MetadataManager` 的双写与循环调用）。
2. **重构 Model 的 `setData` 与重命名流**（剥离文件系统物理操作至专用 Service）。
3. **剥离 `ContentPanel` 内的冗余业务**（Hitbox 计算归还 Delegate，加密/抹除归还 Controller/Service）。

若需针对元数据持久化及命令调度进一步排查，请提供以下最新代码文件（规则 7 格式）：
`CoreEngine.h, CoreEngine.cpp, MetadataManager.h, MetadataManager.cpp, ItemRecord.h, TreeItemDelegate.h, TreeItemDelegate.cpp, ThumbnailDelegate.h, ThumbnailDelegate.cpp`

// ===================|===================

### 深度剖析与架构根因报告

---

### 一、 核心架构根因：元数据持久化与事件链路的“三头分流”

整个工程当前存在**三套并行的修改持久化入口**，彼此逻辑割裂，导致了重复写入、信号风暴与锁竞争假死：

```
                    ┌─────────────────────────┐
                    │      UI 交互层          │
                    └───────────┬─────────────┘
          ┌─────────────────────┼─────────────────────┐
          ▼                     ▼                     ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│  入口 1 (规范)   │  │  入口 2 (侵入)   │  │  入口 3 (倒置)   │
│   CoreEngine     │  │   MetaPanel      │  │  DiskItemModel   │
│  (统一命令中枢)  │  │(直调MetaManager) │  │ (setData直写磁盘)│
└─────────┬────────┘  └─────────┬────────┘  └─────────┬────────┘
          │                     │                     │
          ▼                     ▼                     │
┌────────────────────────────────────────┐            │
│           MetadataManager              │            │
│       (内存 Shard + QuarkMetaJson)     │            │
└──────────────────┬─────────────────────┘            │
                   │                                  │
                   ▼                                  ▼
         ┌───────────────────┐              ┌───────────────────┐
         │  .QuarkMeta.json  │              │  .QuarkMeta.json  │
         │   (双写/重复写)   │ ◄────────────┤   (绕过管理层)    │
         └───────────────────┘              └───────────────────┘
```

1. **入口 1（规范）**：`QuickLookWindow` -> `MainWindow` -> `CoreEngine::executeCommand` -> `MetadataManager` -> `CentralEventHub` 广播。
2. **入口 2（侵入）**：`MetaPanel`（如修改星级/颜色）直接调用 `MetadataManager` 写磁盘，随后又抛出 `metadataChanged` 让 `MainWindow` 再调一次 `CoreEngine`，造成**同一操作连续 2 次读写磁盘 + 2 次事件广播**。
3. **入口 3（倒置）**：`DiskItemModel::setData` 在主线程收到编辑事件后，直接实例化 `QuarkMetaJson` 读写磁盘，彻底绕过 `CoreEngine` 和 `MetadataManager`，导致内存 Shard 与磁盘数据脱节。

---

### 二、 职责违背标记（SRP 违规，规则 5）

1. **`DiskItemModel` 越界充当文件系统与元数据执行器**：
   - 在 `setData(Qt::EditRole)` 中，直接执行了物理更名（`safeRename`）、缩略图磁盘缓存物理重命名、`MetadataManager::renameItem`。
   - 在 `setData(RatingRole/ColorRole/PinnedRole)` 中，直接读取并写入 `.QuarkMeta.json` 与 `global.db`。
2. **`MetaPanel` 混杂展示与业务持久化**：
   - 界面控件内不仅负责展示与事件过滤，还直接调用 `MetadataManager` 静态单例持久化数据，同时抛出信号让外部控制器重复执行。
3. **`MetadataManager::renameBatchAsync` 侵入文件系统缓存迁移**：
   - 元数据管理器内部直接解析并执行 `QuarkMetaJson::migrateFolderCache` 与 `QuarkMetaJson::renameItem`。

---

### 三、 孤儿 / 僵尸 / 幽灵代码通报（规则 14）

1. **`ThumbnailDelegate::helpEvent` 空局部变量**：
   - 内部计算了 `Metrics m` 和 `statusRect`，但未进行任何逻辑判定，直接透传基类调用，属于无效占位代码。
2. **`ItemRecord` 与 `RuntimeMeta` 僵尸字段**：
   - `ItemRecord::isManaged`：无任何业务逻辑读取或赋值。
   - `ItemRecord::thumbStatus` 与 `RuntimeMeta::thumbStatus`：定义为“0:正常, 1:失败”，实际工程全量走 `DiskMediaExtractor` 实时与磁盘缓存体系，该字段已沦为幽灵字段。
3. **`MetadataManager::initFromDatabase` 虚假初始化**：
   - 函数内仅调用 `DatabaseManager::instance().init()` 并将 `m_loaded = true`，并未实际从数据库加载元数据，属于纯打桩代码。

---

### 四、 架构深度剖析与并发/假死隐患（规则 6 & 规则 15）

1. **批量标签修改引发主线程级联卡顿**：
   - `MetaPanel::openTagSelectorOverlay` 关闭时调用 `MetadataManager::setTags`，随后抛出 `tagsChanged`。
   - `MainWindow` 收到信号后，在主线程对选中的 N 个路径循环调用 `m_contentPanel->updateItemMetadata(p)`。
   - `updateItemMetadata` 内部每次均执行 `QuarkMetaJson jsonCache; jsonCache.load();`，当选中数十个文件时，**主线程发生数十次连续的同步文件 Open/Read/Parse**，导致 UI 掉帧卡死。
2. **后台提取尺寸与主线程元数据写锁竞争**：
   - `DiskItemModel::preloadDimensionsAsync` 在线程池中执行，内部对 `DiskMediaExtractor::s_jsonSaveMutex` 上锁并读写 `.QuarkMeta.json`；主线程在响应用户评级/设色时也需要操作该 JSON 文件，引发跨线程 I/O 互斥，导致主线程被迫等待文件锁。
3. **`TreeItemDelegate` 与 `ContentPanel` 的星级命中双重实现**：
   - `TreeItemDelegate::paint` 使用 `CardPainterHelper::drawRatingStars` 绘制星级；
   - `ContentPanel::eventFilter` 却使用一套硬编码公式（`banW + banGap + 5 * starSize + 4 * starSpacing`）手工检测点击，一旦样式或间距发生调整，点击热区与视觉渲染将立即错位。

---

### 五、 根治重构方案（彻底杜绝打补丁）

1. **统一收拢至 `CoreEngine`（单向数据流）**：
   - `MetaPanel`、`DiskItemModel`、`ContentPanel` 严禁直接调用 `MetadataManager::set*` 或自行读写 `.QuarkMeta.json`。
   - 所有元数据变更统一封装为 `AppCommand` 提交给 `CoreEngine`。
   - `CoreEngine` 统一调用 `MetadataManager` 进行内存 Shard 更新与异步/同步落盘，并由 `CentralEventHub` 发布广播。
   - `DiskItemModel` 与 `MetaPanel` 仅监听 `CentralEventHub` 更新内存镜像，彻底消除双写与主线程同步 I/O。
2. **`DiskItemModel::setData` 职责纯净化**：
   - `setData` 仅更新 `m_allRecords` 内存数据并发出 `dataChanged`；文件重命名由 `ShellHelper` 或 `CoreEngine` 处理，严禁在 Model 内部执行磁盘物理重命名。
3. **消除 Delegate 与 View 的 Hitbox 耦合**：
   - 将星级与无色标点击判定逻辑统一下沉至 `ThumbnailDelegate` / `TreeItemDelegate` 或 `CardPainterHelper` 的静态命中检测函数，禁止在 `ContentPanel` 内硬编码坐标运算。

---

### 下一步操作

若确认执行上述重构，请提供以下最新代码文件展开实施（规则 7 格式）：
`CardPainterHelper.h, CardPainterHelper.cpp, QuarkMetaJson.h, QuarkMetaJson.cpp, ShellHelper.h, ShellHelper.cpp`


// ===================|===================

### 架构根因与破损链路全量诊断

---

### 一、 破损链路与核心缺陷通报

#### 1. 重复重命名与元数据撕裂（`ShellHelper` vs `MetadataManager`）
* **链路追踪**：
  `ShellHelper::renameItem(oldPath, newPath)` 执行时：
  1. 调用 `QuarkMetaJson::migrateItemMetadata`（打开原目录 `.QuarkMeta.json` 擦除 `oldName`，写入 `newName` 并落盘）；
  2. 紧接着调用 `MetadataManager::instance().renameItem`；
  3. `MetadataManager::renameItem` 内部异步又调用了 `QuarkMetaJson::renameItem(folder, oldName, newName)`。
* **致命后果**：
  在第 3 步时，`.QuarkMeta.json` 中的 `oldName` 已经在第 1 步被重命名了，第 3 步再次查找 `oldName` 必然落空，导致异步任务出现假警告，同时产生**同一文件元数据 2 次重复 I/O 落盘**。

#### 2. O(N) 级磁盘 JSON 覆写风暴（批量修改时）
* **链路追踪**：
  用户选中同目录下的 50 个文件设置颜色/星级/备注：
  `CoreEngine::handleSetColor` -> 循环 50 次 -> 每次调用 `MetadataManager::setColor` -> 每次调用 `QuarkMetaJson::updateItemMeta`。
* **致命后果**：
  同一目录下的 `.QuarkMeta.json` 在主线程/工作线程被**连续打开、反序列化、修改、序列化、写入 .tmp、MoveFileEx 覆盖、SetFileAttributes 达 50 次**，造成极端 I/O 阻塞。批量操作必须是按**文件夹聚合（Folder Batching）**一次性落盘。

#### 3. 视觉与点击判定脱节（`CardPainterHelper` 与 `ContentPanel`）
* **缺陷事实**：
  `CardPainterHelper::drawRatingStars` 内部强行硬编码 `int unifiedSpacing = -4;` 并标记 `Q_UNUSED(starSpacing)`；
  而 `ContentPanel::eventFilter` 却在外面自行手算 `int totalW = banW + banGap + 5 * starSize + 4 * starSpacing`。
  一旦某处参数变动，界面渲染的星级与鼠标点击的星级立即发生物理错位。

---

### 二、 孤儿与废弃逻辑清理清单（规则 14）

1. **`CardPainterHelper::drawRatingStars` 中的废弃参数**：`starSpacing` 传入后被 `Q_UNUSED`，内部使用 `unifiedSpacing = -4`。
2. **`ThumbnailDelegate::helpEvent` 悬空计算**：计算了 `statusRect` 但无任何逻辑分支。
3. **`ItemRecord` 与 `RuntimeMeta` 中的 `thumbStatus`、`isManaged`**：全生命周期无任何有效逻辑读写。

---

### 三、 根本性重构实施方案

#### 阶段一：收拢元数据与文件重命名流（消除双重迁移）
- `ShellHelper::renameItem` 仅负责底层物理重命名与调用 `MetadataManager::renameItem`；
- 磁盘 `.QuarkMeta.json` 的更新统一由 `MetadataManager::renameItem` 单点负责，彻底移除 `ShellHelper` 对 `QuarkMetaJson::migrateItemMetadata` 的冗余调用。

#### 阶段二：聚合批量更新流水线（消除 N 次 JSON 覆写）
- 在 `MetadataManager` 与 `QuarkMetaJson` 中引入**批量原子写入机制**（按父文件夹的分组字典，同一目录仅执行 1 次 `load -> batch-modify -> save`）。
- `CoreEngine` 的 `handleSetRating` / `handleSetColor` / `handleSetTags` / `handleSetNote` / `handleSetURL` 改造为单次批量下发。

#### 阶段三：统一星级 Hitbox 检测到 `CardPainterHelper`
- 在 `CardPainterHelper` 增加 `hitTestRating(const QPoint& pos, ...)` 静态方法；
- `ContentPanel` 与 `TreeItemDelegate` 彻底移除自行手算的 Hitbox 坐标公式，全量调用统一计算入口。

---

### 下一步指令

请确认是否立即按上述方案展开代码重构。我将输出经过严格治理后的无冗余、单向数据流核心代码。

// ===================|===================

为支撑后续功能无缝扩展并彻底杜绝“打补丁与职责蔓延”，本次重构确立**四层单向流架构（Unidirectional Architecture）**。任何后续新增功能（如 AI 标注、自定义属性、云同步、多维过滤等）均严格遵循该标准管道，禁止跨层逆向调用。

---

### 一、 核心可扩展架构模型（分层治理）

```
┌────────────────────────────────────────────────────────┐
│ 1. 视图表现层 (Presentation Layer)                     │
│    - MainWindow / ContentPanel / MetaPanel / Delegates │
│    - 职责：仅负责事件捕获与纯粹渲染，不写磁盘、不调底层服务│
└───────────────────────────┬────────────────────────────┘
                            │ 发送 AppCommand (参数化数据包)
                            ▼
┌────────────────────────────────────────────────────────┐
│ 2. 命令与调度中心 (Command & Dispatch Layer)            │
│    - CoreEngine (单点命令中枢)                         │
│    - 职责：命令合法性校验、撤销重做(Undo)拦截、分发给底层服务│
└───────────────────────────┬────────────────────────────┘
                            │ 调度批量业务
                            ▼
┌────────────────────────────────────────────────────────┐
│ 3. 业务与持久化服务层 (Domain & Persistence Layer)      │
│    - MetadataManager / QuarkMetaJson / ShellHelper     │
│    - 职责：内存 Shard 状态维护、按目录聚合批量落盘(Batching) │
└───────────────────────────┬────────────────────────────┘
                            │ 驱动 CentralEventHub 广播
                            ▼
┌────────────────────────────────────────────────────────┐
│ 4. 事件总线层 (Event Bus Layer)                         │
│    - CentralEventHub (发布/订阅模式)                   │
│    - 职责：增量状态广播，各 UI 面板异步接收并局部刷新   │
└────────────────────────────────────────────────────────┘
```

---

### 二、 针对未来扩展的四大关键设计保障

#### 1. 命令总线解耦（支持无限制扩展新业务操作）
* **现状痛点**：新增一个属性（如 AI 摘要、多色标签），需要改动 MetaPanel、Model、View 等多个文件的读写代码。
* **重构设计**：
  * 所有业务操作统一抽象为 `AppCommand`。
  * 新增操作只需定义 `AppCommandType::XXX`并在 `CoreEngine` 挂载对应 Handler，UI 控件仅需一行 `CoreEngine::instance().executeCommand(cmd)`，UI 层与持久化层彻底解耦。

#### 2. 父目录聚合写入引擎（Folder-Batching Persistence）
* **现状痛点**：批量修改 N 个文件触发 N 次全量打开/写入 `.QuarkMeta.json`。
* **重构设计**：
  * 持久化层接收到路径列表后，按 `parentDirectory` 自动分组：
    $$\text{TargetPaths} \xrightarrow{\text{Group By}} \{ \text{Folder}_A: [\text{File}_1, \text{File}_2], \text{Folder}_B: [\text{File}_3] \}$$
  * 每个目录只执行 **1 次** `Open -> Batch Mutate -> Atomic Save`。
  * 后续即使一次性修改 10,000 个文件，I/O 复杂度从 $O(N)$ 骤降为 $O(\text{UniqueFolders})$。

#### 3. 渲染度量与交互判定单一信源（Single Source of Truth Metrics）
* **现状痛点**：Delegate 负责画，View 负责算 Hitbox，样式一变点击立即错位。
* **重构设计**：
  * `CardPainterHelper` 升级为度量与渲染中枢，输出统一结构 `RatingBarMetrics` 与 `hitTestRating(pos, metrics)`。
  * `TreeItemDelegate`、`ThumbnailDelegate`、`ContentPanel` 共享同一套几何算法，未来新增列表/自适应视图无需重复编写点击检测。

#### 4. 模型纯内存化与事件响应（Pure In-Memory Model）
* **现状痛点**：`DiskItemModel::setData` 充当文件系统执行器。
* **重构设计**：
  * `DiskItemModel` 彻底退化为标准只读/展示内存镜像；
  * 数据变更统一由 `CentralEventHub::MetadataUpdated` 信号驱动局部 `dataChanged` 刷新，杜绝 Model 内部侵入磁盘 I/O。

---

### 三、 实施文件清单与执行顺序

重构将按底层向顶层的顺序进行彻底治本：

1. **第一步（度量与工具层）**：重构 `CardPainterHelper`（提供统一 Hitbox 与绘制）。
2. **第二步（数据持久化层）**：重构 `QuarkMetaJson` 与 `MetadataManager`（实现按目录批量原子落盘，消除与 `ShellHelper` 的双重重命名）。
3. **第三步（调度总线层）**：重构 `CoreEngine`（统一批量命令分发，对接 `CentralEventHub`）。
4. **第四步（表现展示层）**：重构 `DiskItemModel`、`ContentPanel`、`MetaPanel`、`MainWindow`（移除直接写磁盘与手动计算 Hitbox 逻辑，全面对接单向流）。

确认后即可按步骤交付整套重构代码。

// ===================|===================

### QuarkMeta 核心架构重构实施蓝图

---

### 一、 架构契约与核心设计模式

```
[UI 交互触发]
      │ (1) 提交 AppCommand
      ▼
[CoreEngine] ──(2.1) 记录 Undo/Redo ──► [UndoManager]
      │ (2.2) 调度业务
      ▼
[MetadataManager] ──(3) 按父目录聚合 (Batching) ──► [QuarkMetaJson] / [global.db]
      │ (4) 持久化成功
      ▼
[CentralEventHub] ──(5) 广播 AppEvent (增量路径列表)
      │
      ├───────────────────────┬────────────────────────┐
      ▼                       ▼                        ▼
[DiskItemModel]         [MetaPanel]             [FilterPanel]
(更新内存/发出dataChanged)  (刷新属性显示)         (更新过滤统计)
```

#### 架构不变式（Invariants）
1. **单一写入口**：所有元数据变更（Rating、Color、Tags、Note、URL、Pinned、Encrypted）必须且只能通过 `CoreEngine::executeCommand` 提交。
2. **零磁盘 I/O 视图模型**：`DiskItemModel`、`MetaPanel`、`ContentPanel` 严禁直接调用 `QuarkMetaJson` 或 `MetadataManager::set*`。
3. **父目录聚合写入**：单次批量操作对同一目录的 `.QuarkMeta.json` 只执行一次打开、修改与原子落盘。
4. **统一几何与交互契约**：视图渲染与鼠标点击判定（Hitbox）全量收拢至 `CardPainterHelper`，杜绝任何手动坐标运算。

---

### 二、 分层实施步骤与详细改造清单

---

#### 阶段 1：几何度量与交互层重构 (`CardPainterHelper`)

* **目标**：统一星级/胶囊的渲染尺寸与鼠标点击检测，消除 View 与 Delegate 的重复实现。
* **重构内容**：
  1. 引入标准结构体 `RatingMetrics`：
     ```cpp
     struct RatingMetrics {
         QRect totalRect;       // 整个星级+清除按钮的外部包围盒（含彩色胶囊底色）
         QRect banRect;         // ⊘ 清除按钮热区
         int starsStartX;       // 第一颗星起始 X
         int starSize;          // 单星尺寸
         int starSpacing;       // 间距（锁定为 -4）
         int ratingY;           // 星级 Y 轴起始
         int ratingH;           // 星级行高
     };
     ```
  2. 提供静态计算与命中判定：
     * `RatingMetrics CardPainterHelper::calculateRatingMetrics(const QRect& cellRect, int rating, const QString& colorStr, int zoomLevel);`
     * `int CardPainterHelper::hitTestRating(const QPoint& localPos, const RatingMetrics& metrics);` // 返回 -1 (未命中), 0 (清除), 1~5 (对应星级)
  3. 清理废弃参数：移除 `drawRatingStars` 中的无用参数，消除 `Q_UNUSED(starSpacing)`。

---

#### 阶段 2：数据访问与持久化聚合重构 (`QuarkMetaJson`, `MetadataManager`, `ShellHelper`)

* **目标**：实现按父目录批量写入机制；消除 `ShellHelper` 与 `MetadataManager` 之间的重命名双写冲突。
* **重构内容**：
  1. **`QuarkMetaJson` 改造**：
     * 新增 `batchUpdateFolderItems(const std::wstring& folderPath, const std::vector<std::pair<std::wstring, std::function<void(ItemMeta&)>>>& updates)`：
       * 单次加载该目录的 `.QuarkMeta.json`；
       * 依次执行所有受影响文件的 `updater`；
       * 校验是否有变更，单次原子落盘（`.tmp` + `MoveFileExW`）；
       * 赋予 Windows 隐藏文件属性。
  2. **`MetadataManager` 改造**：
     * 新增统一批量持久化接口：`void setItemMetadataBatch(const QStringList& paths, std::function<void(ItemMeta&, RuntimeMeta&)> updater, bool notify = true)`；
     * 内部实现：将 `paths` 按 `normalizePath(QFileInfo(p).absolutePath())` 分组，盘符根目录分流至 `DriveMetaDao`，常规文件分流至 `QuarkMetaJson::batchUpdateFolderItems`，并同步更新内存 256 分片 Shard；
     * 移除单项修改中重复读写 JSON 的冗余逻辑；
     * 废弃 `MetadataManager::initFromDatabase` 中的空逻辑。
  3. **`ShellHelper` 改造**：
     * `ShellHelper::renameItem` 仅调用系统 API 重命名文件，并调用 `MetadataManager::instance().renameItem`；
     * 彻底移除 `ShellHelper` 中对 `QuarkMetaJson::migrateItemMetadata` 的冗余调用。

---

#### 阶段 3：命令调度与事件中枢收拢 (`CoreEngine`, `CentralEventHub`)

* **目标**：规范所有 `AppCommandType` 命令的批量处理流，实现操作后单次事件广播。
* **重构内容**：
  1. **`CoreEngine` 改造**：
     * 所有 `handleSetRating`、`handleSetColor`、`handleSetTags`、`handleSetNote`、`handleSetURL`、`handleSetPinned` 统一调用 `MetadataManager::instance().setItemMetadataBatch`；
     * 操作完成后，组装单次 `AppEvent`（携带完整的受影响路径列表 `ev.paths = cmd.targetPaths`），由 `CentralEventHub` 发布；
     * `AddTag` / `RemoveTag`：先在 `TagRepository` 登记，再通过批量持久化写入，避免主线程逐项查询与写盘。

---

#### 阶段 4：展示层与数据模型彻底解耦 (`DiskItemModel`, `ContentPanel`, `MetaPanel`, `MainWindow`)

* **目标**：模型纯内存化，界面仅通过命令交互，消除主线程同步 I/O。
* **重构内容**：
  1. **`DiskItemModel` 改造**：
     * `setData(Qt::EditRole)` 仅负责更名成功后的内存字段同步（`record.path`, `record.filename`）并发出 `dataChanged`；
     * 彻底移除 `setData` 中直接调用 `safeRename`、缩略图重命名及直接操作 `QuarkMetaJson` / `DriveMetaDao` 的侵入代码；
     * 修正 `columnCount` 与列表表头的列索引对齐，消除索引 7 越界调用。
  2. **`ContentPanel` 改造**：
     * 鼠标点击命中判定：改用 `CardPainterHelper::hitTestRating`，彻底移除手动计算 `banW`、`starSize`、`starSpacing` 的硬编码代码；
     * 撤销 `ActionEncrypt`、`ActionSecureDelete` 内联的原始异步删除代码，抽离至 Controller / Service；
     * 修复 `m_isPendingEdit` 状态复位遗漏问题。
  3. **`MetaPanel` 改造**：
     * 彻底移除在 `setRating`、`setColor`、`setTags` 中直接调用 `MetadataManager::instance().set*` 的代码；
     * 控件值改变时仅发射信号（或直接提交 `AppCommand`），由外部/单向链路接收 `CentralEventHub` 广播刷新 UI；
     * 消除 `MainWindow` 对 `MetaPanel` 修改后重复提交 `CoreEngine` 导致的双写循环。
  4. **`MainWindow` 改造**：
     * 清理僵尸标志 `m_isTagManagerMode`；
     * 清理未使用的常量 `kProtocolSystem`、空函数 `onDriveBarContextMenu`；
     * 规范化状态栏文本流向，消除 `CoreController` 与 `ContentPanel` 的状态覆盖竞态。

---

### 三、 成果预期对比

| 维度 | 重构前 | 重构后 |
| :--- | :--- | :--- |
| **架构数据流** | 3 个独立入口混杂写磁盘，双向死循环 | 严格单向流动：`View -> CoreEngine -> MetadataManager -> EventHub -> Model/View` |
| **50 项批量修改 I/O** | 50 次文件打开、解析、保存、覆写 | 按目录聚合，同目录下仅 1 次原子读写 |
| **主线程卡顿** | 批量更新标签时主线程连续 50 次同步 I/O 假死 | 0 毫秒主线程阻塞，全链路批量内存更新与异步落盘 |
| **Hitbox 一致性** | View 与 Delegate 双重手算，改动易错位 | `CardPainterHelper` 单一信源判定，100% 视觉对齐 |
| **未来扩展性** | 每加一个新属性需改动 5~6 个模块的写盘代码 | 仅需在 `AppCommand` 与 `MetadataManager` 扩展一个字段闭环 |

---

### 下一步

确认实施蓝图后，请告知是否立即开始按此蓝图生成全套重构后的源码文件。