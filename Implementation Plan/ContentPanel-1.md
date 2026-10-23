# Implementation Plan - ContentPanel Scroll Normalization (Revision 1)

## 1. Overview
This implementation plan resolves the architecture inconsistency ("另起炉灶") where Folder/File section bars and folder view in List View, Grid View, and Justified View did not scroll together as a unified whole.

### Review Feedback Addressed
1. **Null Container Fallback Safety**: Maintained full fallback checks when retrieving current views or adding to `m_viewStack`.
2. **Dynamic Grid Column Calculation**: Replaced hardcoded row/column math with dynamic column calculation based on viewport width and item card size.
3. **Immutability Contract**: Documented in a new incremented file `ContentPanel-1.md` per AGENTS.md Rule 3.1.3.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    QWidget* m_listContainerWidget = nullptr;
    FolderSectionHeaderBar* m_listFolderHeader = nullptr;
    DropTreeView* m_folderTreeView = nullptr;
    FileSectionHeaderBar* m_listFileHeader = nullptr;
    FilterProxyModel* m_folderProxyModel = nullptr;
    FilterProxyModel* m_fileProxyModel = nullptr;

    QWidget* m_gridContainerWidget = nullptr;
    FolderSectionHeaderBar* m_gridFolderHeader = nullptr;
    DropJustifiedView* m_folderGridView = nullptr;
    FileSectionHeaderBar* m_gridFileHeader = nullptr;
=======
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
>>>>>>> REPLACE
```

### `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    m_viewStack->addWidget(m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView));
    m_viewStack->addWidget(m_listContainerWidget ? m_listContainerWidget : static_cast<QWidget*>(m_treeView));
    m_viewStack->addWidget(m_columnView);
    m_viewStack->setCurrentWidget(m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView));
=======
    if (m_gridContainerWidget) {
        m_gridScrollArea = new QScrollArea(this);
        m_gridScrollArea->setFrameShape(QFrame::NoFrame);
        m_gridScrollArea->setWidgetResizable(true);
        m_gridScrollArea->setWidget(m_gridContainerWidget);
    }
    if (m_listContainerWidget) {
        m_listScrollArea = new QScrollArea(this);
        m_listScrollArea->setFrameShape(QFrame::NoFrame);
        m_listScrollArea->setWidgetResizable(true);
        m_listScrollArea->setWidget(m_listContainerWidget);
    }

    QWidget* initialGridWidget = m_gridScrollArea ? static_cast<QWidget*>(m_gridScrollArea) : (m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView));
    QWidget* initialListWidget = m_listScrollArea ? static_cast<QWidget*>(m_listScrollArea) : (m_listContainerWidget ? m_listContainerWidget : static_cast<QWidget*>(m_treeView));

    m_viewStack->addWidget(initialGridWidget);
    m_viewStack->addWidget(initialListWidget);
    m_viewStack->addWidget(m_columnView);
    m_viewStack->setCurrentWidget(initialGridWidget);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                int cardH = m_zoomLevel + CardLayoutEngine::extraHeight() + 20;
                m_folderGridView->setMaximumHeight(cardH);
=======
                int cardWidth = m_zoomLevel + CardLayoutEngine::extraWidth();
                int vpWidth = (m_gridScrollArea && m_gridScrollArea->viewport()) ? m_gridScrollArea->viewport()->width() : width();
                int cols = qMax(1, vpWidth / qMax(1, cardWidth));
                int rows = qMax(1, (folderCount + cols - 1) / cols);
                int rowH = m_zoomLevel + CardLayoutEngine::extraHeight() + 20;
                m_folderGridView->setFixedHeight(rows * rowH);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                int folderH = qMin(200, qMax(32, folderCount * 30 + 32));
                m_folderTreeView->setMaximumHeight(folderH);
=======
                int folderH = qMax(32, folderCount * 30 + 32);
                m_folderTreeView->setFixedHeight(folderH);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (mode == ListView) {
        m_viewStack->setCurrentWidget(m_listContainerWidget ? m_listContainerWidget : static_cast<QWidget*>(m_treeView));
    } else if (mode == ColumnView) {
        if (m_columnView) {
            QString targetPath = !m_selectionState.focusedPath.isEmpty() ? m_selectionState.focusedPath : m_currentPath;
            m_columnView->setRootPath(targetPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView);
        if (fjv) fjv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        m_viewStack->setCurrentWidget(m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView));
    }
