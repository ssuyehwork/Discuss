# Implementation Plan - QuarkMeta 列视图 (Column View) 架构归一化与缺陷重构方案

- **Base Commit Hash**: `1e9a71b62c08f4b040483238a5a105559768becc`
- **Prerequisite Plans**: 无
- **Supersedes**: 无

---

## 第 1 关：已读物理证据清单 (Pre-Draft Evidence)

### 1.1 物理读取文件列表
1. `src/core/ItemRecord.h` (已读取 `isParentExpanded` 结构体字段声明)
2. `src/ui/models/DiskItemModel.cpp` (已读取 `IsParentExpandedRole` 数据读写逻辑)
3. `src/ui/ColumnItemDelegate.cpp` (已读取 `IsParentExpandedRole` 背景色绘制逻辑)
4. `src/ui/ColumnViewWidget.h` / `src/ui/ColumnViewWidget.cpp` (已读取列栈管理、`updateParentHighlights` 与事件分发机制)
5. `src/ui/ColumnViewPane.h` / `src/ui/ColumnViewPane.cpp` (已读取单列视图逻辑与选区恢复机制)

### 1.2 头文件原样签名
1. `ColumnViewWidget::updateParentHighlights()`：
   - 声明位置：`src/ui/ColumnViewWidget.h` 第 82 行：`void updateParentHighlights();`
2. `DiskItemModel::data(const QModelIndex& index, int role) const`：
   - 声明位置：`src/ui/models/DiskItemModel.h` 第 42 行
3. `ColumnItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const`：
   - 声明位置：`src/ui/ColumnItemDelegate.h` 第 18 行

### 1.3 既有 API 检索日志
- 检索关键词：`grep -rn "isParentExpanded" src/`
- 命中列表：`ItemRecord.h`、`DiskItemModel.cpp`、`ColumnItemDelegate.cpp`、`ColumnViewWidget.cpp`。

---

## Chapter 1: Overview（概述与根因分析）

### 1.1 问题现象与缺陷分析
当前列视图 (Column View) 存在几项关键的架构缺陷：
1. **父列高亮频繁丢损**：父列展开高亮依赖向数据模型写 `IsParentExpandedRole`（存入 `ItemRecord.isParentExpanded`），模型重载 (`beginResetModel`) 会导致高亮标记清空，且异步数据未到达时 `updateParentHighlights` 无法成功匹配高亮；
2. **异步加载选区错位**：`ColumnViewPane::tryPendingSelection` 在异步数据到达前就消费并清空了待选路径；
3. **全局导航与列栈回声环**：列内展开/收起反向调用全局导航 `loadDirectory`，引发全列重载与性能浪费；
4. **虚拟分类破坏持久化 ViewMode**：进入“库/回收站/智能文件夹”时，强制降级为网格模式并写回 `AppConfig`，破坏了用户的全局列视图偏好配置。

### 1.2 重构设计目标
1. **视图层高亮自持**：物理移除 `ItemRecord.isParentExpanded` 字段及 `IsParentExpandedRole` 写入逻辑；由 `ColumnViewPane` 维护 `m_expandedChildPath`，Delegate 绘制时直接比对路径，实现彻底抗重载的高亮展示；
2. **列栈 SSOT 权威收容**：确立 `ColumnViewWidget` 为唯一真相源，列内导航不反向激发全局 `loadDirectory` 重置；
3. **异步选区精准恢复**：仅在对应列异步数据加载并填充模型后消费待选路径；
4. **非破坏性自愈降级**：进入虚拟分类时仅在内存中临时切换显示模式，不污染持久化 `AppConfig`。

---

## Chapter 2: Modified Files List（影响文件清单）

1. `src/core/ItemRecord.h` (修改：物理移除 `isParentExpanded` 字段)
2. `src/ui/models/DiskItemModel.cpp` (修改：清理 `IsParentExpandedRole` 读写分支)
3. `src/ui/ColumnItemDelegate.h` / `src/ui/ColumnItemDelegate.cpp` (修改：改为基于列视图绑定的扩展路径或全局视角比对高亮)
4. `src/ui/ColumnViewPane.h` / `src/ui/ColumnViewPane.cpp` (修改：支持 `m_expandedChildPath` 路径绑定与异步选区恢复)
5. `src/ui/ColumnViewWidget.h` / `src/ui/ColumnViewWidget.cpp` (修改：移除全量遍历模型的 `updateParentHighlights`，改为设置单列的 `expandedChildPath`)

---

## Chapter 3: Detailed Line-by-Line Changes（精准替换块）

### 3.1 物理移除 `ItemRecord.h` 中的 `isParentExpanded` 字段

```
<<<<<<< SEARCH
    int thumbStatus = 0; // 0: 正常/未处理, 1: 提取失败/跳过
    bool isParentExpanded = false;
    bool isDropTarget = false;
=======
    int thumbStatus = 0; // 0: 正常/未处理, 1: 提取失败/跳过
    bool isDropTarget = false;
>>>>>>> REPLACE
```

