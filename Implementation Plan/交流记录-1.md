**结论（已审查 TabBarWidget.cpp/.h 的实际代码）：** 不是单一 bug，而是宽度逻辑架构有缺陷。

**已确认的问题**

1. **宽度被硬卡死。** `applyDiffUpdate` 里 `qBound(120, textW + 64, 300)` 给每个标签页设了 300 上限，不分单窗格还是多窗格。合并 3 个以上文件夹时，标题必然超过 300，被强制截断。这是你说的"恶意缩短"。
2. **压缩不分对象。** 空间不足时，所有标签页按同一个 `scale` 等比压缩，短标签页和合并标签页一起被压。
3. **省略号逻辑和实际布局对不上。** `updateElidedTitle` 用 `width() - 44` 算可用宽度，但图标、关闭按钮、边距和间距实际占 56px。省略后的文字比标题标签真实宽度多 12px，被直接裁掉，不显示"…"。你截图里的"Skip upload |"就是这种现象。
4. **宽度测量和显示用的不是同一个字体。** 宽度用按钮的 `font()` 测，显示用标题标签的字体（样式表可能改了字号或加粗）。两者不一致时，测出来的宽度就不够。这一点要看样式表才能确认。
5. **宽度只在 `applyDiffUpdate` 里计算。** `updateSplitTabTitle`、`updateTabsUiState` 只改标题文字、不重算宽度。合并或拆分后，如果标题经这两条路径更新，宽度就是旧值。这一点要看 MainWindow 的调用顺序才能确认。

**架构问题（职责不单一，属于打补丁式结构）**
- 魔法数字 44、64、36、300、120 分散在 `TabBarWidget` 和 `TabItemButton` 两个类里，互相耦合。
- 标签页自己的内容宽度由外层 `TabBarWidget` 估算，应该由 `TabItemButton` 自己负责。
- `800` 的"防御性兜底"是补丁，会掩盖真正的布局问题。

**修改方案（未改动任何代码，需你批准后才执行）**
1. `TabItemButton` 新增 `preferredWidth()`。它用标题标签自己的字体测文字，加上从布局读取的边距、间距、图标和关闭按钮宽度，不再写死数字。
2. 省略逻辑改用标题标签的真实可用宽度，修正 12px 误差，保证出现"…"。
3. `TabBarWidget` 只保留一个 `relayoutTabs()` 负责设置所有宽度。合并、拆分、改标题、恢复状态、窗口缩放都调用它。`updateTabsUiState` 只刷新颜色和图标，不碰宽度。
4. 删除 300 上限、120 下限和 800 兜底。空间足够时，每个标签页显示完整标题。
5. 空间不足时，先保证短标签页完整显示，只压缩超出平均份额的长标签页（合并后的标签页优先被压），最小宽度保留 80。压到最小仍放不下时，由现有的横向滚动处理。

（确认第 4、5 点）：** StyleLibrary.cpp，MainWindow.cpp

// ===================|===================

**已审查 MainWindow.cpp 和 StyleLibrary.h，结论：这两个文件确认不了第 4、5 点，但发现根因很可能在别处。**

**MainWindow.cpp 能确认的**
- 它只调用标签栏的 `restoreStateFromConfig()` 和 `saveStateToConfig()`，不管合并、拆分、标题更新。
- 合并和拆分的信号连接在 PanelMediator 里（`components.titleBar` 被传了进去），所以第 5 点（宽度没有重算）要看 PanelMediator 才能确认。
- `restoreStateFromConfig()` 在 `showEvent` 里调用，此时布局可能还没完成，`applyDiffUpdate` 可能拿到不准的宽度。后续的 `resizeEvent` 大概率会纠正，这条只是风险，不是确认的原因。

**StyleLibrary.h 能确认的**
- 里面只有颜色和尺寸常量，没有 TabItem、TabTitleLabel 的样式表，第 4 点（字体不一致）仍然无法确认。样式表在别的文件里。

**新发现：你截图里的问题可能不在 TabBarWidget 内部**
`TabBarWidget` 计算宽度时用的是 `m_scrollArea->viewport()->width()`。如果 TitleBarWidget 给标签栏限定了宽度，或者红框 1 那片空白其实是另一个控件占着，那么标签栏自己认为可用空间很小，才会压缩合并标签页。这样的话，只改 TabBarWidget 内部的宽度算法解决不了问题，必须先看标题栏的布局。

**还需要你提供的文件：** TitleBarWidget.cpp, TitleBarWidget.h, PanelMediator.cpp, StyleLibrary.cpp

如果项目里标签页样式放在 .qss 或资源文件里，把那个文件也给我。

// ===================|===================

**结论：** 你截图里的问题已经能从代码确认，原因是标签页宽度没有随标题更新。

**已确认的原因**
1. **标题栏没有限制标签栏。** TitleBarWidget 里标签栏是拉伸占满，右侧工具区宽度固定，所以红框 1 那片空白确实属于标签栏。上一条"标题栏限制宽度"的怀疑被排除，宽度算法可用空间是够的。
2. **合并后标题变了，宽度没跟着变。** 合并或切换标签页后，PanelMediator 里的 `layoutChanged` 会调用 `updateSplitTabTitle`。这个函数只改标题文字，不重算宽度，所以按钮保持合并前单窗格标题的宽度（截图里约 120px）。合并标题 "Skip upload | src" 放不下，被省略成 "Skip upload |"。这对应我之前说的第 5 点。
3. **窗格标题的拼接逻辑重复了三份。** `mergeTab`、`updateSplitTabTitle`、`updateCurrentTabTitle` 各写一遍，其中只有 `updateCurrentTabTitle` 的单窗格分支能识别库和智能文件夹。

