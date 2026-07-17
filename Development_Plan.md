# Development Plan —— 物理监控自动同步与右键菜单高级排序开发计划

## 1. 物理监控与侧边栏分类树自动同步需求
### 1.1 核心需求
在停用 USN Journal 后，当用户在内容面板中执行“迁移”将文件夹移动至 `ArcMeta.Library_[盘符]`，或者以其他物理方式在托管库/自定义监控文件夹内创建、拷贝、移入新子目录时，`NativeFolderWatcher` 能够实时检测并在 SQLite 数据库中构建对应的 1:1 分类树记录（即向 `categories` 表里创建分类记录），使新文件夹在无需重启主程序的情况下，立即在侧边栏中刷新呈现。

### 1.2 解决方案概述
在 `NativeFolderWatcher::handleNotification` 中检测到目录级变动（`info.isDir()`）时，通过 `QtConcurrent::run` 异步拉起级联入库同步引擎 `AutoImportManager::instance().handleRecursiveIngestion(fullPath)`。这不仅会自动注册所有子文件和路径，还会递归在 SQLite 的 `categories` 表中补全 1:1 分类结构，最后触发 `"__RELOAD_ALL__"` 信号驱动侧边栏重载，实现完美实时刷新。


## 2. 内容容器右键“排序”主菜单与子选项需求
### 2.1 核心需求
无论在“列表模式”还是“卡片网格模式”下，内容容器中都应该提供统一的交互入口来让用户随意切换数据的排序方式。
系统需要在内容容器右键菜单（空白处及选中项目）中，新增一个主选项为“排序”的二级子菜单。

### 2.2 “排序”二级子菜单定义
- **排序属性**（单选联动，带 check 勾选状态）：
  - **名称**
  - **创建日期**
  - **修改日期**
  - **扩展名**
  - **大小**
  - **尺寸**
  - **评分**
- **分隔线**
- **排序方向**（单选联动，带 check 勾选状态）：
  - **升序**
  - **降序**

### 2.3 解决方案概述
1. **状态维护**：
   在 `ContentPanel` 中新增两个成员变量来持久化记录当前的右键排序属性和方向，并在构造函数中从 `AppConfig` 读取恢复，退出时自动同步保存：
   - `m_rightClickSortType`：支持 `SortByName`, `SortByCreateDate`, `SortByModifyDate`, `SortByExtension`, `SortBySize`, `SortByDimension`, `SortByRating` 7 种枚举。
   - `m_rightClickSortOrder`：支持 `Qt::AscendingOrder` 和 `Qt::DescendingOrder`。
2. **菜单注入**：
   在 `ContentPanel::onCustomContextMenuRequested` 菜单显示逻辑中，空白处菜单与项目右键菜单都注入主选项“排序”，并通过 `QActionGroup` 在子菜单中实现属性和方向的联动勾选。
3. **底层重排重写**：
   重写 `FilterProxyModel::lessThan` 算法。在排除了第一权重（文件夹/分类置顶）和第二权重（置顶优先）后，第三级排序不再单纯依赖 Qt 默认的 `QSortFilterProxyModel::lessThan`，而是直接根据 `m_rightClickSortType` 对当前比较的两行 `ItemRecord` 的物理或元数据属性进行提取和严格对比：
   - **名称**：对比 `record.categoryName` / 物理文件名。
   - **创建日期**：对比物理时间戳 `record.ctime`（QDateTime）。
   - **修改日期**：对比物理时间戳 `record.mtime`（QDateTime）。
   - **扩展名**：对比提取出来的扩展名 `record.suffix`。
   - **大小**：对比 `record.size`。
   - **尺寸**：对比像素乘积（`record.width * record.height`）或宽高比。
   - **评分**：对比 `record.rating` 分数值。
4. **刷新联动**：
   当用户在右键菜单中切换排序维度或升降序后，程序将状态写入 AppConfig 缓存，并立刻显式调用 `m_proxyModel->invalidate()` 和 `m_proxyModel->sort(0, order)` 强制重新进行视觉重排，在网格或列表视图中无缝刷新展示。
