# Implementation Plan - AllViewsCoExpansion.md

## 1. Overview
Unify all four view modes (`GridView`, `JustifiedViewMode`, `ListView`, and `ColumnView`) under a single cohesive layout and scrolling architecture.

Currently, `ContentPanel`'s `GridView`/`JustifiedViewMode` (`m_gridView`) and `ListView` (`m_treeView`) keep internal vertical scrollbars enabled and do not expand to fit their full content height. This creates a disjointed user experience where scrolling only moves the file section, while folder section headers remain static, unlike `ColumnView` where the entire pane scrolls as a single smooth canvas.

This plan unifies all view modes to match `ColumnView`'s canvas architecture:
1. Turn off internal vertical scrollbars on `m_folderGridView`, `m_gridView`, `m_folderTreeView`, and `m_treeView` (`setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff)`).
2. Connect `m_gridView` and `m_folderGridView` to `JustifiedView::totalHeightChanged` to automatically drive `setFixedHeight(m_totalHeight)`.
3. In `ListView` mode, calculate exact content height for `m_folderTreeView` and `m_treeView` using row heights (`sizeHintForRow(0)` plus `header()->height()`) and update `setFixedHeight` dynamically.
4. Delegate all vertical scrolling exclusively to outer containers (`m_gridScrollArea` and `m_listScrollArea`), allowing headers and views to scroll together as a single unified canvas.

---

## 2. Modified Files List
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentPanel.cpp`

#### Change 1: Grid / Justified View setup
Disable internal vertical scrollbars for `m_folderGridView` and `m_gridView`. Connect `m_gridView->totalHeightChanged` to automatically drive `setFixedHeight`.

```
<<<<<<< SEARCH
    // 2. 文件夹专用网格视图
    m_folderGridView = new DropJustifiedView(m_gridContainerWidget);
    m_folderGridView->setFrameShape(QFrame::NoFrame);
    m_folderGridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderGridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderGridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderGridView->setModel(m_folderProxyModel);
=======
    // 2. 文件夹专用网格视图
    m_folderGridView = new DropJustifiedView(m_gridContainerWidget);
    m_folderGridView->setFrameShape(QFrame::NoFrame);
    m_folderGridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderGridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderGridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderGridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderGridView->setModel(m_folderProxyModel);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 4. 普通文件网格视图
    m_gridView = new DropJustifiedView(m_gridContainerWidget);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setModel(m_fileProxyModel);
=======
    // 4. 普通文件网格视图
    m_gridView = new DropJustifiedView(m_gridContainerWidget);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridView->setModel(m_fileProxyModel);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
=======
    if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
        connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int h) {
            if (m_folderGridView && m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                m_folderGridView->setFixedHeight(h);
            }
        });
    }
    if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
        connect(jv, &JustifiedView::totalHeightChanged, this, [this](int h) {
            if (m_gridView && m_fileProxyModel && m_fileProxyModel->rowCount() > 0) {
                m_gridView->setFixedHeight(h);
            }
        });
    }

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                // 1. 计算单张卡片占位宽与单行高度
                int cardW = m_zoomLevel + CardLayoutEngine::totalPaddingHorizontal() + 10;
                int rowH = m_zoomLevel + CardLayoutEngine::extraHeight() + 10;

                // 2. 根据当前视口可用宽度，动态计算一行实际放几张卡
                int availableW = m_folderGridView->width() > 100 ? m_folderGridView->width() : width();
                int cardsPerRow = qMax(1, availableW / cardW);

                // 3. 向上取整计算真实行数：6 个项目 / 8 列 = 1 行，绝不多算
                int rows = qMax(1, (folderCount + cardsPerRow - 1) / cardsPerRow);
                m_folderGridView->setFixedHeight(rows * rowH + 8);
            }
        }
=======
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
                    m_folderGridView->setFixedHeight(fjv->totalHeight());
                }
            }
        }
        if (m_gridView) {
            if (fileCount == 0) {
                m_gridView->hide();
            } else {
                m_gridView->show();
                if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
                    m_gridView->setFixedHeight(jv->totalHeight());
                }
            }
        }
>>>>>>> REPLACE
```

#### Change 2: List View setup
Disable internal vertical scrollbars on `m_folderTreeView` and `m_treeView`. Auto-expand heights using SSOT row heights.

```
<<<<<<< SEARCH
    // 4. 文件夹列表视图
    m_folderTreeView = new DropTreeView(m_listContainerWidget);
    m_folderTreeView->setFrameShape(QFrame::NoFrame);
    m_folderTreeView->setAlternatingRowColors(true);
    m_folderTreeView->setSortingEnabled(true);
    m_folderTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_folderTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
=======
    // 4. 文件夹列表视图
    m_folderTreeView = new DropTreeView(m_listContainerWidget);
    m_folderTreeView->setFrameShape(QFrame::NoFrame);
    m_folderTreeView->setAlternatingRowColors(true);
    m_folderTreeView->setSortingEnabled(true);
    m_folderTreeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_folderTreeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_folderTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 6. 文件列表视图
    m_treeView = new DropTreeView(m_listContainerWidget);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
=======
    // 6. 文件列表视图
    m_treeView = new DropTreeView(m_listContainerWidget);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (m_folderTreeView) {
            if (folderCount == 0) {
                m_folderTreeView->hide();
            } else {
                bool collapsed = m_listFolderHeader ? m_listFolderHeader->isCollapsed() : false;
                m_folderTreeView->setVisible(!collapsed);
                int folderH = qMax(32, folderCount * 30 + 32);
                m_folderTreeView->setFixedHeight(folderH);
            }
        }
=======
        if (m_folderTreeView) {
            if (folderCount == 0) {
                m_folderTreeView->hide();
            } else {
                bool collapsed = m_listFolderHeader ? m_listFolderHeader->isCollapsed() : false;
                m_folderTreeView->setVisible(!collapsed);
                int rowH = m_folderTreeView->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (m_folderTreeView->header() && m_folderTreeView->header()->isVisible()) ? m_folderTreeView->header()->height() : 0;
                int folderH = folderCount * rowH + hdrH + 2;
                m_folderTreeView->setFixedHeight(folderH);
            }
        }
        if (m_treeView) {
            if (fileCount == 0) {
                m_treeView->hide();
            } else {
                m_treeView->show();
                int rowH = m_treeView->sizeHintForRow(0);
                if (rowH <= 0) rowH = 30;
                int hdrH = (m_treeView->header() && m_treeView->header()->isVisible()) ? m_treeView->header()->height() : 0;
                int fileH = fileCount * rowH + hdrH + 2;
                m_treeView->setFixedHeight(fileH);
            }
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure and build:
   ```bash
   cmake -B build -S .
   cmake --build build
   ```
2. Verification:
   - Switch between GridView, JustifiedViewMode, ListView, and ColumnView.
   - Verify that in all view modes, scrolling the mouse wheel over the view smoothly scrolls the entire canvas including section headers and view widgets.
   - Verify that sub-views no longer display independent internal vertical scrollbars.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `JustifiedView::totalHeightChanged` SSOT signal and `totalHeight()` getter.
- Reused `QTreeView::sizeHintForRow(0)` and `QHeaderView::height()` SSOT methods.
- Zero duplicate or split-brain height calculation code introduced.

---

## 6. Header API Signature Verification
- `JustifiedView::totalHeight() const` -> `src/ui/JustifiedView.h`
- `QTreeView::sizeHintForRow(int row) const` -> Qt `QTreeView` API
- `QHeaderView::height() const` -> Qt `QHeaderView` API
