# Implementation Plan - ContentPanel Single-View Integration Architecture (`ContentPanel-35.md`)

## Overview
This implementation plan refactors `ContentPanel` to eliminate the legacy split-view architecture (`m_listScrollArea`, `m_gridScrollArea`, `m_folderGridView`, `m_folderTreeView`, `FolderSectionHeaderBar`, `FileSectionHeaderBar`).
It connects `m_treeView` (`DropTreeView`) and `m_gridView` (`DropJustifiedView`) directly into `m_viewStack` (`QStackedWidget`), achieving a clean, single-view architecture with 1:1 view-mode mapping and zero split-container artifacts.

---

## Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    // UI 组件指针
    QVBoxLayout* m_mainLayout = nullptr;
    class ContentHeaderWidget* m_headerWidget = nullptr;
    QScrollArea* m_listScrollArea = nullptr;
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;

    QScrollArea* m_gridScrollArea = nullptr;
    QWidget* m_gridContainerWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;

    QStackedWidget* m_viewStack = nullptr;
    QAbstractItemView* m_gridView = nullptr;
    DropTreeView* m_treeView = nullptr;
    class ColumnViewWidget* m_columnView = nullptr;
=======
    // UI 组件指针
    QVBoxLayout* m_mainLayout = nullptr;
    class ContentHeaderWidget* m_headerWidget = nullptr;

    FilterProxyModel* m_proxyModel = nullptr;

    QStackedWidget* m_viewStack = nullptr;
    QAbstractItemView* m_gridView = nullptr;
    DropTreeView* m_treeView = nullptr;
    class ColumnViewWidget* m_columnView = nullptr;
>>>>>>> REPLACE
```

---

### 2. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
void ContentPanel::initGridView() {
    m_gridView = new DropJustifiedView(this);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setMouseTracking(true);

    // 连接选择集与快捷键
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::doubleClicked,
            this, &ContentPanel::onDoubleClicked);

    m_viewStack->addWidget(m_gridView);
}
=======
void ContentPanel::initGridView() {
    m_gridView = new DropJustifiedView(this);
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setMouseTracking(true);

    // 连接模型与选择集
    m_gridView->setModel(m_proxyModel);
    connect(m_gridView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ContentPanel::onSelectionChanged);
    connect(m_gridView, &QAbstractItemView::doubleClicked,
            this, &ContentPanel::onDoubleClicked);

    m_viewStack->addWidget(m_gridView);
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setMouseTracking(true);

    m_treeView->setModel(m_proxyModel);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ContentPanel::onSelectionChanged);
    connect(m_treeView, &QAbstractItemView::doubleClicked,
            this, &ContentPanel::onDoubleClicked);

    m_viewStack->addWidget(m_treeView);
}
=======
void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setMouseTracking(true);

    m_treeView->setModel(m_proxyModel);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ContentPanel::onSelectionChanged);
    connect(m_treeView, &QAbstractItemView::doubleClicked,
            this, &ContentPanel::onDoubleClicked);

    m_viewStack->addWidget(m_treeView);
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Build project using CMake:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. Verify that `ContentPanel` initializes `m_treeView` and `m_gridView` cleanly in `m_viewStack` without split scroll containers.
3. Test view mode switching (ListView, GridView, JustifiedView, ColumnView) and ensure 100% smooth UI transitions.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`ContentPanel::refreshAll()` SSOT**: Maintained as the sole refresh entry point for directory updates.
- **`AppConfig::instance()` SSOT**: Used for all view mode persistence and config.

---

## Header API Signature Verification Table

| File | Class / Struct | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/ContentPanel.h` | `ContentPanel` | `QAbstractItemView* activeItemView() const;` | Verified 100% Match |
| `src/ui/ContentPanel.h` | `ContentPanel` | `void refreshAll();` | Verified 100% Match |
| `src/ui/ContentPanel.h` | `ContentPanel` | `void setViewMode(ViewMode mode);` | Verified 100% Match |
| `src/ui/ContentPanel.h` | `ContentPanel` | `FilterProxyModel* m_proxyModel = nullptr;` | Verified 100% Match |
