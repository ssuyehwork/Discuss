# Implementation Plan - ContentPanel (QSplitter Layout Fix for Grid Views)

## 1. Overview
This implementation plan fixes the layout collapse bug (0px black screen height issue) and resolves the squished file view when directories contain large file/folder counts.

Previously, `ContentPanel` wrapped `m_folderGridView` and `m_gridView` inside a `QScrollArea` (`m_gridScrollArea`) and dynamically called `m_folderGridView->setFixedHeight(height)`. When opening directories with many items (e.g. 2000+ files/folders), layout calculation delays produced `totalHeight = 0`, squishing `m_folderGridView` into a 0px collapsed state and pushing `m_gridView` off-screen.

To fix this issue permanently while preserving Qt viewport virtualization and delegates, this change replaces `m_gridScrollArea` with a vertical `QSplitter` (`m_gridSplitter`). Both `m_folderGridView` and `m_gridView` reside in independent split panes within `m_gridSplitter`, eliminating `setFixedHeight` hacks, preventing layout collapse, and restoring responsive section sizing.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/ContentPanel.h`
Replace `QScrollArea* m_gridScrollArea` and `QWidget* m_gridContainerWidget` with `QSplitter* m_gridSplitter`, `QWidget* m_gridFolderWidget`, and `QWidget* m_gridFileWidget`.

```
<<<<<<< SEARCH
    QScrollArea* m_gridScrollArea = nullptr;
    QWidget* m_gridContainerWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    DropJustifiedView* m_folderGridView = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;
=======
    QSplitter* m_gridSplitter = nullptr;
    QWidget* m_gridFolderWidget = nullptr;
    QWidget* m_gridFileWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    DropJustifiedView* m_folderGridView = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/ContentPanel.cpp`
Update `initUi()`, `updateGridSize()`, and `restoreActiveView()` to use `m_gridSplitter`.

```
<<<<<<< SEARCH
    m_gridScrollArea = new QScrollArea(this);
    m_gridScrollArea->setFrameShape(QFrame::NoFrame);
    m_gridScrollArea->setWidgetResizable(true);
    m_gridScrollArea->setWidget(m_gridContainerWidget);

    m_listScrollArea = new QScrollArea(this);
    m_listScrollArea->setFrameShape(QFrame::NoFrame);
    m_listScrollArea->setWidgetResizable(true);
    m_listScrollArea->setWidget(m_listContainerWidget);

    m_viewStack->addWidget(m_gridScrollArea);
    m_viewStack->addWidget(m_listScrollArea);
    m_viewStack->addWidget(m_columnView);
    m_viewStack->setCurrentWidget(m_gridScrollArea);
