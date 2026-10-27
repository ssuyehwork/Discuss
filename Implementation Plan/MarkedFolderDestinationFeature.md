# Marked Folders / Destination Tab Feature Implementation Plan

**Base Commit Hash**: `21871189bf5a34f6ffbe0829c2cefe9a654975b7`
**Prerequisite Plans**: None
**Supersedes**: None

---

## Chapter 1: Answers to Required Questions (核心技术方案与问题解答)

### 1. 标签页宿主结构、新增第三页与标签缩窄方案
- **宿主结构**：侧边栏控件 `SidebarContainerWidget`（位于 `src/ui/SidebarContainerWidget.h/cpp`）包含顶栏 `m_tabBar`（`QTabBar`）与堆叠控件 `m_stackedWidget`（`QStackedWidget`）。
- **新增第三页**：
  - 新建 `MarkedPanel` 控件（继承自 `QWidget`，内部包含 `QTreeWidget`），加入 `m_stackedWidget` 成为第 3 个 Page。
  - 在 `m_tabBar` 添加第三页：`m_tabBar->addTab(UiHelper::getIcon("Destination.svg", "#ff551c", 16), "标记")`。
- **标签缩窄**：修改 `resources/style.qss` 中的 `QTabBar#SidebarTabBar::tab` 样式：
  - 将原有 `padding: 4px 12px; font-size: 13px;` 调整为 `padding: 4px 6px; font-size: 11px;`，使三个标签页在侧边栏宽度为 220px~260px 时不拥挤折叠。

### 2. 标记表结构、50 个上限淘汰逻辑、状态位设计及 DatabaseManager 关系
- **数据库表结构**：在 `global.db` 中新建 `marked_folders` 表：
  ```sql
  CREATE TABLE IF NOT EXISTS marked_folders (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      folder_path TEXT UNIQUE NOT NULL,
      is_marked INTEGER NOT NULL DEFAULT 1,
      updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
  );
  ```
- **状态位设计**：`is_marked`（1 为标记处于激活状态；0 为未标记状态，保留历史路径）。
- **淘汰逻辑（上限 50 条）**：
  - 只能标记文件夹（严格校验 `QFileInfo::isDir()`）。
  - 当尝试添加新记录且表总记录数即将超过 50 条时：
    1. 优先检索并删除 `is_marked = 0` 的最旧记录：`DELETE FROM marked_folders WHERE id = (SELECT id FROM marked_folders WHERE is_marked = 0 ORDER BY updated_at ASC LIMIT 1)`；
    2. 若仍超过 50 条（所有 50 条记录均处于 `is_marked = 1` 激活状态），再删除最旧记录：`DELETE FROM marked_folders WHERE id = (SELECT id FROM marked_folders ORDER BY updated_at ASC LIMIT 1)`。
- **与 DatabaseManager 的关系**：`MarkedFolderDao` 通过 `DatabaseManager::instance().getGlobalDb()` 获取连接，并使用 `DatabaseManager::getGlobalMutex()` 保护事务线程安全。在应用启动初始化（`HistoryService` / `CoreController`）时调用 `MarkedFolderDao::initTable()`。

### 3. Alt+V 快捷键完整迁移时序与失败处理
1. **焦点与选中校验**：按下 `Alt+V`，检查 `AppShortcutController::isEditingFocus()` 为 false 且当前有选中的项目。
2. **目标获取与路径合法性过滤**：
   - 获取所有 `is_marked = 1` 且路径存在的文件夹；若无可用目标，弹出 Toast 提示“未设置处于标记状态的目的地文件夹”并终止。
   - 过滤合法性：排查目标是否为选中项目自己，或目标是否位于选中文件夹内部（如把文件夹移入自身子目录）。剔除非法目标并收集告警信息。
3. **同盘/跨盘判断**：
   - 使用 `QStorageInfo` 判断源与每个目标的盘符/根挂载点。
   - 若存在任一目标与源同盘 ➔ `hasSameVolumeTarget = true`（属于迁移模式）；
   - 若所有目标都与源不同盘 ➔ `hasSameVolumeTarget = false`（属于纯复制模式）。
4. **强确认弹窗**：
   - 弹出确认对话框，清晰展示：迁移项目数、目的地文件夹列表、操作模式（“同盘迁移”或“跨盘复制”）。若用户点击“取消”，立刻终止。
