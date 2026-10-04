## 复核结论

你更新后的文件我读完了。上次列的问题仍然成立，另外新发现 4 处：

1. **分栏视图的标头行没有绘制代码。** `ColumnItemDelegate` 没有处理标头，标头会被画成一个空名字的文件行。
2. **分栏视图点标头会清空选区。** `DropListView::mousePressEvent` 对标头没有处理。
3. **拖放目标计算会出错。** `ContentFileOpsHandler::onPathsDropped` 用 `mapToSource` 转换索引，但索引属于 SectionProxyModel，分栏视图还传入了 FilterProxyModel，模型不匹配。
4. **网格和列表各用一套代理，折叠状态会不一致。** 切换视图模式后，文件夹折叠状态会丢。

好消息：CMakeLists 里旧文件已经移除，无需处理。下面是修正提示词，分 4 步，每步必须能编译。

---

## 通用规则（每一步开头都贴上）

```
你在修正 QuarkMeta（Qt/C++）内容面板的"单视图重构"。背景：原来每种视图由"文件夹视图 + 文件视图"两个子视图组成，现已改为每种视图只有一个视图，数据经 FilterProxyModel（过滤+排序）再经 SectionProxyModel（在前面插入"文件夹 (N)"/"文件 (M)"标头行）提供。

硬性规则：
1. 只做本步骤描述的改动，不得改动其他功能、命名、交互。
2. 不得改变任何现有功能的交互设计。重命名必须保持行内编辑（view->edit），双击=打开/预览，评星、色标、拖放、右键菜单行为不变。分栏视图每列固定宽度 230px、右侧留白画布的设计不得改动。
3. 禁止打补丁：不得保留旧名字做别名，不得用文本内容、特判、延时定时器绕过结构问题。
4. 被删除的成员必须同时删除所有引用，不得留空壳、别名或注释掉的旧代码。
5. 每一步结束必须完整编译通过，行为不退化。
6. 动手前先全局搜索被改动符号的所有调用点，一并处理，并在完成时列出你改动过的调用点清单。
```

---

## 第 1 步：清理残留，统一为"一条数据链"

