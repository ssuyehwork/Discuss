# Implementation Plan - ColumnViewWidget Complete Refactoring & KeyHandler/Multi-Column Bridge & Sort Integration

## 1. Overview（概述与解决的问题）

### 架构三问回答
1. **真理源溯源 (SSOT)**：
   全应用文件数据源于磁盘与 `MetaCacheDecorator` / `DiskScanService`。在 `ColumnViewWidget` 中，多列 `ColumnViewPane` 的私有 `DiskItemModel` 仅作为该路径视图层的呈现模型。`ColumnViewWidget` 充当多列广播路由中心（Mediator），负责在中枢 `ContentPanel` 与多列 `m_panes` 之间维持单向、精准的数据流与选择态连通，消除“多列失联断网”硬伤。
2. **黑盒完整性 (Black-box Integrity)**：
   - 保留 `ContentPanel` 事件过滤器对快捷键（`m_keyHandler`）的捕获能力，仅拦截键盘事件，拒绝接管 mouse 事件，实现按键与双击逻辑彻底解耦；
   - `DropListView` / `ColumnViewPane` 重写自身的 `mouseDoubleClickEvent` 显式向外发射 `blankSpaceDoubleClicked(int paneIndex)` 信号；`ContentPanel` 监听此信号精准响应层级回退。
3. **根因溯源 (Root Cause Analysis)**：
   - 之前快捷键、多列与排序失效的根本原因：`ContentPanel::refreshAll()` 只调用了 `refreshActiveColumn()`；同时 `ContentPanel::applySort()` 未在 `ColumnViewWidget` 中正确实现并联动遍历全列 `m_panes` 调用其 `FilterProxyModel` 进行排序，导致排序规则未广播至所有展开列。

### 解决的核心问题
1. **彻底修复粘贴/删除/回收站全列刷新**：修改 `ContentPanel::refreshAll()` 与 `refresh()`，当处于 `ColumnView` 模式时，强制调用 `m_columnView->refreshAllColumns()` 遍历全列刷新，确保 `Ctrl+V` 粘贴、删除、重命名后所有展开列 100% 毫秒级联动更新。
2. **多列排序精准广播与分发**：在 `ColumnViewWidget.cpp` 中完整实现 `applySort(int sortType, Qt::SortOrder sortOrder)`，遍历所有 `m_panes` 调用 `pane->applySort(...)`；同时在 `ContentPanel::applySort()` 中明确补充调用 `m_columnView->applySort(...)`，保证状态栏或菜单改变排序时，所有列同步精准排序。
3. **精准保留快捷键通道与解耦事件过滤器**：`ContentPanel` 仅对快捷键事件响应 `m_keyHandler`，不再在 `eventFilter` 里混杂 mouseDoubleClick 的硬抹逻辑。
4. **精准锚点双击空白回退与焦点框彻底消除**：通过 `DropListView::blankSpaceDoubleClicked` 触发 `goUpColumnFromIndex(int paneIndex)`，在当前列空白双击时调用 `dismissSubColumns(paneIndex)` 移除其右侧所有子列并保留当前列高亮，且设置 `Qt::NoFocus` 彻底消除虚线焦点框。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/DropListView.h`
2. `src/ui/DropListView.cpp`
3. `src/ui/ColumnViewWidget.h`
4. `src/ui/ColumnViewWidget.cpp`
5. `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/DropListView.h`

```
<<<<<<< SEARCH
signals:
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
};
=======
signals:
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
    void blankSpaceDoubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/DropListView.cpp`

```
<<<<<<< SEARCH
#include "DropListView.h"
#include "../core/ModelContract.h"
=======
#include "DropListView.h"
#include "../core/ModelContract.h"
#include <QMouseEvent>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void DropListView::startDrag(Qt::DropActions supportedActions) {
    QListView::startDrag(supportedActions);
}
=======
void DropListView::startDrag(Qt::DropActions supportedActions) {
    QListView::startDrag(supportedActions);
}

void DropListView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event && event->button() == Qt::LeftButton) {
        QModelIndex idx = indexAt(event->pos());
        if (!idx.isValid()) {
            emit blankSpaceDoubleClicked();
            event->accept();
            return;
        }
    }
    QListView::mouseDoubleClickEvent(event);
}
>>>>>>> REPLACE
```

### 3.3 `src/ui/ColumnViewWidget.h`

```
<<<<<<< SEARCH
signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
=======
signals:
    void folderSelected(const QString& folderPath, int paneIndex);
    void fileSelected(const QString& filePath, int paneIndex);
    void selectionChanged();
    void recordsLoaded(const std::vector<ItemRecord>& records);
    void blankSpaceDoubleClicked(int paneIndex);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
