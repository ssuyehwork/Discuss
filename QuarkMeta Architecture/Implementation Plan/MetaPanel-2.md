# MetaPanel 顶部预览 8px 边缘间距与 4px 统一微圆角卡片实施方案

## Overview
本方案旨在为 `MetaPanel` 的顶部图像预览卡片应用标准的 8px 边缘间距与 4px 统一微圆角规范：
1. **X 轴 100% 像素级对齐**：预览卡片宽度恒等于 `viewportW - 16px`（左右外边距严格为 8px），与下方的名称框、备注框完全同宽对齐。
2. **全局 4px 微圆角视觉语言**：样式设置深灰底盒 `#232325`、精致 1px 细边线 `#333333` 与 `border-radius: 4px`，与全系统的输入框和按钮控件保持 100% 视觉语言同构。
3. **正方形卡片与内呼吸留白**：预览卡片固定尺寸为 `cardWidth x cardWidth`，内部图片等比缩放至 `cardWidth - 16`，四周预留 8px 呼吸留白。

## Modified Files List
- `src/ui/MetaPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/MetaPanel.cpp`

```
<<<<<<< SEARCH
    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setObjectName("MetaImagePreview");
    m_lblImagePreview->setStyleSheet("background: transparent; border: none;");
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignHCenter);
=======
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
        int side = m_container ? qBound(140, m_container->width() - 16, 210) : 200;
        m_lblImagePreview->setFixedSize(side, side);

        QPixmap scaled = pixmap.scaled(QSize(side, side), Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
            syncWidthAndHeight(m_nameEdit);
            syncWidthAndHeight(m_noteEdit);
            if (m_btnAddTagBig) m_btnAddTagBig->setFixedWidth(maxW);
            
            if (m_topPreviewBox) m_topPreviewBox->setFixedWidth(maxW);
            if (m_ratingColorBox) m_ratingColorBox->setFixedWidth(maxW);
            if (m_tagBox) m_tagBox->setFixedWidth(maxW);
            if (m_tagContainer) m_tagContainer->setFixedWidth(maxW);
=======
            syncWidthAndHeight(m_nameEdit);
            syncWidthAndHeight(m_noteEdit);
            if (m_btnAddTagBig) m_btnAddTagBig->setFixedWidth(maxW);
            
            if (m_topPreviewBox) m_topPreviewBox->setFixedWidth(maxW);
            if (m_lblImagePreview && m_lblImagePreview->isVisible()) {
                m_lblImagePreview->setFixedSize(maxW, maxW);
            }
            if (m_ratingColorBox) m_ratingColorBox->setFixedWidth(maxW);
            if (m_tagBox) m_tagBox->setFixedWidth(maxW);
            if (m_tagContainer) m_tagContainer->setFixedWidth(maxW);
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 编译验证：无需修改 `CMakeLists.txt`。
2. 视觉对齐验证：调整属性面板尺寸时，预览正方形卡片宽度与下方的文件名框严格 8px 左右间距（`maxW`）对齐，并保持 4px 圆角。
