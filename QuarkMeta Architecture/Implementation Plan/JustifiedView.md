# Implementation Plan - JustifiedView.md

## 1. Overview
Fix the critical architecture defect regarding Split-Brain height calculation and view-mode initialization desync between `ContentPanel` and `JustifiedView`:
1. **View Mode Initialization Desync**: Initialize `m_currentViewMode` in `ContentPanel.h` to `static_cast<ViewMode>(-1)` so that `setViewMode(savedMode)` on startup is never short-circuited by `if (m_currentViewMode == mode) return;`, ensuring `m_gridView` and `m_folderGridView` are properly configured with `JustifiedView::GridMode`.
2. **SSOT Height Normalization (Eliminating Split-Brain)**:
   - Export `totalHeight()` and emit `totalHeightChanged(int height)` from `JustifiedView` whenever `doLayout()` finishes calculating `m_totalHeight`.
   - Update `JustifiedView::sizeHint()` to return `QSize(230, m_totalHeight)` so Qt layout system and external parents receive the exact layout height.
   - **Completely purge the redundant external "guess-and-calculate" divide-by-width algorithm** in `ContentPanel.cpp` (`updateGridSectionCounts`), replacing it with a direct 1:1 SSOT binding to `m_folderGridView->totalHeight()`.
   - Strip extra trailing `spacing` and arbitrary padding in `JustifiedView::doLayout()` and `ContentPanel.cpp` to ensure exact 0-gap alignment with adjacent widgets.

---

## 2. Modified Files List
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/JustifiedView.h`
Export `totalHeight()` getter and `totalHeightChanged` signal; override `sizeHint()` to reflect true `m_totalHeight`.

```
<<<<<<< SEARCH
    // 🚀【物理契约】：彻底切断 QAbstractItemView 对父容器的尺寸顶推
    QSize minimumSizeHint() const override { return QSize(50, 50); }
    QSize sizeHint() const override { return QSize(230, 200); }
=======
    int totalHeight() const { return m_totalHeight; }

    // 🚀【物理契约】：彻底切断 QAbstractItemView 对父容器的尺寸顶推
    QSize minimumSizeHint() const override { return QSize(50, 50); }
    QSize sizeHint() const override { return QSize(230, m_totalHeight); }

signals:
    void totalHeightChanged(int height);
>>>>>>> REPLACE
```

---

### File 2: `src/ui/JustifiedView.cpp`
In `JustifiedView::doLayout()`, correct the calculation of `m_totalHeight` by preventing redundant trailing spacing on the last row, and emit `totalHeightChanged` when layout changes.

```
<<<<<<< SEARCH
            for (int j = 0; j < numInRow; ++j) {
                int itemIdx = rowStart + j;
                m_geometries[itemIdx] = { QRect(currentX, currentY, itemWidth, itemHeight), itemIdx };
                currentX += itemWidth + standardSpacing;
            }
            currentY += itemHeight + spacing;
        }
=======
            for (int j = 0; j < numInRow; ++j) {
                int itemIdx = rowStart + j;
                m_geometries[itemIdx] = { QRect(currentX, currentY, itemWidth, itemHeight), itemIdx };
                currentX += itemWidth + standardSpacing;
            }
            currentY += itemHeight;
            if (i < count) {
                currentY += spacing;
            }
        }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    m_totalHeight = currentY + 10;
    updateGeometries();
    viewport()->update();
=======
    int oldHeight = m_totalHeight;
    m_totalHeight = currentY;
    updateGeometries();
    viewport()->update();

    if (oldHeight != m_totalHeight) {
        emit totalHeightChanged(m_totalHeight);
    }
>>>>>>> REPLACE
```

---

### File 3: `src/ui/ContentPanel.h`
Set `m_currentViewMode` default to uninitialized sentinel `-1` to ensure initial `setViewMode` call is always executed.

```
<<<<<<< SEARCH
    ViewMode m_currentViewMode = GridView;
=======
    ViewMode m_currentViewMode = static_cast<ViewMode>(-1);
>>>>>>> REPLACE
```

---

### File 4: `src/ui/ContentPanel.cpp`
In `ContentPanel.cpp`, connect `m_folderGridView->totalHeightChanged` to automatically synchronize `setFixedHeight`, and **physically delete** the redundant external divide-by-width "guessing" calculation code.

```
<<<<<<< SEARCH
    connect(m_folderGridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
=======
    if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
        connect(fjv, &JustifiedView::totalHeightChanged, this, [this](int height) {
            if (m_folderGridView && m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
                m_folderGridView->setFixedHeight(height);
            }
        });
    }

    connect(m_folderGridView, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                // 1. 计算单张卡片占位宽与单行高度
                int cardW = m_zoomLevel + CardLayoutEngine::totalPaddingHorizontal() + 10;
                int rowH = m_zoomLevel + CardLayoutEngine::extraHeight() + 10;

                // 2. 根据当前视口可用宽度，动态计算一行实际放几张卡
                int availableW = m_folderGridView->width() > 100 ? m_folderGridView->width() : width();
                int cardsPerRow = qMax(1, availableW / cardW);

                // 3. 向上取整计算真实行数：6 个项目 / 8 列 = 1 行，绝不多算
                int rows = qMax(1, (folderCount + cardsPerRow - 1) / cardsPerRow);
                m_folderGridView->setFixedHeight(rows * rowH + 8);
            }
        }
=======
        if (m_folderGridView) {
            if (folderCount == 0) {
                m_folderGridView->hide();
            } else {
                bool collapsed = m_gridFolderHeader ? m_gridFolderHeader->isCollapsed() : false;
                m_folderGridView->setVisible(!collapsed);
                if (auto* fjv = qobject_cast<JustifiedView*>(m_folderGridView)) {
                    m_folderGridView->setFixedHeight(fjv->totalHeight());
                }
            }
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Configure and build:
   ```bash
   cmake -B build -S .
   cmake --build build
   ```
2. Verify that on startup in GridView mode, `m_gridView` and `m_folderGridView` layout mode is set to `GridMode`.
3. Verify that `m_folderGridView` height is strictly determined by `JustifiedView::totalHeight()` without any external divide-by-width calculations.
4. Verify visually that spacing between folder grid and file section is exactly 0.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- `JustifiedView::totalHeight()` is now the **Single Source of Truth (SSOT)** for view height.
- All external guess-and-calculate logic in `ContentPanel.cpp` has been physically deleted.
- Reused `ContentPanel::setViewMode` entry point.

---

## 6. Header API Signature Verification
- `JustifiedView::totalHeight() const` -> `src/ui/JustifiedView.h`
- `JustifiedView::setLayoutMode(LayoutMode mode)` -> `src/ui/JustifiedView.h`
- `ContentPanel::setViewMode(ViewMode mode)` -> `src/ui/ContentPanel.h`