public:
    explicit ColumnViewWidget(ContentPanel* contentPanel = nullptr, QWidget* parent = nullptr);
    ~ColumnViewWidget() override = default;

    void setRootPath(const QString& path);
    void clearAllColumns();

    ColumnViewPane* activePane() const;
    ColumnViewPane* rightmostPane() const;
    bool containsPath(const QString& path) const;
    void refreshActiveColumn();
    void updateMetadataForPath(const QString& path);
    void applySort(int sortType, Qt::SortOrder sortOrder);
    void scrollToRightmostPane();
    QStringList getSelectedPaths() const;
    QModelIndexList getSelectedIndexes() const;
    void applyFilterState(const FilterState& state);
    void goUpColumn();
=======
public:
    explicit ColumnViewWidget(ContentPanel* contentPanel = nullptr, QWidget* parent = nullptr);
    ~ColumnViewWidget() override = default;

    void setRootPath(const QString& path);
    void clearAllColumns();

    ColumnViewPane* activePane() const;
    ColumnViewPane* rightmostPane() const;
    bool containsPath(const QString& path) const;
    void refreshActiveColumn();
    void refreshAllColumns();
    void updateMetadataForPath(const QString& path);
    void applySort(int sortType, Qt::SortOrder sortOrder);
    void scrollToRightmostPane();
    QStringList getSelectedPaths() const;
    QModelIndexList getSelectedIndexes() const;
    void applyFilterState(const FilterState& state);
    void goUpColumn();
    void goUpColumnFromIndex(int paneIndex);
>>>>>>> REPLACE
```

### 3.4 `src/ui/ColumnViewWidget.cpp`

```
<<<<<<< SEARCH
    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setModel(m_proxyModel);

    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, &ColumnViewPane::tryPendingSelection);
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, &ColumnViewPane::tryPendingSelection);

    auto* delegate = new ColumnItemDelegate(this);
    m_listView->setItemDelegate(delegate);
    layout->addWidget(m_listView);

    if (m_contentPanel) {
        m_listView->installEventFilter(m_contentPanel);
        m_listView->viewport()->installEventFilter(m_contentPanel);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
    }
