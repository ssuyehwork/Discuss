# Implementation Plan - NavPanelHistoryTabRefactor

Base Commit Hash: `943037f85524b65875859191f2d3c1dba8aae1c1`
Prerequisite Plans: 无

---

## Chapter 1: Requirements & Key Problem Answers

### 1. 现有标签页宿主的结构与 NavPanel 融入方案
- **现有标签页宿主结构**：
  工程中现有的侧边栏标签页宿主组件为 `SidebarContainerWidget`（位于 `src/ui/SidebarContainerWidget.h/cpp`），它使用了 `QTabBar` + `QStackedWidget` 结构。
  同时，`NavPanel`（位于 `src/ui/NavPanel.h/cpp`）本身是一个独立的 `QFrame` 容器，被 `MainWindow` 和 `PanelLayoutManager` 作为“目录导航”侧边栏面板管理。
- **NavPanel 放进选项卡一的最小改动方案**：
  为保持外部 `MainWindow`、`PanelLayoutManager` 与 `PanelMediator` 对 `NavPanel` 的生命周期与布局管控零侵入、零破坏，最小改动方案是将 `NavPanel` 内部升级为复用 `QTabBar` + `QStackedWidget` 的宿主结构：
  1. `NavPanel` 顶部的固定标题栏 `titleLabel` 替换为 `QTabBar`（支持两个 Tab 选项卡：「目录导航」与「历史」），图标与外观样式复用 `SidebarContainerWidget` 的 `QTabBar#SidebarTabBar` QSS 规范；
  2. `NavPanel` 内部使用 `QStackedWidget` 承载：
     - Page 0：现有 `NavPanel` 的“目录导航”文件树与最近访问视图（功能、交互、外观 100% 保持不变）；
     - Page 1：新建的 `HistoryPanel`（历史文件记录列表）。

---

### 2. 新表结构、去重与淘汰逻辑及与 DatabaseManager 关系
- **数据库表结构 (`file_history`)**：
  在 unified SQLite 数据库 `.QuarkMeta/QuarkMeta.store` 中新建 `file_history` 表：
  ```sql
  CREATE TABLE IF NOT EXISTS file_history (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      file_path TEXT NOT NULL UNIQUE COLLATE NOCASE,
      accessed_at DATETIME DEFAULT CURRENT_TIMESTAMP
  );
  CREATE INDEX IF NOT EXISTS idx_history_accessed_at ON file_history(accessed_at DESC);
  ```
- **去重与淘汰逻辑**：
  1. **UPSERT 去重更新**：
     当新记录文件路径 `file_path` 时，执行 SQLite UPSERT 语句：
     ```sql
     INSERT INTO file_history (file_path, accessed_at)
     VALUES (?, CURRENT_TIMESTAMP)
     ON CONFLICT(file_path) DO UPDATE SET accessed_at = CURRENT_TIMESTAMP;
     ```
     确保同一文件记录唯一且更新时间戳置顶。
  2. **上限 100 条淘汰**：
     插入/更新后随之执行清理 SQL，删除 100 条以外的最旧记录：
     ```sql
     DELETE FROM file_history
     WHERE id NOT IN (
         SELECT id FROM file_history ORDER BY accessed_at DESC LIMIT 100
     );
     ```
- **与 DatabaseManager 关系**：
  `HistoryDao::initTable()` 在 `DatabaseManager` 初始化后调用，直接在 `DatabaseManager::getGlobalDbPath()` 连接上创建表与索引，完全符合 `FavoriteDao` 与 `DatabaseManager` 的 SSOT 分层设计风格。

---

### 3. 记录入口清单与接入方式
所有上报统一经过 `HistoryService::instance().recordFileAccess(const QString& path)`（自动过滤文件夹与非法路径）：

1. **入口一：空格键预览 / 双击预览 / 回车激活**：
   - **统一位置**：`PanelMediator.cpp` 中 `wireContentPanel` 绑定的 `ContentPanel::fileActivated(const QString& path)` 信号处理 lambda 头部。
   - **不可预览文件记录评估（答复问题 B）**：`wireContentPanel` 原有逻辑在 `UiHelper::canPreviewFile(path)` 为假时会 `return`。现将 `HistoryService::instance().recordFileAccess(path)` 上报置于 `canPreviewFile` 判断之前（仅校验 `!QFileInfo(path).isDir()`），使得不可预览的文件在被双击或按回车试图打开/激活时，也能被成功记录入历史。
