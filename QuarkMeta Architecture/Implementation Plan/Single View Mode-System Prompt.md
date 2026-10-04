以下是给 Jules 的分步提示词，共 5 步，每步完成后都必须能编译运行，不留新旧并存的过渡代码。`ColumnViewPane` 相关文件我没看过，所以第 4 步让 Jules 先读代码再改。

---

## 通用规则（每一步开头都贴上）

```
你在重构 QuarkMeta（Qt/C++）的内容面板。本次重构的唯一目标：废除"文件夹子视图 + 文件子视图"的双视图结构，所有视图模式统一改为单视图。

硬性规则：
1. 只做本步骤描述的改动，不得顺手修改其他功能、命名、交互。
2. 不得改变任何现有功能的交互设计。例如：重命名必须继续是行内编辑（view->edit），双击=打开/预览，不得改成弹窗；评星、色标、拖放、右键菜单的行为保持不变。
3. 禁止打补丁：不得用特判、临时标志位、延时定时器来绕过结构问题。结构问题要从结构上解决。
4. 被删除的类和成员，必须同时删除所有引用，不得保留空壳或注释掉的旧代码。
5. 每一步完成后，项目必须能完整编译，行为与改动前一致（除本步骤明确说明的行为变化外）。
6. 动手前先全局搜索被改动符号的所有调用点，一并处理。
```

---

## 第 1 步：新增 SectionProxyModel

```
新增类 SectionProxyModel（文件 models/SectionProxyModel.h/.cpp），继承 QAbstractProxyModel，源模型是 FilterProxyModel。职责只有一个：在源模型已排序的行前面插入分区标头行。

行结构（自上而下）：
[文件夹标头行] [所有文件夹行] [文件标头行] [所有文件行]

规则：
- 文件夹标头行：仅当文件夹数量 > 0 时存在。显示文本"文件夹 (N)"。
- 文件标头行：仅当文件数量 > 0 且文件夹数量 > 0 时存在。显示文本"文件 (M)"。
- 判断文件夹/文件使用源模型的 TypeRole（"folder"/其他）。FilterProxyModel::lessThan 已保证文件夹排在最前，因此文件夹段和文件段各自连续；不要重新排序。
- 标头行的 flags 不含 ItemIsSelectable、ItemIsEditable、ItemIsDragEnabled、ItemIsDropEnabled。因此 Ctrl+A、框选、Shift 范围选择不会选中标头。
- 标头行的 mapToSource 返回无效索引。
- 新增角色 SectionHeaderRole（加入 ModelContract.h，UserRole+212）：标头行返回 true，普通行返回 false。另增 SectionHeaderTextRole（UserRole+213）返回标头文本，SectionCollapsedRole（UserRole+214）仅对文件夹标头返回当前折叠状态。
- 折叠：提供 setFolderCollapsed(bool) / isFolderCollapsed()。折叠时文件夹行从映射中移除，文件夹标头保留。文件标头不可折叠。折叠状态存放在 SectionProxyModel 内部，切换目录后保持不变。
- 提供 folderCount()、fileCount()（均指源模型过滤后的数量，不受折叠影响），供状态栏统计使用。
- 信号处理：源模型的 dataChanged 必须一对一映射转发（缩略图加载会高频触发，不能整体重置）；rowsInserted/rowsRemoved/layoutChanged/modelReset 时重建映射并发出对应信号。重建映射必须用 beginResetModel/endResetModel 或 layoutAboutToBeChanged/layoutChanged 保证视图一致。
- sort(column, order) 直接转发给源模型。
- 其他需要透传的 role（Qt::DecorationRole、PathRole 等）全部透传给源模型；setData 透传给源模型（标头行返回 false）。
- 列数与源模型一致。

本步骤只新增该类并加入工程（CMake/pro），暂不接入任何视图。完成后写一个最小单元测试或自测代码验证：插入/删除行、折叠切换、dataChanged 转发后，映射正确。
```

---

## 第 2 步：JustifiedView（网格 + 自适应）改为单视图

