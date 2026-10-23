# Implementation Plan - ColumnItemDelegate Clean Single-Row Rendering (ColumnItemDelegate-CleanSingleRow.md)

## 1. Overview
Due to narrow horizontal column width in Column View (`ColumnViewWidget`), drawing star ratings and color markers directly inside the item delegate rows causes text truncation and severe visual clutter.

### Objective
Remove in-row star ratings and color pills rendering from `ColumnItemDelegate::paint`. Keep `ColumnItemDelegate` clean and minimal:
- Left: 18x18px file/folder icon
- Center: Self-adapting filename with `ElideRight` (`...`)
- Right: Folder cascade chevron arrow (`chevron_right`, 20px) if item is a folder.

Bound extended metadata (tags, ratings, colors, notes) will continue to be fully queried and displayed in the right-hand `MetaPanel` upon selection.

---

## 2. Modified Files List
- `src/ui/ColumnItemDelegate.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change: `src/ui/ColumnItemDelegate.cpp`
Remove in-row rating star and color pill calculation and drawing logic from `ColumnItemDelegate::paint`.

```git
<<<<<<< SEARCH
    // 3. 右侧箭头 (如果是文件夹)
    int rightReserved = 0;
    if (isFolder) {
        rightReserved = 20;
        QRect arrowRect(rect.right() - 16, rect.top() + (rect.height() - 16) / 2, 16, 16);
        QIcon arrowIcon = UiHelper::getIcon("chevron_right", QColor("#888888"), 16);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter);
    }

    // 星级/颜色标记
    int rating = index.data(RatingRole).toInt();
    QString colorName = index.data(ColorRole).toString();
    if (rating > 0 || !colorName.isEmpty()) {
        rightReserved += (rating > 0 ? 50 : 16);
    }

    // 4. 文件/文件夹名称文本
    int textLeft = iconRect.right() + 8;
    int textWidth = rect.width() - (textLeft - rect.left()) - rightReserved;
    QRect textRect(textLeft, rect.top(), qMax(10, textWidth), rect.height());

    QString name = index.data(Qt::DisplayRole).toString();
    QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
    painter->setPen(textColor);
    painter->setFont(option.font);

    QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

    // 5. 绘制星级/颜色标识
    if (rating > 0 || !colorName.isEmpty()) {
        QRect ratingRect(textRect.right() + 4, rect.top(), rightReserved - (isFolder ? 20 : 0), rect.height());
        CardPainterHelper::drawRatingStars(painter, ratingRect, ratingRect, 10, 2,
                                          ratingRect.top(), ratingRect.height(), ratingRect.left(),
                                          rating, colorName, selected);
    }
=======
    // 3. 右侧箭头 (如果是文件夹，保留 20px 专属区域)
    int rightReserved = isFolder ? 20 : 0;
    if (isFolder) {
        QRect arrowRect(rect.right() - 16, rect.top() + (rect.height() - 16) / 2, 16, 16);
        QIcon arrowIcon = UiHelper::getIcon("chevron_right", QColor("#888888"), 16);
        arrowIcon.paint(painter, arrowRect, Qt::AlignCenter);
    }

    // 4. 文件/文件夹名称文本 (纯净单行渲染，不做行内星级/颜色标识绘制)
    int textLeft = iconRect.right() + 8;
    int textWidth = rect.width() - (textLeft - rect.left()) - rightReserved;
    QRect textRect(textLeft, rect.top(), qMax(10, textWidth), rect.height());

    QString name = index.data(Qt::DisplayRole).toString();
    QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
    painter->setPen(textColor);
    painter->setFont(option.font);

    QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### Build Command
```bash
cmake -B build -G Ninja
cmake --build build
```

### Verification Method
1. Launch QuarkMeta and switch to Column View (`ColumnView`).
2. Select files/folders that have assigned ratings or colors.
3. Observe Column View item rows:
   - Confirm in-row star icons and color pills are no longer drawn inside column item rows.
   - Confirm filename text occupies full available width with clean `...` elision.
   - Confirm selecting items still updates the right-hand `MetaPanel` with complete rating and color metadata.
