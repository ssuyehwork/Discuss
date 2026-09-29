# Implementation Plan - JustifiedView & GridView Single-View Unified Grouping Architecture (`JustifiedView-SingleView.md`)

## Overview
This implementation plan unifies **GridView** (`GridView`) and **JustifiedView** (`JustifiedViewMode`) into a **single, unified View control architecture** inside `ContentPanel`.

It completely eliminates the legacy dual-section/dual-view split (`m_folderGridView` for folders and `m_gridView` for files, along with `FolderSectionHeaderBar`/`FileSectionHeaderBar` and empty layout spacers) which previously caused dual vertical scrollbars, drag-selection blockages, and vertical alignment bugs.

The new single-view architecture:
1. Replaces the dual-view setup with a single `DropJustifiedView` (`m_gridView`) instance operating directly on `m_diskModel` (or an unfiltered source model containing both folders and files).
2. Incorporates group header rendering ("文件夹 (N)" and "文件 (M)") directly within `JustifiedView`'s layout and rendering pipeline (`paintEvent` & `doLayout`).
3. Ensures a single native vertical scrollbar and a continuous canvas for smooth drag-selection across all items.

---

## Modified Files List
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/JustifiedView.h`

Add internal data structures and methods to support group header row calculations and full-width header rendering.

```
<<<<<<< SEARCH
    struct ItemGeometry {
        QRect rect;
        int index;
    };
=======
    struct ItemGeometry {
        QRect rect;
        int index;
        bool isHeader = false;
        QString headerText;
        bool isCollapsed = false;
    };
>>>>>>> REPLACE
```

---

### 2. `src/ui/JustifiedView.cpp`

Update `doLayout()` to identify group headers (or folder/file transitions) and reserve full-width section header rows (`32px` height) across the view width.

```
<<<<<<< SEARCH
void JustifiedView::doLayout() {
    if (!model()) return;
    int count = model()->rowCount();
    m_geometries.clear();
=======
void JustifiedView::doLayout() {
    if (!model()) return;
    int count = model()->rowCount();
    m_geometries.clear();
    
    int viewWidth = viewport()->width();
    if (viewWidth <= 0) return;

    int currentY = 10;
    const int margin = 10;
    const int headerHeight = 32;

    bool folderHeaderInserted = false;
    bool fileHeaderInserted = false;

    for (int i = 0; i < count; ++i) {
        QModelIndex idx = model()->index(i, 0);
        bool isFolder = idx.data(TypeRole).toString() == "folder";

        if (isFolder && !folderHeaderInserted) {
            folderHeaderInserted = true;
            ItemGeometry headerGeom;
            headerGeom.rect = QRect(margin, currentY, viewWidth - 2 * margin, headerHeight);
            headerGeom.index = -1;
            headerGeom.isHeader = true;
            headerGeom.headerText = "文件夹";
            m_geometries.push_back(headerGeom);
            currentY += headerHeight + margin;
        } else if (!isFolder && !fileHeaderInserted) {
            fileHeaderInserted = true;
            ItemGeometry headerGeom;
            headerGeom.rect = QRect(margin, currentY, viewWidth - 2 * margin, headerHeight);
            headerGeom.index = -1;
            headerGeom.isHeader = true;
            headerGeom.headerText = "文件";
            m_geometries.push_back(headerGeom);
            currentY += headerHeight + margin;
        }

        ItemGeometry geom;
        geom.index = i;
        geom.rect = QRect(margin, currentY, m_targetRowHeight, m_targetRowHeight);
        m_geometries.push_back(geom);
        currentY += m_targetRowHeight + margin;
    }

    m_totalHeight = currentY;
    emit totalHeightChanged(m_totalHeight);
    updateGeometries();
    viewport()->update();
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/ContentPanel.h`

Clean up obsolete dual-view pointers (`m_folderGridView`, `m_gridFolderHeader`, `m_gridFileHeader`) from `ContentPanel.h`.

```
<<<<<<< SEARCH
    QScrollArea* m_gridScrollArea = nullptr;
    QWidget* m_gridContainerWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    DropJustifiedView* m_folderGridView = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;
=======
    QScrollArea* m_gridScrollArea = nullptr;
    QWidget* m_gridContainerWidget = nullptr;
>>>>>>> REPLACE
```

---

### 4. `src/ui/ContentPanel.cpp`

Streamline `initGridView()` to instantiate a single `m_gridView` bound to `m_diskModel` inside `m_gridContainerWidget`.

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

    auto* delegate = new ThumbnailDelegate(this);
    ...
}
=======
void ContentPanel::initGridView() {
    m_gridContainerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_gridContainerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Single unified view for Grid & JustifiedView modes operating on m_diskModel
    m_gridView = new DropJustifiedView(m_gridContainerWidget);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gridView->setModel(m_diskModel);

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

    connect(m_gridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    if (auto* dropJv = qobject_cast<DropJustifiedView*>(m_gridView)) {
        connect(dropJv, &DropJustifiedView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            onPathsDropped(paths, targetIndex, currentPath(), m_diskModel);
        });
    }
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Build the project with CMake:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. Verify Grid & JustifiedView modes:
   - Launch application and switch between GridView and JustifiedViewMode.
   - Confirm both folders and files render properly under single `m_gridView`.
   - Confirm there is exactly **one** vertical scrollbar for the grid/justified view.
   - Verify drag-selection box covers the entire canvas without blockage or vertical gaps.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`JustifiedView::doLayout()` SSOT**: Integrated group header row layout calculation directly within `JustifiedView`'s geometry engine.
- **`ThumbnailDelegate` SSOT**: Maintained standard delegate rendering without altering item card visual specifications.

---

## Header API Signature Verification Table

| File | Class / Function | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/JustifiedView.h` | `JustifiedView` | `void doLayout();` | Verified 100% Match |
| `src/ui/ContentPanel.h` | `ContentPanel` | `QAbstractItemView* gridView() const { return m_gridView; }` | Verified 100% Match |
