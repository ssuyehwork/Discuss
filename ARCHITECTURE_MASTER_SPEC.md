# QuarkMeta 系统架构终态基准白皮书 (ARCHITECTURE_MASTER_SPEC.md)

---

## 1. 工业级五层整洁架构图谱 (5-Layer Clean Architecture Map)

系统遵循严格的**单向依赖原则（Unidirectional Dependency）**，上层仅可单向调用下层，同层之间通过中介者或事件总线通信，严禁跨层反向依赖。

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 【第一层：外壳与平台中立层 (Native Shell & OS Layer)】                       │
│  • FramelessWindowHelper ──► 8方向拉伸 / 拖拽 / 双击最大化 / 跨平台置顶    │
│  • DeviceWatcher         ──► 独立截获 WM_DEVICECHANGE，对外广播标准 Qt 信号│
├─────────────────────────────────────────────────────────────────────────────┤
│ 【第二层：视图呈现层 (View / Presentation Layer)】                           │
│  • MainWindow            ──► 纯粹顶层装配容器 (< 150 行，0 业务状态)       │
│  • 5 大子面板与地址栏     ──► 状态严格私有化，仅作为观察者单向响应数据     │
│  • TaskProgressToolBar   ──► 纯观察者，自动监听任务进度平滑展开/隐藏       │
│  • QuickLookWindow       ──► 纯多态全屏预览器 (带代际熔断与鹰眼小地图)     │
│  • BatchRenameDialog     ──► 纯规则配置对话框 (0 磁盘 I/O，全权委托服务)   │
│  • TagManagerDialog      ──► 纯词库字典编辑器 (0 磁盘扫描)                 │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【第三层：协调与路由层 (Mediator / Router / Handlers Layer)】                │
│  • PanelLayoutManager    ──► 230px 黄金分栏、五栏显隐、动态最小宽度与存盘 │
│  • PanelMediator         ──► 纯信号路由器 (无 friend 特权，无 Model 穿透) │
│  • AppShortcutController ──► 基于 QShortcut(Qt::WindowShortcut) 局内绑定 │
│  • TrayController        ──► 标准 QSystemTrayIcon 托管，0 焦点争夺        │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【第四层：业务领域层 (Domain Service Layer) - 唯一真理源 (SSOT)】             │
│  • NavigationService     ──► 路径唯一持有者、双向历史栈、协议解析         │
│  • TrashService          ──► 可逆软删除、原路冲突还原、7 秒撤销快照        │
│  • PermanentDeleteService──► 不可逆高危确认、物理粉碎、清洗撤销栈         │
│  • ClipboardService      ──► 复制/剪切/canPaste 判定/截图直粘/文件传输     │
│  • ProtectionService     ──► 8字节魔数头、1ms 验密、物理还原与 RAII 预览   │
│  • TagLexiconService     ──► 纯 SQLite global.db 词库字典、前缀联想补全   │
│  • BatchRenameService    ──► 规则解析、同名冲突仲裁、两阶段 UUID 安全中转  │
│  • TaskProgressService   ──► 线程安全任务队列调度与多任务加权进度计算     │
│  • AppLifecycleManager   ──► 四阶段受控退出清场、SQLite WAL 合流与下线    │
├─────────────────────────────────────────────────────────────────────────────┤
│ 【第五层：数据与基础设施层 (Infrastructure & Engine Layer)】                 │
│  • ColorPaletteEngine    ──► 纯底层图像处理、Lab 空间、CIEDE2000 色差     │
│  • ThumbnailPipelineService──► 三级缓存 (内存LRU -> 磁盘Hash -> 后台无锁)  │
│  • FilterProxyModel      ──► 独立复合过滤与加权排序代理模型               │
│  • DatabaseManager       ──► SQLite WAL 模式 Checkpoint 事务安全          │
│  • DiskIoService         ──► 物理磁盘异步传输与安全移动引擎               │
│  • OperationSnapshotEngine──► 全局可逆操作原子快照与 Redo/Undo 闭环       │
│  • UndoManager           ──► 严格时序双向命令栈 (50 步容量限幅与路径清洗) │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 全局状态唯一真理源矩阵 (SSOT State Ownership Matrix)

全系统每一个业务状态必须且只能由**唯一的领域服务（Domain Service）**持有，严禁多头存储与多向同步：

