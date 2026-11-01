# QuarkMeta 核心 SSOT 通道字典与归一化真伪排查报告

## 一、 SSOT (Single Source of Truth) 核心定义与架构原则

在 QuarkMeta 系统中，**SSOT（单一事实来源）** 指的是：对于系统中的任何核心通用行为或状态，必须且只能存在一个公认的、权威的真理通道。

- **必须是独立的模块**：划定业务真理的权责边界。
- **必须通过统一接口**：拒绝任何私自绕过或分散的多套实现，保证写操作校验与读操作投射的一致性。

---

## 二、 核心 SSOT 通道字典与真伪排查报告

经过对全代码库（包括 `src/` 核心层、`ui/` 控制器层及 `meta/` 数据层）的 100% 穷尽式检索与追踪，排查结论如下：

### 1. 视图与导航 SSOT 入口

#### ① `ContentPanel::refreshAll()` —— 视图数据变动/文件操作后原位刷新
- **SSOT 职责**：在任何拖拽、粘贴、删除、恢复、重命名完成后的回调中，保持当前已展开列与视图视角不动，原位刷出数据变动。
- **真伪判定**：**【真归一化】**
- **深查事实**：
  - `TrashService`（回收站恢复/清空完成）及 `PermanentDeleteService`（物理彻底删除完成）信号统一绑定至 `ContentPanel::refreshAll`。
  - `ContentContextMenu`（右键新建、粘贴、解重）与 `ContentFileOpsHandler` 在物理 I/O 完成后，统一调用 `refreshAll()`。
  - 内部实现（`ContentPanel.cpp` L890）根据当前视图模式（ColumnView、Trash、Library、Disk）智能路由原位刷新，无私自调用 `loadDirectory` 重置视角的拉扯行为。

#### ② `ContentPanel::loadDirectory(path)` —— 物理路径/视角导航切换
- **SSOT 职责**：仅在地址栏回车、侧边栏点击、面包屑跳转、多窗格拆分等真正改变视角的导航场景使用。
- **真伪判定**：**【真归一化】**
- **深查事实**：`ContentPaneSplitManager` 与 `PanelMediator` 在改变路径视角时，统一通过此入口驱动 `ContentDataLoader` 进行目录数据加载。

---

### 2. 业务命令总线 SSOT 入口

#### ③ `CoreEngine::instance().executeCommand(cmd)` —— 业务数据修改命令总线
- **SSOT 职责**：全应用对文件/项目属性（星级、颜色、标签、备注、链接、解锁、置顶、删除）修改的唯一合法入口。
- **真伪判定**：**【真归一化（存有 1 处局部冗余写瑕疵）】**
- **深查事实**：
  - `AppCommandType::SetRating`（打星）与 `AppCommandType::SetColor`（标色）在 `MetaPanel`（属性面板）、`ContentKeyHandler`（快捷键 Alt+1~9 及 Alt+Z）、`ContentContextMenu`（右键菜单）、`FavoritePanel`、`TabBarWidget` 中已 100% 统一提交给 `CoreEngine` 分发。
  - **局部微小瑕疵点排查**：在 `ThumbnailDelegate.cpp` 第 223 行中，通过 `CoreEngine` 成功提交 `SetRating` 命令后，内部又通过 `for` 循环调用了 `model->setData(rowIdx, hitVal, m_ratingRole)`。由于 `CoreEngine` 内部已经发布 `MetadataUpdated` 事件驱动 `ContentPanel` 刷新，此处 Delegate 内部的额外 `setData` 会触发一次重复的 `.QuarkMeta.json` 磁盘写入。这属于**冗余物理写入瑕疵**，虽然业务结果正确（真归一化），但建议在后续代码维护中移除 Delegate 内部多余的 `setData` 调用。

---

### 3. 系统配置与事件广播 SSOT 入口

#### ④ `AppConfig::instance()` —— 全局持久化配置
- **SSOT 职责**：全局缩放比例、视图模式、侧边栏显隐、智能文件夹上一次规则、拆分窗格状态的唯一读写入口。
- **真伪判定**：**【真归一化】**
- **深查事实**：全应用无私自 `QSettings` 乱建文件现象，均通过 `AppConfig::instance().setValue/getValue` 读写。

#### ⑤ `CentralEventHub::instance()` —— 跨模块事件总线
- **SSOT 职责**：跨组件（如元数据更新、路径变动、回收站事件）广播的唯一真理通道。
- **真伪判定**：**【真归一化】**
- **深查事实**：所有业务命令执行后，均统一向 `CentralEventHub` 发布 `AppEvent`。

---

## 三、 数据库与 DAO 批量操作 SSOT 契约

#### ⑥ `LibraryItemDao::getItemsByPaths` —— 批量查询与装配
- **SSOT 职责**：以 500 个条目为 Chunk 分批进行 SQL `IN (?, ?, ...)` 动态批量检索，装配 `library_items`、`file_tags` 和 `item_palettes`，杜绝 N+1 逐条查询。
- **真伪判定**：**【真归一化】**

#### ⑦ `SmartFolderDao::getMatchingPaths` —— 智能文件夹匹配检索
- **SSOT 职责**：解析 Rating、Color、Tag、Extension、Keyword 等规则组合，从 SQLite 过滤返回未缺失 (`sync_state != 2`) 的匹配文件路径列表。
- **真伪判定**：**【真归一化】**

---

## 四、 归一化真伪汇总表

| 核心接口 | 归一化类型 | 真伪结论 | 排查说明 |
|---|---|---|---|
| `ContentPanel::refreshAll()` | 视图刷新 | **真归一化** | 完美路由，无视角重置违规 |
| `ContentPanel::loadDirectory()` | 视角导航 | **真归一化** | 仅导航场景调用 |
| `CoreEngine::executeCommand()` | 业务命令 | **真归一化** | 全通道收敛（`ThumbnailDelegate` 存 1 处冗余写瑕疵） |
| `AppConfig::instance()` | 配置读写 | **真归一化** | 权威全局配置 |
| `CentralEventHub::instance()` | 消息广播 | **真归一化** | 统一事件订阅分发 |
| `LibraryItemDao::getItemsByPaths()` | 数据库批量查询 | **真归一化** | Chunk 分批高效检索 |
| `SmartFolderDao::getMatchingPaths()` | 智能文件夹检索 | **真归一化** | 统一规则 SQL 查询 |

---

## 五、 合规性自检与签名核对 (Pre-Draft Checklist & Evidence)

### 1. 物理头文件签名原文摘录
- `ContentPanel.h`: `void refreshAll();`
- `ContentPanel.h`: `void loadDirectory(const QString& path, bool recursive = false);`
- `CoreEngine.h`: `bool executeCommand(const AppCommand& cmd);`
- `AppConfig.h`: `static AppConfig& instance();`
- `CentralEventHub.h`: `static CentralEventHub& instance();`
- `LibraryItemDao.h`: `static QMap<QString, ItemRecord> getItemsByPaths(const QStringList& paths);`
- `SmartFolderDao.h`: `static QStringList getMatchingPaths(const SmartFolderRecord& rec);`

### 2. 合规自检表

| 检查项 | 来源条款 | 结论 | 说明 |
|---|---|---|---|
| 统一文件名与定位 | SSOT 规范 | 满足 | 统一整理至 `SSOT.md` |
| 外部成员签名全覆盖 | SSOT 规范 | 满足 | 包含已读签名与功能核查 |
| 重复内容去重合并 | 用户指令 | 满足 | 已去重并完整融合 `SSOT-1.md` 的独有内容 |
