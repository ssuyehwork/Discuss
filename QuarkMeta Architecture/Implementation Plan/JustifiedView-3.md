# Implementation Plan - JustifiedView Section Header Transparent Background Refactoring (`JustifiedView-3.md`)

## Overview
This implementation plan updates `JustifiedView-2.md` by removing the hardcoded background fill (`painter.fillRect(geo.rect, QColor("#202020"))`) in `JustifiedView::paintEvent`.
It restores the exact original styling from `FolderSectionHeaderBar` (`resources/style.qss`), where section headers have a **`transparent`** background, letting the underlying View background color show through naturally.

---

## Modified Files List
- `src/ui/JustifiedView.cpp`

---

## Detailed Line-by-Line Changes

### `src/ui/JustifiedView.cpp` (Transparent Background in `paintEvent`)

```
<<<<<<< SEARCH
        if (geo.isHeader) {
            painter.save();
            painter.fillRect(geo.rect, QColor("#202020"));

            const int iconSize = 12;
            const int marginX = 10;
            const QColor headerColor("#3498db");

            if (geo.isFolderGroup) {
                // Render SVG vector collapse/expand arrow icon via UiHelper
                const QString iconName = m_folderGroupCollapsed ? "scroll-008.svg" : "scroll-010.svg";
                QPixmap arrowPixmap = UiHelper::getIcon(iconName, headerColor, iconSize).pixmap(iconSize, iconSize);
                int iconY = geo.rect.top() + (geo.rect.height() - iconSize) / 2;
                painter.drawPixmap(geo.rect.left() + marginX, iconY, arrowPixmap);

                // Render group title text next to the SVG icon
                QRect textRect = geo.rect.adjusted(marginX + iconSize + 6, 0, -marginX, 0);
                painter.setPen(headerColor);
                painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, geo.headerText);
            } else {
                QRect textRect = geo.rect.adjusted(marginX, 0, -marginX, 0);
                painter.setPen(headerColor);
                painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, geo.headerText);
            }

            painter.restore();
        }
=======
        if (geo.isHeader) {
            painter.save();
            // Transparent background - do NOT paint a dark rectangle, matching QFrame#FolderSectionHeaderBar { background: transparent; }

            const int iconSize = 12;
            const int marginX = 10;
            const QColor headerColor("#3498db");

            if (geo.isFolderGroup) {
                // Render group title text first
                painter.setPen(headerColor);
                painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));

                QFontMetrics fm(painter.font());
                int textWidth = fm.horizontalAdvance(geo.headerText);
                QRect textRect(geo.rect.left() + marginX, geo.rect.top(), textWidth + 4, geo.rect.height());
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, geo.headerText);

                // Render SVG vector collapse/expand arrow icon right after the text
                const QString iconName = m_folderGroupCollapsed ? "scroll-008.svg" : "scroll-010.svg";
                QPixmap arrowPixmap = UiHelper::getIcon(iconName, headerColor, iconSize).pixmap(iconSize, iconSize);
                int iconX = textRect.right() + 4;
                int iconY = geo.rect.top() + (geo.rect.height() - iconSize) / 2;
                painter.drawPixmap(iconX, iconY, arrowPixmap);
            } else {
                QRect textRect = geo.rect.adjusted(marginX, 0, -marginX, 0);
                painter.setPen(headerColor);
                painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
                painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, geo.headerText);
            }

            painter.restore();
        }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Compile the project:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. Verify that section headers have transparent backgrounds (matching the View) and arrow icons sit compactly after the section text.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`resources/style.qss` SSOT**: Aligns header background with `QFrame#FolderSectionHeaderBar { background: transparent; }`.
- **`FolderSectionWidget.cpp` Layout**: Restores title label + arrow layout order without extra dark fill.

---

## Header API Signature Verification Table

| File | Class / Function | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `resources/style.qss` | `QFrame#FolderSectionHeaderBar` | `background: transparent; border: none;` | Verified 100% Match |