| 业务状态名称 | 唯一真理源持有者 (SSOT) | 外部读取途径 (Getter / Signal) | 外部修改途径 (Method / Action) |
| :--- | :--- | :--- | :--- |
| **当前物理/协议路径** | `NavigationService` | `currentUrl()`, `currentUrlChanged` 信号 | `navigateTo(url)`, `goBack()`, `goForward()`, `goUp()` |
| **导航双向历史栈** | `NavigationService` | `canGoBack()`, `canGoForward()`, `navStateChanged` | 内部状态机自闭环维护 |
| **五栏分栏尺寸与显隐** | `PanelLayoutManager` | `isPanelVisible(id)`, `panelVisibilityChanged` | `setPanelVisible(id)`, `resetSplitterLayout()` |
| **窗口置顶状态** | `FramelessWindowHelper` | `isAlwaysOnTop(window)` | `setAlwaysOnTop(window, bool)` |
| **回收站实体与快照** | `TrashService` | `trashItemCountChanged` 信号 | `moveToTrash(paths)`, `restoreItems(ids)`, `restoreAll()` |
| **全局候选标签词库** | `TagLexiconService` | `querySuggestions()`, `getAllTagGroups()` | `addTag()`, `renameTag()`, `deleteTag()`, `moveTagToGroup()` |
| **多维筛选过滤条件** | `FilterStateModel` | `currentFilter()`, `filterStateChanged` | `setState()`, `setKeyword()`, `clearAllFilters()` |
| **后台并发任务队列** | `TaskProgressService` | `hasActiveJobs()`, `progressUpdated` | `createJob()`, `updateProgress()`, `finishJob()` |
| **全系统撤销时序栈** | `UndoManager` | `canUndo()`, `canRedo()`, `canUndoChanged` | `pushCommand()`, `undo()`, `redo()`, `removeCommandsForPath()` |
| **应用生命周期状态** | `AppLifecycleManager` | `isShuttingDown()`, `shutdownStarted` | `requestShutdown()` |

---

## 3. 核心领域服务接口契约全览 (19 Domain Services Catalog)

### 3.1 窗口与系统底座 (Layer 1 & 2)
1. **`FramelessWindowHelper`** (`src/ui/FramelessWindowHelper.h`)
   - 核心 API：`static void apply(QWidget* window, QWidget* titleBar = nullptr);`
   - 核心 API：`static void setAlwaysOnTop(QWidget* window, bool onTop);`
2. **`DeviceWatcher`** (`src/core/DeviceWatcher.h`)
   - 核心 API：`void startListening(); void stopListening();`
   - 输出信号：`driveMounted(QString driveLetter); driveUnmounted(QString driveLetter);`

### 3.2 布局与路由协调 (Layer 3)
3. **`PanelLayoutManager`** (`src/ui/PanelLayoutManager.h`)
   - 核心 API：`void initLayout(); void resetSplitterLayout(); void setPanelVisible(id, bool);`
   - 核心 API：`void populatePanelMenu(QMenu* menu); void showPanelContextMenu(QPoint pos);`
4. **`PanelMediator`** (`src/ui/PanelMediator.h`)
   - 构造契约：`explicit PanelMediator(NavPanel*, FavoritePanel*, ContentPanel*, MetaPanel*, FilterPanel*, AddressBar*, QObject*);`
   - 核心 API：`void setupConnections();`
5. **`AppShortcutController`** (`src/ui/AppShortcutController.h`)
   - 构造契约：`explicit AppShortcutController(QWidget* window, SearchController* search, QObject*);`
   - 信号绑定：`togglePinRequested();`
6. **`TrayController`** (`src/ui/TrayController.h`)
   - 核心契约：采用 `m_trayIcon->setContextMenu(m_trayMenu)` 官方托管，退出调用 `AppLifecycleManager::instance().requestShutdown()`。

### 3.3 业务领域服务 (Layer 4)
7. **`NavigationService`** (`src/core/NavigationService.h`)
   - 核心 API：`void navigateTo(rawUrl); void goBack(); void goForward(); void goUp(); void refresh();`
   - 输出信号：`currentUrlChanged(url, displayPath); navStateChanged(canBack, canForward, canUp);`
