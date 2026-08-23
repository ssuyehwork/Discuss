# Color Bar Maximum Height 20px Limit Implementation Plan (color_bar_height_fix.md)

## Overview
This implementation plan strictly limits the maximum height of the rating color bar (color pill/capsule background) across both Grid View (`ThumbnailDelegate`) and List View (`TreeItemDelegate`) to **no greater than 20 pixels**.

In Grid View (`ThumbnailDelegate`), the color bar height is reduced from 24px (`ratingHeight = 24`, `starSize = 22`) to 20px (`ratingHeight = 20`, `starSize = 18`). In List View (`TreeItemDelegate`), `starSize` is already 18px (`18 + 2 = 20px` max rounded rectangle height), natively satisfying the 20px height cap.

## Modified Files List
- `src/ui/ThumbnailDelegate.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/ThumbnailDelegate.cpp`
Set `ratingHeight = 20`, `starSize = 18`, and `banW = 12` in `calculateMetrics` to enforce a maximum color bar height of 20px and maintain vertical alignment.

```
<<<<<<< SEARCH
    const int textHeight = 36;
    const int ratingHeight = 24;
    const int gap = 4;

    m.ratingH = ratingHeight;
    // 底部预留高度增加，包含星级区域和间隙
    m.cardRect = option.rect.adjusted(3, 3, -3, -(textHeight + m.ratingH + gap + 3));

    // 星级坐标脱离卡片范围
    m.ratingY = m.cardRect.bottom() + gap;

    m.textRect = QRect(option.rect.left() + 3,
                       m.ratingY + m.ratingH - 5,
                       option.rect.width() - 6,
                       textHeight);

    int zoom = option.decorationSize.width(); // 物理缩放级别

    m.starSize = 22;
    m.starSpacing = -4; // 2026-06-08 优化：默认间距调紧
    int banW = 14;

    // 2026-06-08 按照调试增强版 V2 优化：实现“动态比例星级”
    // 虽然底限是 96，但在接近极限 (100) 时提前缩小星级，确保视觉紧凑感
    if (zoom < 100) {
        m.starSize = 18;
        m.starSpacing = -4;
        banW = 12;
    }
=======
    const int textHeight = 36;
    const int ratingHeight = 20;
    const int gap = 4;

    m.ratingH = ratingHeight;
    // 底部预留高度，包含星级区域和间隙
    m.cardRect = option.rect.adjusted(3, 3, -3, -(textHeight + m.ratingH + gap + 3));

    // 星级坐标脱离卡片范围
    m.ratingY = m.cardRect.bottom() + gap;

    m.textRect = QRect(option.rect.left() + 3,
                       m.ratingY + m.ratingH - 1,
                       option.rect.width() - 6,
                       textHeight);

    Q_UNUSED(option.decorationSize);

    // 强制色条胶囊上限不超过 20px (starSize = 18px, roundedRect adjusted top -1 / bottom +1 => 20px)
    m.starSize = 18;
    m.starSpacing = -4;
    int banW = 12;
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Recompile project:
   ```bash
   cmake --build build --config Debug
   ```
2. Verify in Grid View that the color bar background capsule height does not exceed 20 pixels.
3. Verify in List View that the color bar background capsule height remains 20 pixels.
