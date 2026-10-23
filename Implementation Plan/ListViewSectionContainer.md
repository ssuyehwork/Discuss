# ListView Section Container Implementation Plan (ListViewSectionContainer.md)

## Overview
This implementation plan establishes the architectural solution for displaying collapsible section headers ("文件夹 (N)" and "文件 (M)") in **ListView (列表模式)** through a **Dual-Section Container View Architecture (`ListViewSectionContainer`)**.

By decoupling presentation from the data model layer:
1. `DiskItemModel` remains 100% pure as the SSOT domain model representing physical disk items, free of dummy UI sentinel rows.
2. Two dedicated, zero-overhead proxy models (`m_folderProxyModel` and `m_fileProxyModel`) filter directories (`showFolders = true, showFiles = false`) and files (`showFolders = false, showFiles = true`) respectively.
3. Two `DropTreeView` instances (`m_folderListView` and `m_fileListView`) render folder and file rows independently with synchronised header column widths.
4. `FolderSectionHeaderBar` and `FileSectionHeaderBar` controls toggle section visibility (`m_folderListView->setVisible(!collapsed)`), providing smooth $O(1)$ animation and ZERO risk of proxy index crashes or selection anomalies.

## Modified Files List
1. `CMakeLists.txt`
2. `src/ui/ListViewSectionContainer.h` (New File)
3. `src/ui/ListViewSectionContainer.cpp` (New File)
4. `src/ui/ContentPanel.h`
5. `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `CMakeLists.txt`
Register `ListViewSectionContainer.h` and `ListViewSectionContainer.cpp` in CMake build sources.

<<<<<<< SEARCH
    src/ui/DropTreeView.cpp
    src/ui/DropTreeView.h
=======
    src/ui/DropTreeView.cpp
    src/ui/DropTreeView.h
    src/ui/ListViewSectionContainer.cpp
    src/ui/ListViewSectionContainer.h
>>>>>>> REPLACE

### 2. `src/ui/ListViewSectionContainer.h` (New File)
Define the dual-section list view container header.

```cpp
#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include "FolderSectionWidget.h"
#include "DropTreeView.h"
#include "models/FilterProxyModel.h"
#include "models/DiskItemModel.h"

namespace QuarkMeta {

class ContentPanel;

/**
 * @brief 列表模式双列表解耦容器：上方文件夹列表 + 下方文件列表
 */
class ListViewSectionContainer : public QWidget {
    Q_OBJECT

public:
    explicit ListViewSectionContainer(ContentPanel* panel, QWidget* parent = nullptr);
    ~ListViewSectionContainer() override = default;

    void setModel(QSortFilterProxyModel* mainProxyModel);
    void applyFilters(const FilterState& state);
    void applySort(int sortType, Qt::SortOrder sortOrder);
    void toggleFolderSectionCollapse();

    DropTreeView* folderListView() const { return m_folderListView; }
    DropTreeView* fileListView() const { return m_fileListView; }

signals:
    void selectionChanged();
    void doubleClicked(const QModelIndex& index);
    void customContextMenuRequested(const QPoint& pos);
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex, QAbstractItemModel* sourceModel);

private:
    void setupUi();
    void syncHeaderColumnWidths();

    ContentPanel* m_panel = nullptr;
    QSortFilterProxyModel* m_mainProxyModel = nullptr;

    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    QVBoxLayout* m_mainLayout = nullptr;
    FolderSectionHeaderBar* m_folderHeader = nullptr;
    DropTreeView* m_folderListView = nullptr;
    FileSectionHeaderBar* m_fileHeader = nullptr;
    DropTreeView* m_fileListView = nullptr;
};

} // namespace QuarkMeta
```

### 3. `src/ui/ListViewSectionContainer.cpp` (New File)
Implement `ListViewSectionContainer` dual-list layout, column width synchronization, and collapse toggling.

```cpp
#include "ListViewSectionContainer.h"
#include "ContentPanel.h"
#include "TreeItemDelegate.h"
#include <QSignalBlocker>

namespace QuarkMeta {

ListViewSectionContainer::ListViewSectionContainer(ContentPanel* panel, QWidget* parent)
    : QWidget(parent), m_panel(panel) {
    setupUi();
}

void ListViewSectionContainer::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(4);

    // 1. 文件夹折叠标题栏
    m_folderHeader = new FolderSectionHeaderBar(this);
    m_mainLayout->addWidget(m_folderHeader);

    // 2. 文件夹专用列表控件
    m_folderListView = new DropTreeView(this);
    m_folderListView->setFrameShape(QFrame::NoFrame);
    m_folderListView->setAlternatingRowColors(true);
    m_folderListView->setSortingEnabled(true);
    m_folderListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_folderListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_folderListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_folderListView->setRootIsDecorated(false);
    m_folderListView->setItemDelegate(new TreeItemDelegate(this, true, true));
    if (m_folderListView->header()) {
        m_folderListView->header()->setFixedHeight(32);
        m_folderListView->header()->setMinimumSectionSize(0);
    }
    m_folderListView->applyColumnPolicies();
    m_mainLayout->addWidget(m_folderListView);