**新发现：分类名称会显示错**
`cleanNameForTab` 只识别 `computer://` 和 `trash://`。窗格路径如果是 `library://5` 或 `smartfolder://3`，会被当成普通路径，标题里显示成 "5"、"3" 而不是分类名。你要求"合并后所有文件夹/分类名称可见"，这一点必须一起解决。分类名称不在 URL 里，要从分类数据里查，或者让窗格状态直接带上显示名。

**修改方案（未改动任何代码，需你批准后才执行）**
在上一条方案基础上追加：
- 新增一个统一的"生成标签页标题"函数，三个调用点全部改用它，并且能把 `library://`、`smartfolder://` 解析成分类名称。
- `updateSplitTabTitle` 改标题之后必须调用 `relayoutTabs()`，从根本上消除"标题变了宽度没变"。
- 其余沿用上一条：由 `TabItemButton::preferredWidth()` 自己报告所需宽度；删除 300 上限、120 下限、800 兜底；空间足够时完整显示，不足时优先压缩最长的合并标签页，仍然放不下才用"..."。

**还需要你提供的文件（用来确认字体差异和分类窗格的路径内容）：** ContentPaneSplitManager.cpp，以及包含 `#TabItem`、`TabTitleLabel` 样式的那个文件（可能是 .qss 文件，也可能是 UiHelper.cpp 里的样式字符串，你知道文件名的话直接给我）

// ===================|===================

**结论：部分属实。** 问题方向对，但有 3 处事实错误，方案本身还违背 SSOT。以下均已对照你贴的代码逐项核实。

## 一、属实

- **DiskItemModel 越界：** `setData()` 直接调用 `safeRename`、缩略图缓存文件改名和 `MetadataManager::renameItem`。评分、颜色、置顶、标签也直接写 `MetadataManager`、`DriveMetaDao`、`LastOperationManager`。`preloadDimensionsAsync` 在工作线程里直接写 `QuarkMetaJsonStore`。
- **FavoritePanel / LibraryPanel 越界：** 直接调用 `*Dao`。`loadFavorites` 自己用 `QtConcurrent` 解码缩略图。`onPathsDroppedToCategory` 里判断 nodeId 业务并调用 `PresetTagService`。
- **MetaPanel 越界：** 失焦时自己过滤非法字符、`QFile::exists` 查重名、拼新路径，还自己做 500ms 删标签聚合。
- **PanelMediator 补丁：** `m_panelMetaDebounceMap` 的 30ms 定时器加路径求交集。
- **`singleShot(0)` 补丁：**
  - LibraryPanel 4 处：`libraryChanged`、`itemChanged`、`markOrderDirty` 到 `flushOrder`、新建后进入编辑。
  - MetaPanel 4 处：`setRating`、`setColor`、`noteEdited`、`linkEdited`。

## 二、不属实或夸大

1. **"ContentPanel 越权决断文件操作"不成立。** `createNewItem`、`performBatchRename`、`onPathsDropped` 都是一行转发给 `m_fileOpsHandler`。统计由 `ContentStatsWorker` 执行，ContentPanel 只是持有并调用。它不是"职责过载之王"，真正的问题见第四节。
2. **"pathsCopy 被改写，上个文件的修改写到下个文件"不成立。** `pathsCopy` 是在 `singleShot` 之前按值拷贝的，选区切换改不到它。
3. **`m_visibleTimer(60ms)` 不能直接定性为补丁。** 它是在合并滚动事件。
4. **行号无法核实。** 我这边看到的文件没有行号，但不影响结论。

## 三、文档方案本身违背 SSOT

文档提议新建 `CommandDispatcher` 和 `SetMetadataCommand`。但代码里已经有 `CoreEngine::executeCommand(AppCommand)`，PanelMediator 和 FavoritePanel 都在用，另有 `ActionCommand` 基类和 `CentralEventHub`。再建一套就是两个命令入口。正确做法是把 UI 里漏网的直写收进现有 `CoreEngine` 命令通道。`SelectionService` 和 `FilterService` 在这批代码里不存在，是新建提议，需要你批准。

## 四、文档漏掉、但代码里确认存在的问题

- **过滤状态三份拷贝：** `ContentPanel::m_currentFilter`、`FilterProxyModel::currentFilter`、`ContentHeaderWidget::setFilterState` 各持一份，靠赋值同步。
- **顺序真理源方向反了：** `FavoritePanel::saveFavorites` 遍历 `QStandardItemModel` 写库，拖拽先改 UI 再落库。`LibraryPanel::saveLibraryOrder` 同理。UI 成了真理源。
- **视图在做数据源决策：** `ContentPanel::refreshAll` 按 `m_currentCategoryType` 分支，并直接调用 `LibraryService::getCategoryPaths`。PanelMediator 的标签页刷新和地址栏刷新里还各复制了一份 "library"/"smart_folder" 判断。
- **统计重算有三条触发路径：** 模型 `dataChanged` 触发 `m_statsDebounceTimer`，`updateItemMetadata` 触发同一个定时器，PanelMediator 的 `MetadataUpdated` 处理又直接调用 `recalculateAndEmitStats()`。`recalculateAndEmitStats` 还在主线程整份拷贝 `allRecords()`。这才是"防抖掩盖多路触发"的真补丁。
- **DiskItemModel 双真理源：** `data()` 在记录字段为空时回落到 `MetadataManager::getMeta()`，覆盖评分、颜色、标签、备注，而且在绘制热路径上。`updateRecordMetadataBatch` 里还有"评分为 0 再用 `record.path` 重查"的二次兜底。
- **MetaPanel 的真实竞态：**
  - `setSelectedPaths` 无条件把 `m_editingPathsSnapshot` 覆盖为新选区。
  - 编辑中 `m_isUserEditing` 为真，`setNote` 会被跳过。
  - 如果备注框还有焦点时选区变了，随后的失焦会把旧文件的备注文本提交到新选区路径上。
  - 正常鼠标点击是先失焦再换选区，所以这条路径我只确认了代码允许，没有确认实际触发。

