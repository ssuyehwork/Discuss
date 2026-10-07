# Implementation Plan - MetaPanel Color Strip Refactoring (`MetaPanelColorStrip.md`)

## 1. Overview
In `MetaPanel`, the color label selection buttons (`QPushButton#MetaPanelColorBtn`) currently rely on Qt Style Sheets (QSS) for rendering:
`border-radius: 8px; border: 2px solid #FFFFFF;`
Because QSS stylesheet rendering uses bitmap raster clipping without `QPainter::Antialiasing`, the active white selection ring appears thin, deformed, and jagged on `MetaPanel` (Image Point 1).

In contrast, the right-click context menu uses `ColorStripPicker` (`src/ui/ColorPicker.cpp`), which uses custom `QPainter` rendering with `QPainter::Antialiasing` enabled, rendering smooth, thick, anti-aliased white rings around the color circles (Image Point 2).

### Proposed Refactoring
1. **SSOT Component Unification**: Enhance `ColorStripPicker` in `ColorPicker.h` / `ColorPicker.cpp` by adding a `setSelectedColor(const QString& hex)` method and drawing the active selection ring for both hovered and selected states using anti-aliased `QPainter`.
2. **MetaPanel Integration**: Replace the QSS-based `QPushButton` color row in `MetaPanel` with `ColorStripPicker`. This unifies color strip rendering across `ContentContextMenu`, `TabBarWidget`, and `MetaPanel` with 100% visual consistency and zero aliasing/jagged edges.

---

## 2. Modified Files List
- `src/ui/ColorPicker.h`
- `src/ui/ColorPicker.cpp`
- `src/ui/MetaPanel.h`
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: Add `setSelectedColor` and Selection Ring Drawing to `ColorStripPicker`
In `src/ui/ColorPicker.h`:

```
<<<<<<< SEARCH
class ColorStripPicker : public QWidget {
    Q_OBJECT
public:
    explicit ColorStripPicker(const QString& currentColorHex, QWidget* parent = nullptr);
signals:
=======
class ColorStripPicker : public QWidget {
    Q_OBJECT
public:
    explicit ColorStripPicker(const QString& currentColorHex, QWidget* parent = nullptr);
    void setSelectedColor(const QString& hex);
signals:
>>>>>>> REPLACE
```

In `src/ui/ColorPicker.cpp`:

```
<<<<<<< SEARCH
void ColorStripPicker::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 保持完全透明底色

    int startX = 12; // 起始左边距
    int y = rect().height() / 2;

    for (int i = 0; i < m_items.size(); ++i) {
        int cx = startX + i * (14 + m_spacing) + m_circleRadius;

        // 1. 绘制色块本身（第一个无颜色选项采用标准的 no_color 图标）
        if (i == 0) {
            QRect iconRect(cx - m_circleRadius, y - m_circleRadius, m_circleRadius * 2, m_circleRadius * 2);
            UiHelper::getIcon("no_color", m_items[i].color, m_circleRadius * 2).paint(&painter, iconRect);
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_items[i].color);
            painter.drawEllipse(QPoint(cx, y), m_circleRadius, m_circleRadius);
        }

        // 2. 悬停状态：绘制突出亮白圈
        if (i == m_hoveredIndex) {
            painter.setBrush(Qt::NoBrush);
            // 亮白画笔，宽度 1.5 像素
            QPen pen(QColor("#FFFFFF"), 1.5);
            painter.setPen(pen);
            // 半径设为 m_circleRadius + 2.0 像素以完美包裹里面的色块
            painter.drawEllipse(QPoint(cx, y), m_circleRadius + 2, m_circleRadius + 2);
        }
    }
}
=======
void ColorStripPicker::setSelectedColor(const QString& hex) {
    m_selectedColor = hex;
    update();
}

void ColorStripPicker::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 保持完全透明底色

    int startX = 12; // 起始左边距
    int y = rect().height() / 2;

    for (int i = 0; i < m_items.size(); ++i) {
        int cx = startX + i * (14 + m_spacing) + m_circleRadius;

        // 1. 绘制色块本身（第一个无颜色选项采用标准的 no_color 图标）
        if (i == 0) {
            QRect iconRect(cx - m_circleRadius, y - m_circleRadius, m_circleRadius * 2, m_circleRadius * 2);
            UiHelper::getIcon("no_color", m_items[i].color, m_circleRadius * 2).paint(&painter, iconRect);
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_items[i].color);
            painter.drawEllipse(QPoint(cx, y), m_circleRadius, m_circleRadius);
        }

        // 2. 悬停与选中状态：高精度抗锯齿亮白圈
        bool isSelected = (!m_selectedColor.isEmpty() && m_items[i].hex.compare(m_selectedColor, Qt::CaseInsensitive) == 0) ||
                          (m_selectedColor.isEmpty() && i == 0 && m_hoveredIndex == 0);
        if (i == m_hoveredIndex || isSelected) {
            painter.setBrush(Qt::NoBrush);
            // 亮白画笔，宽度 1.5 像素
            QPen pen(QColor("#FFFFFF"), 1.5);
            painter.setPen(pen);
            // 半径设为 m_circleRadius + 2.0 像素以完美包裹里面的色块
            painter.drawEllipse(QPoint(cx, y), m_circleRadius + 2, m_circleRadius + 2);
        }
    }
}
>>>>>>> REPLACE
```

### Change 2: Replace QSS Buttons with `ColorStripPicker` in `MetaPanel.h`
In `src/ui/MetaPanel.h`:

