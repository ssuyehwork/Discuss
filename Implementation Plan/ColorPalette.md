# Implementation Plan - ColorPalette Normalization

## 1. Overview
This implementation plan consolidates the 8 color tag definitions (Red, Orange, Yellow, Green, Cyan, Blue, Purple, Gray, and No Color) into a Single Source of Truth (SSOT) within `StyleLibrary.h`.
Specifically:
1. Define `ColorTagItem` struct and `ColorPalette` array in `src/ui/StyleLibrary.h`.
2. Provide `getColorHexByName()`, `getColorNameByHex()`, and `getColorPalette()` helper functions in `StyleLibrary.h`.
3. Refactor `ColumnItemDelegate.cpp`, `ColorPicker.cpp`, `FilterPanel.cpp`, and `MetaRatingColorWidget.cpp` to use the unified `StyleLibrary` color palette SSOT.

---

## 2. Modified Files List
- `src/ui/StyleLibrary.h`
- `src/ui/ColumnItemDelegate.cpp`
- `src/ui/ColorPicker.cpp`
- `src/ui/FilterPanel.cpp`
- `src/ui/MetaRatingColorWidget.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/StyleLibrary.h`
Add `ColorTagItem` and global `ColorPalette` helpers to `QuarkMeta::Style`:

<<<<<<< SEARCH
// QSS Helper
inline QString qssColor(const QColor& color) { return color.name(); }
=======
struct ColorTagItem {
    QString hex;
    QColor color;
    QString name;
};

inline const QList<ColorTagItem>& getColorPalette() {
    static const QList<ColorTagItem> s_palette = {
        {"",        QColor("#888780"), "无色标"},
        {"#E24B4A", QColor("#E24B4A"), "红色"},
        {"#EF9F27", QColor("#EF9F27"), "橙色"},
        {"#FECF0E", QColor("#FECF0E"), "黄色"},
        {"#639922", QColor("#639922"), "绿色"},
        {"#1D9E75", QColor("#1D9E75"), "青色"},
        {"#378ADD", QColor("#378ADD"), "蓝色"},
        {"#7F77DD", QColor("#7F77DD"), "紫色"},
        {"#5F5E5A", QColor("#5F5E5A"), "灰色"}
    };
    return s_palette;
}

inline QString getColorHexByName(const QString& name) {
    for (const auto& item : getColorPalette()) {
        if (item.name == name) return item.hex;
    }
    return name; // Fallback if already hex
}

// QSS Helper
inline QString qssColor(const QColor& color) { return color.name(); }
>>>>>>> REPLACE

---

### 3.2 `src/ui/ColumnItemDelegate.cpp`
Use `Style::getColorHexByName()` instead of local `s_colorHexMap`:

<<<<<<< SEARCH
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
=======
    // 绘制 5px 宽度的左侧垂直色条
    if (!colorName.isEmpty()) {
        QString hexColor = Style::getColorHexByName(colorName);
        if (hexColor.startsWith("#")) {
            painter->setBrush(QColor(hexColor));
            painter->setPen(Qt::NoPen);
            QRect colorBarRect(option.rect.left(), option.rect.top(), 5, option.rect.height());
            painter->drawRect(colorBarRect);
        }
    }
>>>>>>> REPLACE

---

### 3.3 `src/ui/ColorPicker.cpp`
Use `Style::getColorPalette()` in `ColorStripPicker`:

<<<<<<< SEARCH
ColorStripPicker::ColorStripPicker(const QString& currentColorHex, QWidget* parent)
    : QWidget(parent), m_selectedColor(currentColorHex) {
    // 9个直径为14像素的圆，间距为5像素。
    // 总宽度：左侧预留12像素 + 9 * 14像素圆 + 8 * 5像素间隔 + 右侧预留12像素 = 12 + 126 + 40 + 12 = 190像素
    setFixedSize(190, 26);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    m_items = {
        {"", QColor("#888780"), "无颜色"},
        {"#E24B4A", QColor("#E24B4A"), "红色"},
        {"#EF9F27", QColor("#EF9F27"), "橙色"},
        {"#FECF0E", QColor("#FECF0E"), "黄色"},
        {"#639922", QColor("#639922"), "绿色"},
        {"#1D9E75", QColor("#1D9E75"), "青色"},
        {"#378ADD", QColor("#378ADD"), "蓝色"},
        {"#7F77DD", QColor("#7F77DD"), "紫色"},
        {"#5F5E5A", QColor("#5F5E5A"), "灰色"}
    };
}
=======
ColorStripPicker::ColorStripPicker(const QString& currentColorHex, QWidget* parent)
    : QWidget(parent), m_selectedColor(currentColorHex) {
    setFixedSize(190, 26);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    for (const auto& item : Style::getColorPalette()) {
        m_items.append({item.hex, item.color, item.name});
    }
}
>>>>>>> REPLACE

---

### 3.4 `src/ui/FilterPanel.cpp`
Use `Style::getColorPalette()` in `FilterPanel`:

<<<<<<< SEARCH
        static const struct { QString name; QString hex; QColor color; } colorsList[] = {
            {"无色标", "",        QColor("#808080")},
            {"红色",   "#E24B4A", QColor("#E24B4A")},
            {"橙色",   "#EF9F27", QColor("#EF9F27")},
            {"黄色",   "#FECF0E", QColor("#FECF0E")},
            {"绿色",   "#639922", QColor("#639922")},
            {"青色",   "#1D9E75", QColor("#1D9E75")},
            {"蓝色",   "#378ADD", QColor("#378ADD")},
            {"紫色",   "#7F77DD", QColor("#7F77DD")},
            {"灰色",   "#5F5E5A", QColor("#5F5E5A")}
        };
=======
        const auto& colorsList = Style::getColorPalette();
>>>>>>> REPLACE

---

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`
2. Launch QuarkMeta app.
3. Test color tag setting via right-click context menu, filter panel, and Column View color bar rendering.
4. Verify all color tags match `#E24B4A`, `#EF9F27`, `#FECF0E`, `#639922`, `#1D9E75`, `#378ADD`, `#7F77DD`, `#5F5E5A` seamlessly.