我没有改动任何代码。如果要出整改方案，告诉我范围，是先做 DiskItemModel 越界，还是先做过滤状态与统计的多份拷贝。

// ===================|===================

收到，这条标准已生效。我按它重新过了一遍你贴的全部代码，以下都**不通过**，均未改动代码。

| # | 位置 | 浪费手法（代码里的事实） | 建议 |
|---|---|---|---|
| 1 | `ContentPanel::recalculateAndEmitStats` | 在主线程把整个 `allRecords()`（含 QString、色板）完整拷贝一份再丢给统计线程。触发源有 3 路：模型 `dataChanged`、`updateItemMetadata`、PanelMediator 的 `MetadataUpdated` 直调。改一个文件的星级，整个文件夹被拷贝并重新统计 | 统计改为增量：模型按变更行更新计数；全量统计只在加载完成时做一次。不拷贝，只传只读快照 |
| 2 | `DiskItemModel::data()` | 评分、颜色、标签、备注为空时，每次绘制都回落到 `MetadataManager::getMeta()`（含 QString 转 wstring 和 `RuntimeMeta` 拷贝）。`DecorationRole` 每次调用都做一次 `QDir::cleanPath` | 加载时一次性写入记录，绘制只读字段。缓存键在入模时规范化并存进记录 |
| 3 | `DiskItemModel::applyIncrementalChanges` / `updateRecordMetadataBatch` | 增量更新开头就对**全部记录**做 `cleanPath + toLower` 建临时映射，哪怕只变 1 个文件。查找未命中时，线性遍历整个 `m_pathToIndex`，每项再 `cleanPath` | 规范化键在 `setRecords` 时存入记录，维护唯一一份增量索引，未命中直接放弃 |
| 4 | `PanelMediator::updateMetaPanelFromPanel`（多选） | 每次选区变化或 30ms 去抖触发，主线程对**每个选中项**调用 `getMeta` 拷贝，同时又走一遍 `data()` 的回落（同一数据读两次）。全选几千个文件就是几千次拷贝 | 选区聚合（共有标签、字段是否一致）交给服务层，按选区增量求交集，或放到后台算完再投递 |
| 5 | `FavoritePanel::loadFavorites` | 每次收藏变化都整树清空重建。主线程对每个收藏项调 `QFileInfo::exists()`，指向离线网络盘或移动盘时会卡住。图片类收藏每次重建都重新投递缩略图提取 | 增量更新单个节点。存在性检查放后台，结果回投。缩略图只对新增项提取 |
| 6 | `FavoritePanel::refreshItemIcons` | 连在全局 `IconLoadNotifier` 上，每收到一次信号就遍历整棵树，对每个文件项重新取图标 | 按路径精确更新对应节点，不要全树遍历 |
| 7 | `LibraryPanel::loadLibrary` | 每次库变化整树重建。每个分类为了拿数量，调 `getCategoryPaths(id).size()` 把全部路径取出来再数。每次还调用 `initTable()` | 用单条 `GROUP BY` 统计出所有数量。`initTable` 只在启动时调一次。改名只更新单个节点 |
| 8 | `TabBarWidget::applyDiffUpdate` / `saveStateToConfig` | `resizeEvent` 每个像素都触发 `applyDiffUpdate`，它对**所有**标签重新取图标、重算字体宽度、`unpolish/polish`。每次切换、改标题都把全部标签序列化成 JSON 写入 QSettings（带互斥锁） | 宽度重排与内容刷新分离，缩放只重算宽度。状态保存做脏标记，合并为延迟单次写入 |
| 9 | `PanelMediator` QuickLook 上一张/下一张 | 遇到不可预览文件就循环 `getAdjacentFilePath(delta++)`，每次都从头线性扫描整个模型找当前行，复杂度 O(n×跳过数) | 一次定位当前行，再顺序向前或向后找第一个可预览项 |
| 10 | `CoreController::initializeCoreComponents` | 1 秒定时器**无条件**每秒往线程池投递一次 `flushPendingUpdates`，常驻永不停。该函数内部是否提前返回，我没看到实现，无法确认 | 有待落盘数据时才启动单次定时器，而不是空转轮询 |

其中 **#5 对"假死"风险最高**，因为它在主线程做磁盘存在性检查。**#1、#4、#2** 最耗 CPU，且每次评分或选区操作都会触发。

需要确认 #10 和 #3 的话，请提供：DiskMediaExtractor.cpp, MetadataManager.cpp

// ===================|===================

**结论：MetadataManager.cpp 和 DiskMediaExtractor.cpp 里有 9 处爆刷盘或耗 CPU 的实现，全部不通过。** 我没有改动任何代码。

上一轮的 #10（每秒无条件投递 `flushPendingUpdates`）我降级处理：函数内部有空队列提前返回，实际开销很小，只是轮询设计不优雅。真正的刷盘风险在下面第 2 项。