```
目标：删除所有旧结构残留，网格和列表共用同一条数据链，使项目能编译。

1. ContentPanel：
   - 只保留一个 FilterProxyModel（命名 m_proxyModel）和一个 SectionProxyModel（命名 m_sectionModel）。网格视图（JustifiedView）和列表视图（DropTreeView）共用这同一个 m_sectionModel（两个视图各有自己的 QItemSelectionModel）。
   - 删除这些成员及全部引用：m_folderTreeView、m_folderGridView、m_folderProxyModel、m_fileProxyModel、m_gridFolderProxyModel、m_gridFileProxyModel、m_gridProxyModel、m_gridSectionProxyModel、m_listProxyModel、m_listSectionProxyModel，以及 ContentPanel.h 里 FolderSectionHeaderBar、FileSectionHeaderBar 的前置声明。
   - 删除所有对 m_gridCanvas、m_listCanvas、SectionedScrollCanvas 的引用（toggleFolderSectionCollapse、setViewMode、updateGridSize、refreshVisibleThumbnails 等）。
   - applyFilters：只对 m_proxyModel 设置一次 currentFilter 并 updateFilter()，showFolders/showFiles 直接使用用户顶栏开关的值，不得再强制覆盖。
   - applySort：只对 m_proxyModel 排序。
   - m_statsWorker 的 statsReady 回调里 setCachedDuplicatePaths 作用于 m_proxyModel。
   - ensureSourceModelIsDiskModel：只对 m_proxyModel 设置 setSourceModel。
   - updateStatusBarStats：文件夹数/文件数统一使用 m_sectionModel->folderCount()/fileCount()（所有非分栏模式）。
   - toggleFolderSectionCollapse：非分栏模式切换 m_sectionModel 的折叠状态；分栏模式调用 m_columnView->toggleFolderSectionCollapse()。
   - initGridView 改用 DropJustifiedView（保留拖放能力），并把它的 pathsDropped 信号连接到 ContentPanel::onPathsDropped；initListView 里 DropTreeView 的 pathsDropped 同样连接。拖放时的目标目录以 currentPath() 为准（与改造前行为一致）。
   - updateGridSize / ContentViewCoordinator::updateGridSize：恢复缩放生效。网格模式调用 JustifiedView::setTargetRowHeight(zoom)；列表模式调用 DropTreeView::setIconSize(QSize(max(16, zoom-8), max(16, zoom-8)))、header 的 setZoomLevel(zoom)（如果 header 是 ContentHeaderView）并 doItemsLayout()。ContentViewCoordinator::updateGridSize 负责这件事，ContentPanel::updateGridSize 只转发。
   - 列表视图初始化补齐：setAlternatingRowColors(true)、setRootIsDecorated(false)、header()->setFixedHeight(32)、header()->setMinimumSectionSize(0)、applyColumnPolicies()，与改造前一致。
   - 滚动条：列表视图 setVerticalScrollBarPolicy(ScrollBarAsNeeded)，列头固定在顶部。

2. ColumnViewPane：
   - 删除 m_folderListView、m_folderProxyModel、m_fileProxyModel、m_paneScrollArea，以及访问器 folderListView()、folderProxyModel()、fileProxyModel()、folderHeader()。
   - 在 ColumnViewPane.h 加入 class SectionProxyModel; 的前置声明。
   - 删除 eventFilter 里对 m_paneScrollArea、m_panel 的滚轮和点击处理（这些成员不存在或永远为空）。paintEvent 里关于 m_folderListView 的底部蓝线删除。
   - 删除后更新所有调用点：ColumnViewWidget、ContentPanel、ContentViewCoordinator、ContentContextMenu、ContentKeyHandler 等。ColumnViewWidget::toggleFolderSectionCollapse 改为调用激活列的 sectionProxyModel()->setFolderCollapsed(!isFolderCollapsed())。
   - setPendingSelectPaths 中遍历行时已跳过标头，保持。

3. ContentViewCoordinator：
   - currentActiveViews 和 getSelectedIndexes 在分栏模式下只取 activePane()->listView()，不再取 folderListView()。
   - 全部选区获取（ContentPanel::getSelectedIndexes、activeItemView、getActiveProxyModel）只保留 ContentViewCoordinator 一份实现，ContentPanel 里的同名方法只转发给它。

4. ContentPanel::activeItemView：非分栏模式直接返回当前模式的唯一视图。

完成后：全局搜索 m_folder、folderView、fileView、folderProxy、fileProxy、gridCanvas、listCanvas、folderHeader，确认零残留。
```

---

## 第 2 步：契约与 SectionProxyModel 修正

