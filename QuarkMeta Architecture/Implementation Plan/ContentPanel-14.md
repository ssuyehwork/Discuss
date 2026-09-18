# Implementation Plan - ContentPanel-14.md

## 1. Overview
Fix two critical layout and view-mode synchronization architecture defects in `ContentPanel` and `JustifiedView`:
1. **View Mode Initialization Desync**: On application startup, `ContentPanel::setViewMode(savedMode)` was blocked by `if (m_currentViewMode == mode) return;` because `m_currentViewMode` was default-initialized to `GridView` (0), matching `savedMode` (0). Consequently, `m_gridView` and `m_folderGridView` remained in `JustifiedView::JustifiedMode` instead of being switched to `JustifiedView::GridMode`.
2. **Folder/File Grid Spacing Defect**: `m_folderGridView` fixed height calculation added an arbitrary `+8` extra pixels. Additionally, `JustifiedView::doLayout()` added a trailing `spacing` (5px) after the last row, and layout margins created a huge non-zero gap between folder grid, file header bar, and file grid.

---

## 2. Modified Files List
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/JustifiedView.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/ContentPanel.h`
Initialize `m_currentViewMode` to an uninitialized invalid sentinel state (`static_cast<ViewMode>(-1)`) so that the initial call to `setViewMode(savedMode)` will never be short-circuited.

```
<<<<<<< SEARCH
    ViewMode m_currentViewMode = GridView;
=======
    ViewMode m_currentViewMode = static_cast<ViewMode>(-1);
>>>>>>> REPLACE
```

---

### File 2: `src/ui/ContentPanel.cpp`
Remove the arbitrary `+8` pixel padding when setting `m_folderGridView` fixed height, ensuring precise single-line/multi-row height fitting without extra bottom empty space.

```
<<<<<<< SEARCH
                // 3. 向上取整计算真实行数：6 个项目 / 8 列 = 1 行，绝不多算
                int rows = qMax(1, (folderCount + cardsPerRow - 1) / cardsPerRow);
                m_folderGridView->setFixedHeight(rows * rowH + 8);
=======
                // 3. 向上取整计算真实行数：6 个项目 / 8 列 = 1 行，绝不多算
                int rows = qMax(1, (folderCount + cardsPerRow - 1) / cardsPerRow);
                m_folderGridView->setFixedHeight(rows * rowH);
>>>>>>> REPLACE
```

---

### File 3: `src/ui/JustifiedView.cpp`
In `JustifiedView::doLayout()`, strip trailing `spacing` from `m_totalHeight` and ensure `currentY` on the last row does not append redundant bottom spacing, achieving exact 0-gap alignment with adjacent widgets.

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
=======
    m_totalHeight = currentY;
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Re-configure CMake build if needed: `cmake -B build -S .`
2. Compile target: `cmake --build build`
3. Verify that on startup in GridView mode, `m_gridView` and `m_folderGridView` layout mode is set to `GridMode`.
4. Verify visually that spacing between folder section and file section is compact with zero extra padding.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `ContentPanel::setViewMode` SSOT entry point.
- Reused `JustifiedView::setLayoutMode` SSOT method.
- Zero redundant view code introduced.

---

## 6. Header API Signature Verification
- `ContentPanel::setViewMode(ViewMode mode)` -> `src/ui/ContentPanel.h`
- `JustifiedView::setLayoutMode(LayoutMode mode)` -> `src/ui/JustifiedView.h`
- `JustifiedView::setTargetRowHeight(int h)` -> `src/ui/JustifiedView.h`