5. **创建进度条（TaskProgressService）**：
   - 确认后，**主线程入口第一时间**调用 `TaskProgressService::instance().createJob("发送至目的地", totalUnits)` 获得 `jobId`，强制弹出 `TaskProgressStrip`。
6. **静默文件系统监视器**：
   - 调用 `ContentChangeWatcher::beginSuppress()` 静默文件系统监视，避免搬运过程中界面抖动。
7. **复制执行与同名冲突**：
   - 逐个目标执行复制，复用现有的 `FileCollisionDialog` 和 `DiskIoService::executeAsync` 处理冲突。
   - 每完成一个工作单元驱动进度更新。
   - **失败处理**：若复制到任一目的地失败，停止后续原文件回收操作，原文件一律保留，记录已完成的副本配对（用于 Undo），并弹出失败告警 Toast。
8. **回收原文件**：
   - 若复制到全部目的地 **100% 成功** 且 `hasSameVolumeTarget == true`：
     - 调用 `DiskTrashService::moveToDiskTrash(sources)` 将原文件移至应用私有回收站。
9. **压入 UndoManager**：
   - 创建 `MarkedTransferUndoCommand` 压入 `UndoManager`，记录原文件回收的 `trashId` 和各目的地生成的副本路径。
10. **界面刷新与落盘**：
    - 调用 `CentralEventHub::publishEvent` 触发界面刷新与 Sidecar 异步落盘。
11. **恢复文件监视与关闭进度条**：
    - 调用 `ContentChangeWatcher::endSuppress()`；
    - 在唯一收口点调用 `TaskProgressService::instance().finishJob(jobId)` 结束进度条。

### 4. 同盘/跨卷判断的具体实现
- 使用 Qt 原生 `QStorageInfo` 类：
  ```cpp
  bool isSameVolume(const QString& pathA, const QString& pathB) {
      QStorageInfo infoA(QDir::cleanPath(pathA));
      QStorageInfo infoB(QDir::cleanPath(pathB));
      return (infoA.rootPath().compare(infoB.rootPath(), Qt::CaseInsensitive) == 0 &&
              infoA.device() == infoB.device());
  }
  ```

### 5. 撤销命令数据结构与私有回收站流程
- **命令类**：`MarkedTransferUndoCommand : public ActionCommand`
- **数据结构**：
  ```cpp
  struct MarkedTransferItem {
      QString originalPath;
      int trashId = -1;
      QStringList copiedPaths;
  };
  QVector<MarkedTransferItem> m_items;
  ```
- **撤销流程**：
  1. 调用 `TaskProgressService` 创建撤销进度条；
  2. 遍历 `copiedPaths` 副本，调用 `DiskTrashService::moveToDiskTrash` 移动到私有回收站进行删除；
  3. 若 `trashId != -1`，调用 `DiskTrashService::restoreFromDiskTrash(trashId, originalPath)` 还原原文件；
  4. 触发 `CentralEventHub` 刷新 UI，关闭进度条。

### 6. 同名冲突与 DiskIoService 衔接
- 在 `MarkedFolderService` 内进行高层编排，对每个目标目录利用 `DiskIoContext` 依次异步/同步执行复制并处理冲突，最后由 `MarkedFolderService` 统一调用 `DiskTrashService::moveToDiskTrash` 回收原文件，不修改底层 `DiskIoService` 核心结构。

### 7. 文件监视静默评估
- 使用 `ContentChangeWatcher::beginSuppress()` 和 `endSuppress()`，在整批迁移期间屏蔽文件系统变更信号，搬运完成后统一调用 `refreshAll()`，彻底避免中间过程界面闪烁。

### 8. 快捷键 Alt+C 确认与全工程冲突查证
- **全工程查证**：经查证，`Alt+C` 与 `Alt+V` 在全工程中完全未被占用。在编辑框焦点状态（`AppShortcutController::isEditingFocus()` 为 true）下，屏蔽快捷键触发。
- **Alt+C 确认建议**：`Alt+C` 仅修改标记状态标号及渲染 Badge 图标，属于低风险轻量操作，**建议无需弹窗确认**，仅通过轻量级 Toast 提示；而 `Alt+V` 涉及文件转移，**必须弹窗确认**。

