# ColumnViewWidget-6.md Implementation Plan

## Overview
本实施方案旨在彻底解决列视图 (Column View) 存在的三个核心架构与交互缺陷：
1. **彻底消除 C++ 内联硬编码 `setStyleSheet`**：将 `ColumnViewPane` 和 `ColumnViewWidget` 中内联写死的样式迁移至 `resources/style.qss`，通过对象名 (`ColumnViewPaneListView` 与 `ColumnViewScrollArea`) 进行样式隔离与统一渲染。
2. **解决文件夹选中高亮秒消失问题**：
   - 调整选区清理逻辑（保持父列高亮）：在展开第 $N$ 列子文件夹时，仅清理第 $N+1$ 列及右侧更深层列的选区，严格保留第 $N$ 列及其左侧所有父列的高亮选中状态。
3. **彻底根治“点击文件夹导致第 1 列/根列全局无故刷新”的缺陷**：
   - **根因**：此前点击文件夹时发射了 `pathNavigated(folderPath)`，被 `ContentPanel` 捕捉后调用了 `NavigationService::instance().navigateTo(path)`，进而触发全局 `currentUrlChanged` 信号广播，导致 `ContentPanel` 重新执行 `loadDirectory(path)` -> `m_columnView->setRootPath(path)`，引发整套列视图自顶向下全部销毁与重建（并在 SSD 上放大了瞬时重绘闪烁现象）。
   - **修复策略**：在列视图内单击展开级联子列时，**不触发全局 URL 导航服务**；仅在 `ContentPanel` 捕捉到 `ColumnViewWidget` 的增量文件夹展开时做局域列追加 (`appendColumn`)，彻底切断对第 1 列及其他父列的重构冲刷！

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
/* 列视图 ColumnView 专属外联 QSS 选择器 */
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

### 2. `src/ui/ColumnViewWidget.cpp` (剔除 setStyleSheet，保持父列高亮与局域展开)

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
        // 1. 保留父列高亮：仅清空 paneIdx 右侧深层列的选区，保留 paneIdx 及其左侧所有父列的高亮选中
        for (int i = paneIdx + 1; i < m_panes.size(); ++i) {
            m_panes[i]->clearSelection();
        }
        // 2. 局域局域展开下一列，绝对不发射全局 pathNavigated 避免触发全局 URL 刷新
        appendColumn(folderPath);
    });
>>>>>>> REPLACE
```

---

### 3. `src/ui/ContentPanel.cpp` (切断列视图展开与 NavigationService 的全局重置联动)

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
1. **样式外联性验证**：检查 `ColumnViewWidget.cpp` 中不再包含任何内联 `setStyleSheet`；确认列视图依赖 `resources/style.qss` 中的选择器正确呈现暗色视觉。
2. **高亮持续呈现与跨列隔离验证**：
   - 打开列视图，连续点击进入多级子文件夹（如点击第 5 列中的文件夹 ①）；
   - 验证最左侧第 1 列 ② 及中间各级父列**绝对不会发生任何闪烁、重绘或数据刷新**；
   - 验证被点击的文件夹 ① 在父列中**持续保持蓝色高亮背景 (`#378ADD`)**，不会发生秒消失现象。
