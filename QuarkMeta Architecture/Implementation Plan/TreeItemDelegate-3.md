# 列表视图最左侧 1:1 正方形卡片 Center-Crop 满铺修复实施方案

## 1. Overview（概述与解决的问题）

在列表视图（`DropTreeView`）下，最左侧列的缩略图在遇到 AI / EPS / PSD 等非 1:1 比例（如 512x273）的长方形图像时，会出现上下留空或浮动成细长条的视觉拉伸问题。
本方案在 `TreeItemDelegate::paint` 中为 `squareRect` 微卡片添加 1:1 暗色背景底板，并使用 `Qt::KeepAspectRatioByExpanding` + `QPainterPath` 裁剪路径实现 1:1 正方形 Center-Crop 居中裁剪满铺，彻底恢复极致平整的标准 1:1 圆角正方形微卡片（Mini Card）视觉效果。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/TreeItemDelegate.h`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/TreeItemDelegate.h`

```
<<<<<<< SEARCH
            // 1. 绘制微型卡片背景
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::transparent);
            QPainterPath cardPath;
            cardPath.addRoundedRect(squareRect, 4, 4);
            painter->drawPath(cardPath);

            // 2. 图像/图标平滑居中绘制（严格约束在 1:1 正方形 squareRect 内部）
            QVariant decoData = index.data(Qt::DecorationRole);
            bool hasThumb = index.data(HasThumbnailRole).toBool();

            if (hasThumb) {
                QPixmap thumb;
                if (decoData.canConvert<QPixmap>()) {
                    thumb = decoData.value<QPixmap>();
                } else if (decoData.canConvert<QIcon>()) {
                    QIcon icon = decoData.value<QIcon>();
                    if (!icon.isNull()) thumb = icon.pixmap(squareRect.size());
                }

                if (!thumb.isNull()) {
                    painter->save();
                    QPainterPath clipPath;
                    clipPath.addRoundedRect(squareRect, 4, 4);
                    painter->setClipPath(clipPath);

                    // 强制将缩略图在 1:1 正方形 squareRect 内部按物理比例居中渲染
                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2;
                    int y = squareRect.center().y() - scaled.height() / 2;
                    painter->drawPixmap(x, y, scaled);

                    painter->restore();
                } else {
                    QIcon icon = qvariant_cast<QIcon>(decoData);
                    if (!icon.isNull()) {
                        int iconSize = qRound(side * 0.75);
                        QRect iconRect(squareRect.center().x() - iconSize / 2,
                                       squareRect.center().y() - iconSize / 2,
                                       iconSize, iconSize);
                        icon.paint(painter, iconRect, Qt::AlignCenter);
                    }
                }
            } else {
                QIcon icon = qvariant_cast<QIcon>(decoData);
                if (!icon.isNull()) {
                    int iconSize = qRound(side * 0.75);
                    QRect iconRect(squareRect.center().x() - iconSize / 2,
                                   squareRect.center().y() - iconSize / 2,
                                   iconSize, iconSize);
                    icon.paint(painter, iconRect, Qt::AlignCenter);
                }
            }
=======
            // 1. 绘制 1:1 正方形微型卡片暗色底板与边框
            painter->setPen(QPen(QColor("#383838"), 1));
            painter->setBrush(QColor("#252526"));
            QPainterPath cardPath;
            cardPath.addRoundedRect(squareRect, 4, 4);
            painter->drawPath(cardPath);

            // 2. 图像/图标平滑 Center-Crop 正方形满铺绘制
            QVariant decoData = index.data(Qt::DecorationRole);
            bool hasThumb = index.data(HasThumbnailRole).toBool();

            if (hasThumb) {
                QPixmap thumb;
                if (decoData.canConvert<QPixmap>()) {
                    thumb = decoData.value<QPixmap>();
                } else if (decoData.canConvert<QIcon>()) {
                    QIcon icon = decoData.value<QIcon>();
                    if (!icon.isNull()) thumb = icon.pixmap(squareRect.size() * 2);
                }

                if (!thumb.isNull()) {
                    painter->save();
                    QPainterPath clipPath;
                    clipPath.addRoundedRect(squareRect, 4, 4);
                    painter->setClipPath(clipPath);

                    // 🚀【核心 Center-Crop 算法】：按 Expands 模式等比放大裁切，完全平铺充满 1:1 正方形微卡片，消除浮动长条感
                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2;
                    int y = squareRect.center().y() - scaled.height() / 2;
                    painter->drawPixmap(x, y, scaled);

                    painter->restore();
                } else {
                    QIcon icon = qvariant_cast<QIcon>(decoData);
                    if (!icon.isNull()) {
                        int iconSize = qRound(side * 0.75);
                        QRect iconRect(squareRect.center().x() - iconSize / 2,
                                       squareRect.center().y() - iconSize / 2,
                                       iconSize, iconSize);
                        icon.paint(painter, iconRect, Qt::AlignCenter);
                    }
                }
            } else {
                QIcon icon = qvariant_cast<QIcon>(decoData);
                if (!icon.isNull()) {
                    int iconSize = qRound(side * 0.75);
                    QRect iconRect(squareRect.center().x() - iconSize / 2,
                                   squareRect.center().y() - iconSize / 2,
                                   iconSize, iconSize);
                    icon.paint(painter, iconRect, Qt::AlignCenter);
                }
            }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   ```bash
   cmake --build build --config Release
   ```
2. **列表卡片效果验证**：
   切换至列表视图，查看 AI、EPS、PSD、JPG 等文件：
   - 验证最左侧呈现为统一 1:1 圆角正方形卡片底板；
   - 验证任意长宽比的图像均完美 Center-Crop 正方形满铺，绝无上下留白或细条悬空线框。
