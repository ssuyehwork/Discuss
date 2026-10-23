# MetaPanel Preview Fixed Dimensions Implementation Plan (MetaPanel-2.md)

> **Note**: As per AGENTS.md Protocol 3.1 (Immutable Plan Files), this is an iterative implementation plan (`MetaPanel-2.md`) to strictly enforce the top preview area dimensions in `MetaPanel`:
> - The top preview image/icon container (`m_lblImagePreview` and `m_topPreviewBox`) must be rigidly fixed at **220x220 pixels**.

## 1. Overview
This implementation plan addresses the exact sizing contract for `MetaPanel`'s top preview widget:
- Both `m_lblImagePreview` and `m_topPreviewBox` will be explicitly constrained to `setFixedSize(220, 220)`.
- When rendering previews in `setImagePreview`, content will be rendered centered onto a 220x220 transparent canvas so the label and preview box maintain a rigid 220x220px aspect and size regardless of image aspect ratio or icon dimensions.

---

## 2. Modified Files List
1. `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/MetaPanel.cpp` - Lock Initial Container Dimensions
In `MetaPanel::initUi`, set `m_topPreviewBox` and `m_lblImagePreview` to fixed size `220x220`.

```
<<<<<<< SEARCH
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    // TopPreviewBox style in style.qss
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(6);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setObjectName("MetaImagePreview");
    // MetaImagePreview style in style.qss
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignHCenter);
=======
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    m_topPreviewBox->setFixedSize(220, 220);
    // TopPreviewBox style in style.qss
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setFixedSize(220, 220);
    m_lblImagePreview->setObjectName("MetaImagePreview");
    // MetaImagePreview style in style.qss
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignCenter);
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/MetaPanel.cpp` - Canvas Centered 220x220 Image Preview
In `MetaPanel::setImagePreview`, draw preview pixmaps scaled proportionally and centered on a 220x220 transparent canvas.

```
<<<<<<< SEARCH
void MetaPanel::setImagePreview(const QPixmap& pixmap, bool isDefaultIcon) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        if (isDefaultIcon) {
            // 非图形图像（默认文件/文件夹图标）：预览视口 Canvas 设定为 220x220px，中间绘制 35x45px 图标
            QSize canvasSize(220, 220);
            QPixmap canvas(canvasSize);
            canvas.fill(Qt::transparent);

            // 强制将默认 OS 图标等比例缩放到 35x45 目标范围
            QPixmap iconScaled = pixmap.scaled(QSize(35, 45), Qt::KeepAspectRatio, Qt::SmoothTransformation);

            {
                QPainter painter(&canvas);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);

                int x = (220 - iconScaled.width()) / 2;
                int y = (220 - iconScaled.height()) / 2;
                painter.drawPixmap(x, y, iconScaled);
            }

            m_lblImagePreview->setPixmap(canvas);
            m_lblImagePreview->setFixedSize(canvasSize);
        } else {
            int maxW = m_container ? (m_container->width() - 16) : 214;
            maxW = qBound(120, maxW, 230);
            int maxH = 220;

            QPixmap scaled = (pixmap.width() > maxW || pixmap.height() > maxH)
                ? pixmap.scaled(QSize(maxW, maxH), Qt::KeepAspectRatio, Qt::SmoothTransformation)
                : pixmap;

            QImage roundedImg(scaled.size(), QImage::Format_ARGB32_Premultiplied);
            roundedImg.fill(Qt::transparent);
            {
                QPainter painter(&roundedImg);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);

                QPainterPath path;
                path.addRoundedRect(QRectF(0, 0, scaled.width(), scaled.height()), 4.0, 4.0);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, scaled);
            }

            m_lblImagePreview->setPixmap(QPixmap::fromImage(roundedImg));
            m_lblImagePreview->setFixedSize(scaled.size());
        }

        m_lblImagePreview->show();
        if (m_topPreviewBox) m_topPreviewBox->show();
    }
    m_adjustTimer->start();
}
=======
void MetaPanel::setImagePreview(const QPixmap& pixmap, bool isDefaultIcon) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        QSize canvasSize(220, 220);
        QPixmap canvas(canvasSize);
        canvas.fill(Qt::transparent);

        QSize targetSize = isDefaultIcon ? QSize(35, 45) : QSize(220, 220);
        QPixmap scaled = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        {
            QPainter painter(&canvas);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            int x = (220 - scaled.width()) / 2;
            int y = (220 - scaled.height()) / 2;

            if (!isDefaultIcon) {
                QPainterPath path;
                path.addRoundedRect(QRectF(x, y, scaled.width(), scaled.height()), 4.0, 4.0);
                painter.setClipPath(path);
            }
            painter.drawPixmap(x, y, scaled);
        }

        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setFixedSize(canvasSize);

        m_lblImagePreview->show();
        if (m_topPreviewBox) {
            m_topPreviewBox->setFixedSize(canvasSize);
            m_topPreviewBox->show();
        }
    }
    m_adjustTimer->start();
}
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/MetaPanel.cpp` - Maintain Fixed Size in `adjustFlowHeights`
Ensure `adjustFlowHeights` respects the 220x220 fixed dimensions.

```
<<<<<<< SEARCH
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox && m_paletteFlowLayout) {
        int contentH = m_paletteFlowLayout->heightForWidth(m_topPreviewBox->width());
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        bool hasPalette = (m_paletteFlowLayout->count() > 0);
        if (hasPreview || hasPalette) {
            m_topPreviewBox->show();
            int previewH = hasPreview ? m_lblImagePreview->pixmap().height() : 0;
            int totalSpacing = (hasPreview && hasPalette) ? 6 : 0;
            m_topPreviewBox->setFixedHeight(contentH + previewH + totalSpacing);
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
        m_paletteFlowLayout->activate();
    }
=======
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox) {
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        if (hasPreview) {
            m_topPreviewBox->show();
            m_topPreviewBox->setFixedSize(220, 220);
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build and verify:
   - Select any image or file in `ContentPanel`.
   - Verify that the top preview container (`m_topPreviewBox`) and preview image (`m_lblImagePreview`) are rigidly fixed to **220x220px**.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Directly modifies `MetaPanel.cpp` preview sizing logic without altering frozen public `.h` API contracts or creating redundant controls.