```
改造 ContentPanel 的网格和自适应模式，使其只使用一个 DropJustifiedView，数据来自 SectionProxyModel。

具体改动：
1. JustifiedView::doLayout：
   - 模型行是标头行（SectionHeaderRole 为 true）时，该行占整行宽度、高度 28px，独占一行，其前后各强制换行。
   - 删除现有的"文件夹与文件交界处强制换行"逻辑（isCurrentDir / isPrevDir / forceBreak 相关），改由标头行天然分隔。
   - 最后一行是否做两端对齐的判断（isLastRow）：以"本分区的最后一行"为准，即下一行是标头行或已到末尾时，不做两端对齐拉伸。
   - m_geometries[行号] 与模型行号保持一一对应。
2. 删除 ItemGeometry 中不再需要的字段（isHeader、headerText、isCollapsed），标头信息改从模型角色读取。paintEvent 中标头绘制：文本取 SectionHeaderTextRole，颜色 #3498db，粗体 10pt；文件夹标头在文字右侧绘制折叠箭头（折叠=scroll-008.svg，展开=scroll-010.svg，12px，颜色 #3498db，沿用 UiHelper::getIcon），文件标头不画箭头。
3. indexAt：命中标头行时，返回该标头行的有效模型索引（以便点击折叠）；其他不变。
   mousePressEvent：点击文件夹标头行时调用 SectionProxyModel::setFolderCollapsed 切换折叠，并 accept，不改变选区；点击文件标头行不做任何事。
4. moveCursor：MoveLeft/MoveRight/MoveUp/MoveDown 计算目标行时跳过标头行。
5. setSelection（框选）与 Shift 范围选择：跳过标头行。
6. 选择模式统一为 ExtendedSelection（文件夹可多选）。
7. 双击空白处返回上级：保持现有行为。
8. 缩略图可见区扫描：rowsInRange 返回的模型行需过滤掉标头行。
9. 在 ContentPanel 中：m_gridView 指向唯一的 DropJustifiedView，其 model 为 SectionProxyModel（源为 FilterProxyModel，源为 DiskItemModel）。ContentPanel 里网格模式只保留一个 FilterProxyModel（原 m_gridFileProxyModel，改名为 m_gridProxyModel），其 currentFilter 使用 m_currentFilter 原值，showFolders/showFiles 保持用户顶栏开关的含义，不再被强制覆盖为 true/false。
10. 删除网格相关的 m_gridFolderProxyModel、m_folderGridView，删除网格用的 SectionedScrollCanvas 实例。
11. 全局搜索所有遍历代理行并读取 PathRole 的位置（selectAndEditPath、selectAndScrollToItem、getAdjacentFilePath、restoreSelections、ContentViewCoordinator 等），遇到标头行必须跳过。getAdjacentFilePath 上下切换时要跳过标头。
12. 右键菜单：点击在标头行上，按"点击空白处"处理。
13. 拖放：标头行不可作为拖放目标。DragDropEventFilter 中判断 isTargetable 时排除标头行。
14. 状态栏统计（updateStatusBarStats）：文件夹数/文件数改用 SectionProxyModel::folderCount()/fileCount()。
15. 滚动：使用 DropJustifiedView 自带滚动条，删除所有外层 QScrollArea 撑高逻辑。
16. ContentPanel::applyFilters：网格代理只设置一次 currentFilter 并 updateFilter()。
```

---

## 第 3 步：列表（DropTreeView）改为单视图