8. **`TrashService`** (`src/core/TrashService.h`)
   - 核心 API：`bool moveToTrash(paths, parent); bool restoreItems(ids, parent); bool restoreAll(parent);`
   - 核心 API：`bool restoreToDirectory(trashPaths, targetDir, parent); bool emptyTrash(parent);`
9. **`PermanentDeleteService`** (`src/core/PermanentDeleteService.h`)
   - 核心 API：`bool execute(paths, parent, isSecureShred = true); bool executeTrashItems(trashItems, parent);`
10. **`ClipboardService`** (`src/core/ClipboardService.h`)
    - 核心 API：`void copyItems(paths); void cutItems(paths); bool canPaste(targetDir); void executePaste(targetDir, parent);`
11. **`EncryptionManager`** (`src/crypto/EncryptionManager.h`)
    - 核心 API：`bool isEncryptedFile(wpath); bool verifyPassword(wpath, pwd);`
    - 核心 API：`bool encryptFile(src, dest, pwd); bool decryptFile(amenc, dest, pwd);`
    - 核心 API：`std::shared_ptr<DecryptedFileHandle> decryptToTemp(amenc, pwd);`
12. **`ProtectionService`** (`src/core/ProtectionService.h`)
    - 核心 API：`bool protectFiles(paths, parent); bool unprotectFiles(paths, parent); bool changePassword(paths, parent);`
    - 核心 API：`std::shared_ptr<DecryptedFileHandle> previewProtectedFile(amencPath, parent);`
13. **`TagLexiconService`** (`src/core/TagLexiconService.h`)
    - 核心 API：`QStringList querySuggestions(prefix, limit) const; QList<TagGroup> getAllTagGroups() const;`
    - 核心 API：`bool addTag(name, groupId, color); bool renameTag(oldName, newName); bool deleteTag(name);`
    - 核心 API：`bool createGroup(name, color); bool deleteGroup(id); bool moveTagToGroup(name, targetId);`
14. **`BatchRenameService`** (`src/core/BatchRenameService.h`)
    - 核心 API：`std::vector<std::wstring> computePreview(paths, rules);`
    - 核心 API：`void executeAsync(paths, newNames, mode, targetDir, parent, callback);`
15. **`TaskProgressService`** (`src/core/TaskProgressService.h`)
    - 核心 API：`int createJob(title, totalSteps); void updateProgress(jobId, curStep, totalSteps, detail); void finishJob(jobId);`
    - 输出信号：`jobStarted(id, title); progressUpdated(percent, title, detail, count); allJobsFinished();`
16. **`AppLifecycleManager`** (`src/core/AppLifecycleManager.h`)
    - 核心 API：`void initialize(QApplication* app); void requestShutdown(); static bool isShuttingDown();`

### 3.4 基础设施与数据模型 (Layer 5)
17. **`ColorPaletteEngine`** (`src/util/ColorPaletteEngine.h`)
    - 核心 API：`static bool isGraphicsFile(ext); static QColor extractDominantColor(filePath);`
    - 核心 API：`static QVector<QPair<QColor, float>> extractPalette(filePath, maxColors);`
    - 核心 API：`static QColor quantizeToStandardColor(color); static double calculateDeltaE(c1, c2);`
18. **`ThumbnailPipelineService`** (`src/util/ThumbnailPipelineService.h`)
    - 核心 API：`QPixmap getFromMemoryCache(path, size) const;`
    - 核心 API：`void loadBatchAsync(paths, size, onSingleLoaded); void cancelAll(); void incrementGeneration();`
19. **`FilterProxyModel`** (`src/ui/models/FilterProxyModel.h`)
    - 核心 API：`void updateFilter(); void setCachedDuplicatePaths(paths);`

---

## 4. 架构十大绝对红线与防御机制 (Ten Inviolable Architectural Red Lines)

1. **【零友元侵入（No Friend Classes）】**
   - 严禁任何类向外暴露 `friend class` 声明；组件间必须通过强类型接口、公有方法或标准信号槽解耦通信。
2. **【单一执行入口（Single Entrypoint）】**
   - 同一项业务能力（如移入回收站、复制粘贴、批量重命名、永久删除），全系统所有触发入口（快捷键、右键菜单、预览窗口、拖拽投放）**100% 汇聚调用同一个 Service 专属函数**。
3. **【实体与动词严格隔离（Entity vs Verb）】**
   - 严禁创建泛化的万能 `DeleteManager`；删文件必须走 `TrashService` / `PermanentDeleteService`，删标签必须走 `TagLexiconService`。