### 9. 路径失效处置建议（回答问题 13）
- 建议遵循“路径已失效”语义，当被标记文件夹被删除、重命名或移动时，通过 `CentralEventHub` 监听 `ItemsDeleted` 与 `ItemsRenamed` 事件，自动将其在数据库中的状态位置为未标记 (`is_marked = 0`)，保留其历史路径记录，防止误发送。

---

## Chapter 2: Modified Files List (涉及文件清单)

1. `src/meta/MarkedFolderDao.h` (NEW)
2. `src/meta/MarkedFolderDao.cpp` (NEW)
3. `src/meta/MarkedFolderService.h` (NEW)
4. `src/meta/MarkedFolderService.cpp` (NEW)
5. `src/ui/MarkedPanel.h` (NEW)
6. `src/ui/MarkedPanel.cpp` (NEW)
7. `src/ui/SidebarContainerWidget.h`
8. `src/ui/SidebarContainerWidget.cpp`
9. `src/ui/FavoritePanel.cpp`
10. `src/ui/NavPanel.cpp`
11. `src/ui/controllers/ContentContextMenu.cpp`
12. `src/ui/controllers/ContentKeyHandler.cpp`
13. `src/ui/controllers/PanelMediator.cpp`
14. `resources/style.qss`
15. `CMakeLists.txt`

---

## Chapter 3: Implementation Steps & Verification (分步实施顺序与验收方法)

### Step 1: 创建 `MarkedFolderDao` 与 `MarkedFolderService`
- **内容**：实现 `marked_folders` 数据库表创建、增删改查、50 条上限淘汰逻辑（优先淘汰未标记记录，仍超额淘汰最旧记录）、状态位更新与单例事件通知。
- **验收方法**：单元测试/方法测试 `addMarked` 超过 50 条时能否按规则淘汰，且只允许添加文件夹路径。

### Step 2: 实现 `MarkedPanel` 与侧边栏第 3 选项卡扩展
- **内容**：
  - 创建 `MarkedPanel` 控件（展现标记列表、透明度区分标记状态、双击/单击导航定位、右键四大语义菜单）；
  - 修改 `SidebarContainerWidget` 添加“标记”Tab（图标 `Destination.svg`，颜色 `#ff551c`）；
  - 调整 `resources/style.qss` 中的 `QTabBar#SidebarTabBar::tab` 内边距样式。
- **验收方法**：检查侧边栏正确呈现三个 Tab 且样式美观不折叠，点击“标记”Tab 能够显示标记列表。

### Step 3: 右键菜单与 Badge 图标绘制
- **内容**：
  - 在 `FavoritePanel`、`NavPanel` 及 `ContentContextMenu` 的右键菜单中增加“标记为目的地”/“取消标记”操作；
  - 在 `FavoritePanel`（Delegate 绘制）和 `NavPanel` 中，为处于标记状态的文件夹绘制 `Destination.svg`（橙色 `#ff551c`）图标。
- **验收方法**：被标记的文件夹在收藏夹和目录导航右侧成功显示橙色 Destination 图标。

### Step 4: 实现 `Alt+C` 快捷键与 `Alt+V` 强确认/迁移编排
- **内容**：
  - 在 `ContentKeyHandler` 中处理 `Alt+C`（切换标记状态）和 `Alt+V`（弹出确认框、执行迁移）；
  - 实现同/跨盘判断、文件复制、私有回收站移入、进度条驱动及撤销命令压入；
  - 接入 `UndoManager`，支持 `Ctrl+Z` 撤销发送。
- **验收方法**：
  - 选中文件夹按 `Alt+C` 能快捷标记/取消标记；
  - 选中文件按 `Alt+V` 能弹出确认框，确认后驱动 TaskProgressStrip 进度条，同盘时复制完成后将原文件移入私有回收站；按 `Ctrl+Z` 能精确还原。

### Step 5: 自动清除失效路径状态
- **内容**：在 `MarkedFolderService` 中监听 `CentralEventHub` 的 `eventOccurred` 信号，当文件夹被删除或重命名后自动将其状态更新为未标记 (`is_marked = 0`)。
- **验收方法**：删除一个已标记文件夹后，标记列表中该项自动变为未标记状态。

---

## Chapter 4: Zero-Value-Alteration & Policy Table

- 既有“收藏夹”与“库”核心逻辑 100% 不受影响；
- 严禁重构无关代码；
- 快捷键在编辑焦点（QLineEdit/QTextEdit）下严格被屏蔽。