2. **入口二：用系统默认程序打开（答复问题 C）**：
   - `ContentContextMenu.cpp` 中的 `ActionOpenDefault` 选项（调用 `QDesktopServices::openUrl` 处）：接入 `HistoryService::instance().recordFileAccess(filePath)`；
   - `QuickLookWindow.cpp` 底部与快捷键“用默认程序打开”按钮处（调用 `QDesktopServices::openUrl` 处）：接入 `HistoryService::instance().recordFileAccess(m_currentPath)`；
   - `MetaPanel.cpp` 中的 URL / 关联文件打开处：接入 `HistoryService::instance().recordFileAccess(path)`。

---

### 4. 文件删除、移动、重命名事件监听与清理理由
- **选择理由**：`CentralEventHub` 是 QuarkMeta 中全域解耦的事件总线，当文件被物理删除（`AppEventType::ItemsDeleted`）或重命名/移动（`AppEventType::ItemsRenamed`）时，`CoreEngine` 与 `DiskIoService` 会及时向 `CentralEventHub` 广播该事件。
- **具体响应处理**：
  `HistoryService` 订阅 `CentralEventHub` 事件：
  1. `AppEventType::ItemsDeleted`：提取删除的 `paths`，调用 `HistoryDao::removePaths(paths)` 从 `file_history` 表中清除记录；
  2. `AppEventType::ItemsRenamed`：提取 `oldPaths` 与 `newPaths`，调用 `HistoryDao::updatePaths(oldPaths, newPaths)` 将旧路径更新为新路径（若新路径已存在则自动合并去重），并同步通知 UI 更新。

---

### 5. 历史页刷新时机与增量 DOM 更新机制
为避免重蹈 `FavoritePanel::loadFavorites` 全树销毁重建导致的闪烁与性能损耗，`HistoryPanel` 采用**增量 Item 动态调整机制**：

1. **首次装载 / 切 Tab 激活**：切换到「历史」选项卡时，从 `HistoryDao` 全量加载一次 100 条记录构建 `QListWidget` / `QTreeWidget` 项。
2. **动态新增/激活**：`HistoryService` 发射 `historyItemAdded(const QString& path)` 信号：
   - `HistoryPanel` 检查列表中是否已存在对应路径的 Item；
   - 若存在：直接 `takeItem` 并 `insertItem(0, item)` 移动到顶部，不销毁 Widget，不重新提取图标；
   - 若不存在：新建 Item 插入第 0 行，并异步提取缩略图/系统图标回填。若项总数 > 100，销毁最后一项。
3. **动态移除/清理**：`HistoryService` 发射 `historyItemRemoved(const QString& path)` 信号，仅精准移除列表中对应的 Item。

---

### 6. 开发者不确定点确认与解答
- **确认点**：已确认所有要求（仅记录文件、复用标签页宿主、复用 FavoritePanel 缩略图异步回填、定位信号与拖拽至收藏栏逻辑），无猜测或不确定点。

---

## Chapter 2: Modified Files List

### 新建文件
- `src/meta/HistoryDao.h`
- `src/meta/HistoryDao.cpp`
- `src/meta/HistoryService.h`
- `src/meta/HistoryService.cpp`
- `src/ui/HistoryPanel.h`
- `src/ui/HistoryPanel.cpp`

### 修改文件
- `CMakeLists.txt` (注册新建文件)
- `src/ui/NavPanel.h`
- `src/ui/NavPanel.cpp`
- `src/ui/PanelMediator.cpp`
- `src/ui/controllers/ContentContextMenu.cpp`
- `src/ui/QuickLookWindow.cpp`
- `src/ui/MetaPanel.cpp`

---

## Chapter 3: Step-by-Step Implementation Order

