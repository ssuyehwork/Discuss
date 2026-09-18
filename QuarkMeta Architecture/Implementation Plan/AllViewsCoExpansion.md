# QuarkMeta 实施方案：全视图（网格/流式/列表）联合撑开与统一滚动 (AllViewsCoExpansion)

## 1. Overview（概述与解决的问题）
- **问题**：网格视图（GridView/JustifiedView）与列表视图（ListView）中，普通文件区域（`m_gridView` 与 `m_treeView`）未进行物理高度撑开，导致当文件夹少、文件极大时，外层 `QScrollArea` 不超高，文件仍在底部局部滚动，折叠栏无法整页向上流动。
- **解法**：
  1. 关闭 `m_gridView` 与 `m_treeView` 的私有垂直滚动条；
  2. 在 `updateGridSectionCounts()` 中计算文件的真实折行数并调用 `m_gridView->setFixedHeight(...)`；
  3. 在 `updateListSectionCounts()` 中根据文件行数计算物理高度并调用 `m_treeView->setFixedHeight(...)`；
  4. 让所有视图与列视图完全看齐，全由【文件夹 + 文件】联合决定整页滚动条！

---

## 2. Modified Files List（影响文件清单）
1. `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 修改文件：`src/ui/ContentPanel.cpp`

#### 替换块 A：网格与流式视图文件区域关闭私有滚动、联合撑高

```
<<<<<<< SEARCH
    // 4. 普通文件网格视图
    m_gridView = new DropJustifiedView(m_gridContainerWidget);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setModel(m_fileProxyModel);

    auto* justifiedView = qobject_cast<JustifiedView*>(m_gridView);
    if (justifiedView) {
        justifiedView->setAspectRatioRole(AspectRatioRole);
        auto* delegate = new ThumbnailDelegate(this);
        delegate->setHasThumbnailRole(HasThumbnailRole);
        delegate->setRatingRole(RatingRole);
        delegate->setPathRole(PathRole);
        delegate->setPinnedRole(PinnedRole);
        delegate->setTypeRole(TypeRole);
        delegate->setIsEmptyRole(IsEmptyRole);
        delegate->setColorRole(ColorRole);
        m_gridView->setItemDelegate(delegate);
    }

    m_gridView->installEventFilter(this);
    m_gridView->viewport()->installEventFilter(this);
    layout->addWidget(m_gridView, 1);
=======
    // 4. 普通文件网格视图（关闭私有垂直滚动条）
    m_gridView = new DropJustifiedView(m_gridContainerWidget);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridView->setModel(m_fileProxyModel);

    auto* justifiedView = qobject_cast<JustifiedView*>(m_gridView);
    if (justifiedView) {
        justifiedView->setAspectRatioRole(AspectRatioRole);
        auto* delegate = new ThumbnailDelegate(this);
        delegate->setHasThumbnailRole(HasThumbnailRole);
        delegate->setRatingRole(RatingRole);
        delegate->setPathRole(PathRole);
        delegate->setPinnedRole(PinnedRole);
        delegate->setTypeRole(TypeRole);
        delegate->setIsEmptyRole(IsEmptyRole);
        delegate->setColorRole(ColorRole);
        m_gridView->setItemDelegate(delegate);
    }

    m_gridView->installEventFilter(this);
    m_gridView->viewport()->installEventFilter(this);
    layout->addWidget(m_gridView);
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
        if (m_gridFileHeader) {
            m_gridFileHeader->setCount(fileCount);
            m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
    };
=======
        // 1. 统一计算单张卡片占位宽、单行高度与每行卡片数
        int cardW = m_zoomLevel + CardLayoutEngine::totalPaddingHorizontal() + 10;
        int rowH = m_zoomLevel + CardLayoutEngine::extraHeight() + 10;
        int availableW = m_gridScrollArea && m_gridScrollArea->viewport() ? m_gridScrollArea->viewport()->width() : width();
        int cardsPerRow = qMax(1, availableW / cardW);

        // 2. 文件夹网格动态撑开
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                int folderRows = qMax(1, (folderCount + cardsPerRow - 1) / cardsPerRow);
                m_folderGridView->setFixedHeight(folderRows * rowH + 8);
            }
        }
        if (m_gridFileHeader) {
            m_gridFileHeader->setCount(fileCount);
            m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }

        // 3. 🚀【文件网格同步物理撑开】：彻底消除局部滚动，联合决定整页滚动
        if (m_gridView) {
            if (fileCount == 0) {
                m_gridView->hide();
            } else {
                m_gridView->show();
                int fileRows = qMax(1, (fileCount + cardsPerRow - 1) / cardsPerRow);
                m_gridView->setFixedHeight(fileRows * rowH + 8);
            }
        }

        if (m_gridContainerWidget) {
            m_gridContainerWidget->adjustSize();
        }
    };
>>>>>>> REPLACE
```

#### 替换块 B：列表视图文件区域关闭私有滚动、联合撑高

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
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_treeView->setModel(m_fileProxyModel);
    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);
    layout->addWidget(m_treeView, 1);
=======
    // 6. 文件列表视图（关闭内部垂直滚动条）
    m_treeView = new DropTreeView(m_listContainerWidget);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_treeView->setModel(m_fileProxyModel);
    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);
    layout->addWidget(m_treeView);
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
        if (m_listFileHeader) {
            m_listFileHeader->setCount(fileCount);
            m_listFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
    };
=======
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
        if (m_listFileHeader) {
            m_listFileHeader->setCount(fileCount);
            m_listFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
        // 🚀【文件列表同步物理撑高】：消除局部内卷，联合撑大整页
        if (m_treeView) {
            if (fileCount == 0) {
                m_treeView->hide();
            } else {
                m_treeView->show();
                int fileH = qMax(64, fileCount * 30 + 32);
                m_treeView->setFixedHeight(fileH);
            }
        }

        if (m_listContainerWidget) {
            m_listContainerWidget->adjustSize();
        }
    };
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译与验证方法）

### 编译命令
```bash
cmake --build . --config Release --target QuarkMeta
```

### 验证标准
1. **网格与流式视图测试**：
   - 进入文件夹少、文件极大（例如 2 文件夹 + 2489 文件）的目录；
   - 验证：右侧主垂直滚动条立即正确出现，并且滑块比例真实反映 2489 个文件的总量；
   - 滚动滚轮时：顶部的文件夹和分界条自然随内容向上平移滑出屏幕，底部文件不再独立局部滚动！
2. **列表视图测试**：
   - 切换到列表视图，同样验证整页统一垂直滚动，上下平滑移动，行为与列视图 100% 绝对一致！