| # | 位置 | 浪费手法（代码里的事实） | 建议 |
|---|---|---|---|
| 1 | `seedMemoryCacheBatch` → `ensureActivated`，由 `DiskItemModel::setRecords` 在**主线程**调用 | 首次打开文件夹时，缓存里没有的每个文件，都要再调一次 `CreateFileW + GetFileInformationByHandle` 去取大小和时间。这些数据目录枚举时已经在 `ItemRecord` 里了。几千个文件就是几千次磁盘调用，网络盘或离线盘会直接卡住界面 | 用 `ItemRecord` 已有的 size/ctime/mtime 直接播种，不做系统调用。`ensureActivated` 只留给单个路径的设置操作 |
| 2 | `DiskItemModel::preloadDimensionsAsync`、`flushPendingUpdates`、`updateExtractedMediaFeaturesBatch` | 三处都是**逐文件**调用 `updateItemMeta`，而且 width/height 被**三个入口**重复写入（按尺寸、按失败标记、按特征批量）。首次打开 1568 张图就是 1568 次调用。是否每次都整份重写文件夹的 `.QuarkMeta.json`，取决于 Store 实现，我没看到 | 见文末文件。方向：Store 提供按文件夹的批量更新，加载一次、全部应用、标脏后延迟合并写一次，内容没变就不写。尺寸只保留一个写入入口 |
| 3 | `getDiskThumbCachePath` → `fetchPhysicalFileId` | **每次**计算缓存路径都先开一次文件句柄取文件 ID。一次缓存未命中的提取里至少 3 次（读缓存、等锁后再读缓存、写缓存），改名时 2 次。另外 `DiskItemModel::setRecords` 对文件夹里**每张图**都做"句柄 + exists"，只是为了标记缩略图就绪 | 一个任务里只解析一次文件 ID，路径向下传。整夹预扫描改为只检查可见行（`loadThumbnailsForRows` 已有此路径）。或把就绪标志持久化 |
| 4 | `renameItem`、`removeMetadataSync` | `renameItem` 为了改一个文件名，要持全局写锁遍历**整个缓存**。`removeMetadataSync` 同样遍历全部分片，而 `SecureDeleteCommand` 是**每个路径调用一次**，删 1000 个文件就是 1000 次全缓存扫描 | 缓存按父目录建分层索引，改名和删除只访问该子树。提供批量删除接口，一次遍历处理整组路径。单个文件改名直接定位，不扫描 |
| 5 | `renameItem` / `renameBatchAsync` 的侧车迁移 | 判断是不是文件夹用的是**旧路径**的 `isDir()`。但 `DiskItemModel::setData` 是先物理改名、再调用本函数，旧路径已不存在，所以结果恒为 false，文件夹被当成文件处理。同时缓存里的 `meta.isFolder` 本来就有，却不用。对每个被改名文件夹下的缓存子孙也逐个调用侧车改名，而子孙只是目录前缀变了，文件名没变 | 用缓存里的 `meta.isFolder`，不再查磁盘。文件夹改名只做一次迁移，子孙只更新内存缓存里的键 |
| 6 | `notifyUI` / `m_uiSignalTimer` | 每次调用都抢全局写锁、插入集合、再投递一次队列事件。但定时器回调里**什么都不做**（全是 `Q_UNUSED`），只是清空集合。整套通知是空转 | 删除，或接上真正的消费者。通知只保留 `CentralEventHub` 这一条通道 |
| 7 | `updateExtractedMediaFeaturesBatch` | 每个条目都 `QFileInfo.isFile()`（一次磁盘调用，判断后缀根本不需要）。再整份拷贝 `RuntimeMeta` 来对比。每条再 `notifyUI` 一次。最后发一个带全部路径的事件，PanelMediator 收到后触发模型批量更新（未命中时 O(n) 扫描）并整份重算统计 | 去掉磁盘调用，在缓存锁内直接比较字段。批内只发一次事件。模型按规范化键直接查，不扫描 |
| 8 | `normalizePath` / `getMeta` | 每次调用做 4 次字符串转换和分配（wstring → QString → `cleanPath` → 小写 → wstring）。`getMeta` 返回整份 `RuntimeMeta` 拷贝（含标签、色板）。这条在 `DiskItemModel::data()` 绘制路径上被反复调用 | 规范化键入缓存时算一次存起来。读取接口按字段读取，或返回只读共享指针 |
| 9 | `searchInCache`、`getAllTags/getTopTags`、`getLightweightCacheSnapshot` | 每次都全缓存遍历（搜索还持有分片锁，会挡住提取线程写入）。`getTopTags` 每调用一次就重建完整的标签计数表 | 标签计数在更新缓存时增量维护。搜索用倒排索引，或下沉到数据库 |

**较轻的几项：**
- `slideRecentWindow` 为了检查队首，每次拷贝整个 deque。`recordAccess` 在 deque 里线性查找并从中间删除。
- `saveDiskThumbnail` 每张缩略图都 `mkpath`，可以缓存已建过的桶目录。
- `InflightGuard` 让线程池线程 50ms 轮询等待别的线程解码同一文件，浪费线程。应该在入队时去重，不要阻塞线程。
- 缩略图失败状态有 6 份：`s_failedNames`、`s_pendingFailures`、JSON 的 `thumbStatus`、`ItemRecord.thumbStatus`、`RuntimeMeta.thumbStatus`、`thumbnailState`。违背 SSOT。
- 宽度读取失败的图片，下次打开文件夹仍会再被 `preloadDimensionsAsync` 重新读一遍文件头，因为判断条件是 `width == 0`。

**确认第 2 项和第 5 项还需要：** QuarkMetaJsonStore.cpp, MediaExtractorPipeline.cpp, ThumbnailPipelineService.cpp

// ===================|===================

