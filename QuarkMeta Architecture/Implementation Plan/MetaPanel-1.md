# MetaPanel 顶部预览“隐式正方形网格 (Implicit Square Canvas)”实施方案

## Overview
本方案旨在为 `MetaPanel` 的顶部图像预览区引入“隐式正方形网格（Implicit Square Canvas）”现代视觉呈现逻辑：
1. **隐式透明与 0 显式边框**：将预览 `QLabel` (`m_lblImagePreview`) 设置为 `background: transparent; border: none;`，让透明背景的图像/矢量图标自由居中悬浮。
2. **正方形几何约束与等比缩放**：根据 `m_container` 的实时宽度计算正方形边长 `side = qBound(140, width - 16, 210)`，并调用 `pixmap.scaled(QSize(side, side), Qt::KeepAspectRatio, Qt::SmoothTransformation)` 实现等比自适应缩放，彻底消灭横向/纵向的物理截断与硬切缺陷。

## Modified Files List
- `src/ui/MetaPanel.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/MetaPanel.cpp`

```
<<<<<<< SEARCH
    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setMinimumHeight(60);
    m_lblImagePreview->setStyleSheet("background: transparent;");
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview);
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
    } else {
        QPixmap scaled = pixmap.scaled(QSize(220, 140), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_lblImagePreview->setPixmap(scaled);
        m_lblImagePreview->show();
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
>>>>>>> REPLACE
```

## Build & Verification Steps
1. 编译验证：无需修改 `CMakeLists.txt`。
2. 逻辑验证：选中长图、宽图、异形 PNG/SVG 时，图片在正方形隐式网格中居中悬浮，细节全显，无切割截断。