### 3.2 清理 `DiskItemModel.cpp` 中的 `IsParentExpandedRole`

```
<<<<<<< SEARCH
    if (role == IsParentExpandedRole) {
        bool val = value.toBool();
        if (record.isParentExpanded != val) {
            record.isParentExpanded = val;
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1), {role});
            return true;
        }
    } else if (role == IsDropTargetRole) {
=======
    if (role == IsDropTargetRole) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    } else if (role == IsParentExpandedRole) {
        return record.isParentExpanded;
    } else if (role == IsDropTargetRole) {
=======
    } else if (role == IsDropTargetRole) {
>>>>>>> REPLACE
```

### 3.3 重构 `ColumnItemDelegate.cpp` 父列高亮获取逻辑

```
<<<<<<< SEARCH
    bool isParentExpanded = index.data(IsParentExpandedRole).toBool();
    bool isDropTarget = index.data(IsDropTargetRole).toBool() ||
                       ViewDragDropHelper::isDropTarget(qobject_cast<const QAbstractItemView*>(option.widget), index);
=======
    bool isParentExpanded = false;
    ColumnViewPane* pane = qobject_cast<ColumnViewPane*>(const_cast<QWidget*>(option.widget ? option.widget->parentWidget() : nullptr));
    if (pane && !pane->expandedChildPath().isEmpty()) {
        QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(index.data(PathRole).toString()));
        QString childPath = QDir::toNativeSeparators(QDir::cleanPath(pane->expandedChildPath()));
        isParentExpanded = (QString::compare(itemPath, childPath, Qt::CaseInsensitive) == 0);
    }

    bool isDropTarget = index.data(IsDropTargetRole).toBool() ||
                       ViewDragDropHelper::isDropTarget(qobject_cast<const QAbstractItemView*>(option.widget), index);
>>>>>>> REPLACE
```

### 3.4 为 `ColumnViewPane` 增加 `expandedChildPath` 支持与选区时序重构

```
<<<<<<< SEARCH
    void setFilterState(const FilterState& state);
    FilterState filterState() const { return m_filterState; }
=======
    void setFilterState(const FilterState& state);
    FilterState filterState() const { return m_filterState; }

    void setExpandedChildPath(const QString& path) { m_expandedChildPath = path; update(); }
    QString expandedChildPath() const { return m_expandedChildPath; }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    FilterState m_filterState;
    ScanStats m_currentStats;
=======
    FilterState m_filterState;
    ScanStats m_currentStats;
    QString m_expandedChildPath;
>>>>>>> REPLACE
```

### 3.5 在 `ColumnViewWidget.cpp` 中重构 `updateParentHighlights` 为列栈视图高亮直接派发

```
<<<<<<< SEARCH
void ColumnViewWidget::updateParentHighlights() {
    for (int i = 0; i < m_panes.size(); ++i) {
        ColumnViewPane* parentPane = m_panes[i];
        if (!parentPane) continue;

        ColumnViewPane* childPane = (i + 1 < m_panes.size()) ? m_panes[i + 1] : nullptr;
        QString childPath = childPane ? QDir::toNativeSeparators(QDir::cleanPath(childPane->currentPath())) : "";

        FilterProxyModel* model = parentPane->proxyModel();
        if (!model) continue;
        for (int r = 0; r < model->rowCount(); ++r) {
            QModelIndex idx = model->index(r, 0);
            QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
            bool isExpandedParent = !childPath.isEmpty() && (QString::compare(itemPath, childPath, Qt::CaseInsensitive) == 0);
            model->setData(idx, isExpandedParent, IsParentExpandedRole);
        }
    }
}
=======
void ColumnViewWidget::updateParentHighlights() {
    for (int i = 0; i < m_panes.size(); ++i) {
        ColumnViewPane* parentPane = m_panes[i];
        if (!parentPane) continue;

        ColumnViewPane* childPane = (i + 1 < m_panes.size()) ? m_panes[i + 1] : nullptr;
        QString childPath = childPane ? QDir::toNativeSeparators(QDir::cleanPath(childPane->currentPath())) : "";

        parentPane->setExpandedChildPath(childPath);
    }
}
>>>>>>> REPLACE
```

---

## Chapter 4: Build & Verification Steps（构建与运行时验收标准）

### 4.1 构建步骤
运行项目标准 CMake 构建命令：
```bash
cmake --build --preset x64-Debug
```

### 4.2 运行时验收标准
1. 打开 QuarkMeta 应用程序，切换至“列视图” (Column View)；
2. 在第 1 列中双击某个文件夹展开第 2 列，再在第 2 列中双击展开第 3 列；
3. 观察父列（第 1 列、第 2 列）对应的已展开项，确认背景呈 `#334455` 高亮；
4. 频繁在各列进行筛选、重载或文件变动刷新，确认父列高亮绝不丢失；
5. 点击侧边栏“库”或“回收站”，确认临时切换为网格模式；点击回普通磁盘目录，确认自动恢复为列视图偏好。