=======
    m_listView = new DropListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setFocusPolicy(Qt::NoFocus);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setDragEnabled(true);
    m_listView->setAcceptDrops(true);
    m_listView->setDropIndicatorShown(true);
    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setModel(m_proxyModel);

    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, &ColumnViewPane::tryPendingSelection);
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, &ColumnViewPane::tryPendingSelection);

    auto* delegate = new ColumnItemDelegate(this);
    m_listView->setItemDelegate(delegate);
    layout->addWidget(m_listView);

    connect(m_listView, &DropListView::blankSpaceDoubleClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        emit blankSpaceDoubleClicked(paneIdx);
    });

    if (m_contentPanel) {
        // 保留 installEventFilter 用于捕获按键快捷键 (m_keyHandler)
        m_listView->installEventFilter(m_contentPanel);
        connect(m_listView, &QListView::customContextMenuRequested, m_contentPanel, &ContentPanel::onCustomContextMenuRequested);
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
ColumnViewPane* ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    if (m_contentPanel) {
        pane->installEventFilter(m_contentPanel);
    }
    pane->setProperty("paneIndex", newIdx);
=======
ColumnViewPane* ColumnViewWidget::appendColumn(const QString& path) {
    int newIdx = m_panes.size();
    ColumnViewPane* pane = new ColumnViewPane(path, m_contentPanel, m_container);
    pane->setProperty("paneIndex", newIdx);

    connect(pane, &ColumnViewPane::blankSpaceDoubleClicked, this, [this](int paneIdx) {
        goUpColumnFromIndex(paneIdx);
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewWidget::applySort(int sortType, Qt::SortOrder sortOrder) {
    for (auto* pane : m_panes) {
        if (pane) pane->applySort(sortType, sortOrder);
    }
}
=======
void ColumnViewWidget::applySort(int sortType, Qt::SortOrder sortOrder) {
    for (auto* pane : m_panes) {
        if (pane) {
            pane->applySort(sortType, sortOrder);
        }
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewWidget::refreshActiveColumn() {
    ColumnViewPane* pane = activePane();
    if (pane) {
        pane->loadDirectory();
    }
}
=======
void ColumnViewWidget::refreshActiveColumn() {
    ColumnViewPane* pane = activePane();
    if (pane) {
        pane->loadDirectory();
    }
}

void ColumnViewWidget::refreshAllColumns() {
    for (auto* pane : m_panes) {
        if (pane) {
            pane->loadDirectory();
        }
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ColumnViewWidget::goUpColumn() {
    if (m_panes.size() > 1) {
        dismissSubColumns(m_panes.size() - 2);
        ColumnViewPane* newActive = rightmostPane();
        if (newActive) {
            m_activePaneIndex = m_panes.size() - 1;
            emit pathNavigated(newActive->currentPath());
            emit selectionChanged();
        }
    } else {
        NavigationService::instance().goUp();
    }
}
=======
void ColumnViewWidget::goUpColumn() {
    goUpColumnFromIndex(m_panes.size() - 1);
}

void ColumnViewWidget::goUpColumnFromIndex(int paneIndex) {
    if (paneIndex >= 0 && paneIndex < m_panes.size()) {
        if (paneIndex == 0 && m_panes.size() == 1) {
            NavigationService::instance().goUp();
            return;
        }
        dismissSubColumns(paneIndex);
        ColumnViewPane* newActive = rightmostPane();
        if (newActive) {
            m_activePaneIndex = m_panes.size() - 1;
            emit pathNavigated(newActive->currentPath());
            emit selectionChanged();
        }
    } else {
        NavigationService::instance().goUp();
    }
}
>>>>>>> REPLACE
```

### 3.5 `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    if (event && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent && mouseEvent->button() == Qt::LeftButton) {
            if (m_currentViewMode == ColumnView && m_columnView) {
                QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
                if (!view && obj) {
                    view = qobject_cast<QAbstractItemView*>(obj->parent());
                }
                if (view) {
                    QModelIndex idx = view->indexAt(mouseEvent->pos());
                    if (!idx.isValid()) {
                        m_columnView->goUpColumn();
                        return true;
                    }
                } else {
                    // 匹配区域 ⑥ 背景留白处 (ColumnViewWidget及其container/viewport)
                    m_columnView->goUpColumn();
                    return true;
                }
            } else {
                QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
                if (!view && obj) {
                    view = qobject_cast<QAbstractItemView*>(obj->parent());
                }
                if (view) {
                    QModelIndex idx = view->indexAt(mouseEvent->pos());
                    if (!idx.isValid()) {
                        NavigationService::instance().goUp();
                        return true;
                    }
                }
            }
        }
    }

    if (m_keyHandler && m_keyHandler->handleEvent(obj, event)) return true;
    return QFrame::eventFilter(obj, event);
}
=======
bool ContentPanel::eventFilter(QObject* obj, QEvent* event) {
    // 专门用于捕获并交由 ContentKeyHandler 处理全局按键快捷键（Ctrl+C/V/F2/Delete等）
    if (m_keyHandler && m_keyHandler->handleEvent(obj, event)) return true;
    return QFrame::eventFilter(obj, event);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::applySort() {
    if (m_sortController) {
        m_sortController->applySortToModel(m_proxyModel);
        if (m_columnView) {
            m_columnView->applySort(static_cast<int>(m_sortController->sortType()), m_sortController->sortOrder());
        }
    }
}
=======
void ContentPanel::applySort() {
    if (m_sortController) {
        m_sortController->applySortToModel(m_proxyModel);
        if (m_columnView) {
            m_columnView->applySort(static_cast<int>(m_sortController->sortType()), m_sortController->sortOrder());
        }
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::refreshAll() {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView) m_columnView->refreshActiveColumn();
        return;
    }
    if (!m_currentPath.isEmpty() && m_currentPath != "computer://") loadDirectory(m_currentPath, m_isRecursive);
    else loadDirectory("computer://");
}
=======
void ContentPanel::refreshAll() {
    if (m_currentViewMode == ColumnView) {
        if (m_columnView) m_columnView->refreshAllColumns();
        return;
    }
    if (!m_currentPath.isEmpty() && m_currentPath != "computer://") loadDirectory(m_currentPath, m_isRecursive);
    else loadDirectory("computer://");
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

### 4.1 编译验证命令
在终端运行以下 CMake 命令进行编译：
```bash
cmake -B build -S .
cmake --build build --config Release
```

### 4.2 逻辑功能验证
1. **快捷键通道验证**：在列视图（ColumnView）中选中文件，测试 `Ctrl+C`、`Ctrl+V`、`Delete`、`F2`，确认快捷键响应完好无损。
2. **多列连通刷新验证**：在第 3 列或第 4 列中按下 `Ctrl+V` 粘贴或执行删除，确认全链条所有列（1-4列）全自动毫秒级刷新。
3. **多列排序精准分发验证**：在状态栏或快捷键切换升降序/按名称/按时间排序，确认 `ContentPanel::applySort()` 被触发后，列视图中所有已展开的列（1-4列）均实时响应并按最新规则重新排序！
4. **精准双击回退验证**：在第 2 列双击空白，确认调用 `dismissSubColumns(1)`，精准只关闭第 3、4 列，保留第 2 列及第 1 列的高亮。
