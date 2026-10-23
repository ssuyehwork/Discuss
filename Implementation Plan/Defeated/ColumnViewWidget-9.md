# ColumnViewWidget-9.md Implementation Plan

## Overview
本实施方案旨在将列视图 (Column View) 的图标、缩略图与空文件夹绘制完全接入 `RowLayoutEngine` 物理 1:1 正方形微型卡片架构，与列表视图 (`ListView`) 达成底层渲染与几何计算的 100% 绝对一致：
1. **RowLayoutEngine 1:1 正方形微型卡片契约**：
   在 `TreeItemDelegate::paint` 处理 `col == 0` 时，统一通过 `RowLayoutEngine::calculate(option.rect, option.rect.height())` 动态推导 1:1 比例的正方形隐式微型卡片矩形 (`layout.cardRect`)；
2. **居中裁切与视觉对齐**：
   - 文件与文件夹图标在 `layout.cardRect` 正方形卡片内部 100% 物理居中绘制；
   - 图像缩略图在 `layout.cardRect` 内部按 `Qt::KeepAspectRatio` 进行 4px 圆角裁切与居中渲染；
   - 空文件夹状态器 (`IsEmptyRole`) 的 `#41F2F2` 青蓝色虚线指示框，严格锚定在 `layout.cardRect` 的正方形物理边界上进行绘制，杜绝硬编码偏移。

---

## Modified Files List
- `src/ui/TreeItemDelegate.h`

---

## Detailed Line-by-Line Changes

### `src/ui/TreeItemDelegate.h` (列视图全面接入 RowLayoutEngine 1:1 正方形卡片)

```diff
<<<<<<< SEARCH
        if (col == 0 && !m_drawMiniCards && isFolder) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QRect arrowRect(option.rect.right() - 16, option.rect.top(), 12, option.rect.height());
            painter->setPen(selected ? QColor("#FFFFFF") : QColor("#888888"));
            painter->drawText(arrowRect, Qt::AlignCenter, ">");
            painter->restore();
        }
=======
        if (col == 0 && !m_drawMiniCards) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);

            // 1. 接入 RowLayoutEngine 计算 1:1 比例正方形隐式微卡片与文本区域
            RowLayout layout = RowLayoutEngine::calculate(option.rect, option.rect.height());
            QRect squareRect = layout.cardRect;

            // 2. 图像/图标平滑居中绘制在 1:1 cardRect 内
            QVariant decoData = index.data(Qt::DecorationRole);
            bool hasThumb = index.data(HasThumbnailRole).toBool();

            if (hasThumb) {
                QPixmap thumb;
                if (decoData.canConvert<QPixmap>()) thumb = decoData.value<QPixmap>();
                else if (decoData.canConvert<QIcon>()) thumb = decoData.value<QIcon>().pixmap(squareRect.size());

                if (!thumb.isNull()) {
                    painter->save();
                    QPainterPath clipPath;
                    clipPath.addRoundedRect(squareRect, 4, 4);
                    painter->setClipPath(clipPath);

                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2;
                    int y = squareRect.center().y() - scaled.height() / 2;
                    painter->drawPixmap(x, y, scaled);
                    painter->restore();
                } else {
                    QIcon icon = qvariant_cast<QIcon>(decoData);
                    if (!icon.isNull()) {
                        int iconSize = qRound(squareRect.width() * 0.75);
                        QRect iconRect(squareRect.center().x() - iconSize / 2, squareRect.center().y() - iconSize / 2, iconSize, iconSize);
                        icon.paint(painter, iconRect, Qt::AlignCenter);
                    }
                }
            } else {
                QIcon icon = qvariant_cast<QIcon>(decoData);
                if (!icon.isNull()) {
                    int iconSize = qRound(squareRect.width() * 0.75);
                    QRect iconRect(squareRect.center().x() - iconSize / 2, squareRect.center().y() - iconSize / 2, iconSize, iconSize);
                    icon.paint(painter, iconRect, Qt::AlignCenter);
                }
            }

            // 3. 空文件夹青蓝色虚线框 (#41F2F2 Qt::DashLine) 物理锚定在 squareRect 边缘
            bool isEmpty = index.data(IsEmptyRole).toBool();
            if (isFolder && isEmpty) {
                painter->save();
                painter->setPen(QPen(QColor("#41F2F2"), 1, Qt::DashLine));
                painter->setBrush(Qt::NoBrush);
                painter->drawRoundedRect(squareRect, 4, 4);
                painter->restore();
            }

            // 4. 绘制右侧级联箭头 (>)
            if (isFolder) {
                QRect arrowRect(option.rect.right() - 16, option.rect.top(), 12, option.rect.height());
                painter->setPen(selected ? QColor("#FFFFFF") : QColor("#888888"));
                painter->drawText(arrowRect, Qt::AlignCenter, ">");
            }

            painter->restore();
        }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### 1. 编译验证
```bash
cmake --build build --config Release
```

### 2. 视觉对齐测试
1. 打开“列视图”，观察列视图最左侧的图标、缩略图及空文件夹框；
2. 确认空文件夹青蓝色虚线框与文件/文件夹图标严格对齐居中，且正方形几何比例与列表视图 100% 保持一致。
