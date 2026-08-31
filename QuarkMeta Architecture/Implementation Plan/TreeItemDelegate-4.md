# QuarkMeta 架构重构方案：TreeItemDelegate 与 ContentHeaderView 列表缩放几何定位修复

## 一、 Overview
当用户在列表视图模式下使用顶部滑块调整缩略图/行高时，`TreeItemDelegate` 中绘制文件名文本的起始 X 坐标被写死为 `rect.left() + 40`，而缩略图宽度 `side` 随着行高拉大扩展至 70px+，导致文件名文本重叠绘制在缩略图图像的正上方。
本方案彻底消除 `rect.left() + 40` 硬编码死数值，恢复基于 `squareRect.right() + 10` 的动态几何计算，使文本与编辑框永远跟随卡片右侧自适应偏移；同时恢复 `ContentHeaderView` 第 0 列表头标题与缩略图大小 `m_zoomLevel` 的自适应对齐。

## 二、 Modified Files List
- `src/ui/TreeItemDelegate.h`
- `src/ui/DropTreeView.h`

## 三、 Detailed Line-by-Line Changes

### 1. `src/ui/TreeItemDelegate.h`
```git
<<<<<<< SEARCH
            // 4. 文本排版向右偏移（在 32px 行高下，固定起始起点 40px = 6px left + 26px 卡片 + 8px 间距，保持绝对对齐与稳定）
            QString name = index.data(Qt::DisplayRole).toString();
            QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");

            painter->setPen(textColor);
            painter->setFont(option.font);

            QRect textRect = option.rect;
            textRect.setLeft(option.rect.left() + 40);
=======
            // 4. 文本排版向右偏移：动态紧跟左侧正方形微卡片右边缘 + 10px，自适应缩放行高
            QString name = index.data(Qt::DisplayRole).toString();
            QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");

            painter->setPen(textColor);
            painter->setFont(option.font);

            QRect textRect = option.rect;
            textRect.setLeft(squareRect.right() + 10);
>>>>>>> REPLACE
```

### 2. `src/ui/DropTreeView.h`
```git
<<<<<<< SEARCH
        if (logicalIndex == 0) {
            int textStartX = rect.left() + 40; // 🚀 表头第 0 列名称固定从 40px 起始，与 TreeItemDelegate 40px 文本起点绝对物理居中垂直对齐

            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        }
=======
        if (logicalIndex == 0) {
            int side = m_zoomLevel - 6;
            if (side <= 0) side = 16;
            int textStartX = rect.left() + 6 + side + 10;

            QRect textRect = rect;
            textRect.setLeft(textStartX);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        }
>>>>>>> REPLACE
```

## 四、 Build & Verification Steps
1. 确认 `src/ui/TreeItemDelegate.h` 与 `src/ui/DropTreeView.h` 修改准确无误。
2. 检查 `TreeItemDelegate::paint` 中 `textRect.left()` 正确取值于 `squareRect.right() + 10`。
3. 检查 `updateEditorGeometry` 中的 `targetRect.setLeft(squareRect.right() + 10)` 保持完全几何契合。
