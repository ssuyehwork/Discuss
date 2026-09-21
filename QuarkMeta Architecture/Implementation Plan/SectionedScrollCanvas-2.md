# Implementation Plan - SectionedScrollCanvas-2 (Dynamic Viewport Stretch Handover)

## Overview
This implementation plan resolves the structural coverage gap in `SectionedScrollCanvas` where pure folder directories (`folderCount > 0 && fileCount == 0`) left the bottom 80%~90% of the viewport as bare container space, causing rubber-band multi-selection (框选/滑选) to fail in empty areas below folder items.

### Architectural Fix
1. **Dynamic Stretch Handover (动态视口拉伸权无缝移交)**:
   - When both files and folders exist (`fileCount > 0`): `folderView` remains compact (`baseH`), while `fileView` stretches to fill viewport remaining height via `qMax(fileH, fileViewMinHeight())`.
   - When only folders exist (`fileCount == 0 && folderCount > 0`): `fileView` is hidden, and `folderView` dynamically receives the stretch handover, setting its height to `qMax(baseH, fileViewMinHeight())`.
   - This ensures 100% of the viewport area below folder items is covered by `folderView`'s interactive viewport, allowing rubber-band drag selection and context menu events to function flawlessly in all directory scenarios.

---

## Modified Files List
- `src/ui/SectionedScrollCanvas.cpp`

---

## Detailed Line-by-Line Changes

### `src/ui/SectionedScrollCanvas.cpp`
Update `SectionedScrollCanvas::updateSectionCounts()` to implement dynamic stretch handover when `fileCount == 0`.

```diff
<<<<<<< SEARCH
    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                folderView->setFixedHeight(fjv->totalHeight());
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            folderView->setFixedHeight(folderCount * rowH + hdrH + 2);
            folderView->updateGeometry();
        }
    }
=======
    if (folderView && folderCount > 0 && folderView->isVisible()) {
        if (m_type == CanvasType::Grid) {
            if (auto* fjv = qobject_cast<JustifiedView*>(folderView)) {
                int baseH = fjv->totalHeight();
                if (fileCount == 0) {
                    folderView->setFixedHeight(qMax(baseH, m_panel->fileViewMinHeight()));
                } else {
                    folderView->setFixedHeight(baseH);
                }
            }
        } else {
            auto* tv = static_cast<QTreeView*>(folderView);
            int rowH = tv->sizeHintForRow(0);
            int iconH = tv->iconSize().height();
            if (rowH <= iconH) rowH = iconH + 10;
            if (rowH <= 0) rowH = 30;
            int hdrH = (tv->header() && tv->header()->isVisible()) ? tv->header()->height() : 0;
            int baseH = folderCount * rowH + hdrH + 2;
            if (fileCount == 0) {
                folderView->setFixedHeight(qMax(baseH, m_panel->fileViewMinHeight()));
            } else {
                folderView->setFixedHeight(baseH);
            }
            folderView->updateGeometry();
        }
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Perform CMake build to compile the project.
2. Open a folder containing only subfolders (`folderCount > 0 && fileCount == 0`).
3. Press and hold left mouse button in the empty space below the folder items and drag to form a rubber-band selection box: verify rubber-band multi-selection works smoothly across the entire viewport height.
4. Open a mixed folder containing both folders and files (`folderCount > 0 && fileCount > 0`): verify `folderView` remains compact and `fileView` fills the remaining height smoothly.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- **SSOT Channel Re-used**: Re-used `m_panel->fileViewMinHeight()` for calculating viewport remaining height.
- **Zero-Value-Alteration**: Kept all existing row heights, paddings, and header sizing unchanged.

---

## Header API Signature Verification
- `DualSectionPanel::fileViewMinHeight()` -> Returns `int`.
- `SectionedScrollCanvas::updateSectionCounts()` -> Returns `void`.