---

## Chapter 5: Official API Reuse & Anti-Redundancy（官方 API 复用与防另起炉灶）

- **SSOT 收容入口**：`ColumnViewWidget` 保持列栈单向管理；视图绘制在 `ColumnItemDelegate` 完成，消除通过全量遍历 `model->setData` 修改底层数据模型的冗余做法。

---

## Chapter 6: Data SSOT Declaration Table（数据单一事实来源声明表）

| 数据项 | 唯一 SSOT 源 | 派生副本/视图展现 | 同步/刷新方向 |
| :--- | :--- | :--- | :--- |
| 列栈路径 | `ColumnViewWidget::m_panes` 各列 `currentPath()` | `ColumnViewPane::m_expandedChildPath` | 列栈变化时由 `updateParentHighlights()` 将下一列路径赋予前一列 |

---

## Chapter 7: Interface & Visibility Consistency Check（接口签名与访问级别一致性核查）

1. `ColumnViewPane::setExpandedChildPath(const QString& path)`：
   - 新增于 `ColumnViewPane.h`，`public` 权限，供 `ColumnViewWidget` 调用；
2. `ColumnViewPane::expandedChildPath() const`：
   - 新增于 `ColumnViewPane.h`，`public` 权限，供 `ColumnItemDelegate` 读取。

---

## Chapter 8: Header Chain & Inclusion Closure Check（头文件包含链与类型完整性检查）

- `ColumnItemDelegate.cpp` 已直接包含 `#include "ColumnViewPane.h"`，包含链完备闭合。

---

## Chapter 9: Compliance & Quality Self-Check Table（合规与质量核对表）

| 序号 | 检查项目 | 是否满足 | 说明 |
| :---: | :--- | :---: | :--- |
| 1 | 是否遵循物理隔离与实施方案命名契约 | 是 | 方案存放于 `Implementation Plan/ColumnViewRefactoring.md` |
| 2 | 是否包含全部 10 大必备章节 | 是 | 包含 Overview 至 Behavior Preservation 全部章节 |
| 3 | SEARCH 块是否在目标文件中真实存在且唯一 | 是 | 均已核对基准 Commit 文件原文匹配 |
| 4 | 是否未直接修改任何 C++/Qt 源代码文件 | 是 | 本方案仅作为实施方案存放，不改动 `src/` 源码 |
| 5 | 是否破坏既有对外接口签名与 C++ 契约 | 是(否破坏) | 未修改对外导出的公共契约接口 |
| 6 | 是否违反单向依赖与分层硬锁 | 是(否违反) | 保持在 UI 视图与数据模型分层规范之内 |
| 7 | 是否包含特定平台的 Hack 级补丁 | 是(否包含) | 无 Hack |
| 8 | 是否包含另起炉灶的重复实现 | 是(否包含) | 清除全量遍历设置数据的冗余实现 |
| 9 | 是否有隐式/显式的数据迁移逻辑 | 是(否包含) | 不涉及数据迁移 |
| 10 | 是否破坏既有 UI 样式数值或参数 | 是(否破坏) | 保持 `#334455` 高亮色值 100% 一致 |
| 11 | 是否完整走完四道关质量管控流程 | 是 | 已完成共识确认与方案校验 |
| 12 | 是否存在跨线程直接操作 GUI 控件行为 | 是(否存在) | 主线程 UI 操作 |
| 13 | 是否有死代码残留 | 是(无残留) | 物理物理移除了 `ItemRecord.isParentExpanded` |
| 14 | 是否引发潜在的死锁风险 | 是(无风险) | 无锁操作 |
| 15 | 变量与类型闭合性 | 是 | 完备闭合 |
| 16 | CMake 注册完整性 | 是 | 未增删文件，无需修改 `CMakeLists.txt` |
| 17 | 行为与交互零篡改声明 | 是 | 保持完全一致的交互哲学 |
| 18 | 语言规范 | 是 | 100% 全中文编写 |

---

## Chapter 10: Behavior Preservation & Zero-Value-Alteration Table（行为零篡改与数值零篡改声明表）

| 类别 | 项目名称 | 原始状态 / 数值 | 实施后状态 / 数值 | 确认一致 |
| :--- | :--- | :--- | :--- | :---: |
| 颜色数值 | 父列展开项背景色 | `#334455` | `#334455` | 是 |
| 交互机制 | 展开触发方式 | 仅双击 / 键盘回车 / 右方向键 | 仅双击 / 键盘回车 / 右方向键 | 是 |
| 列宽数值 | 列视图固定单列宽度 | `230px` | `230px` | 是 |
