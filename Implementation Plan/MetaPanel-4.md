# MetaPanel Implementation Plan - Version 4

## 1. Overview
This implementation plan addresses the top preview size issue in `MetaPanel`:
Adjust the top preview box (`m_topPreviewBox`) and image label (`m_lblImagePreview`) canvas size from `220x220` pixels to **`210x210` pixels**. This provides proper 10px margins on both sides of the 230px-wide `MetaPanel` container, removing edge touching and 0-margin display bugs.

---

## 2. Modified Files List
1. `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change: `src/ui/MetaPanel.cpp`
Update preview box sizes and canvas rendering calculations from `220x220` to `210x210`.

```
<<<<<<< SEARCH
    // 1. 顶部预览与色板区
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
=======
    // 1. 顶部预览与色板区
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    m_topPreviewBox->setFixedSize(210, 210);
    // TopPreviewBox style in style.qss
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setFixedSize(210, 210);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
=======
void MetaPanel::setImagePreview(const QPixmap& pixmap, bool isDefaultIcon) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        QSize canvasSize(210, 210);
        QPixmap canvas(canvasSize);
        canvas.fill(Qt::transparent);

        QSize targetSize = isDefaultIcon ? QSize(35, 45) : QSize(210, 210);
        QPixmap scaled = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        {
            QPainter painter(&canvas);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            int x = (210 - scaled.width()) / 2;
            int y = (210 - scaled.height()) / 2;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
=======
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox) {
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        if (hasPreview) {
            m_topPreviewBox->show();
            m_topPreviewBox->setFixedSize(210, 210);
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Skip compilation in sandbox environment as Qt6 is not installed.
2. Verify that top preview box is resized to `210x210` px in `MetaPanel.cpp`.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Metadata SSOT**: Directly reuses `MetaPanel::setImagePreview` and canvas centering math.

---

## 6. Header API Signature Verification
- `MetaPanel::setImagePreview(const QPixmap&, bool)`: Double-checked in `MetaPanel.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/MetaPanel.cpp` includes `<QPainter>`, `<QPainterPath>`, `<QPixmap>`.