```
目标：用模型角色表达标头类型，修正 SectionProxyModel 的性能与信号问题。

1. ModelContract.h 新增角色 SectionKindRole = Qt::UserRole + 215：普通行返回 0，文件夹标头返回 1，文件标头返回 2。SectionHeaderRole 保留，等价于 Kind != 0。
   全局搜索所有 startsWith("文件夹") 判断，统一替换为读取 SectionKindRole == 1：TreeItemDelegate、DropTreeView、JustifiedView、ColumnViewPane。不得再通过显示文本判断标头类型。

2. SectionProxyModel 重写要求：
   a. mapFromSource 不得线性扫描。每个分区内的条目按 sourceRow 递增排列，据此对相应分区做二分查找（先用源索引的 TypeRole 判断属于文件夹区还是文件区）。折叠时文件夹区的源行 mapFromSource 返回无效索引。
   b. onSourceDataChanged：把源范围内每个能映射到代理的行，聚合成连续范围后发 dataChanged（列范围保持原样），不得逐行逐列发信号。尺寸预加载会发全表 dataChanged，必须能快速处理。
   c. setData 不得再自己发 dataChanged（源模型的 dataChanged 已经会转发，会重复）。
   d. 折叠/展开：用 beginRemoveRows/endRemoveRows 与 beginInsertRows/endInsertRows 操作文件夹条目这一段连续行，不得 reset 模型。这样折叠不会丢失文件区的选区。
   e. 源模型 rowsInserted/rowsRemoved（筛选变化和新建/删除项目会触发）：增量处理，不得整体 reset。
      - 删除：在 rowsAboutToBeRemoved 中找出被删源行对应的代理行，按连续段从后往前 beginRemoveRows/endRemoveRows；rowsRemoved 后把其余条目的 sourceRow 减去相应数量；再按需移除不再满足条件的标头行（文件夹数为 0 时移除文件夹标头；文件夹数或文件数为 0 时移除文件标头）。
      - 插入：rowsInserted 后先把已有条目 sourceRow 大于等于 start 的整体加上插入数量，再为每个新源行按 TypeRole 在对应分区内按 sourceRow 有序位置 beginInsertRows/endInsertRows；按需新增标头行。
      - 单次批量超过 500 行时才允许退化为 reset。
      - 任何标头文本（数量）变化，对标头行发 dataChanged。
   f. 源模型 layoutAboutToBeChanged/layoutChanged（排序会触发）：不得在两者之间 beginResetModel。按 Qt 规范：layoutAboutToBeChanged 时记录所有持久索引对应的源索引；重建映射后用 changePersistentIndexList 迁移，再发 layoutChanged。目的是排序后选区与当前项保持。
   g. modelReset：重建映射并 reset。
   h. 标头行的 flags 保持仅 ItemIsEnabled（不可选中、不可编辑、不可拖拽、不可作为放置目标）。
   i. folderCount()/fileCount() 始终反映源模型过滤后的数量，与是否折叠无关。

3. 为 SectionProxyModel 编写自测代码（可放在测试目录，不接入主程序）：覆盖折叠切换后选区保持、筛选增删行、排序后选区保持、全表 dataChanged 的耗时。
```

---

## 第 3 步：视图、委托与分栏视图

