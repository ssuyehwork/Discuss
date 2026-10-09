# Implementation Plan - MetaPanel Preview Size & Palette Alignment Adjustment

## Architecture Gate 3-Question Answers (架构三问回答)
1. **SSOT Source (真理源溯源)**:
   The UI layout specifications and widget geometry for the top preview box (`m_topPreviewBox`), image preview label (`m_lblImagePreview`), and palette flow layout (`m_paletteFlowLayout`) are strictly owned and managed by `MetaPanel` and `FlowLayout`.
2. **Black-box Integrity (黑盒完整性)**:
   `FlowLayout` exposes a clean public layout alignment API (`setAlignment(Qt::Alignment)`). `MetaPanel` configures its child widgets via public/protected layout parameters without exposing internal widget implementations.
3. **Root Cause Analysis (根因 vs 症状)**:
   Previously, `m_topPreviewBox` and `m_lblImagePreview` were hardcoded to `210x210px`, spacing was set to `0px`, and `FlowLayout` lacked horizontal alignment support, defaulting to left alignment. By adding alignment support to `FlowLayout` and updating `MetaPanel`'s layout parameters (200x200px preview, 10px vertical spacing, and `Qt::AlignHCenter` palette alignment), the UI is cleanly updated according to specification.

---

## 1. Overview
This plan adjusts the top preview box in `MetaPanel`:
1. Reduces the image preview size (`m_lblImagePreview`) and top preview box width from `210x210px` to **`200x200px`**.
2. Adds a **`10px`** vertical spacing between `m_lblImagePreview` and `m_paletteContainer`.
3. Adds `Qt::Alignment` support to `FlowLayout` and configures `m_paletteFlowLayout` to align palette color pills horizontally centered (`Qt::AlignHCenter`).

---

## 2. Modified Files List
1. `src/ui/components/FlowLayout.h`
2. `src/ui/components/FlowLayout.cpp`
3. `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/components/FlowLayout.h`
Add `setAlignment` and `alignment` public member functions and `m_alignment` member variable.

```
<<<<<<< SEARCH
    QSize sizeHint() const override;
    QLayoutItem *takeAt(index) override;
private:
    int doLayout(const QRect &rect, bool testOnly) const;
    QList<QLayoutItem *> itemList;
    int m_hSpace;
    int m_vSpace;
};
=======
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

    void setAlignment(Qt::Alignment align) { m_alignment = align; invalidate(); }
    Qt::Alignment alignment() const { return m_alignment; }

private:
    int doLayout(const QRect &rect, bool testOnly) const;
    QList<QLayoutItem *> itemList;
    int m_hSpace;
    int m_vSpace;
    Qt::Alignment m_alignment = Qt::AlignLeft;
};
>>>>>>> REPLACE
```

### File 2: `src/ui/components/FlowLayout.cpp`
Update `doLayout` to calculate and apply line-by-line horizontal offset based on `m_alignment`.

```
<<<<<<< SEARCH
int FlowLayout::doLayout(const QRect &rect, bool testOnly) const {
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;
    for (QLayoutItem *item : itemList) {
        int spaceX = horizontalSpacing();
        int spaceY = verticalSpacing();
        int nextX = x + item->sizeHint().width() + spaceX;
        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + item->sizeHint().width() + spaceX;
            lineHeight = 0;
        }
        if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }
    return y + lineHeight - rect.y() + bottom;
}
=======
int FlowLayout::doLayout(const QRect &rect, bool testOnly) const {
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    QList<QLayoutItem*> currentLineItems;

    auto applyLineAlignment = [this, &effectiveRect](const QList<QLayoutItem*>& items, int lineWidth) {
        if (items.isEmpty()) return;
        int xOffset = 0;
        if (m_alignment & Qt::AlignHCenter) {
            xOffset = qMax(0, (effectiveRect.width() - lineWidth) / 2);
        } else if (m_alignment & Qt::AlignRight) {
            xOffset = qMax(0, effectiveRect.width() - lineWidth);
        }
        if (xOffset > 0) {
            for (QLayoutItem* item : items) {
                QRect g = item->geometry();
                item->setGeometry(g.translated(xOffset, 0));
            }
        }
    };

    for (QLayoutItem *item : itemList) {
        int spaceX = horizontalSpacing();
        int spaceY = verticalSpacing();
        int itemW = item->sizeHint().width();
        int nextX = x + itemW + spaceX;

        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            if (!testOnly) {
                int lineWidth = x - spaceX - effectiveRect.x();
                applyLineAlignment(currentLineItems, lineWidth);
            }
            currentLineItems.clear();

            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + itemW + spaceX;
            lineHeight = 0;
        }

        if (!testOnly) {
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
            currentLineItems.append(item);
        }

        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }

    if (!testOnly && !currentLineItems.isEmpty()) {
        int lineWidth = x - horizontalSpacing() - effectiveRect.x();
        applyLineAlignment(currentLineItems, lineWidth);
    }

    return y + lineHeight - rect.y() + bottom;
}
>>>>>>> REPLACE
```