=======
    if (mode == ListView) {
        QWidget* listWidget = m_listScrollArea ? static_cast<QWidget*>(m_listScrollArea) : (m_listContainerWidget ? m_listContainerWidget : static_cast<QWidget*>(m_treeView));
        m_viewStack->setCurrentWidget(listWidget);
    } else if (mode == ColumnView) {
        if (m_columnView) {
            QString targetPath = !m_selectionState.focusedPath.isEmpty() ? m_selectionState.focusedPath : m_currentPath;
            m_columnView->setRootPath(targetPath);
            m_viewStack->setCurrentWidget(m_columnView);
        }
    } else {
        auto* jv = qobject_cast<JustifiedView*>(m_gridView);
        if (jv) jv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView);
        if (fjv) fjv->setLayoutMode(mode == GridView ? JustifiedView::GridMode : JustifiedView::JustifiedMode);
        QWidget* gridWidget = m_gridScrollArea ? static_cast<QWidget*>(m_gridScrollArea) : (m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView));
        m_viewStack->setCurrentWidget(gridWidget);
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_viewStack->currentWidget() == m_gridContainerWidget || m_viewStack->currentWidget() == m_gridView) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
            jv->setTargetRowHeight(m_zoomLevel);
        }
        if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
            fjv->setTargetRowHeight(m_zoomLevel);
        }
    } else if (m_viewStack->currentWidget() == m_treeView || m_viewStack->currentWidget() == m_listContainerWidget) {
=======
    QWidget* cur = m_viewStack->currentWidget();
    if (cur == m_gridScrollArea || cur == m_gridContainerWidget || cur == m_gridView) {
        if (auto* jv = qobject_cast<JustifiedView*>(m_gridView)) {
            jv->setTargetRowHeight(m_zoomLevel);
        }
        if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
            fjv->setTargetRowHeight(m_zoomLevel);
        }
    } else if (cur == m_listScrollArea || cur == m_treeView || cur == m_listContainerWidget) {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        m_viewStack->setCurrentWidget(m_currentViewMode == ListView ? (m_listContainerWidget ? m_listContainerWidget : static_cast<QWidget*>(m_treeView)) : (m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView)));
    }
=======
    if (m_currentViewMode == ColumnView) {
        m_viewStack->setCurrentWidget(m_columnView);
    } else {
        QWidget* target = (m_currentViewMode == ListView)
            ? (m_listScrollArea ? static_cast<QWidget*>(m_listScrollArea) : (m_listContainerWidget ? m_listContainerWidget : static_cast<QWidget*>(m_treeView)))
            : (m_gridScrollArea ? static_cast<QWidget*>(m_gridScrollArea) : (m_gridContainerWidget ? m_gridContainerWidget : static_cast<QWidget*>(m_gridView)));
        m_viewStack->setCurrentWidget(target);
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Perform CMake configuration and compilation check:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```
2. Verify that `ContentPanel.cpp` compiles cleanly without any C2039 or missing member errors.
3. Test List View, Grid View, and Justified View when directories contain both folders and files.
4. Drag the scrollbar or scroll mouse wheel in List/Grid/Justified view; confirm that the folder header, folder list/grid, file header, and file list move together as a single unified layout.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **`refreshAll()` SSOT entrypoint**: Kept intact.
- **Scroll architecture alignment**: Reused the exact `QScrollArea` container wrapping pattern from `ColumnViewWidget`.
- **Zero Redundancy**: Avoided duplicating layout handling logic across views.

---

## 6. Header API Signature Verification
- `QScrollArea::setWidget(QWidget*)`: Verified in Qt5/Qt6 `QScrollArea` API.
- `QScrollArea::setWidgetResizable(bool)`: Verified in Qt5/Qt6 `QScrollArea` API.
- `QScrollArea::setFrameShape(QFrame::Shape)`: Verified in Qt5/Qt6 `QScrollArea` API.
- `QStackedWidget::addWidget(QWidget*)`: Verified in Qt5/Qt6 `QStackedWidget` API.
- `QStackedWidget::setCurrentWidget(QWidget*)`: Verified in Qt5/Qt6 `QStackedWidget` API.