```
目标：补齐标头的交互与绘制，修正选区与拖放。

A. JustifiedView / DropJustifiedView
1. indexAt：命中标头行时返回标头行的有效模型索引（不再返回无效索引）。
2. mousePressEvent：点击标头行时——文件夹标头（SectionKindRole==1）切换折叠，文件标头无动作——两种情况都 accept，不清空选区、不开始框选、不触发父类处理。找 SectionProxyModel 时不得遍历代理链猜测，改为：JustifiedView 提供 setSectionModel(SectionProxyModel*)，由 ContentPanel 初始化时设置；DropTreeView 同理。
3. mouseDoubleClickEvent：命中标头行时 accept 并直接返回，不发射 doubleClicked，也不触发"双击空白返回上级"。
4. moveCursor：MoveLeft/Right/Up/Down 计算目标时跳过标头行；当前项为空时从第一个非标头行开始。
5. Shift 范围选择与 setSelection（框选）：跳过标头行。
6. doLayout 中"一行是否做两端对齐"的判断：该行是其分区的最后一行（下一行是标头行，或已到末尾）时不做两端对齐拉伸。
7. paintEvent 标头绘制：文本取 SectionHeaderTextRole，颜色 #3498db，粗体 10pt，左侧留 10px；文件夹标头（Kind==1）在文字右侧绘制折叠箭头（折叠状态取 SectionCollapsedRole），12px，#3498db，图标名与原 FolderSectionHeaderBar 使用的完全一致（先查看 git 历史中 FolderSectionWidget.cpp 的 UiHelper::getIcon 调用，包括是否带 .svg 后缀，并确认 UiHelper::getIcon 的实现能解析该名字；TreeItemDelegate 里目前的 "scroll-008"/"scroll-010" 写法如与原写法不一致需改回原写法）。文件标头不画箭头。
8. ItemGeometry 中 isHeader/headerText/isCollapsed 若不再被使用则删除，标头信息统一从模型读取。

B. DropTreeView / TreeItemDelegate
1. TreeItemDelegate 的斑马纹：不得使用 index.row()%2。改为"所在分区内的序号"的奇偶——文件夹区与文件区各自从 0 开始。实现方式：由模型提供 SectionRowRole（UserRole+216）返回分区内序号，SectionProxyModel 实现。
2. 点击标头行（已有 mousePressEvent 处理）保持：accept，不改变选区。双击标头不触发打开。
3. 列头点击排序：不使用 QTreeView 自带的排序（保持 setSortingEnabled(false)）。监听 header()->sectionClicked，映射列到 SortType 后调用 ContentPanel 的 setSortCriteria（点击同一列则切换升降序），并让列头排序箭头与 sortController 当前状态同步（sortCriteriaChanged 时更新）。列与 SortType 对应：Name→SortByName，Rating→SortByRating，Dimension→SortByDimension，Type→SortByExtension，Size→SortBySize，ModifiedDate→SortByModifyDate；Status 列不排序。

C. 分栏视图
1. ColumnItemDelegate：增加标头行分支。文件夹/文件标头的底色 #1E1E1E，文字 #3498db 粗体 12px，左侧 10px，高度 28px；文件夹标头在右侧画折叠箭头（同上图标规则）。标头行不绘制图标、星级、箭头 chevron。
2. DropListView：mousePressEvent 命中标头行时，发射新信号 sectionHeaderClicked(const QModelIndex&) 并 accept，不调用父类（不清空选区）；mouseDoubleClickEvent 命中标头行时 accept 直接返回。ColumnViewPane 监听 sectionHeaderClicked 完成折叠切换，删除原先放在 clicked 信号里处理标头的代码。
3. ColumnViewPane 向 DropListView 传递 SectionProxyModel 的方式与 JustifiedView 一致（不得遍历代理链）。

D. 拖放
1. ContentFileOpsHandler::onPathsDropped：删除对 sourceModelOverride 的 mapToSource 逻辑，目标项直接使用 targetIndex.data(PathRole)（SectionProxyModel 已透传）；标头行 PathRole 为空，自然回落到当前目录。同步删除 sourceModelOverride 参数及 ContentPanel::onPathsDropped、ColumnViewPane、其他调用点中对应的传参。
2. DragDropEventFilter 判断 isTargetable：标头行（SectionHeaderRole）不可作为放置目标，也不显示拖拽高亮。
```

---

## 第 4 步：所有取代理/遍历行的调用点