```
改造 ContentPanel 的列表模式，使其只使用一个 DropTreeView，数据来自 SectionProxyModel。

具体改动：
1. m_treeView 指向唯一的 DropTreeView，model 为 SectionProxyModel（源为列表专用 FilterProxyModel，原 m_fileProxyModel，改名 m_listProxyModel）。选择模式 ExtendedSelection。
2. 标头行（SectionHeaderRole 为 true）：使用 setFirstColumnSpanned(row, QModelIndex(), true) 合并整行。模型重置或行变化后需重新应用合并。在 TreeItemDelegate::paint 中，标头行绘制：背景 #1E1E1E，文本 SectionHeaderTextRole，颜色 #3498db、粗体 12px，左边距 10px，高度 28px（sizeHint 返回 28）；文件夹标头在文字右侧画折叠箭头（同第 2 步规格），文件标头不画箭头。
3. 点击文件夹标头行：切换 SectionProxyModel 的折叠状态，不改变选区。点击文件标头行无动作。标头行不可展开、不可编辑、不可拖拽。
4. TreeItemDelegate 斑马纹：现用 index.row() % 2，标头会使奇偶错位。改为"当前分区内的序号"的奇偶（文件夹分区与文件分区各自从 0 开始计数）。计算方式：行号减去该行之前的标头行数量与分区起点。
5. 排序：去掉 setSortingEnabled(true)。点击列头排序时，转发给 ContentSortController（setSortCriteria），由其对 FilterProxyModel 排序；标头行不参与排序。列头排序指示箭头与当前排序状态同步显示。
6. 列头：只有一套，固定在顶部，不随内容滚动。
7. 删除列表相关的 m_folderTreeView、m_folderProxyModel，删除列表用的 SectionedScrollCanvas 实例。
8. 与第 2 步第 11~16 点相同的配套修改（跳过标头、右键、拖放、状态栏统计、可见区缩略图扫描、applyFilters 只设置一次）一并应用到列表模式。可见区扫描：用 viewport 的顶部/底部 indexAt 确定行范围并前后各缓冲 4 行，过滤标头行。
9. 修正 ContentPanel::isTreeView：现在只有一个 DropTreeView，直接 view == m_treeView 即可，ContentKeyHandler 里按 isTreeView 分流的逻辑保持不变。
10. 使用 DropTreeView 自带滚动条，删除外层撑高逻辑。
```

---

## 第 4 步：分栏视图（ColumnViewPane）改为单视图

```
ColumnViewPane 目前同样是 folderListView + listView 两个列表（各自的 folderProxyModel / fileProxyModel）。请先完整阅读 ColumnViewPane.h/.cpp、ColumnViewWidget.h/.cpp，以及所有调用 folderListView()、listView()、folderProxyModel()、fileProxyModel()、folderHeader() 的位置，弄清楚现有实现后再改，不要猜。

目标：每一列只有一个 DropListView，使用一个 FilterProxyModel + 一个 SectionProxyModel（复用第 1 步实现，行为规则相同：文件夹标头可折叠、文件标头不可折叠、标头不可选中），选择模式 ExtendedSelection。

要求：
1. 列宽固定 230px 的设计保持不变，右侧留白画布设计保持不变，不得改动这些逻辑。
2. 单击文件夹展开下一列、单击文件预览、双击行为、父列高亮（IsParentExpandedRole）、列内空白处双击回退，均保持现有交互与行为。
3. 删除 ColumnViewPane 里的 folderListView、folderProxyModel、fileProxyModel 拆分及 QWidget 版标头条；折叠由 SectionProxyModel 管理。ColumnViewWidget::toggleFolderSectionCollapse 改为对当前激活列的 SectionProxyModel 切换折叠。
4. ColumnViewWidget::getSelectedPaths / getSelectedIndexes、ContentPanel 中涉及列视图的 activeItemView / getSelectedIndexes / selectAndEditPath / restoreSelections / getActiveProxyModel，改为单视图写法。
5. 选区行为：同列内 Ctrl+A 一次选中该列所有文件夹和文件；右键不得清掉多选。
6. updateParentHighlights 遍历代理行时跳过标头行。
7. 筛选状态（setFilterState）：最右列使用完整筛选，父列只应用 showHidden，保持现有逻辑。
```

---

## 第 5 步：清理

