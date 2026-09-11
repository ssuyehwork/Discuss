# ColumnViewWidget-8.md Implementation Plan

## Overview
本实施方案旨在全面提升与补齐“列视图 (Column View)”的核心交互能力，使其与网格视图 (`GridView`) / 列表视图 (`ListView`) 达成 100% 绝对一致的功能契约：
1. **地址栏与导航服务实时同步**：
   - 修复在列视图中选中子文件夹时地址栏未更新的问题。当在列视图中选中子文件夹时，`ContentPanel` 会发射 `directorySelected` 触发 `NavigationService::navigateTo(path)`，`NavigationService` 随即广播 `currentUrlChanged` 更新顶部地址栏。
   - 在 `ContentPanel::loadDirectory` 中，优化 `ViewModeColumn` 模式下的响应机制：当导航到的路径已包含在列视图当前的级联列栈内时，仅静默更新 `m_currentPath`，避免触发全列清空重置 (`setRootPath`)，从而实现地址栏实时平滑更新且右侧级联列不闪烁不重置。
2. **拖拽支持（支持拖拽至收藏夹 / 外部目标）**：
   - 将 `ColumnViewPane` 中的列表控件升阶为具备拖拽能力与 MIME 数据打包的 `DropListView`；支持将选中项直接拖拽放入左侧 `FavoritePanel`（触发 `requestAddFavorite` 动作）或外部应用。
3. **全局筛选器与焦点穿梭**：
   - 确保 `ColumnViewWidget` 的所有级联列在动态创建与筛选变更时均完美应用全局 `FilterState`。

---

## Modified Files List
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ColumnViewWidget.cpp` (引入 DropListView 开启拖拽)

```diff
<<<<<<< SEARCH
#include "TreeItemDelegate.h"
#include "UiHelper.h"
=======
#include "DropListView.h"
#include "TreeItemDelegate.h"
#include "UiHelper.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    m_listView = new QListView(this);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setModel(m_proxyModel);
=======
    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setModel(m_proxyModel);
>>>>>>> REPLACE
```

---

### 2. `src/ui/ContentPanel.cpp` (地址栏静默平滑同步)

```diff
<<<<<<< SEARCH
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ViewModeColumn && m_columnView) {
        if (m_currentPath != path) {
            m_currentPath = path;
            m_columnView->setRootPath(path);
        }
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    if (m_currentViewMode == ViewModeColumn && m_columnView) {
        bool pathInPanes = false;
        ColumnViewPane* active = m_columnView->activePane();
        if (active && active->path() == path) {
            pathInPanes = true;
        }
        m_currentPath = path;
        if (!pathInPanes) {
            m_columnView->setRootPath(path);
        }
        updateStatusBarStats();
        return;
    }
    if (m_dataLoader) m_dataLoader->loadDirectory(path, recursive);
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译验证
```bash
cmake --build build --config Release
```

### 2. 功能测试步骤
1. **地址栏实时同步测试**：
   - 打开“列视图”，从左向右依次点击进入子文件夹；
   - 观察顶部地址栏，确认地址栏随着选中文件夹的变化实时更新路径，且列视图右侧已展开的级联列平滑保留；
2. **拖拽至收藏夹测试**：
   - 在列视图中按住任意文件夹/文件拖拽至左侧“收藏夹” (`FavoritePanel`) 区域释放，确认触发添加收藏夹响应；
3. **全局筛选器与体验验证**：
   - 在列视图模式下输入搜索关键词或切换隐藏文件显示，确认所有级联列同步筛选。
