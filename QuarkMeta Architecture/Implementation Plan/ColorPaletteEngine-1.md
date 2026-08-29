# ColorPaletteEngine-1 Implementation Plan

## Overview
This implementation plan outlines the exact changes required to enhance the file extension badge coloring logic in `ColorPaletteEngine` and `CardPainterHelper`. It provides custom high-visibility colors for specific file formats (`psd`, `eps`, `ai`) as requested, and a deterministic hash-based color generation scheme for all other unlisted file extensions. It also ensures high visual contrast for foreground text.

## Modified Files List
1. `src/util/ColorPaletteEngine.h`
2. `src/util/ColorPaletteEngine.cpp`
3. `src/ui/CardPainterHelper.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/util/ColorPaletteEngine.h`
Add `getExtensionBadgeColors` to return both background color and high-contrast text foreground color.

```
<<<<<<< SEARCH
    static QColor getExtensionColor(const QString& ext);
=======
    static QColor getExtensionColor(const QString& ext);
    static QPair<QColor, QColor> getExtensionBadgeColors(const QString& ext);
>>>>>>> REPLACE
```

---

### 2. `src/util/ColorPaletteEngine.cpp`
Implement `getExtensionBadgeColors` with explicit mappings for `psd`, `eps`, `ai`, and deterministic `qHash` calculation for others.

```
<<<<<<< SEARCH
QColor ColorPaletteEngine::getExtensionColor(const QString& ext) {
    QString e = ext.toLower().trimmed();
    if (e == "psd" || e == "psb") return QColor("#31A8FF");
    if (e == "ai" || e == "eps")  return QColor("#FF9A00");
    if (e == "svg")               return QColor("#FFB13B");
    if (e == "png")               return QColor("#2ECC71");
    if (e == "jpg" || e == "jpeg") return QColor("#E67E22");
    if (e == "gif")               return QColor("#9B59B6");
    if (e == "webp")              return QColor("#1ABC9C");
    if (e == "pdf")               return QColor("#E74C3C");
    if (e == "txt" || e == "md")  return QColor("#95A5A6");
    return QColor("#7F8C8D");
}
=======
QColor ColorPaletteEngine::getExtensionColor(const QString& ext) {
    return getExtensionBadgeColors(ext).first;
}

QPair<QColor, QColor> ColorPaletteEngine::getExtensionBadgeColors(const QString& ext) {
    QString e = ext.toLower().trimmed();

    // 1. 特性定制扩展名配色
    if (e == "psd" || e == "psb") {
        return { QColor("#001D26"), QColor("#02B1DD") };
    }
    if (e == "eps") {
        return { QColor("#35483D"), QColor("#F88025") };
    }
    if (e == "ai") {
        return { QColor("#F88025"), QColor("#35483D") };
    }
    if (e == "svg") {
        return { QColor("#FFB13B"), QColor("#1A1A1A") };
    }
    if (e == "png") {
        return { QColor("#2ECC71"), QColor("#FFFFFF") };
    }
    if (e == "jpg" || e == "jpeg") {
        return { QColor("#E67E22"), QColor("#FFFFFF") };
    }
    if (e == "pdf") {
        return { QColor("#E74C3C"), QColor("#FFFFFF") };
    }

    // 2. 其他未指定扩展名：基于 qHash 确定性 HSL 生成唯一背景色
    uint hashVal = qHash(e);
    int hue = static_cast<int>(hashVal % 360);
    int saturation = 130 + static_cast<int>((hashVal >> 8) % 80); // 130~210
    int lightness = 80 + static_cast<int>((hashVal >> 16) % 60);  // 80~140

    QColor bgColor = QColor::fromHsl(hue, saturation, lightness);

    // 3. 计算亮度自动平衡文字颜色 (YIQ Luminance)
    double luminance = (0.299 * bgColor.red() + 0.587 * bgColor.green() + 0.114 * bgColor.blue()) / 255.0;
    QColor textColor = (luminance < 0.55) ? QColor("#FFFFFF") : QColor("#1A1A1A");

    return { bgColor, textColor };
}
>>>>>>> REPLACE
```

---

### 3. `src/ui/CardPainterHelper.cpp`
Update `drawExtensionBadge` to retrieve and use both background and text colors.

```
<<<<<<< SEARCH
void CardPainterHelper::drawExtensionBadge(QPainter* painter, const QRect& cardRect,
                                           const QString& ext, bool hasThumb) {
    QColor badgeColor = UiHelper::getExtensionColor(ext);

    if (!hasThumb) {
        badgeColor.setAlpha(160);
    }

    QRect extRect(cardRect.left() + 8, cardRect.top() + 8, 36, 18);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(badgeColor);
    painter->drawRoundedRect(extRect, 2, 2);
    painter->setPen(hasThumb ? QColor("#FFFFFF") : QColor(255, 255, 255, 180));
    QFont extFont = painter->font(); extFont.setPointSize(8); extFont.setBold(true);
    painter->setFont(extFont);
    painter->drawText(extRect, Qt::AlignCenter, ext);
    painter->restore();
}
=======
void CardPainterHelper::drawExtensionBadge(QPainter* painter, const QRect& cardRect,
                                           const QString& ext, bool hasThumb) {
    auto colors = ColorPaletteEngine::getExtensionBadgeColors(ext);
    QColor badgeColor = colors.first;
    QColor textColor = colors.second;

    if (!hasThumb) {
        badgeColor.setAlpha(160);
        textColor.setAlpha(200);
    }

    QRect extRect(cardRect.left() + 8, cardRect.top() + 8, 36, 18);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(badgeColor);
    painter->drawRoundedRect(extRect, 2, 2);
    painter->setPen(textColor);
    QFont extFont = painter->font(); extFont.setPointSize(8); extFont.setBold(true);
    painter->setFont(extFont);
    painter->drawText(extRect, Qt::AlignCenter, ext);
    painter->restore();
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps
1. Verify implementation plan file existence in `QuarkMeta Architecture/Implementation Plan/ColorPaletteEngine-1.md`.
2. Clean and build the project using CMake if building binary targets:
   ```bash
   cmake -B build -S .
   cmake --build build
   ```
3. Run unit tests to confirm MOC and ColorPaletteEngine static API linkage.
