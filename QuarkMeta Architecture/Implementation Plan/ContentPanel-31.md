# Implementation Plan - ContentPanel (Unified View Single View Refactoring)

## 1. Overview
This implementation plan deprecates the dual-subview partitioning (`m_folderGridView` / `m_folderTreeView` alongside `m_gridView` / `m_treeView`) in `ContentPanel`. It refactors content presentation to use a single unified parent view (`m_gridView` for grid mode, `m_treeView` for list mode) powered by a single filter proxy model (`m_proxyModel`), while retaining the folder/file section header bars and collapse/expand behavior for sectioning.

By eliminating secondary subviews, this refactoring completely removes nested view scrolling/geometry synchronization issues, simplifies selection and focus management, and eliminates zero-height layout collapses.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/ContentPanel.h`
Remove dual subview pointers (`m_folderGridView`, `m_folderTreeView`, `m_folderProxyModel`, `m_fileProxyModel`) and replace with a single unified proxy model `m_proxyModel`.

```
<<<<<<< SEARCH
    QScrollArea* m_listScrollArea = nullptr;
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    DropTreeView* m_folderTreeView = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    QScrollArea* m_gridScrollArea = nullptr;
    QWidget* m_gridContainerWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    DropJustifiedView* m_folderGridView = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;
=======
    QScrollArea* m_listScrollArea = nullptr;
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;

    QScrollArea* m_gridScrollArea = nullptr;
    QWidget* m_gridContainerWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/ContentPanel.cpp`
In `initListView()` and `initGridView()`, instantiate a single `m_proxyModel` that drives both `m_treeView` and `m_gridView`. Remove secondary subview creation (`m_folderTreeView`, `m_folderGridView`).

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

        connect(fJustifiedView, &JustifiedView::totalHeightChanged, this, [this](int height) {
            if (!m_folderGridView || !m_folderProxyModel) return;
            if (m_folderProxyModel->rowCount() > 0 && height > 0) {
                m_folderGridView->setMinimumHeight(height);
                m_folderGridView->setMaximumHeight(height);
            } else {
                m_folderGridView->setMinimumHeight(0);
                m_folderGridView->setMaximumHeight(QWIDGETSIZE_MAX);
            }
        });
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
                m_folderGridView->setMinimumHeight(0);
                m_folderGridView->setMaximumHeight(QWIDGETSIZE_MAX);
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
                    int h = fjv->totalHeight();
                    if (h > 0) {
                        m_folderGridView->setMinimumHeight(h);
                        m_folderGridView->setMaximumHeight(h);
                    } else {
                        m_folderGridView->setMinimumHeight(0);
                        m_folderGridView->setMaximumHeight(QWIDGETSIZE_MAX);
                    }
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
    m_gridContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_gridContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 1. 文件夹折叠标题栏
    m_gridFolderHeader = new FolderSectionHeaderBar(m_gridContainerWidget);
    m_gridFolderHeader->hide();
    layout->addWidget(m_gridFolderHeader);

    // 2. 文件分界标题栏
    m_gridFileHeader = new FileSectionHeaderBar(m_gridContainerWidget);
    m_gridFileHeader->hide();
    layout->addWidget(m_gridFileHeader);

    // 3. 统一网格视图（主视图）
    m_gridView = new DropJustifiedView(m_gridContainerWidget);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setModel(m_proxyModel);

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

    connect(m_gridFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_proxyModel) {
            FilterState state = m_currentFilter;
            state.showFolders = !collapsed;
            m_proxyModel->currentFilter = state;
            m_proxyModel->updateFilter();
        }
    });

    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    if (auto* dropJv = qobject_cast<DropJustifiedView*>(m_gridView)) {
        connect(dropJv, &DropJustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            onPathsDropped(paths, targetIndex, currentPath(), m_proxyModel);
        });
    }

    auto updateGridSectionCounts = [this]() {
        if (!m_model) return;
        int folderCount = 0;
        int fileCount = 0;
        const auto& records = m_model->allRecords();
        for (const auto& rec : records) {
            if (rec.isDir) folderCount++;
            else fileCount++;
        }

        if (m_gridFolderHeader) {
            m_gridFolderHeader->setCount(folderCount);
            m_gridFolderHeader->setVisible(folderCount > 0);
        }
        if (m_gridFileHeader) {
            m_gridFileHeader->setCount(fileCount);
            m_gridFileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }
    };

    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, updateGridSectionCounts);
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, updateGridSectionCounts);
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
   - Open `QuarkMeta` in Grid and List views for folders containing subfolders and files.
   - Confirm that single `m_gridView` and `m_treeView` display all items seamlessly without double scrollbars or 0px viewport collapses.
   - Click the Folder section header bar collapse button and verify that folders collapse/expand smoothly by updating `m_proxyModel` filtering.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: Reused `FilterProxyModel`, `FolderSectionHeaderBar`, and `FileSectionHeaderBar`.
- **Anti-Redundancy**: Completely removed duplicate subviews (`m_folderGridView`, `m_folderTreeView`) and duplicate proxy models, reducing memory overhead and view syncing complexity.

---

## 6. Header API Signature Verification

| Header File | Class Name | Verified Signature |
| :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `ContentPanel` | `FilterProxyModel* m_proxyModel = nullptr;` |
| `src/ui/models/FilterProxyModel.h` | `FilterProxyModel` | `void updateFilter()` |