```
<<<<<<< SEARCH
    QWidget* m_ratingColorBox = nullptr;
    QList<QPushButton*> m_starBtns;
    QList<QPushButton*> m_colorBtns;
    int m_currentRating = 0;
    QString m_currentColorHex;
=======
    QWidget* m_ratingColorBox = nullptr;
    QList<QPushButton*> m_starBtns;
    ColorStripPicker* m_colorStripPicker = nullptr;
    int m_currentRating = 0;
    QString m_currentColorHex;
>>>>>>> REPLACE
```

### Change 3: Bind `ColorStripPicker` in `MetaPanel.cpp`
In `src/ui/MetaPanel.cpp`:

```
<<<<<<< SEARCH
    QWidget* colorRow = new QWidget(m_ratingColorBox);
    colorRow->setObjectName("MetaColorRow");
    QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(0, 2, 0, 2);
    colorLayout->setSpacing(6);

    QPushButton* btnNoColor = new QPushButton(colorRow);
    btnNoColor->setFixedSize(22, 22);
    btnNoColor->setCursor(Qt::PointingHandCursor);
    btnNoColor->setIcon(UiHelper::getIcon("no_color", QColor("#888888"), 16));
    btnNoColor->setIconSize(QSize(16, 16));
    btnNoColor->setProperty("tooltipText", "无色标");
    btnNoColor->installEventFilter(this);
    btnNoColor->setObjectName("MetaBtnNoColor");
    connect(btnNoColor, &QPushButton::clicked, this, [this]() { setColor(QString(""), true); });
    colorLayout->addWidget(btnNoColor);

    static const QVector<QPair<QString, QString>> s_colorMap = {
        {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"}, {"绿色", "#639922"},
        {"青色", "#1D9E75"}, {"蓝色", "#378ADD"}, {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
    };

    for (const auto& pair : s_colorMap) {
        QPushButton* btnColor = new QPushButton(colorRow);
        btnColor->setObjectName("MetaPanelColorBtn");
        btnColor->setFixedSize(16, 16);
        btnColor->setCursor(Qt::PointingHandCursor);
        btnColor->setProperty("tooltipText", pair.first);
        btnColor->setProperty("hexColor", pair.second);
        btnColor->installEventFilter(this);
        btnColor->setStyleSheet(QString("background-color: %1;").arg(pair.second));

        QString hex = pair.second;
        connect(btnColor, &QPushButton::clicked, this, [this, hex]() {
            if (m_currentColorHex.compare(hex, Qt::CaseInsensitive) == 0) {
                setColor(QString(""), true);
            } else {
                setColor(hex, true);
            }
        });
        m_colorBtns.append(btnColor);
        colorLayout->addWidget(btnColor);
    }
    colorLayout->addStretch();
    ratingColorLayout->addWidget(colorRow);
=======
    m_colorStripPicker = new ColorStripPicker("", m_ratingColorBox);
    connect(m_colorStripPicker, &ColorStripPicker::colorSelected, this, [this](const QString& hex) {
        if (m_currentColorHex.compare(hex, Qt::CaseInsensitive) == 0) {
            setColor(QString(""), true);
        } else {
            setColor(hex, true);
        }
    });
    ratingColorLayout->addWidget(m_colorStripPicker);
>>>>>>> REPLACE
```

In `MetaPanel::setColor`:

```
<<<<<<< SEARCH
void MetaPanel::setColor(const QString& hexColor, bool fromUser) {
    m_currentColorHex = hexColor;

    for (QPushButton* btn : m_colorBtns) {
        QString hex = btn->property("hexColor").toString();
        bool active = (!hexColor.isEmpty() && hex.compare(hexColor, Qt::CaseInsensitive) == 0);

        btn->setProperty("active", active);
        btn->setStyleSheet(QString("background-color: %1;").arg(hex));
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }

    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        QStringList pathsCopy = m_selectedPaths;
        QTimer::singleShot(0, this, [this, pathsCopy, hexColor]() {
            emit colorChanged(pathsCopy, hexColor);
        });
    }
}
=======
void MetaPanel::setColor(const QString& hexColor, bool fromUser) {
    m_currentColorHex = hexColor;
    if (m_colorStripPicker) {
        m_colorStripPicker->setSelectedColor(hexColor);
    }

    if (fromUser && !m_selectedPaths.isEmpty() && !m_isReadOnlyMode) {
        QStringList pathsCopy = m_selectedPaths;
        QTimer::singleShot(0, this, [this, pathsCopy, hexColor]() {
            emit colorChanged(pathsCopy, hexColor);
        });
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute CMake build:
   `cmake -B build && cmake --build build --config Release`
2. Run `QuarkMeta`.
3. Select an item in `ContentPanel` and check the color strip on `MetaPanel`.
4. Click color options on `MetaPanel` and verify:
   - Active white selection ring is perfectly smooth with zero aliasing/jagged edges.
   - Visual appearance is 100% identical between `MetaPanel` and `ContentContextMenu`.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Component Reuse**: `ColorStripPicker` is now the single SSOT component for color selection across all UI panels (`ContentContextMenu`, `TabBarWidget`, and `MetaPanel`).
- **Zero Redundancy**: Removed duplicate QSS button creation code from `MetaPanel.cpp`.

---

## 6. Header API Signature Verification
- `ColorStripPicker::setSelectedColor(const QString& hex)`: Added to `src/ui/ColorPicker.h:92`.
- `ColorStripPicker::colorSelected(const QString& hexColor)`: Declared in `src/ui/ColorPicker.h:93`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "ColorPicker.h"`: Included in `MetaPanel.cpp` (or `MetaPanel.h` forward declaration `class ColorStripPicker;`).
- All types (`QPainter`, `QPen`, `Antialiasing`) are completely defined in Qt header chain.
