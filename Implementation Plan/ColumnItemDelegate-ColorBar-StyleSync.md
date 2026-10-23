# Implementation Plan - ColumnItemDelegate ColorBar & Style Sync

## 1. Overview
This implementation plan addresses three visual and structural refinements in `ColumnViewWidget` and `ColumnItemDelegate`:
1. **Color Bar Redesign**: Replace the small color dot with a 5px wide vertical color bar on the left edge of each item row when a color tag exists.
2. **Rating Star Color Normalization**: Change rating star icon and text color in Column View from yellow (`#FFC107`) to the application-wide active orange (`#FF551C`, `Style::ActiveOrange`).
3. **Column Width Standardization**: Set `ColumnViewPane` width from 240px to 230px to align with all other main application panel widths.

---

## 2. Modified Files List
- `src/ui/ColumnItemDelegate.cpp`
- `src/ui/ColumnViewWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ColumnItemDelegate.cpp`

<<<<<<< SEARCH
    // 绘制色标圆点 (在左侧 2px 处)
    if (!colorName.isEmpty()) {
        static const QMap<QString, QString> s_colorHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };
        QString hexColor = s_colorHexMap.value(colorName, colorName);
        if (hexColor.startsWith("#")) {
            painter->setBrush(QColor(hexColor));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(option.rect.left() + 2, option.rect.top() + (option.rect.height() - 6) / 2, 6, 6);
        }
    }
=======
    // 绘制 5px 宽度的左侧垂直色条
    if (!colorName.isEmpty()) {
        static const QMap<QString, QString> s_colorHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };
        QString hexColor = s_colorHexMap.value(colorName, colorName);
        if (hexColor.startsWith("#")) {
            painter->setBrush(QColor(hexColor));
            painter->setPen(Qt::NoPen);
            QRect colorBarRect(option.rect.left(), option.rect.top(), 5, option.rect.height());
            painter->drawRect(colorBarRect);
        }
    }
>>>>>>> REPLACE

<<<<<<< SEARCH
    // 5. 绘制星级标示 (若 rating > 0)
    if (rating > 0) {
        int starRight = option.rect.right() - (isDir ? 22 : 6);
        int starSize = 12;
        int numWidth = 12;
        QRect starIconRect(starRight - numWidth - starSize, option.rect.top() + (option.rect.height() - starSize) / 2, starSize, starSize);
        QRect numRect(starRight - numWidth, option.rect.top() + (option.rect.height() - starSize) / 2, numWidth, starSize);

        QIcon starIcon = UiHelper::getIcon("star_filled", QColor("#FFC107"), starSize);
        starIcon.paint(painter, starIconRect, Qt::AlignCenter);

        painter->setPen(QColor("#FFC107"));
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter->drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(rating));
    }
=======
    // 5. 绘制星级标示 (若 rating > 0)
    if (rating > 0) {
        int starRight = option.rect.right() - (isDir ? 22 : 6);
        int starSize = 12;
        int numWidth = 12;
        QRect starIconRect(starRight - numWidth - starSize, option.rect.top() + (option.rect.height() - starSize) / 2, starSize, starSize);
        QRect numRect(starRight - numWidth, option.rect.top() + (option.rect.height() - starSize) / 2, numWidth, starSize);

        QColor orangeColor = QColor("#FF551C");
        QIcon starIcon = UiHelper::getIcon("star_filled", orangeColor, starSize);
        starIcon.paint(painter, starIconRect, Qt::AlignCenter);

        painter->setPen(orangeColor);
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter->drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(rating));
    }
>>>>>>> REPLACE


### 3.2 `src/ui/ColumnViewWidget.cpp`

<<<<<<< SEARCH
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 240;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}
=======
void ColumnViewWidget::updatePaneWidths() {
    if (m_panes.isEmpty()) return;
    int defaultWidth = 230;
    for (auto* pane : m_panes) {
        pane->setFixedWidth(defaultWidth);
        pane->setMinimumWidth(defaultWidth);
        pane->setMaximumWidth(defaultWidth);
    }
}
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`
2. Launch QuarkMeta app.
3. Switch to Column View and inspect items with color tags to confirm the 5px vertical color bar on the left edge.
4. Verify rating stars and rating numbers render in `#FF551C` (orange).
5. Verify column width is 230px matching navigation & metadata panel width.