4. **【撤销重做对称闭环（Undo/Redo Symmetry）】**
   - 所有接入快照系统的可逆操作，必须同时提供 `undo()` 逆向回滚与 `redo()` 正向重做逻辑，严禁留空 `redo()`。
5. **【统一 7 秒交互反馈（7-Second Toast Rule）】**
   - 全系统轻量级操作反馈与撤销提示统一固定为 **7000ms（7 秒）**，气泡右侧“撤销”按钮显式绑定 `UndoManager::instance().undo()`。
6. **【代际即时熔断机制（Generation Lock）】**
   - 目录切换、快速滑动与窗口切图时，必须递增原子代际号，后台子线程循环必须在 1 毫秒内感知并抛弃旧任务，CPU 算力 100% 聚焦当前视口。
7. **【三级缓存与无锁提图（3-Tier Cache Pipeline）】**
   - 严格遵循 `内存 LRU -> 磁盘 Hash -> 后台无锁 QImage 解码`；后台子线程绝不触碰 `QPixmap` 与全局 GUI 锁，保障 60FPS 满帧滑动。
8. **【词库与文件私有属性彻底解耦（Lexicon vs Property）】**
   - SQLite `global.db` 仅为纯候选词典，严禁在词条增删改时对全盘 `.QuarkMeta.json` 发起遍历；文件打标解绑仅读写当前目录 JSON。
9. **【UI 主线程零阻塞（Non-Blocking Main Thread）】**
   - 所有涉及文件物理复制/移动、哈希计算、加解密、大目录遍历的重型 I/O，必须在工作线程池执行，UI 线程严禁同步阻塞。
10. **【受控优雅退出四阶段（4-Phase Graceful Exit）】**
    - 彻底废除 `QApplication::exit(0)`；退出必须顺序执行：`防重入阻断 -> 任务硬熔断 -> 状态与临时文件焚毁 -> SQLite WAL Checkpoint 合流下线`。

---

## 5. 物理文件废除与治理清单 (Purged Obsolete Files Matrix)

以下 8 个历史遗留的冗余、错位与碎片化文件已被**彻底物理删除并从 CMake 构建链中注销**：

| 废除文件路径 | 废除原因 | 归一化替代者 |
| :--- | :--- | :--- |
| `src/ui/ResizeEventFilter.h / .cpp` | 窗口边缘拉伸逻辑碎片化，与主窗口事件打架 | `src/ui/FramelessWindowHelper.h / .cpp` |
| `src/ui/TitleBarEventFilter.h / .cpp` | 标题栏拖拽与双击逻辑碎片化 | `src/ui/FramelessWindowHelper.h / .cpp` |
| `src/ui/DiskBatchRenameService.h / .cpp` | 物理磁盘重命名服务错误放置在 `src/ui/` 目录下 | `src/core/BatchRenameService.h / .cpp` |
| `src/ui/GlobalShortcutController.h / .cpp` | 手写 eventFilter 按键补丁，命名名不副实 | `src/ui/AppShortcutController.h / .cpp` |
| `src/ui/TaskProgressController.h / .cpp` | 侵入式持有主窗口私有指针控制进度条 | `src/core/TaskProgressService.h / .cpp` |
| `src/ui/MediaColorExtractor.h / .cpp` | 纯数学图像色彩算法错误放置在 `src/ui/` 目录 | `src/util/ColorPaletteEngine.h / .cpp` |
| `src/ui/ColorAlgorithmEngine.h / .cpp` | 色差与量化算法重复且分层倒挂 | `src/util/ColorPaletteEngine.h / .cpp` |

---

## 6. 终审结论

《QuarkMeta 终态系统架构白皮书》正式确立。本工程从此拥有了清晰的工业级架构基准与不可逾越的技术宪法。后续任何功能迭代与模块扩展，均须严格以此白皮书为最高准绳执行。

// ===================|===================

针对 **【全系统架构终态白皮书与契约总览归档 (Master Blueprint)】**，我们对其结构框架、涵盖的核心内容与治理红线进行深度梳理：

---

### 一、 白皮书核心定位与使命