### Step 1: 建立 `HistoryDao` 与 `HistoryService` 数据持久层
1. 创建 `src/meta/HistoryDao.h` 与 `src/meta/HistoryDao.cpp`：
   - 实现 `initTable()`，创建 `file_history` 表与索引；
   - 实现 `addOrUpdateHistory(const QString& path)`（UPSERT + >100 自动淘汰）；
   - 实现 `getAllHistory()`、`removeHistory(const QString& path)`、`clearAllHistory()`、`updateHistoryPaths(const QStringList& oldPaths, const QStringList& newPaths)`。
2. 创建 `src/meta/HistoryService.h` 与 `src/meta/HistoryService.cpp`：
   - 暴露 `recordFileAccess(const QString& path)`，校验非空且 `!QFileInfo(path).isDir()`；
   - 监听 `CentralEventHub` 的 `ItemsDeleted` 与 `ItemsRenamed` 事件，自动清理/更新失效路径；
   - 发射 `historyItemAdded(path)`、`historyItemRemoved(path)`、`historyCleared()` 信号。
3. 在 `CMakeLists.txt` 中同步注册新文件。

### Step 2: 实现 `HistoryPanel` 界面与交互
1. 创建 `src/ui/HistoryPanel.h` 与 `src/ui/HistoryPanel.cpp`：
   - 列表控件：`QListWidget` / `QTreeWidget`；
   - 缩略图/图标提取：复用 `FavoritePanel` 的系统图标获取与 `MediaExtractorPipeline` / `ThumbnailPipelineService` 异步缩略图回填；
   - 单击定位：发射 `requestLocateFile(path)` 信号；
   - 拖拽至收藏栏：配置 MIME 类型 `text/uri-list` 或自定义格式，支持拖拽到收藏栏；
   - 右键菜单：提供「从历史中移除」和「清空历史」菜单项，分别触发 `HistoryService` 清除操作。

### Step 3: 重构 `NavPanel` 宿主为双选项卡结构
1. 修改 `src/ui/NavPanel.h` 与 `src/ui/NavPanel.cpp`：
   - 在 `initUi()` 中将顶部 `titleLabel` 替换为 `QTabBar`（项 0：「目录导航」，项 1：「历史」）；
   - 实例化 `m_historyPanel = new HistoryPanel(this)`，与现有导航控件一同加入 `QStackedWidget`；
   - 连接 `m_tabBar->currentChanged` 切换 `m_stackedWidget` 页面，并在首次切到历史页时装载数据；
   - 转发 `m_historyPanel` 的 `requestLocateFile` 信号。

### Step 4: 埋点接入文件访问上报
1. 修改 `src/ui/PanelMediator.cpp`：
   - 连接 `NavPanel::requestLocateFile`（包含 `HistoryPanel` 转发的定位请求）至文件定位与导航逻辑；
   - 在 `wireContentPanel` 中的 `ContentPanel::fileActivated` 响应处，加入 `HistoryService::instance().recordFileAccess(path)`。
2. 修改 `ContentContextMenu.cpp`、`QuickLookWindow.cpp`、`MetaPanel.cpp`：
   - 在调用 `QDesktopServices::openUrl` 用系统程序打开文件前，调用 `HistoryService::instance().recordFileAccess(filePath)`。

---

## Chapter 4: Verification & Testing Methods

1. **选项卡与 UI 样式验证**：
   - 打开应用，观察左侧导航面板顶部出现「目录导航」与「历史」两个选项卡，样式与收藏栏统一；点击切换流畅，导航树功能不受任何影响。
2. **记录与去重/淘汰验证**：
   - 连续预览/打开多个不同文件（包含普通文件与图片），切换至「历史」页，确认出现对应历史列表；
   - 重复打开某个已有文件，确认该文件移动至历史列表最顶部，不产生重复项；
   - 连续打开 105 个文件，确认历史列表中最多保留 100 条，最旧的 5 条自动被淘汰。
3. **文件删除与改名同步验证**：
   - 删除或重命名历史列表中的某个文件，确认历史列表中对应条目自动删除或更新路径。
4. **定位与拖拽验证**：
   - 单击历史列表项，确认主界面自动跳转至对应文件夹并选中该文件；
   - 拖拽历史列表项至“收藏栏”，确认成功添加为收藏项。
5. **持久化验证**：
   - 重启应用，切换至「历史」选项卡，确认历史记录完整恢复。