**先修正我上一轮的两条结论**，这三个文件看完后有变化：

- **第 2 项（逐文件写盘）夸大了。** `updateItemMeta` 只是标脏并走 `markDirtyLocked` + 防抖 flush，**不会每个文件写一次盘**，同一文件夹会合并成一次整文件写。真正的代价是每次调用的 CPU 开销、监听器回调和内存拷贝，见下表 E、F。
- **第 5 项（子孙逐个迁移）夸大了。** 子孙的文件名没变，`renameItem` 里 `oldName == newName` 直接返回，没有磁盘 I/O，只多构造两个 `QFileInfo`。**用旧路径调 `isDir()` 恒为 false 这一点仍然成立**：文件夹改名时 `migrateFolderCache` 分支永远走不到，只能靠"当文件处理"这条路改父目录 JSON 的键。这样是否漏迁移，要看 `QuarkMetaJson::migrateFolderCache` 做什么。

**以下全部不通过，均未改动代码：**

| # | 位置 | 浪费手法（代码里的事实） | 建议 |
|---|---|---|---|
| A | `MediaExtractorPipeline` 每批 32 个 → `updateExtractedMediaFeaturesBatch` → 发一次 `MetadataUpdated` | 一个 1568 张图的文件夹产生约 49 个事件。PanelMediator 每收到一次就做模型批量更新，并调用 `recalculateAndEmitStats()`（主线程整份拷贝全部记录）。这是提取期间界面卡顿的主因 | 提取结果在接收端合并：定时（约 500ms–1s）发一次合并后的路径集合。统计改为队列清空后算一次，过程中最多低频刷新 |
| B | `m_timer` 1.5 秒心跳 | 代码注释自己写了"兜底防止队列未消费完"。根因是**丢唤醒竞态**：工人线程看到队列空 `break` 后、`fetch_sub` 前，新入队的 `dispatchWorkersIfNeeded` 看到活跃数还没减，就不派新线程。心跳只是靠轮询把它捞回来，**属于打补丁**，而且永不停止 | 工人线程 `fetch_sub` 之后再检查一次队列，非空就自己重新派发。删掉心跳 |
| C | 同上，入队与出队 | 入队没有去重，出队也不复查新鲜度，重复访问同一文件夹会重复提取。失败项（`thumbStatus==1`）每次都被重新入队。调色板为空时直接 `QImage fullImg(qPath)` **整图解码**重试，而且不落失败标记，下次打开又来一遍 | 入队用"排队中/处理中"集合去重。出队前复查。失败项在入队时过滤。调色板为空记为终态，禁止整图重试 |
| D | 三个线程池：`DiskItemModel::thumbnailPool`、`ThumbnailPipelineService::m_decodePool`、`MediaExtractorPipeline::m_workerPool` | 各自开 `核数/2` 条线程，互不协调，合计可达 1.5 倍核数还不算全局池。后台提取与可见区缩略图抢同一批 CPU，没有优先级，靠 `InflightGuard` 让线程阻塞等待去重。另外 `loadBatchAsync` 把一批路径放在**一个任务里串行解码**，可见区的缩略图实际只用 1 条线程 | 合并成一个带优先级的调度器：可见区 > 后台提取。按单个路径派任务，由调度器去重 |
| E | `QuarkMetaJsonStore::updateItemMeta` 每次调用 | 一次 `isDir()`（磁盘调用）、约 3 次路径规范化、两次抢全局互斥锁、一次队列事件（`scheduleFlush` 每次都投递）、一次 `notifyListeners`（复制整个监听器列表并逐个回调）。提取 1568 张图就是这些的 1568 倍，叠加 `MetadataManager` 与提取器里的重复 `stat`，每张图约 5–6 次磁盘调用 | 提供按文件夹的批量接口：一次规范化、一次加锁、一次标脏。`scheduleFlush` 用原子标志，只投递一次。监听器按批通知 |
| F | `flushAllDirtyBuffers` | 在**主线程**、持**全局锁**时，为每个脏文件夹构造 `FlushTicket`，按构造方式是把整个 `QuarkMetaJson` 拷贝一份。防抖计时器每次 `start()` 都会重启，持续提取期间可能一直不落盘，**没有最长延迟上限**，崩溃会丢数据。落盘是整文件重写，写放大 | 锁内只序列化成字节，写盘放后台线程。首次标脏起计时，最长 N 秒必须落盘。`FlushTicket` 的定义我没看到，具体见文末 |
| G | Store 批量接口 | `setRatingBatch/Color/Note/URL/Pinned` 五份复制粘贴，且**不预取**，在全局锁内直接读盘解析 JSON，未缓存的文件夹会阻塞所有读写。`removeTagsBatch` 一把大锁贯穿所有文件夹，`addTagsBatch` 却是逐文件夹加锁，两者不一致。`transferItems` 每对路径都 `prefetch`，没有先去重。`readFolderMeta` 按值返回整个 map。`m_folderCacheMap` 在本文件内没有淘汰逻辑 | 抽成一个通用的"按文件夹分组 → 预取 → 加锁应用"。读接口返回只读共享快照。给缓存加 LRU |
| H | `ThumbnailPipelineService` | `m_currentGeneration` 和 `m_currentToken` 是**全局**的，所有窗格共用。任一窗格触发 `cancelAll`，其他窗格已排队的任务会静默返回、**不调用回调**。而 `DiskItemModel::m_requestedPaths` 只在回调里移除，这些路径会一直被当成"已请求"，直到模型自己重置。每张缩略图还单独投递一个队列事件，并在主线程做 `QPixmap::fromImage` | 按窗格或模型分代，取消时对未完成的路径统一回调"已取消"以清除请求标记。回调按批合并，主线程转换也按批 |