《QuarkMeta 终态系统架构白皮书》（`ARCHITECTURE_MASTER_SPEC.md`）将作为本工程的**最高宪法级技术档案**。
它的核心使命是：
1. **固化 19 大领域中枢的职责边界与通信契约**，彻底杜绝任何未来的开发者或 AI 再次写出“上帝类”、“友元侵入”或“局部补丁”；
2. **冻结核心公有接口签名（Public API Freezing）**，为后续迭代提供绝对的向后兼容保障；
3. **记录 10 大架构绝对红线与物理清场清单**，让代码审查有法可依。

---

### 二、 白皮书包含的 5 大核心章节纲要

```
                       【QuarkMeta 架构白皮书 5 维结构】
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. 工业级五层架构图谱 (The 5-Layer Clean Architecture Map)                  │
│    • 外壳层 ──► 呈现层 ──► 协调层 ──► 领域层 ──► 基础设施层 的单向拓扑流向    │
├─────────────────────────────────────────────────────────────────────────────┤
│ 2. 全局状态唯一真理源矩阵 (SSOT State Ownership Matrix)                     │
│    • 19 大核心业务状态（路径、历史、选区、分栏、回收站、词库、任务等）唯一归属表│
├─────────────────────────────────────────────────────────────────────────────┤
│ 3. 19 大独立领域服务接口契约全览 (19 Domain Services Contract Catalog)       │
│    • 每个 Service 的标准头文件原型、输入参数、输出信号与单向依赖链            │
├─────────────────────────────────────────────────────────────────────────────┤
│ 4. 架构十大绝对红线与防御机制 (Ten Inviolable Architectural Red Lines)      │
│    • 零补丁、零友元、双轨删除、代际熔断、三级缓存、优雅退出等核心铁律        │
├─────────────────────────────────────────────────────────────────────────────┤
│ 5. 物理文件治理与废除清单 (Purged Obsolete Files Matrix)                     │
│    • 记录 8 个已被彻底物理删除的冗余类，防止死代码复活                      │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

### 三、 架构十大绝对红线（核心宪法预览）

白皮书将永久载入以下 **10 条不可逾越的工程铁律**：

1. **【零友元侵入（No Friend Classes）】**：严禁为了跨层调用而声明 `friend class`，组件间只准通过标准信号槽或服务接口通信；
2. **【单一执行入口（Single Entrypoint）】**：同一业务（如移入回收站、复制粘贴）无论在按键、菜单还是预览窗口触发，100% 路由至同一个 Service 函数；
3. **【实体与动词隔离（Domain vs Verb）】**：删文件走 `TrashService` / `PermanentDeleteService`，删标签走 `TagLexiconService`，严禁混用通用删除；
4. **【撤销重做对称闭环（Undo/Redo Symmetry）】**：凡是接入快照的操作，必须同时具备 `undo()` 逆向回滚与 `redo()` 正向重做能力；
5. **【统一 7 秒操作反馈（7-Second Toast Rule）】**：全系统轻量级撤销提示统一固定为 7000ms，显式绑定 `UndoManager::instance().undo()`；
6. **【代际即时熔断机制（Generation Lock）】**：目录切换与快速切图时，必须通过原子代际号秒级抛弃旧后台任务，杜绝无效 CPU 消耗；
7. **【三级缓存与无锁提图（3-Tier Cache Pipeline）】**：内存 LRU -> 磁盘 Hash -> 后台 `QImage` 无锁并发解码，保障 60FPS 丝滑滚动；
8. **【词库与文件属性解耦（Lexicon vs Property）】**：SQLite `global.db` 仅为纯词典库，严禁在词库变动时进行全盘扫描改写；
9. **【UI 主线程零阻塞（Non-Blocking Main Thread）】**：所有涉及文件读写、哈希计算、加解密的重型 I/O 必须在 Worker Thread 执行；
10. **【优雅退出四阶段（4-Phase Graceful Exit）】**：彻底废除 `QApplication::exit(0)`，退出时必须执行 WAL Checkpoint、焚毁临时文件与释放连接。

---

### 探讨结论：

将这 19 大模块的成果与十大铁律完整固化为一份标准白皮书，不仅标志着本次大规模系统重构的**圆满终点**，更是本工程未来保持长期高质量健康演进的**坚实基石**。

请问您对上述关于《QuarkMeta 终态系统架构白皮书》的章节规划与十大红线定义是否完全认可？如果认可，我们是否可以开始正式生成归档白皮书？