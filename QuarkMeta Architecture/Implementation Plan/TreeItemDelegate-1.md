# 列表视图最左侧 1:1 圆角正方形微型卡片绘制修复实施方案

## 1. Overview（概述与解决的问题）

在列表视图（m_treeView）模式下，由于 `TreeItemDelegate::paint` 中处理 `HasThumbnailRole` 和 `DecorationRole` 的逻辑直接将非正方形或原始尺寸图像绘制到列表单元格区域，导致 AI/EPS/PSD 等设计/矢量文件呈现为上下长条形拉伸充填。
本方案强制约束所有缩略图与格式图标必须严格绘制在 1:1 正方形的 `squareRect` 微型卡片内部，使用 `Qt::KeepAspectRatio` 进行按比例居中 (Center-Fit)，彻底恢复标准 1:1 圆角正方形微卡片（Mini Card）外观。

---

## 2. Modified Files List（影响文件清单）

1. `src/ui/TreeItemDelegate.h`

---

## 3. Detailed Line-by-Line Changes（精准替换块）

### 3.1 `src/ui/TreeItemDelegate.h`

```
<<<<<<< SEARCH
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

                    QPixmap scaled = thumb.scaled(squareRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = squareRect.center().x() - scaled.width() / 2;
                    int y = squareRect.center().y() - scaled.height() / 2;
                    painter->drawPixmap(x, y, scaled);

                    painter->restore();
                } else {
                    QIcon icon = qvariant_cast<QIcon>(decoData);
                    if (!icon.isNull()) {
                        int iconSize = qRound(side * 0.65);
                        QRect iconRect(squareRect.center().x() - iconSize / 2,
                                       squareRect.center().y() - iconSize / 2,
                                       iconSize, iconSize);
                        // 🚨 物理修复 ②：传入 Qt::AlignCenter，强制占位符图标在微卡片内部绝对居中！
                        icon.paint(painter, iconRect, Qt::AlignCenter);
                    }
                }
            } else {
                QIcon icon = qvariant_cast<QIcon>(decoData);
                if (!icon.isNull()) {
                    int iconSize = qRound(side * 0.65);
                    QRect iconRect(squareRect.center().x() - iconSize / 2,
                                   squareRect.center().y() - iconSize / 2,
                                   iconSize, iconSize);
                    // 🚨 物理修复 ②：传入 Qt::AlignCenter，强制占位符图标在微卡片内部绝对居中！
                    icon.paint(painter, iconRect, Qt::AlignCenter);
                }
            }
=======
            // 2. 图像/图标平滑居中绘制（严格约束在 1:1 正方形 squareRect 内部）
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
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps（编译命令与验证方法）

1. **编译确认**：
   ```bash
   cmake --build build --config Release
   ```
2. **正方形微卡片外观验证**：
   打开包含不同长宽比图像、AI、EPS、PSD 文件的目录，切换至列表视图：
   - 验证最左侧图标与缩略图 100% 居中锁定在 1:1 正方形圆角微型卡片框内；
   - 验证绝无任何拉伸成整行高度或超出正方形边框的情况。
