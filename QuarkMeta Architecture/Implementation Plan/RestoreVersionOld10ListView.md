# Restore Version-Old-10 ListView Architecture (RestoreVersionOld10ListView.md)

## Overview
This implementation plan restores the **ListView (列表模式)** in `ContentPanel` to the exact `Version-Old-10` pure single-view architecture:
1. Removes `ListViewSectionContainer` dual-list wrapper from `ContentPanel` in favor of a single, native `DropTreeView` control (`m_treeView`).
2. Guarantees 100% single global vertical scrollbar behavior with zero scrollbar nesting or selection sync issues.
3. Preserves all exact Version-Old-10 parameters: alternating row colors, `ExtendedSelection`, `NoEditTriggers`, `NoFrame`, and `TreeItemDelegate(this, true, true)`.

## Modified Files List
1. `src/ui/ContentPanel.h`
2. `src/ui/ContentPanel.cpp`
3. `CMakeLists.txt` (Clean up unused `ListViewSectionContainer`)

## Detailed Line-by-Line Changes

### 1. `src/ui/ContentPanel.h`
Revert `m_listContainer` forward declaration and pointer member.

<<<<<<< SEARCH
    class ListViewSectionContainer* m_listContainer = nullptr;
    DropTreeView* m_treeView = nullptr;
=======
    DropTreeView* m_treeView = nullptr;
>>>>>>> REPLACE

### 2. `src/ui/ContentPanel.cpp`
Restore `initListView()`, `activeItemView()`, `getSelectedIndexes()`, and `setViewMode()` to exact Version-Old-10 single-`m_treeView` logic.

<<<<<<< SEARCH
#include "ListViewSectionContainer.h"
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_listContainer);
    m_viewStack->addWidget(m_columnView);
=======
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);
    m_viewStack->addWidget(m_columnView);
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::initListView() {
    m_listContainer = new ListViewSectionContainer(this, this);
    m_listContainer->setModel(m_proxyModel);
    m_treeView = m_listContainer->folderListView(); // 保留主引用指针
    m_viewStack->addWidget(m_listContainer);

    connect(m_listContainer, &ListViewSectionContainer::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_listContainer, &ListViewSectionContainer::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_listContainer, &ListViewSectionContainer::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_listContainer, &ListViewSectionContainer::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex, QAbstractItemModel* sourceModel) {
        onPathsDropped(paths, targetIndex, currentPath(), sourceModel);
    });
}
=======
void ContentPanel::initListView() {
    m_treeView = new DropTreeView(this);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_treeView->setModel(m_proxyModel);
    m_treeView->installEventFilter(this);
    m_treeView->viewport()->installEventFilter(this);

    connect(m_treeView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ContentPanel::onSelectionChanged);
    connect(m_treeView, &QAbstractItemView::customContextMenuRequested, this, &ContentPanel::onCustomContextMenuRequested);
    connect(m_treeView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        onPathsDropped(paths, targetIndex, currentPath(), m_proxyModel);
    });
}
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_currentViewMode == ListView && m_listContainer) {
        m_listContainer->applyFilters(m_currentFilter);
    }
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_currentViewMode == ListView && m_listContainer) {
        m_listContainer->toggleFolderSectionCollapse();
        return;
    }
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_listContainer);
=======
    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_currentViewMode == ListView && m_listContainer) {
        if (m_listContainer->folderListView()->hasFocus() || m_listContainer->folderListView()->selectionModel()->hasSelection()) {
            return m_listContainer->folderListView();
        }
        if (m_listContainer->fileListView()->hasFocus() || m_listContainer->fileListView()->selectionModel()->hasSelection()) {
            return m_listContainer->fileListView();
        }
        return m_listContainer->folderListView();
    }
=======
    if (m_currentViewMode == ListView) {
        return m_treeView;
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    } else if (m_currentViewMode == ListView && m_listContainer) {
        if (m_listContainer->folderListView()) views << m_listContainer->folderListView();
        if (m_listContainer->fileListView()) views << m_listContainer->fileListView();
    } else { // GridView / JustifiedViewMode
=======
    } else if (m_currentViewMode == ListView) {
        if (m_treeView) views << m_treeView;
    } else { // GridView / JustifiedViewMode
>>>>>>> REPLACE

<<<<<<< SEARCH
    } else if (m_currentViewMode == ListView) {
        m_viewStack->setCurrentWidget(m_listContainer);
    } else {
        m_viewStack->setCurrentWidget(m_gridView);
    }
=======
    } else {
        m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? static_cast<QWidget*>(m_treeView) : static_cast<QWidget*>(m_gridView));
    }
>>>>>>> REPLACE

### 3. `CMakeLists.txt`
Remove `ListViewSectionContainer.cpp` and `ListViewSectionContainer.h` from build target.

<<<<<<< SEARCH
    src/ui/ListViewSectionContainer.cpp
    src/ui/ListViewSectionContainer.h
=======
>>>>>>> REPLACE

## Build & Verification Steps
1. Rebuild project with CMake.
2. Verify ListView mode restores single `DropTreeView` with exact Version-Old-10 styling and a single vertical scrollbar.
3. Confirm selection, double click, drag and drop, and context menus work seamlessly for both folders and files.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Fully reused `m_proxyModel` and `m_treeView` without creating unnecessary wrapper models.

## Header API Signature Verification
- `ContentPanel::m_treeView` in `src/ui/ContentPanel.h`
- `DropTreeView::setModel(QAbstractItemModel*)` in `src/ui/DropTreeView.h`
