# Implementation Plan - MetaPanel Top Preview Centering & Palette Flow Alignment Fix

## Architecture Gate 3-Question Answers (架构三问回答)
1. **SSOT Source (真理源溯源)**:
   The UI layout specifications and geometry alignment for `m_topPreviewBox`, `m_lblImagePreview`, and `m_paletteFlowLayout` are solely owned by `MetaPanel` and `FlowLayout`.
2. **Black-box Integrity (黑盒完整性)**:
   `FlowLayout` uses a clean line-by-line flow-alignment method (`setFlowAlignment(Qt::AlignHCenter)`). `MetaPanel` aligns child widgets inside parent layouts using standard Qt alignment flags (`Qt::AlignHCenter`).
3. **Root Cause Analysis (根因 vs 症状)**:
   - **Root Cause 1**: `m_topPreviewBox` was added to `m_containerLayout` without `Qt::AlignHCenter`, causing the top preview box to sit left-aligned inside `MetaPanel` with a 14px gap on the right.
   - **Root Cause 2**: `FlowLayout::doLayout` previously calculated items sequentially without line-by-line width aggregation, causing palette pills (`ColorPill`) to remain left-aligned.

---

## 1. Overview
This plan resolves both issues shown in `image.png`:
1. Adds `Qt::AlignHCenter` when adding `m_topPreviewBox` to `m_containerLayout`, ensuring the top preview box and image preview (`m_lblImagePreview`, 200x200px) are **horizontally centered** within `MetaPanel`.
2. Refactors `FlowLayout::doLayout` to collect items line-by-line and center-align each line (`xOffset = (effectiveRect.width() - lineWidth) / 2`), ensuring palette color pills in `m_paletteContainer` are **100% horizontally centered**.

---

## 2. Modified Files List
1. `src/ui/components/FlowLayout.cpp`
2. `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### File 1: `src/ui/components/FlowLayout.cpp`
Refactor `doLayout` to calculate and center-align items line-by-line based on `m_alignment`.

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
    int y = effectiveRect.y();
    int lineHeight = 0;

    struct LineItem {
        QLayoutItem* item;
        int width;
        int height;
    };
    QList<LineItem> currentLine;

    auto flushLine = [this, &effectiveRect, &y, &lineHeight, testOnly](QList<LineItem>& line, int lineWidth) {
        if (line.isEmpty()) return;
        int xOffset = 0;
        if (m_alignment & Qt::AlignHCenter) {
            xOffset = qMax(0, (effectiveRect.width() - lineWidth) / 2);
        } else if (m_alignment & Qt::AlignRight) {
            xOffset = qMax(0, effectiveRect.width() - lineWidth);
        }

        int currX = effectiveRect.x() + xOffset;
        for (const auto& li : line) {
            if (!testOnly) {
                li.item->setGeometry(QRect(QPoint(currX, y), li.item->sizeHint()));
            }
            currX += li.width + horizontalSpacing();
        }
        y += lineHeight + verticalSpacing();
        line.clear();
        lineHeight = 0;
    };

    for (QLayoutItem *item : itemList) {
        int itemW = item->sizeHint().width();
        int itemH = item->sizeHint().height();

        int currentLineWidth = 0;
        for (const auto& li : currentLine) {
            currentLineWidth += li.width + horizontalSpacing();
        }

        if (!currentLine.isEmpty() && (currentLineWidth + itemW > effectiveRect.width())) {
            flushLine(currentLine, currentLineWidth - horizontalSpacing());
        }

        currentLine.append({item, itemW, itemH});
        lineHeight = qMax(lineHeight, itemH);
    }

    if (!currentLine.isEmpty()) {
        int currentLineWidth = 0;
        for (const auto& li : currentLine) {
            currentLineWidth += li.width + horizontalSpacing();
        }
        flushLine(currentLine, currentLineWidth - horizontalSpacing());
    }

    return y - rect.y() + bottom;
}
>>>>>>> REPLACE
```

### File 2: `src/ui/MetaPanel.cpp`
Add `Qt::AlignHCenter` when adding `m_topPreviewBox` to `m_containerLayout`.

```
<<<<<<< SEARCH
    m_topPreviewBox->hide();
    m_containerLayout->addWidget(m_topPreviewBox);
=======
    m_topPreviewBox->hide();
    m_containerLayout->addWidget(m_topPreviewBox, 0, Qt::AlignHCenter);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Build the project using `mkdir build && cd build && cmake .. && cmake --build .`.
2. Inspect `MetaPanel` with an image selected:
   - Verify `m_topPreviewBox` and `m_lblImagePreview` are **horizontally centered** within `MetaPanel`.
   - Verify `m_paletteContainer` palette pills are **horizontally centered** across every row (`Qt::AlignHCenter`).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Zero Patching**: Layout alignment uses Qt standard `Qt::AlignHCenter` and `FlowLayout::setFlowAlignment`.

---

## 6. Header API Signature Verification
| Class / Function | Declared Header | Verified Exact Signature |
| :--- | :--- | :--- |
| `FlowLayout::setFlowAlignment` | `src/ui/components/FlowLayout.h` | `void setFlowAlignment(Qt::Alignment alignment);` |
| `FlowLayout::flowAlignment` | `src/ui/components/FlowLayout.h` | `Qt::Alignment flowAlignment() const;` |

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/MetaPanel.cpp`: `m_containerLayout` is a `QVBoxLayout*`. `addWidget(QWidget*, int stretch, Qt::Alignment alignment)` is a standard Qt layout overload.
