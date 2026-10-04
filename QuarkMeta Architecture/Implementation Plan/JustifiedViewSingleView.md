# Implementation Plan - Step 2: Single View Transformation for JustifiedView (Grid & Justified Modes)

This plan details transforming `ContentPanel`'s Grid/Justified view modes to use a single `DropJustifiedView` attached directly to `SectionProxyModel`.

## 1. Overview
Currently, `ContentPanel` uses `SectionedScrollCanvas` (`m_gridCanvas`) containing two `DropJustifiedView` instances (`m_folderGridView` and `m_gridView`).
This refactoring removes `SectionedScrollCanvas` for grid view, makes `m_gridView` (a `DropJustifiedView`) the sole grid view instance, and sets its model to `SectionProxyModel`.

---

## 2. Modified Files List
1. `src/ui/JustifiedView.h` (Update `ItemGeometry`, method declarations)
2. `src/ui/JustifiedView.cpp` (`doLayout`, `paintEvent`, `indexAt`, `mousePressEvent`, `moveCursor`, `setSelection`, `rowsInRange`)
3. `src/ui/ContentPanel.h` (Remove `m_gridCanvas`, `m_folderGridView`, `m_gridFolderProxyModel`; rename `m_gridFileProxyModel` -> `m_gridProxyModel`)
4. `src/ui/ContentPanel.cpp` (Setup single grid view & proxy model, update filter & status bar handling)
5. `src/ui/controllers/ContentViewCoordinator.cpp` (Update selection traversal to skip header rows)
6. `src/ui/controllers/ContentKeyHandler.cpp` (Update grid view references)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/JustifiedView.h`
<<<<<<< SEARCH
    struct ItemGeometry {
        QRect rect;
        bool isHeader = false;
        QString headerText;
        bool isCollapsed = false;
    };
=======
    struct ItemGeometry {
        QRect rect;
    };
>>>>>>> REPLACE

---

### 3.2 `src/ui/JustifiedView.cpp`
<<<<<<< SEARCH
    // Section Header row layout logic in doLayout()
=======
    // In doLayout():
    // Iterate over model rows. Check if row returns SectionHeaderRole == true.
    // If true:
    //   Force new line before header row.
    //   Geometry: x = 0, y = currentY, width = viewportWidth, height = 28px.
    //   m_geometries[row] = { rect }.
    //   currentY += 28 + lineSpacing.
    //   Force new line after header row.
    // If normal item:
    //   Compute aspect ratio/grid geometry as usual.
    //   Last row justification check (isLastRow): row is last row of partition if row + 1 >= total || model()->index(row + 1, 0).data(SectionHeaderRole).toBool().
>>>>>>> REPLACE

<<<<<<< SEARCH
    // Mouse press event on section headers:
=======
void JustifiedView::mousePressEvent(QMouseEvent* event) {
    QModelIndex idx = indexAt(event->pos());
    if (idx.isValid() && idx.data(SectionHeaderRole).toBool()) {
        if (idx.data(SectionHeaderTextRole).toString().startsWith("文件夹")) {
            auto* secModel = qobject_cast<SectionProxyModel*>(model());
            if (secModel) {
                secModel->setFolderCollapsed(!secModel->isFolderCollapsed());
            }
        }
        event->accept();
        return;
    }
    QAbstractItemView::mousePressEvent(event);
}
>>>>>>> REPLACE

---

### 3.3 `src/ui/ContentPanel.h`
<<<<<<< SEARCH
    SectionedScrollCanvas* m_gridCanvas = nullptr;
    JustifiedView* m_folderGridView = nullptr;
    FilterProxyModel* m_gridFolderProxyModel = nullptr;
    FilterProxyModel* m_gridFileProxyModel = nullptr;
=======
    DropJustifiedView* m_gridView = nullptr;
    FilterProxyModel* m_gridProxyModel = nullptr;
    SectionProxyModel* m_gridSectionProxyModel = nullptr;
>>>>>>> REPLACE

---

### 3.4 `src/ui/ContentPanel.cpp`
<<<<<<< SEARCH
    // Remove grid canvas initialization and replace with single m_gridView
=======
    m_gridProxyModel = new FilterProxyModel(this);
    m_gridSectionProxyModel = new SectionProxyModel(this);
    m_gridSectionProxyModel->setSourceModel(m_gridProxyModel);

    m_gridView = new DropJustifiedView(this);
    m_gridView->setModel(m_gridSectionProxyModel);
    m_gridView->setSelectionMode(QAbstractItemView::ExtendedSelection);
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Apply changes to `JustifiedView.h/.cpp`, `ContentPanel.h/.cpp`.
2. Compile project.
3. Test grid and justified view modes in UI.
4. Verify section headers for folders and files render correctly and folder collapse operates smoothly.
