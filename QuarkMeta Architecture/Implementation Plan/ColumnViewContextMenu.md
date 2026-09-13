# ColumnViewContextMenu.md Implementation Plan

## Overview
本实施方案旨在彻底解决列视图 (Column View) 中鼠标右键不弹出上下文菜单 (ContentContextMenu) 的问题：
1. 在 `ColumnViewPane` 构造函数中，对 `m_listView` 显式调用 `m_listView->setContextMenuPolicy(Qt::CustomContextMenu)`。
2. 确保列视图内任何列表项在右键点击时，能正确触发 `customContextMenuRequested` 信号，并路由至 `ContentPanel::onCustomContextMenuRequested` 弹出应用专属深色右键菜单。

---

## Modified Files List
- `src/ui/ColumnViewWidget.cpp`

---

## Detailed Line-by-Line Changes

### `src/ui/ColumnViewWidget.cpp`

```diff
<<<<<<< SEARCH
    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new ColumnItemDelegate(this));
=======
    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new ColumnItemDelegate(this));
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译代码
```bash
cmake --build build --config Release
```

### 2. 功能验证
1. 打开应用并切换到列视图 (Column View) 模式。
2. 在任意列的文件或文件夹上点击鼠标右键。
3. **验证结果**：即时弹出包含“打开”、“复制”、“剪切”、“删除”、“属性”等操作的 QuarkMeta 专属暗色右键上下文菜单。