### File 3: `src/ui/MetaPanel.cpp`
Update preview sizes to `200x200px`, vertical spacing to `10px`, and set `m_paletteFlowLayout->setAlignment(Qt::AlignHCenter)`.

```
<<<<<<< SEARCH
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
    m_lblImagePreview->setObjectName("MetaImagePreview");
    // MetaImagePreview style in style.qss
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignCenter);

    m_paletteContainer = new QWidget(m_topPreviewBox);
    m_paletteFlowLayout = new FlowLayout(m_paletteContainer, 0, 4, 4);
    previewLayout->addWidget(m_paletteContainer);
=======
    // 1. 顶部预览与色板区
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    m_topPreviewBox->setFixedWidth(200);
    // TopPreviewBox style in style.qss
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(10); // 10px 物理垂直间距

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setFixedSize(200, 200);
    m_lblImagePreview->setObjectName("MetaImagePreview");
    // MetaImagePreview style in style.qss
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignCenter);

    m_paletteContainer = new QWidget(m_topPreviewBox);
    m_paletteFlowLayout = new FlowLayout(m_paletteContainer, 0, 4, 4);
    m_paletteFlowLayout->setAlignment(Qt::AlignHCenter); // 水平居中对齐色块
    previewLayout->addWidget(m_paletteContainer);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
=======
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox) {
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        if (hasPreview) {
            m_topPreviewBox->show();
            int paletteH = (m_paletteContainer && m_paletteContainer->isVisible()) ? m_paletteContainer->height() : 0;
            int totalH = 200 + (paletteH > 0 ? (10 + paletteH) : 0);
            m_topPreviewBox->setFixedSize(200, totalH);
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
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
=======
        QSize canvasSize(200, 200);
        QPixmap canvas(canvasSize);
        canvas.fill(Qt::transparent);

        QSize targetSize = isDefaultIcon ? QSize(35, 45) : QSize(200, 200);
        QPixmap scaled = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        {
            QPainter painter(&canvas);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            int x = (200 - scaled.width()) / 2;
            int y = (200 - scaled.height()) / 2;
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute `mkdir build && cd build && cmake .. && cmake --build .`.
2. Ensure there are zero compilation errors or warnings.
3. Open QuarkMeta, select an image file, and inspect the right-hand `MetaPanel`:
   - Verify `m_lblImagePreview` renders at **`200x200px`**.
   - Verify `m_paletteContainer` is separated from `m_lblImagePreview` by **`10px`** vertical spacing.
   - Verify palette color pills in `m_paletteContainer` are **horizontally centered** (`Qt::AlignHCenter`).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Zero Numerical Modification Breach**: 200x200px preview and 10px spacing match user specifications 100%.
- **Zero Patching**: `FlowLayout` alignment implemented as a clean, reusable layout alignment mechanism (`setAlignment`).

---

## 6. Header API Signature Verification
| Class / Function | Declared Header | Verified Exact Signature |
| :--- | :--- | :--- |
| `FlowLayout::setAlignment` | `src/ui/components/FlowLayout.h` | `void setAlignment(Qt::Alignment align);` |
| `FlowLayout::alignment` | `src/ui/components/FlowLayout.h` | `Qt::Alignment alignment() const;` |

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/MetaPanel.cpp`: Uses `FlowLayout.h`, which includes `<QLayout>`. `Qt::AlignHCenter` is defined in `<Qt>` / `<QNamespace>`.
