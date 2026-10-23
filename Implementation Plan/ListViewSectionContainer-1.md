# ListView Section Container Fix (ListViewSectionContainer-1.md)

## Overview
This implementation plan increment fixes MSVC compilation errors in `ListViewSectionContainer.cpp`:
1. `C2248: 'QSortFilterProxyModel::invalidateFilter': cannot access protected member`: Replaces `invalidateFilter()` with `updateFilter()` / `invalidate()` which are public on `FilterProxyModel` / `QSortFilterProxyModel`.
2. `C2039: 'collapseToggled' is not a member of 'QuarkMeta::FileSectionHeaderBar'` & `C2039: 'isCollapsed' is not a member of 'QuarkMeta::FileSectionHeaderBar'`: Updates `FileSectionHeaderBar` connection and visibility checks to match `FileSectionHeaderBar`'s actual interface contract in `FolderSectionWidget.h`.

## Modified Files List
1. `src/ui/ListViewSectionContainer.cpp`

## Detailed Line-by-Line Changes

### `src/ui/ListViewSectionContainer.cpp`
<<<<<<< SEARCH
    connect(m_fileHeader, &FileSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        m_fileListView->setVisible(!collapsed);
    });
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
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
=======
    if (m_folderProxyModel) {
        FilterState st = state;
        st.showFolders = true;
        st.showFiles = false;
        m_folderProxyModel->currentFilter = st;
        m_folderProxyModel->updateFilter();
    }
    if (m_fileProxyModel) {
        FilterState st = state;
        st.showFolders = false;
        st.showFiles = true;
        m_fileProxyModel->currentFilter = st;
        m_fileProxyModel->updateFilter();
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_fileListView) {
        m_fileListView->setVisible(fileCount > 0 && (!m_fileHeader || !m_fileHeader->isCollapsed()));
    }
=======
    if (m_fileListView) {
        m_fileListView->setVisible(fileCount > 0);
    }
>>>>>>> REPLACE

## Build & Verification Steps
1. Verify `ListViewSectionContainer.cpp` compiles cleanly without C2248 or C2039 errors.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `FilterProxyModel::updateFilter()` public API instead of calling protected base class `invalidateFilter()`.
- Verified `FolderSectionHeaderBar` vs `FileSectionHeaderBar` class signatures directly from `FolderSectionWidget.h`.

## Header API Signature Verification
- `FilterProxyModel::updateFilter()` in `src/ui/models/FilterProxyModel.h`
- `FolderSectionHeaderBar::collapseToggled(bool)` in `src/ui/FolderSectionWidget.h`
- `FileSectionHeaderBar::setCount(int)` in `src/ui/FolderSectionWidget.h`