```
目标：model() 现在是 SectionProxyModel，不再是 QSortFilterProxyModel。所有 qobject_cast<QSortFilterProxyModel*>(view->model()) 都会得到空指针。逐一处理。

1. 只写数据的调用点（评星、置顶、色标、标签、F4 重复）：直接使用 view->model()->setData(index, ...)，SectionProxyModel 会转发。涉及：ContentKeyHandler（Ctrl+0~5、Alt+D、Alt+1~9、Ctrl+Shift+V、F4）、ContentContextMenu（粘贴标签、重复上次操作）、ThumbnailDelegate/TreeItemDelegate 里的 model->setData。
2. 需要遍历行的调用点：用 QAbstractItemModel（view->model()）按行读取，遇到 SectionHeaderRole 为 true 的行一律跳过。涉及：selectAndEditPath、selectAndScrollToItem、getAdjacentFilePath（上下切换文件时跳过标头，且可跨过标头到下一分区）、ContentViewCoordinator::restoreSelections、ColumnViewPane、ColumnViewWidget::updateParentHighlights。
3. ContentPanel::getActiveProxyModel / getProxyModel 的返回类型改为 QAbstractItemModel*（改名为 activeViewModel()，并更新所有调用点；如外部文件有调用，一并改）。
4. 需要把视图索引还原成 DiskItemModel 行号的地方（缩略图加载、restoreSelections）：在 ContentViewCoordinator 新增静态函数 QModelIndex toSourceIndex(const QModelIndex& idx, const QAbstractItemModel* target)，沿 QAbstractProxyModel::mapToSource 链一路映射到 target（DiskItemModel）为止，遇到标头行返回无效索引。所有需要的地方都调用它，不得各处手写 qobject_cast 链。
   restoreSelections 反向：沿代理链逐层 mapFromSource 把 DiskItemModel 索引转换到视图的 model。
5. selectAndEditPath：只需在视图 model 里按路径找到该行（不再区分文件夹/文件视图），然后 setFocus、scrollTo、setCurrentIndex、select、edit。
6. ContentContextMenu::showMenu：点击到标头行视为点击空白处（onItem=false），并且点击空白处/标头时不得清空已有选区之外的行为保持与改造前一致。右键点在已选中的项目上必须保持当前多选（Ctrl+A 之后右键，菜单操作作用于全部选中项）。
7. ContentKeyHandler::handleMousePress 中的网格 Hitbox 判断：命中标头行时直接返回 false（不属于卡片）。
8. 缩略图可见区扫描（ContentPanel::refreshVisibleThumbnails 与 ContentViewCoordinator::refreshVisibleThumbnails 合并为一份实现）：
   - ContentPanel 创建一个 60ms 单次 QTimer 作为防抖（m_visibleTimer），startVisibleTimer() 启动它，超时执行扫描。
   - 触发源：视图垂直滚动条 valueChanged、JustifiedView::layoutFinished、模型 modelReset/layoutChanged/rowsInserted、视图 resize、setViewMode、缩放变化、loadDirectory/loadPaths 完成后。
   - 网格模式：使用 JustifiedView::rowsInRange(滚动值, 滚动值+视口高度)（范围上下各扩展一个视口高度的 1/2 作为缓冲）；布局未就绪（isLayoutReady 为 false）时不扫描，等 layoutFinished 再触发。
   - 列表模式：用 viewport 顶部/底部的 indexAt 得到行范围，前后各缓冲 4 行；顶部/底部落在标头行上时向下/向上取最近的非标头行。
   - 每个视图索引经 toSourceIndex 转成 DiskItemModel 行号后，汇总调用 loadThumbnailsForRows。
   - 分栏模式沿用 ColumnViewPane::refreshVisibleThumbnails 的现有行为，不改。
9. 全局搜索 qobject_cast<QSortFilterProxyModel*>，每一处都说明如何处理（保留的需注明理由，例如 FilterProxyModel 内部仍合法）。
```

---

## 最终验收清单（Jules 完成后逐项回报结果）

```
- 编译零错误零警告（/W4）。
- 网格、自适应、列表、分栏四种视图：Ctrl+A 一次选中全部文件夹和文件；Ctrl+A 后右键，多选保持，菜单操作作用于全部。
- 点击文件夹标头折叠/展开，文件区选区不丢；点击文件标头无反应；Ctrl+S 折叠快捷键有效；切换视图模式后折叠状态一致。
- 点击标头不清空选区、不开始框选；双击标头不打开、不返回上级。
- Shift 跨文件夹/文件范围选择；框选跨区；方向键跨区；标头始终不被选中。
- 网格里文件夹正常显示在文件前面；启动后第一屏和第二排缩略图都正常加载，滚动加载正常。
- 评星、置顶、色标快捷键、F4、粘贴标签正常；新建文件夹后行内编辑正常；QuickLook 上下切换正常。
- 排序切换后选区保持；列头点击排序有效；缩放有效；拖放进文件夹有效。
- 搜索框输入过程中选区合理保持；状态栏计数正确（不翻倍）。
```