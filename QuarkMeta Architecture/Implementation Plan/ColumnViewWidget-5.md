# ColumnViewWidget-5.md Implementation Plan

## Overview
本实施方案旨在彻底解决列视图 (Column View) 存在的两个核心问题：
1. **彻底消除 C++ 内联硬编码 `setStyleSheet`**：将 `ColumnViewPane` 和 `ColumnViewWidget` 中内联写死的样式迁移至 `resources/style.qss`，通过对象名 (`ColumnViewPaneListView` 与 `ColumnViewScrollArea`) 进行优雅的样式隔离与统一渲染。
2. **解决文件夹选中高亮秒消失问题**：
   - 当用户在某列点击文件夹时，此前发射了 `pathNavigated`，被 `ContentPanel` 捕捉后调用了 `NavigationService::instance().navigateTo(path)`，进而触发 `currentUrlChanged` 信号广播，导致 `ContentPanel` 重新执行 `loadDirectory(path)`，使 `m_columnView->setRootPath(path)` 被触发，整套列视图全部被清空重构并冲刷掉了父列中的高亮选中状态。
   - 修复策略：在列视图内展开级联子列时，**不发射 `pathNavigated`**（或由 `ContentPanel` 仅在双击/明确导航时才通知 `NavigationService`），阻止 `NavigationService` 对全列进行全盘销毁与重建；同时调整选区清理逻辑（或保持跨列高亮链路），确保被点击展开的文件夹在父列中持续保持高亮选中状态！

---

## Modified Files List
- `resources/style.qss`
- `src/ui/ColumnViewWidget.h`
- `src/ui/ColumnViewWidget.cpp`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `resources/style.qss` (新增列视图外联 QSS 选择器)

```diff
<<<<<<< SEARCH
/* ContentHeaderWidget 栏分割线 */
=======
/* 列视图 ColumnView 专属 QSS 选择器 */
QScrollArea#ColumnViewScrollArea {
    background: #181818;
    border: none;
}

QListView#ColumnViewPaneListView {
    background: #1E1E1E;
    border: none;
    border-right: 1px solid #2D2D2D;
    color: #CCCCCC;
    outline: none;
}

QListView#ColumnViewPaneListView::item:selected {
    background: #378ADD;
    color: #FFFFFF;
    outline: none;
}

/* ContentHeaderWidget 栏分割线 */
>>>>>>> REPLACE
```

---

### 2. `src/ui/ColumnViewWidget.cpp` (剔除 setStyleSheet & 优化事件响应)

```diff
<<<<<<< SEARCH
    m_listView = new QListView(this);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setModel(m_proxyModel);

    auto* delegate = new TreeItemDelegate(this, false, false);
    m_listView->setItemDelegate(delegate);
    m_listView->setStyleSheet("QListView { background: #1E1E1E; border: none; border-right: 1px solid #2D2D2D; color: #CCCCCC; outline: none; }"
                              "QListView::item:selected { background: #3E3E42; color: #FFFFFF; outline: none; }");
    layout->addWidget(m_listView);
=======
    m_listView = new QListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setModel(m_proxyModel);

    auto* delegate = new TreeItemDelegate(this, false, false);
    m_listView->setItemDelegate(delegate);
    layout->addWidget(m_listView);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
ColumnViewWidget::ColumnViewWidget(ContentPanel* contentPanel, QWidget* parent)
    : QScrollArea(parent), m_contentPanel(contentPanel)
{
    setWidgetResizable(true);
    setStyleSheet("QScrollArea { background: #181818; border: none; }");

    m_container = new QWidget(this);
=======
ColumnViewWidget::ColumnViewWidget(ContentPanel* contentPanel, QWidget* parent)
    : QScrollArea(parent), m_contentPanel(contentPanel)
{
    setObjectName("ColumnViewScrollArea");
    setWidgetResizable(true);

    m_container = new QWidget(this);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        dismissSubColumns(paneIdx);
        clearOtherSelections(paneIdx);
        appendColumn(folderPath);
        emit pathNavigated(folderPath);
    });
=======
    connect(pane, &ColumnViewPane::folderSelected, this, [this](const QString& folderPath, int paneIdx) {
        dismissSubColumns(paneIdx);
        // 保持父列高亮：仅清空 paneIdx 右侧深层列的选区，保留 paneIdx 及其左侧父列的高亮
        for (int i = paneIdx + 1; i < m_panes.size(); ++i) {
            m_panes[i]->clearSelection();
        }
        appendColumn(folderPath);
    });
>>>>>>> REPLACE
```

---

### 3. `src/ui/ContentPanel.cpp` (避免展开列时误发 pathNavigated 覆盖全局 URL)

```diff
<<<<<<< SEARCH
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        if (QFileInfo(path).isDir()) {
            m_currentPath = path;
            emit directorySelected(path);
            emit selectionChanged({path});
            updateStatusBarStats();
        } else {
            emit fileActivated(path);
        }
    });
=======
    m_columnView = new ColumnViewWidget(this, this);
    connect(m_columnView, &ColumnViewWidget::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_columnView, &ColumnViewWidget::pathNavigated, this, [this](const QString& path) {
        if (!QFileInfo(path).isDir()) {
            emit fileActivated(path);
        }
    });
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译代码
运行 CMake 构建命令，确保编译零 Error / Warning：
```bash
cmake --build build --config Release
```

### 2. 验证方案
1. **样式外联性验证**：检查 `ColumnViewWidget.cpp` 中不再包含任何内联 `setStyleSheet`；确认列视图渲染依赖 `resources/style.qss` 中的选择器。
2. **持续高亮选中验证**：在列视图模式下点击任意文件夹，右侧成功级联展开子列；同时被点击的文件夹在父列中保持 `QListView::item:selected` 高亮背景（#378ADD），不会发生高亮闪烁秒消失现象。