=======
    m_gridSplitter = new QSplitter(Qt::Vertical, this);
    m_gridSplitter->setChildrenCollapsible(false);
    m_gridSplitter->setHandleWidth(4);

    m_listScrollArea = new QScrollArea(this);
    m_listScrollArea->setFrameShape(QFrame::NoFrame);
    m_listScrollArea->setWidgetResizable(true);
    m_listScrollArea->setWidget(m_listContainerWidget);

    m_viewStack->addWidget(m_gridSplitter);
    m_viewStack->addWidget(m_listScrollArea);
    m_viewStack->addWidget(m_columnView);
    m_viewStack->setCurrentWidget(m_gridSplitter);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::updateGridSize() {
    if (m_viewStack->currentWidget() == m_gridScrollArea) {
=======
void ContentPanel::updateGridSize() {
    if (m_viewStack->currentWidget() == m_gridSplitter) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::restoreActiveView() {
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? (m_listScrollArea ? static_cast<QWidget*>(m_listScrollArea) : static_cast<QWidget*>(m_treeView)) : (m_gridScrollArea ? static_cast<QWidget*>(m_gridScrollArea) : static_cast<QWidget*>(m_gridView)));
    }
}
=======
void ContentPanel::restoreActiveView() {
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? (m_listScrollArea ? static_cast<QWidget*>(m_listScrollArea) : static_cast<QWidget*>(m_treeView)) : (m_gridSplitter ? static_cast<QWidget*>(m_gridSplitter) : static_cast<QWidget*>(m_gridView)));
    }
}
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/ContentPanel.cpp`
Refactor `initGridView()` to organize folder and file grid views into `m_gridFolderWidget` and `m_gridFileWidget` inside `m_gridSplitter`. Remove `setFixedHeight` height forcing logic.

```
<<<<<<< SEARCH
void ContentPanel::initGridView() {
    m_gridContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_gridContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 1. 文件夹折叠标题栏
    m_gridFolderHeader = new FolderSectionHeaderBar(m_gridContainerWidget);
    m_gridFolderHeader->hide();
    layout->addWidget(m_gridFolderHeader);

    // 2. 文件夹专用网格视图
    m_folderGridView = new DropJustifiedView(m_gridContainerWidget);
    m_folderGridView->setFrameShape(QFrame::NoFrame);
    m_folderGridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderGridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderGridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderGridView->setModel(m_folderProxyModel);
    auto* fJustifiedView = qobject_cast<JustifiedView*>(m_folderGridView);
    if (fJustifiedView) {
        fJustifiedView->setAspectRatioRole(AspectRatioRole);
        auto* fDelegate = new ThumbnailDelegate(this);
        fDelegate->setHasThumbnailRole(HasThumbnailRole);
        fDelegate->setRatingRole(RatingRole);
        fDelegate->setPathRole(PathRole);
        fDelegate->setPinnedRole(PinnedRole);
        fDelegate->setTypeRole(TypeRole);
        fDelegate->setIsEmptyRole(IsEmptyRole);
        fDelegate->setColorRole(ColorRole);
        m_folderGridView->setItemDelegate(fDelegate);
    }
    m_folderGridView->installEventFilter(this);
    m_folderGridView->viewport()->installEventFilter(this);
    m_folderGridView->hide();
    layout->addWidget(m_folderGridView);

    connect(m_gridFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderGridView && m_gridFolderHeader->count() > 0) {
            m_folderGridView->setVisible(!collapsed);
        }
    });

    // 3. 文件分界标题栏
    m_gridFileHeader = new FileSectionHeaderBar(m_gridContainerWidget);
    m_gridFileHeader->hide();
    layout->addWidget(m_gridFileHeader);

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

    if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
        connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int height) {
            if (m_folderGridView && m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                m_folderGridView->setFixedHeight(height);
            }
        });
    }

    connect(m_folderGridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_folderGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_folderGridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_folderGridView, &DropJustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_folderProxyModel);
    });

    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    if (auto* dropJv = qobject_cast<DropJustifiedView*>(m_gridView)) {
        connect(dropJv, &DropJustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            onPathsDropped(paths, targetIndex, currentPath(), m_fileProxyModel);
        });
    }

    auto updateGridSectionCounts = [this]() {
        if (!m_folderProxyModel || !m_fileProxyModel) return;
        int folderCount = m_folderProxyModel->rowCount();
        int fileCount = m_fileProxyModel->rowCount();

        if (m_gridFolderHeader) {
            m_gridFolderHeader->setCount(folderCount);
            m_gridFolderHeader->setVisible(folderCount > 0);
        }
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
        if (m_gridFileHeader) {
            m_gridFileHeader->setCount(fileCount);
            m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
    };

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
}
=======
void ContentPanel::initGridView() {
    // 1. 文件夹分栏 Widget 容器
    m_gridFolderWidget = new QWidget(this);
    auto* folderLayout = new QVBoxLayout(m_gridFolderWidget);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    folderLayout->setSpacing(0);

    m_gridFolderHeader = new FolderSectionHeaderBar(m_gridFolderWidget);
    m_gridFolderHeader->hide();
    folderLayout->addWidget(m_gridFolderHeader);

    m_folderGridView = new DropJustifiedView(m_gridFolderWidget);
    m_folderGridView->setFrameShape(QFrame::NoFrame);
    m_folderGridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_folderGridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderGridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderGridView->setModel(m_folderProxyModel);
    auto* fJustifiedView = qobject_cast<JustifiedView*>(m_folderGridView);
    if (fJustifiedView) {
        fJustifiedView->setAspectRatioRole(AspectRatioRole);
        auto* fDelegate = new ThumbnailDelegate(this);
        fDelegate->setHasThumbnailRole(HasThumbnailRole);
        fDelegate->setRatingRole(RatingRole);
        fDelegate->setPathRole(PathRole);
        fDelegate->setPinnedRole(PinnedRole);
        fDelegate->setTypeRole(TypeRole);
        fDelegate->setIsEmptyRole(IsEmptyRole);
        fDelegate->setColorRole(ColorRole);
        m_folderGridView->setItemDelegate(fDelegate);
    }
    m_folderGridView->installEventFilter(this);
    m_folderGridView->viewport()->installEventFilter(this);
    m_folderGridView->hide();
    folderLayout->addWidget(m_folderGridView, 1);

    connect(m_gridFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderGridView && m_gridFolderHeader->count() > 0) {
            m_folderGridView->setVisible(!collapsed);
        }
    });

    // 2. 文件分栏 Widget 容器
    m_gridFileWidget = new QWidget(this);
    auto* fileLayout = new QVBoxLayout(m_gridFileWidget);
    fileLayout->setContentsMargins(0, 0, 0, 0);
    fileLayout->setSpacing(0);

    m_gridFileHeader = new FileSectionHeaderBar(m_gridFileWidget);
    m_gridFileHeader->hide();
    fileLayout->addWidget(m_gridFileHeader);

    m_gridView = new DropJustifiedView(m_gridFileWidget);
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
    fileLayout->addWidget(m_gridView, 1);

    if (m_gridSplitter) {
        m_gridSplitter->addWidget(m_gridFolderWidget);
        m_gridSplitter->addWidget(m_gridFileWidget);
        m_gridSplitter->setStretchFactor(0, 1);
        m_gridSplitter->setStretchFactor(1, 2);
    }

    connect(m_folderGridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_folderGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_folderGridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_folderGridView, &DropJustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_folderProxyModel);
    });

    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    if (auto* dropJv = qobject_cast<DropJustifiedView*>(m_gridView)) {
        connect(dropJv, &DropJustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            onPathsDropped(paths, targetIndex, currentPath(), m_fileProxyModel);
        });
    }

    auto updateGridSectionCounts = [this]() {
        if (!m_folderProxyModel || !m_fileProxyModel) return;
        int folderCount = m_folderProxyModel->rowCount();
        int fileCount = m_fileProxyModel->rowCount();

        if (m_gridFolderHeader) {
            m_gridFolderHeader->setCount(folderCount);
            m_gridFolderHeader->setVisible(folderCount > 0);
        }
        if (m_gridFolderWidget) {
            m_gridFolderWidget->setVisible(folderCount > 0);
        }
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
            }
        }
        if (m_gridFileHeader) {
            m_gridFileHeader->setCount(fileCount);
            m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
    };

    connect(m_folderProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_folderProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_fileProxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Build Verification**:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
2. **Behavior Verification**:
   - Open `QuarkMeta` in grid view for a directory containing both subfolders and 2000+ files (e.g. `H:\测试`).
   - Confirm that `m_folderGridView` and `m_gridView` render within their split panes without collapsing to 0px or creating black viewports.
   - Confirm that dragging the `QSplitter` handle dynamically resizes both section views.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Retained existing models, proxies, delegates (`ThumbnailDelegate`), and event filters.
- **Anti-Redundancy**: Removed `setFixedHeight(...)` calls, eliminating manual layout height forcing and timing conflicts.

---

## 6. Header API Signature Verification

| Header File | Class Name | Verified Signature |
| :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `ContentPanel` | `QSplitter* m_gridSplitter = nullptr;` |
| `<QSplitter>` | `QSplitter` | `explicit QSplitter(Qt::Orientation orientation, QWidget *parent = nullptr)` |
| `<QSplitter>` | `QSplitter` | `void addWidget(QWidget *widget)` |
| `<QSplitter>` | `QSplitter` | `void setStretchFactor(int index, int stretch)` |
| `<QSplitter>` | `QSplitter` | `void setHandleWidth(int width)` |
