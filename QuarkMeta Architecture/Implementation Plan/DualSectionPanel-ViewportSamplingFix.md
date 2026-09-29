# Implementation Plan - DualSectionPanel Viewport Sampling Repair (`DualSectionPanel-ViewportSamplingFix.md`)

## 1. Overview
This implementation plan repairs a critical flaw in `DualSectionPanel::refreshVisibleThumbnails`. Previously, when sample coordinates (`sampleTopY` or `sampleBtmY`) fell on card padding/spacing or section margins, `indexAt` returned invalid indices (`topIdx.isValid() == false`), causing `top` and `bottom` to fallback to `0` and `proxy->rowCount() - 1`. This effectively degraded virtual scrolling into a full-model (e.g. 2000+ items) thumbnail loading dispatch, triggering `CacheOverflowException` loops.

The fix introduces step-scanning towards the viewport center to find valid card items if edge sampling hits padding, ensuring full-model thumbnail dispatches are prevented.

---

## 2. Modified Files List
- `src/ui/DualSectionPanel.cpp` (Step-scan viewport center for valid indices when edge coordinates hit margins/padding; prevent fallback to 0..rowCount-1)
- `src/ui/models/DiskItemModel.h` (Purge unused `addItemRecord` declaration)
- `src/ui/models/DiskItemModel.cpp` (Purge unused `addItemRecord` implementation)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/DualSectionPanel.cpp`
```
<<<<<<< SEARCH
        int top = topIdx.isValid() ? qMax(0, topIdx.row() - 4) : 0;
        int bottom = btmIdx.isValid() ? qMin(proxy->rowCount() - 1, btmIdx.row() + 4) : proxy->rowCount() - 1;

        for (int r = top; r <= bottom; ++r) {
            QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
            if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
        }
=======
        int top = -1;
        int bottom = -1;

        if (topIdx.isValid()) {
            top = topIdx.row();
        }
        if (btmIdx.isValid()) {
            bottom = btmIdx.row();
        }

        if (top == -1 || bottom == -1) {
            int stepY = 16;
            int currentY = sampleTopY;
            while (top == -1 && currentY <= sampleBtmY) {
                QModelIndex idx = view->indexAt(QPoint(subVp->width() / 2, currentY));
                if (idx.isValid()) {
                    top = idx.row();
                }
                currentY += stepY;
            }

            currentY = sampleBtmY;
            while (bottom == -1 && currentY >= sampleTopY) {
                QModelIndex idx = view->indexAt(QPoint(subVp->width() / 2, currentY));
                if (idx.isValid()) {
                    bottom = idx.row();
                }
                currentY -= stepY;
            }
        }

        if (top != -1 && bottom != -1) {
            int clampedTop = qMax(0, top - 4);
            int clampedBottom = qMin(proxy->rowCount() - 1, bottom + 4);
            for (int r = clampedTop; r <= clampedBottom; ++r) {
                QModelIndex srcIdx = proxy->mapToSource(proxy->index(r, 0));
                if (srcIdx.isValid()) visibleRows.insert(srcIdx.row());
            }
        }
>>>>>>> REPLACE
```

---

## 4. Verification Steps
1. Open a directory with > 2000 files in Grid View.
2. Verify debug trace logs show exact row counts (e.g. 15-25 rows) being dispatched instead of full rowCount.
