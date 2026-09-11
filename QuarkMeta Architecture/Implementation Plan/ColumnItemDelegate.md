# ColumnItemDelegate Implementation Plan (ColumnItemDelegate.md)

## 1. Overview
This implementation plan specifies switching the item delegate used in Column View (`ColumnViewWidget` / `ColumnViewPane`) from `TreeItemDelegate` to a dedicated `ColumnItemDelegate`.
By isolating Column View rendering from `TreeItemDelegate`'s `RowLayoutEngine` square card logic, we eliminate layout collision issues (such as offset SVG icons, overlapping text, and arrow alignment issues in Miller Columns).

## 2. Modified Files List
- `src/ui/ColumnItemDelegate.h` (Enhanced single-row delegate implementation)
- `src/ui/ColumnViewWidget.cpp` (Switch `m_listView->setItemDelegate(...)` to use `ColumnItemDelegate`)

## 3. Detailed Line-by-Line Changes

### 3.1 Update `src/ui/ColumnItemDelegate.h`

```cpp
<<<<<<< SEARCH
        // 2. 绘制图标
        QVariant deco = index.data(Qt::DecorationRole);
        QRect iconRect(option.rect.left() + 8, option.rect.top() + (option.rect.height() - 18) / 2, 18, 18);
        if (deco.canConvert<QIcon>()) {
            QIcon icon = deco.value<QIcon>();
            if (!icon.isNull()) {
                icon.paint(painter, iconRect, Qt::AlignCenter);
            }
        }

        // 3. 绘制文字
        QString name = index.data(Qt::DisplayRole).toString();
        QRect textRect = option.rect.adjusted(32, 0, -28, 0);
        QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
        painter->setPen(textColor);
        painter->setFont(option.font);
        QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

        // 4. 如果是文件夹，最右侧绘制向右箭头 chevron_right
        bool isDir = index.data(Qt::UserRole + 2).toBool();
        if (isDir) {
            QRect arrowRect(option.rect.right() - 20, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
            QColor arrowColor = selected ? QColor("#FFFFFF") : QColor("#888888");
            UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
        }
=======
        // 2. 绘制图标或缩略图 (精确定位 18x18px)
        QRect iconRect(option.rect.left() + 8, option.rect.top() + (option.rect.height() - 18) / 2, 18, 18);
        QVariant deco = index.data(Qt::DecorationRole);
        if (deco.canConvert<QIcon>()) {
            QIcon icon = deco.value<QIcon>();
            if (!icon.isNull()) {
                icon.paint(painter, iconRect, Qt::AlignCenter);
            }
        } else if (deco.canConvert<QPixmap>()) {
            QPixmap pix = deco.value<QPixmap>();
            if (!pix.isNull()) {
                painter->drawPixmap(iconRect, pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }

        // 3. 绘制文件名
        QString name = index.data(Qt::DisplayRole).toString();
        QRect textRect = option.rect.adjusted(32, 0, -24, 0);
        QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
        painter->setPen(textColor);
        painter->setFont(option.font);
        QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

        // 4. 如果是文件夹，最右侧绘制向右箭头 chevron_right；若是空文件夹可辅助描边
        bool isDir = index.data(TypeRole).toString() == "folder" || index.data(Qt::UserRole + 2).toBool();
        bool isEmpty = index.data(IsEmptyRole).toBool();

        if (isDir) {
            QRect arrowRect(option.rect.right() - 18, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
            QColor arrowColor = selected ? QColor("#FFFFFF") : (isEmpty ? QColor("#41F2F2") : QColor("#888888"));
            UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
        }
>>>>>>> REPLACE
```

### 3.2 Update `src/ui/ColumnViewWidget.cpp`

```cpp
<<<<<<< SEARCH
#include "TreeItemDelegate.h"
=======
#include "ColumnItemDelegate.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    auto* delegate = new TreeItemDelegate(this, false, false);
    m_listView->setItemDelegate(delegate);
=======
    auto* delegate = new ColumnItemDelegate(this);
    m_listView->setItemDelegate(delegate);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild the application via CMake:
   ```bash
   cmake --build build --config Release
   ```
2. Verify Column View rendering:
   - Ensure file and folder icons align precisely at 18x18px on the left (8px margin).
   - Ensure text is elided cleanly with `Qt::ElideRight`.
   - Ensure SVG files and thumbnails display without overlap or misaligned offset.
