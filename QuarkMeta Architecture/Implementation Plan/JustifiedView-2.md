# Implementation Plan - JustifiedView SVG Vector Icon Refactoring (`JustifiedView-2.md`)

## Overview
This implementation plan updates `JustifiedView-1.md` by replacing the draft Unicode text character arrow ("▶ / ▼") with proper SVG vector icon rendering using `UiHelper::getIcon`.
This ensures 100% visual consistency with `FolderSectionHeaderBar`, support for high-DPI scaling without font distortion, and dynamic theme coloring (`#3498db`).

---

## Modified Files List
- `src/ui/JustifiedView.h`
- `src/ui/JustifiedView.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/JustifiedView.cpp` (Include `UiHelper.h`)

```
<<<<<<< SEARCH
#include "JustifiedView.h"
#include <QPainter>
=======
#include "JustifiedView.h"
#include "UiHelper.h"
#include <QPainter>
>>>>>>> REPLACE
```

---

### 2. `src/ui/JustifiedView.cpp` (SVG Vector Icon Drawing in `paintEvent`)

```
<<<<<<< SEARCH
        if (geo.isHeader) {
            painter.save();
            painter.fillRect(geo.rect, QColor("#202020"));
            painter.setPen(QColor("#CCCCCC"));
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            QString text = (geo.isFolderGroup ? (m_folderGroupCollapsed ? "▶ " : "▼ ") : "") + geo.headerText;
            painter.drawText(geo.rect.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
            painter.restore();
        }
=======
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
>>>>>>> REPLACE
```

---

## Build & Verification Steps

1. Compile the project:
   ```bash
   cmake -B build
   cmake --build build --config Debug
   ```
2. Verify that section headers render sharp SVG vector arrows (`scroll-008.svg` and `scroll-010.svg`) colored with `#3498db`.

---

## SSOT API Reuse & Anti-Redundancy Self-Check

- **`UiHelper::getIcon` SSOT**: Reuses the project-wide SVG icon loader and renderer (`UiHelper.h`).
- **SVG Icon Asset Alignment**: Aligns arrow icons with existing `FolderSectionHeaderBar` (`scroll-008.svg` / `scroll-010.svg`).

---

## Header API Signature Verification Table

| File | Class / Function | Exact Physical Signature | Verification Result |
| :--- | :--- | :--- | :--- |
| `src/ui/UiHelper.h` | `UiHelper` | `static QIcon getIcon(const QString& name, const QColor& color = QColor(), int size = 16);` | Verified 100% Match |