    // 3. 文件折叠标题栏
    m_fileHeader = new FileSectionHeaderBar(this);
    m_mainLayout->addWidget(m_fileHeader);

    // 4. 文件专用列表控件
    m_fileListView = new DropTreeView(this);
    m_fileListView->setFrameShape(QFrame::NoFrame);
    m_fileListView->setAlternatingRowColors(true);
    m_fileListView->setSortingEnabled(true);
    m_fileListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_fileListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileListView->setRootIsDecorated(false);
    m_fileListView->setItemDelegate(new TreeItemDelegate(this, true, true));
    if (m_fileListView->header()) {
        m_fileListView->header()->setFixedHeight(32);
        m_fileListView->header()->setMinimumSectionSize(0);
    }
    m_fileListView->applyColumnPolicies();
    m_mainLayout->addWidget(m_fileListView);

    // 5. 安装 ContentPanel 事件过滤器
    if (m_panel) {
        m_folderListView->installEventFilter(m_panel);
        m_folderListView->viewport()->installEventFilter(m_panel);
        m_fileListView->installEventFilter(m_panel);
        m_fileListView->viewport()->installEventFilter(m_panel);
    }

    // 6. 折叠信号双向连接
    connect(m_folderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        m_folderListView->setVisible(!collapsed);
    });

    connect(m_fileHeader, &FileSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        m_fileListView->setVisible(!collapsed);
    });

    // 7. 信号转发代理至 ListViewSectionContainer
    connect(m_folderListView, &QTreeView::doubleClicked, this, &ListViewSectionContainer::doubleClicked);
    connect(m_folderListView, &QTreeView::customContextMenuRequested, this, &ListViewSectionContainer::customContextMenuRequested);
    connect(m_folderListView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        emit pathsDropped(paths, targetIndex, m_folderProxyModel);
    });

    connect(m_fileListView, &QTreeView::doubleClicked, this, &ListViewSectionContainer::doubleClicked);
    connect(m_fileListView, &QTreeView::customContextMenuRequested, this, &ListViewSectionContainer::customContextMenuRequested);
    connect(m_fileListView, &DropTreeView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
        emit pathsDropped(paths, targetIndex, m_fileProxyModel);
    });

    // 8. 跨列表表头列宽像素级对齐
    syncHeaderColumnWidths();
}

void ListViewSectionContainer::syncHeaderColumnWidths() {
    auto* folderHeaderView = m_folderListView->header();
    auto* fileHeaderView = m_fileListView->header();

    connect(folderHeaderView, &QHeaderView::sectionResized, this, [fileHeaderView](int logicalIndex, int oldSize, int newSize) {
        QSignalBlocker blocker(fileHeaderView);
        fileHeaderView->resizeSection(logicalIndex, newSize);
    });

    connect(fileHeaderView, &QHeaderView::sectionResized, this, [folderHeaderView](int logicalIndex, int oldSize, int newSize) {
        QSignalBlocker blocker(folderHeaderView);
        folderHeaderView->resizeSection(logicalIndex, newSize);
    });
}

void ListViewSectionContainer::setModel(QSortFilterProxyModel* mainProxyModel) {
    m_mainProxyModel = mainProxyModel;
    if (!m_mainProxyModel) return;

    // 上方文件夹模型：仅放行 isDir == true
    m_folderProxyModel = new FilterProxyModel(this);
    m_folderProxyModel->setSourceModel(m_mainProxyModel->sourceModel());
    FilterState folderState;
    folderState.showFolders = true;
    folderState.showFiles = false;
    m_folderProxyModel->currentFilter = folderState;

    // 下方文件模型：仅放行 isDir == false
    m_fileProxyModel = new FilterProxyModel(this);
    m_fileProxyModel->setSourceModel(m_mainProxyModel->sourceModel());
    FilterState fileState;
    fileState.showFolders = false;
    fileState.showFiles = true;
    m_fileProxyModel->currentFilter = fileState;

    m_folderListView->setModel(m_folderProxyModel);
    m_fileListView->setModel(m_fileProxyModel);

    // 绑定选区变更
    if (m_folderListView->selectionModel()) {
        connect(m_folderListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            if (m_folderListView->selectionModel()->hasSelection() && m_fileListView->selectionModel()) {
                QSignalBlocker blocker(m_fileListView->selectionModel());
                m_fileListView->selectionModel()->clearSelection();
            }
            emit selectionChanged();
        });
    }

    if (m_fileListView->selectionModel()) {
        connect(m_fileListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            if (m_fileListView->selectionModel()->hasSelection() && m_folderListView->selectionModel()) {
                QSignalBlocker blocker(m_folderListView->selectionModel());
                m_folderListView->selectionModel()->clearSelection();
            }
            emit selectionChanged();
        });
    }

    // 初始统计显示数量
    int folderCount = m_folderProxyModel->rowCount();
    int fileCount = m_fileProxyModel->rowCount();

    m_folderHeader->setCount(folderCount);
    m_fileHeader->setCount(fileCount);

    m_folderHeader->setVisible(folderCount > 0);
    m_folderListView->setVisible(folderCount > 0);
    m_fileHeader->setVisible(fileCount > 0);
    m_fileListView->setVisible(fileCount > 0);
}