```
前四步完成后，全局清理：
1. 删除文件：SectionedScrollCanvas.h/.cpp、DualSectionPanel.h/.cpp、FolderSectionWidget.h/.cpp（QWidget 版标头条），并从工程文件中移除。
2. 删除 ContentPanel 里 gridCanvas()、listCanvas()、folderView 相关访问器及成员，以及 ContentViewCoordinator 中对应的 updateListSectionCounts / updateGridSectionCounts。
3. 全局搜索 folderView、fileView、folderProxy、fileProxy、gridCanvas、listCanvas、DualSectionPanel、SectionedScrollCanvas、FolderSectionHeaderBar、FileSectionHeaderBar、setFixedHeight（与撑高有关的），确保没有任何残留引用。
4. 回归检查（逐项验证并给出结果）：
   - 三种视图模式 + 分栏视图下 Ctrl+A，文件夹和文件同时被选中。
   - Ctrl+A 之后右键，多选保持；右键菜单操作作用于全部选中项。
   - Shift 跨文件夹与文件范围选择；框选跨文件夹与文件；方向键跨区移动。
   - 标头行无法被选中；点击文件夹标头折叠/展开，点击文件标头无反应；Ctrl+S 折叠快捷键仍有效。
   - 重命名为行内编辑；双击打开/预览；拖放进文件夹；评星与色标；缩略图滚动加载（启动后第一屏与第二排都正常）。
   - 切换目录后折叠状态保持；排序切换后文件夹始终在前。
```

---

执行顺序必须是 1 → 2 → 3 → 4 → 5，每步合并前先验证编译和行为。

所有实施方案文件已创建于 QuarkMeta Architecture/Implementation Plan/ 目录中，内容详尽且符合 Git Merge Diff 精准替换块标准与工程硬锁规范：

第 1 步：SectionProxyModel.md

新增 SectionProxyModel 类，继承 QAbstractProxyModel。
实现 [文件夹标头行] -> [文件夹行] -> [文件标头行] -> [文件行] 行映射结构。
在 ModelContract.h 中定义 SectionHeaderRole (UserRole+212)、SectionHeaderTextRole (UserRole+213) 及 SectionCollapsedRole (UserRole+214)。
标头行 flags 剥离选择、编辑、拖放权限，实现不可被 Ctrl+A / 框选选中的特性；折叠状态支持 setFolderCollapsed。
注册至 CMakeLists.txt。
第 2 步：JustifiedViewSingleView.md

改造 DropJustifiedView（网格与自适应模式），使用单一 DropJustifiedView 控件与 SectionProxyModel。
适配 doLayout 标头独占整行（高度 28px、两端强制换行）与两端对齐逻辑。
适配 indexAt 与 mousePressEvent 的文件夹标头点击折叠。
ContentPanel 中移除 m_gridCanvas 与 m_folderGridView，保留唯一的 m_gridView 与 m_gridProxyModel。
第 3 步：ListViewSingleView.md

改造 DropTreeView（列表模式），使用单一 DropTreeView 控件与 SectionProxyModel。
结合 setFirstColumnSpanned 实现标头跨全列合并；在 TreeItemDelegate 中绘制标头样式与分区奇偶斑马纹。
ContentPanel 中移除 m_listCanvas 与 m_folderTreeView，保留唯一的 m_treeView 与 m_listProxyModel。
第 4 步：ColumnViewSingleView.md

深入排查 ColumnViewPane，废除内部的 folderListView / listView 双视图与 DualSectionPanel。
保持 230px 固定列宽与延伸留白画布不变；各列使用单一 DropListView 绑定 SectionProxyModel。
适配单视图下单击展开下一列、双击预览、父列高亮与空白处双击回退等既有交互。
第 5 步：SingleViewCleanupAndVerification.md

物理删除旧的双视图容器 SectionedScrollCanvas.h/.cpp、DualSectionPanel.h/.cpp 与 FolderSectionWidget.h/.cpp 并从 CMakeLists.txt 移除。
清理 ContentPanel 与 ContentViewCoordinator 中的废弃访问器与计数方法。
制定详细的全功能回归验证矩阵（包含 Ctrl+A 跨区多选、右键菜单保持、行内重命名、双击预览、缩略图可见区扫描等）。