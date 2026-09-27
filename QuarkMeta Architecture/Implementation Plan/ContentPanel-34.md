# Implementation Plan - ContentPanel-34.md

## Overview
Fix grid view layout spacing gap when collapsing folder section or when grid height is smaller than scroll area viewport height in `ContentPanel`.
Adds a bottom stretch (`addStretch(1)`) to `m_gridContainerWidget`'s `QVBoxLayout` so all sections remain top-aligned, and ensures `m_folderGridView`'s height bounds are strictly set to 0 when collapsed.

## Modified Files List
- `src/ui/ContentPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    connect(m_gridFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderGridView && m_gridFolderHeader->count() > 0) {
            m_folderGridView->setVisible(!collapsed);
        }
    });
=======
    connect(m_gridFolderHeader, &FolderSectionHeaderBar::collapseToggled, this, [this](bool collapsed) {
        if (m_folderGridView && m_gridFolderHeader->count() > 0) {
            m_folderGridView->setVisible(!collapsed);
            if (collapsed) {
                m_folderGridView->setMinimumHeight(0);
                m_folderGridView->setMaximumHeight(0);
            } else if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
                int h = fjv->totalHeight();
                if (h > 0) {
                    m_folderGridView->setMinimumHeight(h);
                    m_folderGridView->setMaximumHeight(h);
                }
            }
        }
    });
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_gridView->installEventFilter(this);
    m_gridView->viewport()->installEventFilter(this);
    layout->addWidget(m_gridView, 1);
=======
    m_gridView->installEventFilter(this);
    m_gridView->viewport()->installEventFilter(this);
    layout->addWidget(m_gridView, 0);
    layout->addStretch(1);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
=======
        if (m_folderGridView) {
            bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
            if (folderCount == 0 || collapsed) {
                m_folderGridView->setVisible(false);
                m_folderGridView->setMinimumHeight(0);
                m_folderGridView->setMaximumHeight(folderCount == 0 ? QWIDGETSIZE_MAX : 0);
            } else {
                m_folderGridView->setVisible(true);
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
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Build the target `QuarkMeta` or `all` via CMake / MSVC.
2. Launch QuarkMeta and test folding/unfolding the folder header in Grid View mode.
3. Verify that items and headers stay top-aligned without empty vertical gaps in between.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Reused existing `m_gridFolderHeader` signals and `JustifiedView::totalHeight()` without adding duplicate layout managers or splitters.

## Header API Signature Verification
- `FolderSectionHeaderBar::isCollapsed() const`
- `FolderSectionHeaderBar::count() const`
- `JustifiedView::totalHeight() const`
