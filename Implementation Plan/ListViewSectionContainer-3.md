# ListView Section Container Integration Fix (ListViewSectionContainer-3.md)

## Overview
This implementation plan increment fixes `ContentPanel` view stack and selection model integration for `ListViewSectionContainer`:
1. Swaps `m_treeView` in `m_viewStack` for `m_listContainer` in `ListView` view mode so the collapsible "文件夹 (N)" and "文件 (M)" section headers and dual lists are visible to the user.
2. Updates `getSelectedIndexes()`, `activeItemView()`, and `restoreActiveView()` in `ContentPanel` to inspect both `m_folderListView` and `m_fileListView` within `m_listContainer`.

## Modified Files List
1. `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_treeView);
    m_viewStack->addWidget(m_columnView);
=======
    m_viewStack->addWidget(m_gridView);
    m_viewStack->addWidget(m_listContainer);
    m_viewStack->addWidget(m_columnView);
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_treeView);
=======
    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_listContainer);
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_currentViewMode == ListView) {
        return m_treeView;
    }
=======
    if (m_currentViewMode == ListView && m_listContainer) {
        if (m_listContainer->folderListView()->hasFocus() || m_listContainer->folderListView()->selectionModel()->hasSelection()) {
            return m_listContainer->folderListView();
        }
        if (m_listContainer->fileListView()->hasFocus() || m_listContainer->fileListView()->selectionModel()->hasSelection()) {
            return m_listContainer->fileListView();
        }
        return m_listContainer->folderListView();
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    } else if (m_currentViewMode == ListView) {
        if (m_treeView) views << m_treeView;
    } else { // GridView / JustifiedViewMode
=======
    } else if (m_currentViewMode == ListView && m_listContainer) {
        if (m_listContainer->folderListView()) views << m_listContainer->folderListView();
        if (m_listContainer->fileListView()) views << m_listContainer->fileListView();
    } else { // GridView / JustifiedViewMode
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? static_cast<QWidget*>(m_treeView) : static_cast<QWidget*>(m_gridView));
    }
=======
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else if (m_currentViewMode == ListView) {
        m_viewStack->setCurrentWidget(m_listContainer);
    } else {
        m_viewStack->setCurrentWidget(m_gridView);
    }
>>>>>>> REPLACE

## Build & Verification Steps
1. Switch to ListView mode in QuarkMeta application.
2. Confirm `m_listContainer` with "文件夹 (N)" and "文件 (M)" headers renders as the current active widget in `m_viewStack`.
3. Select items in both folder and file lists; confirm context menus and status bar statistics report active selections accurately.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `ContentPanel::getSelectedIndexes()` multi-view traversal architecture to naturally query both `folderListView()` and `fileListView()`.

## Header API Signature Verification
- `ListViewSectionContainer::folderListView()` in `src/ui/ListViewSectionContainer.h`
- `ListViewSectionContainer::fileListView()` in `src/ui/ListViewSectionContainer.h`
