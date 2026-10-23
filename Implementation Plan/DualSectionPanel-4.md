# Implementation Plan - DualSectionPanel-4

## Overview
This implementation plan fixes viewport sampling deadzones in `DualSectionPanel::refreshVisibleThumbnails`.
When grid items are displayed in `JustifiedView`, sampling at a single fixed X coordinate (`width() / 2`) can hit card gaps/padding, causing `indexAt()` to return invalid indices and skipping thumbnail requests for visible items.
This change introduces multi-point X sampling (at 25%, 50%, and 75% of viewport width) to guarantee valid card detection.

## Modified Files List
- `src/ui/DualSectionPanel.cpp`

## Detailed Line-by-Line Changes

### `src/ui/DualSectionPanel.cpp`

```
<<<<<<< SEARCH
        // 🚨【关键修复】：若采样的边缘坐标刚好命中卡片间隙（padding），向中心步进扫描寻找视口内第一个/最后一个有效索引
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
=======
        // 🚨【关键修复】：若采样的边缘坐标刚好命中卡片间隙（padding），采用多点 X 坐标向中心步进扫描寻找视口内有效索引
        if (top == -1 || bottom == -1) {
            int stepY = 16;
            int currentY = sampleTopY;
            int xPoints[] = { subVp->width() / 2, subVp->width() / 4, (subVp->width() * 3) / 4 };

            while (top == -1 && currentY <= sampleBtmY) {
                for (int x : xPoints) {
                    QModelIndex idx = view->indexAt(QPoint(x, currentY));
                    if (idx.isValid()) {
                        top = idx.row();
                        break;
                    }
                }
                currentY += stepY;
            }

            currentY = sampleBtmY;
            while (bottom == -1 && currentY >= sampleTopY) {
                for (int x : xPoints) {
                    QModelIndex idx = view->indexAt(QPoint(x, currentY));
                    if (idx.isValid()) {
                        bottom = idx.row();
                        break;
                    }
                }
                currentY -= stepY;
            }
        }
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile `src/ui/DualSectionPanel.cpp`.
2. Scroll through grid views and verify thumbnail requests are correctly dispatched regardless of grid item spacing.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Modifies viewport sampling in `DualSectionPanel` without duplicating view layout logic.

## Header API Signature Verification
- `QAbstractItemView::indexAt(const QPoint &point)` -> standard Qt view method.

## Header Inclusion Chain Check
- Verify `DualSectionPanel.cpp` includes `<QPoint>`.
