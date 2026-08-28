# MetaPanel 顶部预览“图片自身 4px 自适应圆角与高度紧贴”实施方案

## Overview
本方案旨在为 `MetaPanel` 的顶部图像预览实现“图片自身 4 像素自适应圆角（Adaptive 4px Rounded Pixmap）与高度紧贴（0 死黑缝隙）”自适应悬浮视觉：
1. **彻底移除死板外框与黑底盒**：`m_lblImagePreview` 设置为 `background: transparent; border: none;`，消灭任何外层硬黑底盒。
2. **图片边缘 4px 平滑圆角烘焙**：利用 `QPainter` + `QPainterPath` (`addRoundedRect(..., 4.0, 4.0)`) 在内存中将缩放后的 `QPixmap` 直接裁切烘焙出 4 像素自适应圆角。
3. **尺寸紧贴图片本身**：`m_lblImagePreview->setFixedSize(scaled.size())`，控件高度严格随原图宽高比自适应伸展，彻底消灭上下两端大面积死黑缝隙。

## Modified Files List
- `src/ui/MetaPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/MetaPanel.cpp`

```
<<<<<<< SEARCH
    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setObjectName("MetaImagePreviewCard");
    m_lblImagePreview->setStyleSheet(
        "QLabel#MetaImagePreviewCard {"
        "  background-color: #232325;"
        "  border: 1px solid #333333;"
        "  border-radius: 4px;"
        "}"
    );
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignHCenter);
=======
    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setObjectName("MetaImagePreview");
    m_lblImagePreview->setStyleSheet("background: transparent; border: none;");
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignHCenter);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        int cardWidth = m_container ? (m_container->width() - 16) : 214;
        cardWidth = qBound(120, cardWidth, 230);
        m_lblImagePreview->setFixedSize(cardWidth, cardWidth);

        int innerSize = cardWidth - 16;
        QPixmap scaled = pixmap.scaled(QSize(innerSize, innerSize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_lblImagePreview->setPixmap(scaled);

        m_lblImagePreview->show();
        if (m_topPreviewBox) m_topPreviewBox->show();
    }
    adjustFlowHeights();
    if (m_container) m_container->adjustSize();
}
=======
void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        int maxW = m_container ? (m_container->width() - 16) : 214;
        maxW = qBound(120, maxW, 230);
        int maxH = 220;

        QPixmap scaled = pixmap.scaled(QSize(maxW, maxH), Qt::KeepAspectRatio, Qt::SmoothTransformation);

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

        m_lblImagePreview->show();
        if (m_topPreviewBox) m_topPreviewBox->show();
    }
    adjustFlowHeights();
    if (m_container) m_container->adjustSize();
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 编译验证：无需修改 `CMakeLists.txt`。
2. 视觉自适应验证：选中横图/竖图时，外层无黑色方盒框，图片边界自带 4px 细腻平滑圆角，且控件高度严格紧贴图片。