**另一个确认到的缺陷：** `cancelAll` 把 `m_activeCount` 置 0，之后正在跑的批次仍然 `fetch_sub`，计数会变成负数。

**要彻底确认 F 的写放大与监听器的实际开销，还需要：** QuarkMetaJson.cpp, QuarkMetaJsonStore.h, MediaExtractorPipeline.h, MetaIndexService.cpp, MetaCacheDecorator.cpp

// ===================|===================

**结论：F 和监听器两项已用代码确认，另有 7 项新增不通过。** 我没有改动任何代码。

**先更正上一轮：**
- **心跳竞态已确认。** 工人线程看到队列为空后 `break`，在执行 `fetch_sub` 之前，新入队的调度看到活跃数没变就不派线程，任务搁浅。`m_timer` 只在析构时 `stop()`，所以入队后心跳**永不停止**。`m_activeCount` 在这两个文件里没有任何读取点，是死计数。
- **监听器：** Store 对每个路径各通知一次，每次复制整个监听器列表再逐个回调。订阅者的实际开销我仍没看到，见文末。

| # | 位置 | 事实 | 建议 |
|---|---|---|---|
| I | `flushAllDirtyBuffers` / `FlushTicket` / `QuarkMetaJson::save` | `FlushTicket` 持有整个 `QuarkMetaJson` 的**完整拷贝**，在主线程、持全局锁时构造。异步版本的 lambda 又按值捕获 `tickets`，**至少第二次深拷贝**。`save()` 先把整个文件夹构造成完整的 QJsonObject 树，再用 **Indented** 格式序列化（带缩进更大更慢）。`scheduleFlush` 每次都重启 500ms 计时器，**没有最长延迟上限**，持续提取期间可能一直不落盘，崩溃会丢全部。**`m_dirtyFolderPaths` 在写之前就被清空，写失败（文件被占用、目录已改名）不会重试，改动静默丢失** | 锁内只做标脏并取不可变快照（写时复制，不深拷贝）。序列化和写盘放后台线程，用 Compact。从首次标脏起计时，最长 N 秒必须落盘。写失败重新标脏。内容没变不写盘 |
| J | `QuarkMetaJson::updateItemMeta / renameItem / roamBatchMetadata / roamItemMetadata / removeItemMetadata` | 这批静态包装每次调用都 `flushAllDirtyBuffers(true)`，**同步整份落盘并绕过 500ms 防抖**。调用点我没看到，在循环里调用就是爆刷盘。`QuarkMetaJson.cpp` 还包含 `MetadataManager`、`MetaMemoryCache`、`QuarkMetaJsonStore`，而 Store 又包含 `QuarkMetaJson.h`：**格式类反过来依赖业务层，循环依赖** | 删除包装，所有调用统一走 Store。`QuarkMetaJson` 只保留序列化与反序列化 |
| K | `roamBatchMetadata` | 对**每一对路径**调用一次 `MetadataManager::renameItem`，而它每次起一个异步任务、持全局写锁扫描**整个缓存**。移动 1000 个文件就是 1000 个任务乘一次全缓存扫描。`transferItems` 已经搬了侧车，这里是**同一动作的第二条路径** | 提供单次批量接口：只对内存缓存重键，不再碰侧车 |
| L | `MetaCacheDecorator::decorate` | 每次都**无条件查一次数据库**（`getAllDriveMeta`），即使没有盘符根目录。`readFolderMeta` 按值返回整个文件夹的 map，等于整份拷贝，只为查找几条。每条记录还做 4 次路径规范化分配。`applyIncrementalChanges` 新增 1 个文件也走一遍，等于复制整个文件夹的 map。`setRecords` 在主线程调用它，首次读 JSON 的磁盘读取与解析也在这条路径上，是否被提前预取，取决于 ContentDataLoader | 盘符元数据只在存在根目录记录时才查。Store 提供按键读取的接口，不复制整个 map。规范化键存入记录，不重复计算 |
| M | `MediaExtractorPipeline` 队列 | `m_queue` 是 `vector`，**每批 32 个从头部 `erase`**，每次搬移整个剩余队列。10 万项约 3000 批，累计搬移上亿次。`cancelBatch` 复杂度 O(队列长度 × 前缀数)，且对**每个队列项**重复规范化（队列里本来就是规范化过的），内层循环还拼接字符串（分配）。`cancelAll` 之后工人线程仍 `fetch_sub`，计数变负 | 改用 `deque` 加"已入队"集合去重。取消时只做一次前缀判断。删掉 `m_activeCount`。心跳换成"工人线程退出前复查队列" |
| N | `itemToEntry` / `entryToItem` | 条目的 `fileId` 为空时，**每次保存都生成新的随机 UUID**，且 `entryToItem` **根本不读 `file_id`**，所以永远读回来是空，下次保存又换一个。`entryToFolder` 读到缺失的 `meta_file_id` 也现生成随机值，但不标脏，不保存就每次启动都变。这直接破坏"JSON→DB 同步以 File_ID 作父 ID"的设计，也使内容无法做"未变不写" | 读入 `file_id`，首次生成后写回内存并标脏一次，此后稳定 |
| O | `MetaIndexService::processFolder` | 同一 `.QuarkMeta.json` 的 **mtime/size 变化就整夹重建**：先 `DELETE` 全部，再逐条 `INSERT`。每条触发 FTS 触发器，一删一插；加上 6 个二级索引，改 1 个文件的星级也要写约 4N 次索引。FTS 更新触发器对**任何列**变化都触发，包括尺寸和调色板。Store 内存里已有解析好的数据，这里却**重新读盘并再解析一遍**。`items_fts` 又存一份 name/note/tags，数据重复。持续提取期间 JSON 持续变化，会反复重建。另外该类还负责"归入库分类"和进度条，属于遗留职责（注释已写由 `LibrarySyncService` 负责） | 索引由 Store 落盘时带上**变更条目集合**，增量 UPSERT/DELETE。FTS 触发器限定 `AFTER UPDATE OF name, note, tags`。FTS 改用外部内容表（`content='items_index'`）。尺寸、调色板、缩略图状态不触发索引。删除类里的分类归入逻辑 |
| P | `QuarkMetaJsonStore::migrateFolderCache` / `QuarkMetaJson::migrateFolderCache` | 文件夹**已经物理改名**后，旧目录里的 `.QuarkMeta.json` 已不存在，`QFile::exists(oldMetaFile)` 为 false，直接返回 true，**什么都没做**（结果正确但属于空操作）。若走到复制分支，用的是 `copy + remove`，是 2 倍 I/O，而不是原子重命名。Store 版本先**同步整份落盘**所有脏文件夹，再驱逐缓存。旧键下的缓存条目若还有未落盘改动，写入已不存在的目录会失败，因为脏标记已被清除，不会重试 | 物理改名后只做缓存重键，不做文件拷贝，也不全局同步落盘 |