void ListViewSectionContainer::applyFilters(const FilterState& state) {
    if (m_folderProxyModel) {
        FilterState st = state;
        st.showFolders = true;
        st.showFiles = false;
        m_folderProxyModel->currentFilter = st;
        m_folderProxyModel->invalidateFilter();
    }
    if (m_fileProxyModel) {
        FilterState st = state;
        st.showFolders = false;
        st.showFiles = true;
        m_fileProxyModel->currentFilter = st;
        m_fileProxyModel->invalidateFilter();
    }

    int folderCount = m_folderProxyModel ? m_folderProxyModel->rowCount() : 0;
    int fileCount = m_fileProxyModel ? m_fileProxyModel->rowCount() : 0;

    if (m_folderHeader) {
        m_folderHeader->setCount(folderCount);
        m_folderHeader->setVisible(folderCount > 0);
    }
    if (m_folderListView) {
        m_folderListView->setVisible(folderCount > 0 && (!m_folderHeader || !m_folderHeader->isCollapsed()));
    }
    if (m_fileHeader) {
        m_fileHeader->setCount(fileCount);
        m_fileHeader->setVisible(fileCount > 0);
    }
    if (m_fileListView) {
        m_fileListView->setVisible(fileCount > 0 && (!m_fileHeader || !m_fileHeader->isCollapsed()));
    }
}

void ListViewSectionContainer::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_folderProxyModel) {
        m_folderProxyModel->setSortType(sortType);
        m_folderProxyModel->setSortOrder(sortOrder);
    }
    if (m_fileProxyModel) {
        m_fileProxyModel->setSortType(sortType);
        m_fileProxyModel->setSortOrder(sortOrder);
    }
}

void ListViewSectionContainer::toggleFolderSectionCollapse() {
    if (m_folderHeader) {
        m_folderHeader->setCollapsed(!m_folderHeader->isCollapsed());
    }
}

} // namespace QuarkMeta
```

### 4. `src/ui/ContentPanel.h`
Integrate `ListViewSectionContainer` into `ContentPanel`.

<<<<<<< SEARCH
    DropTreeView* m_treeView = nullptr;
=======
    class ListViewSectionContainer* m_listContainer = nullptr;
    DropTreeView* m_treeView = nullptr;
>>>>>>> REPLACE

### 5. `src/ui/ContentPanel.cpp`
Switch `initListView()` to use `ListViewSectionContainer` and bind section collapse triggers.

<<<<<<< SEARCH
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
=======
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
>>>>>>> REPLACE

<<<<<<< SEARCH
void ContentPanel::toggleFolderSectionCollapse() {
    if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
        return;
    }
    if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
        jv->toggleFolderSectionCollapse();
    }
}
=======
void ContentPanel::toggleFolderSectionCollapse() {
    if (m_currentViewMode == ColumnView && m_columnView) {
        m_columnView->toggleFolderSectionCollapse();
        return;
    }
    if (m_currentViewMode == ListView && m_listContainer) {
        m_listContainer->toggleFolderSectionCollapse();
        return;
    }
    if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
        jv->toggleFolderSectionCollapse();
    }
}
>>>>>>> REPLACE

## Build & Verification Steps
1. Build project using CMake:
   ```bash
   cmake -B build -S .
   cmake --build build --config Debug
   ```
2. Open QuarkMeta application and navigate to any directory containing both folders and files.
3. Switch to **ListView (列表模式)**.
4. Verify that "文件夹 (N)" header appears above the folder list and "文件 (M)" header appears above the file list.
5. Resize any column header in the folder list and confirm column widths in the file list automatically synchronize.
6. Click the "文件夹 (N)" section header or press the section collapse shortcut. Confirm the folder list collapses smoothly while the file list stays in place.

## SSOT API Reuse & Anti-Redundancy Self-Check
- **Component Reuse**: 100% reused existing `FolderSectionHeaderBar` and `FileSectionHeaderBar` controls from `FolderSectionWidget.h`.
- **View Delegate Reuse**: Reused `TreeItemDelegate` and `DropTreeView` without modifying data models or injecting sentinel records.
- **Header Column Sync**: Reused `QHeaderView::sectionResized` with `QSignalBlocker` to ensure pixel-perfect column alignment across list views.

## Header API Signature Verification
- `FolderSectionHeaderBar::setCount(int)` in `src/ui/FolderSectionWidget.h`
- `FolderSectionHeaderBar::setCollapsed(bool)` in `src/ui/FolderSectionWidget.h`
- `FileSectionHeaderBar::setCount(int)` in `src/ui/FolderSectionWidget.h`
- `DropTreeView::header()` in `src/ui/DropTreeView.h`
- `ContentPanel::toggleFolderSectionCollapse()` in `src/ui/ContentPanel.h`
