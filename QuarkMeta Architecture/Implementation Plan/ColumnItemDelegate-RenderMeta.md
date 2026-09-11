# ColumnItemDelegate Metadata Painter Integration Plan (ColumnItemDelegate-RenderMeta.md)

## Overview
本方案旨在彻底解决分栏视图（`ColumnViewWidget` / `ColumnItemDelegate`）在内存数据更新（星级 `RatingRole`、色标 `ColorRole`）后界面缺乏视觉呈现的问题。通过在 `ColumnItemDelegate::paint` 中融入色标小圆点与星级标示的 `QPainter` 渲染逻辑，使分栏视图列表项能 100% 直观展示内存中最新的元数据状态。

## Modified Files List
- `src/ui/ColumnItemDelegate.h`

## Detailed Line-by-Line Changes

### `src/ui/ColumnItemDelegate.h`
在 `paint` 方法中追加对 `RatingRole` 与 `ColorRole` 的提取，并在文本右侧与图标旁绘制色标小圆点与黄星：

```cpp
        // 1. 读取内存中的色标与星级数据
        QString colorHex = index.data(ColorRole).toString();
        int rating = index.data(RatingRole).toInt();

        // 2. 绘制色标小圆点 (若存在)
        if (!colorHex.isEmpty()) {
            painter->setBrush(QColor(colorHex));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(option.rect.left() + 2, option.rect.top() + (option.rect.height() - 6) / 2, 6, 6);
        }

        // 3. 动态调整文本区域，为星级留出空间
        int rightOffset = isDir ? -22 : -6;
        if (rating > 0) rightOffset -= 32;
        QRect textRect = option.rect.adjusted(32, 0, rightOffset, 0);

        // 4. 绘制星级标示 (若 rating > 0)
        if (rating > 0) {
            QRect starRect(option.rect.right() + rightOffset, option.rect.top() + (option.rect.height() - 12) / 2, 30, 12);
            painter->setPen(QColor("#FFC107"));
            painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
            painter->drawText(starRect, Qt::AlignRight | Qt::AlignVCenter, QString("★%1").arg(rating));
        }
```

## Build & Verification Steps
1. 编译项目：`cmake --build build`
2. 启动应用并切换至分栏视图模式；
3. 在 `MetaPanel` 中对当前选中的文件/文件夹打星或添加颜色标记；
4. 验证：分栏视图列表项左侧**即时刷出色标圆点**，右侧**即时刷出黄星标志**，内存数据渲染 100% 实时生效！