**这几项我仍无法确认，需要你提供：**
- `hasUserOperations()` 的定义：`save()` 只写满足它的条目。如果"仅有尺寸、调色板的条目"不满足，提取结果**根本不会写入侧车**，每次启动都要重新提取。
- `ItemMap` 的比较方式：Store 里同一张 map 用了三种键写法（`toLower`、`normalizeItemKey`、原样），需要确认是否一致。
- 谁订阅了 `addChangeListener`、谁调用了 `enqueueFolder`：决定监听器的真实开销与索引重建的频率。
- `ContentChangeWatcher` 是否忽略 `.QuarkMeta.json` 及其 `.tmp`：每次保存都先创建非隐藏的临时文件再改属性，若没有忽略，自己的写盘会触发文件夹变更事件，再引起增量刷新，可能形成反馈环。

**需要的文件：** MetadataDefs.h, QuarkMetaJson.h, LibrarySyncService.cpp, ContentChangeWatcher.cpp, ContentDataLoader.cpp

// ===================|===================

**建议自底向上、先根后枝。** 上层的去抖定时器、延迟提交这些"补丁"，大多是因为底层事件太吵、数据不稳才长出来的。先把底层修好，上层补丁才能删掉；反过来在上层加补丁，只会越来越多。

| 顺序 | 处理对象 | 先做什么 | 为什么排这里 | 做完你能看到的结果 |
|---|---|---|---|---|
| **1** | **存盘层**：`QuarkMetaJsonStore`、`QuarkMetaJson` | 写失败要重新标脏。落盘设最长延迟上限。落盘快照不再深拷贝，序列化放后台。删除 `QuarkMetaJson` 里每次同步落盘的静态包装，并理清它与业务层的循环依赖。稳定 `file_id`，不再每次保存生成新值 | **会丢数据，所有写入最终都经过这里。** 后面所有改造都要接到它的批量接口上。`file_id` 不稳定，你规划的 JSON 同步数据库也无法成立 | 崩溃或写失败不再丢改动。连续提取期间不再积压或反复整份重写 |
| **2** | **元数据缓存与入口**：`MetadataManager`、`MetaMemoryCache` | 播种缓存时不再对每个文件做磁盘调用，直接用目录枚举已有的数据。路径规范化键只算一次。改名、删除按目录子树定位，不再扫描整个缓存。删掉空转的 `notifyUI` | **这是首次打开大文件夹时界面卡死的主因**，且改动小、独立 | 首次打开 1500+ 文件的文件夹，界面不再卡 |
| **3** | **提取流水线**：`MediaExtractorPipeline`、`DiskMediaExtractor`、`ThumbnailPipelineService` | 修丢唤醒竞态并删除 1.5 秒心跳。队列改 `deque` 并去重。失败为终态，不再整图重试。三个线程池合并成带优先级的调度器。提取结果合并后再通知，不再每 32 个发一次事件。尺寸只保留一个写入入口 | 提取期间持续发出事件，是上层卡顿的**源头** | 提取期间 CPU 和磁盘写入平稳，界面不被事件冲刷 |
| **4** | **事件消费与统计**：`ContentPanel` 统计、`PanelMediator`、`DiskItemModel` | 统计改增量，去掉三条重复触发和主线程整份拷贝。`DiskItemModel::data()` 去掉回落查询。`setData` 里的物理改名与写库，收进现有的 `CoreEngine` 命令通道 | 依赖第 2、3 步稳定后才能安全改。这一步同时解决 SSOT 与职责越界 | 改一个星级不再触发整夹重算 |
| **5** | **面板层**：`FavoritePanel`、`LibraryPanel`、`MetaPanel` | 收藏和库改增量更新。存在性检查移出主线程。面板里的 `*Dao` 调用下沉到服务层。顺序真理源改回数据层 | 收藏栏拆分重构本来就要动 FavoritePanel，**合并做，避免改两遍** | 收藏、库变化时不再整树重建 |
| **独立** | **标签页宽度**：`TabBarWidget` | 之前的方案：`TabItemButton` 自报宽度，宽度重排与内容刷新分离，统一标题生成 | 与上面 5 步完全不相关，风险小，可随时单独做 | 合并后的标签页完整显示名称 |

**暂时不要动的：** `PanelMediator` 的 30ms 去抖、`m_statsDebounceTimer`、各处 `singleShot(0)`。等第 1 到 4 步稳定后，再逐个评估能不能删，现在动只会引入新补丁。

**开始第 1 步前还需要这几个文件，否则有两处无法确认：** MetadataDefs.h, QuarkMetaJson.h, ContentChangeWatcher.cpp

你指定从哪一步开始，我就先出这一步的详细修改方案，等你批准后才会动代码。

// ===================|===================

**三个待确认项的结果：**

1. **`hasUserOperations()`：提取结果会写入侧车。** 只要有宽高、`autoColor`、调色板或 `thumbStatus` 任一项，条目就会保留，我之前担心的"提取结果不落盘"不成立。但这也带来写放大：每张图都有条目，含最多 10 项调色板，`save()` 每次整份 Indented 重写。流水线每处理完一批 32 张就更新一次，只要一批的处理时间超过 500ms，防抖就合并不了，总写入量约等于"批数 × 文件大小"，随文件夹增大呈平方增长。我没实测批间隔。
2. **`ItemMap` 是大小写不敏感的 `std::map`。** Store 里 `toLower`、`normalizeItemKey`、原样三种键写法互不冲突，上轮的担心不成立。但静态 `QuarkMetaJson::readFolderMeta` 返回的是**区分大小写**的 `unordered_map`，与 `ItemMap` 语义不一致，调用点我没看到。
3. **ContentChangeWatcher 有过滤。** 它用 `FileFilterService::isAuxiliaryFile` 排除辅助文件，设计上避免了自触发。`.QuarkMeta.json` 和 `.QuarkMeta.json.tmp` 是否被判为辅助文件，要看 FileFilterService.cpp，我没看到。

**这三个文件里新确认的问题（均不通过）：**

| 位置 | 事实 | 建议 |
|---|---|---|
| `ContentChangeWatcher::onDirectoryChanged` | 递归模式和分栏模式下，任何非辅助文件变化都**直接** `refreshAll` 或 `refreshAllColumns`，没有合并也没有节流。递归模式等于整棵树重载 | 三种模式统一走同一个合并节流入口 |
| `processAccumulatedChanges` | 单文件变化也要付整个文件夹的代价：每个名字两次存在性检查，`decorate` 复制该文件夹整份 map 并查一次数据库，`applyIncrementalChanges` 对全模型建临时映射，`registerItemsAsync` 对新增文件做句柄打开。印证上轮 L 项 | 按键读取、规范化键入记录（见第 4 步方案） |
| 同上，线程池 lambda | 裸捕获 `this` 提交到全局线程池，结束后再 `invokeMethod(this, …)`。窗格关闭时任务还在跑，就会访问已销毁对象。代码没有任何保护，实际是否触发取决于时序 | 用 `QPointer` 或在析构时等待任务 |
| `isDelayed()` + 300ms 定时器 | 菜单打开或编辑中，每 300ms 重试一次，是轮询。`onLoadFinished` 里的 `singleShot(0)` 是延迟补丁 | 改为事件驱动：菜单关闭、编辑结束、加载完成时各调用一次 |
| `beginSuppress/endSuppress` | 手动配对，没有 RAII。中途提前 return 会让计数永远大于 0，窗格从此不再自动刷新。调用点我没看到 | 改成作用域守卫 |
| `QuarkMetaJson` 的路径 | `m_folderPath`、`m_filePath` 在构造时固定，没有 setter。头文件注释写明文件夹改名时侧车由操作系统一起带走，所以 `migrateFolderCache` 是历史遗留。真正的问题是 Store 缓存里旧路径键下的条目，包括脏条目，改名后失效，写回时写不进已不存在的目录 | 新增"按子树换键"和 `QuarkMetaJson` 的换路径方法，删除 `migrateFolderCache` 的拷贝与全局同步落盘 |
| 序列化漏字段 | `ItemMeta` 的 `encryptSalt/Iv/VerifyHash`、`folderId`，以及 `FolderMeta` 的 `encrypt*`、`folderId`，`itemToEntry/folderToEntry` 都**不写**，`entry*` 也**不读**，重启后全部丢失。这些字段是否另有存储，我没看到 | 需要你决定：补齐读写，还是从结构里删除 |

**第 1 步（存盘层）方案要点，待你批准，批准前不动代码：**

1. 写失败要重新标脏。落盘设最长延迟上限，另外在批次结束、切换文件夹、退出时强制落盘。
2. 快照不再深拷贝：锁内只序列化为字节，写盘放后台。用 Compact 格式，内容未变不写。
3. 删除 `QuarkMetaJson` 的静态包装及其对 `MetadataManager`、`MetaMemoryCache` 的依赖，调用方统一走 Store。
4. 文件夹改名走"子树换键"，不再拷贝文件和全局同步落盘。
5. `file_id` 和 `meta_file_id` 读入后保持稳定，首次生成时标脏一次。
6. Store 增加按文件夹批量更新接口，供第 3 步的提取流水线使用。读盘预取放在锁外。
7. 五个 `set*Batch` 合并成一个通用实现。

你需要先回答：**漏字段那一项，补齐还是删除？** 另外请指定从哪一步开始。

**要确认监听器开销、索引重建频率和辅助文件判断，还需要：** LibrarySyncService.cpp, FileFilterService.cpp, DirectoryWatchService.cpp, ContentDataLoader.cpp

// ===================|===================

用户补充与选择
1. 漏字段那一项，需要补齐
2. 从第一步开始吧