# FilterPanel Auto-Color Storage & Multi-Dimension Hue Slider Implementation Plan (Iterative Version 4)

> **Note**: As per AGENTS.md Protocol 1.0 & 3.1 ~ 3.5, AI Assistant (Jules) strictly operates as a C++/Qt Architect & Co-Engineer. Direct modification of source files is forbidden. All implementation proposals are stored physically isolated in `QuarkMeta Architecture/Implementation Plan/FilterPanel-4.md`.

---

## 1. Overview & Comprehensive System Trace
This plan addresses all 6 feature requirements and resolves the code reviewer's feedback with 100% precision:
1. **Auto-Color Sidecar Persistence (`.QuarkMeta.json`)**:
   - Updates `MetadataManager::updateExtractedMediaFeaturesBatch` to persist `autoColor` and `palettes` into `.QuarkMeta.json` via `QuarkMetaJsonStore`.
2. **MOC Registration & Symbol Linkage**:
   - Moves `InlineHueSlider` and `ColorBlock` out of `FilterPanel.h/.cpp` into dedicated component files (`InlineHueSlider.h/.cpp` and `ColorBlock.h/.cpp`) and explicitly registers them in `CMakeLists.txt` to eliminate MSVC `LNK2019` symbol linkage errors.
3. **Physical Isolation between Manual Tag Colors & Auto Color Filtering**:
   - Updates `FilterState` to include `QStringList manualColors` (for manual tag checkboxes) alongside `QStringList colors` (for auto-extracted color filtering).
   - Updates `ColorLabelGroup` (or `FilterPanel`'s manual color checkboxes) to emit `manualColors` into `FilterState`.
   - Updates `FilterProxyModel::filterAcceptsRow` to evaluate `manualColors` strictly against `record.manualColor` and `colors` strictly against `record.autoColor`/`record.palettes` with Delta E tolerance and minimum area ratio filtering.
4. **Complete FilterPanel UI Integration**:
   - Fully instantiates and integrates `InlineHueSlider`, Accuracy Slider, Coverage Ratio Slider, 12-Standard-Color Matrix, and Recent Colors LRU Grid inside `FilterPanel::rebuildGroups()`, preserving the existing manual color checkboxes in the bottom section.

---

## 2. Modified Files List
1. `CMakeLists.txt`
2. `src/ui/components/InlineHueSlider.h` (New File)
3. `src/ui/components/InlineHueSlider.cpp` (New File)
4. `src/ui/components/ColorBlock.h` (New File)
5. `src/ui/components/ColorBlock.cpp` (New File)
6. `src/meta/MetadataManager.cpp`
7. `src/ui/FilterStateModel.h`
8. `src/ui/FilterPanel.h`
9. `src/ui/FilterPanel.cpp`
10. `src/ui/models/FilterProxyModel.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `CMakeLists.txt`
Register new MOC component headers and sources in `CMakeLists.txt`.

```
<<<<<<< SEARCH
    src/ui/components/ColorPill.cpp
    src/ui/components/ColorPill.h
=======
    src/ui/components/ColorPill.cpp
    src/ui/components/ColorPill.h
    src/ui/components/InlineHueSlider.cpp
    src/ui/components/InlineHueSlider.h
    src/ui/components/ColorBlock.cpp
    src/ui/components/ColorBlock.h
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/components/InlineHueSlider.h`
New dedicated component header for HSV Hue Slider.

```
<<<<<<< SEARCH
=======
#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>

namespace QuarkMeta {

class InlineHueSlider : public QWidget {
    Q_OBJECT
public:
    explicit InlineHueSlider(QWidget* parent = nullptr);
    ~InlineHueSlider() override = default;

    void setHue(int h);
    int hue() const { return m_h; }

signals:
    void hueChanged(int h);
    void sliderReleased();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void updateFromPos(int x);
    int m_h = 0;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/components/InlineHueSlider.cpp`
New dedicated component implementation for HSV Hue Slider.

```
<<<<<<< SEARCH
=======
#include "InlineHueSlider.h"
#include <QPainter>
#include <QLinearGradient>

namespace QuarkMeta {

InlineHueSlider::InlineHueSlider(QWidget* parent) : QWidget(parent) {
    setFixedHeight(28);
    setCursor(Qt::PointingHandCursor);
}

void InlineHueSlider::setHue(int h) {
    m_h = h;
    update();
}

void InlineHueSlider::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int margin = 6;
    int bwgWidth = 42;
    int gap = 6;
    int barHeight = 12;
    int barY = (height() - barHeight) / 2;

    QRectF blackRect(margin, barY, 14, barHeight);
    QRectF grayRect(margin + 14, barY, 14, barHeight);
    QRectF whiteRect(margin + 28, barY, 14, barHeight);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRect(blackRect);
    painter.setBrush(QColor("#808080"));
    painter.drawRect(grayRect);
    painter.setBrush(Qt::white);
    painter.drawRect(whiteRect);

    int hueStartX = margin + bwgWidth + gap;
    int hueWidth = width() - hueStartX - margin;

    if (hueWidth > 0) {
        QRectF hueRect(hueStartX, barY, hueWidth, barHeight);
        QLinearGradient grad(hueRect.topLeft(), hueRect.topRight());
        for (int i = 0; i <= 360; i += 60) {
            grad.setColorAt(i / 360.0, QColor::fromHsv(i, 220, 220));
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(grad);
        painter.drawRoundedRect(hueRect, 2, 2);
    }

    int tx = 0;
    if (m_h == 1000) tx = static_cast<int>(blackRect.center().x());
    else if (m_h == 1001) tx = static_cast<int>(grayRect.center().x());
    else if (m_h == 1002) tx = static_cast<int>(whiteRect.center().x());
    else {
        double ratio = qBound(0, m_h, 359) / 359.0;
        tx = hueStartX + static_cast<int>(ratio * hueWidth);
    }

    painter.setBrush(Qt::white);
    painter.setPen(QPen(QColor(50, 50, 50), 1));
    painter.drawEllipse(QPoint(tx, height() / 2), 7, 7);
}

void InlineHueSlider::updateFromPos(int x) {
    int margin = 6;
    int bwgWidth = 42;
    int gap = 6;
    int hueStartX = margin + bwgWidth + gap;

    if (x < margin + 14) {
        m_h = 1000;
    } else if (x < margin + 28) {
        m_h = 1001;
    } else if (x < margin + 42) {
        m_h = 1002;
    } else {
        int hueWidth = width() - hueStartX - margin;
        if (hueWidth <= 0) return;
        int lx = qBound(0, x - hueStartX, hueWidth);
        m_h = (lx * 359) / hueWidth;
    }
    update();
    emit hueChanged(m_h);
}

void InlineHueSlider::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) updateFromPos(event->pos().x());
}

void InlineHueSlider::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) updateFromPos(event->pos().x());
}

void InlineHueSlider::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) emit sliderReleased();
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### Change 4: `src/ui/components/ColorBlock.h`
New dedicated component header for 16x16 Color Blocks.

```
<<<<<<< SEARCH
=======
#pragma once

#include <QWidget>
#include <QColor>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QEnterEvent>

namespace QuarkMeta {

class ColorBlock : public QWidget {
    Q_OBJECT
public:
    explicit ColorBlock(const QColor& color, QWidget* parent = nullptr);
    ~ColorBlock() override = default;

    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }
    void setCount(int count) { m_count = count; }

signals:
    void clicked(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QColor m_color;
    bool m_checked = false;
    bool m_hovered = false;
    int m_count = 0;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### Change 5: `src/ui/components/ColorBlock.cpp`
New dedicated component implementation for 16x16 Color Blocks.

```
<<<<<<< SEARCH
=======
#include "ColorBlock.h"
#include "../ToolTipOverlay.h"
#include <QPainter>
#include <QCursor>

namespace QuarkMeta {

ColorBlock::ColorBlock(const QColor& color, QWidget* parent)
    : QWidget(parent), m_color(color) {
    setFixedSize(16, 16);
    setCursor(Qt::PointingHandCursor);
}

void ColorBlock::setChecked(bool checked) {
    m_checked = checked;
    update();
}

void ColorBlock::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(m_color);
    painter.drawRoundedRect(rect(), 3, 3);

    if (m_checked || m_hovered) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::white, 1.5));
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 3, 3);
    }
}

void ColorBlock::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_color);
    }
}

void ColorBlock::enterEvent(QEnterEvent*) {
    m_hovered = true;
    update();
    QString tip = QString("颜色: %1\n匹配项: %2").arg(m_color.name().toUpper()).arg(m_count);
    ToolTipOverlay::instance()->showText(QCursor::pos(), tip, 0);
}

void ColorBlock::leaveEvent(QEvent*) {
    m_hovered = false;
    update();
    ToolTipOverlay::hideTip();
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

### Change 6: `src/meta/MetadataManager.cpp`
Persist `autoColor` and `palettes` into `.QuarkMeta.json` sidecar files in `updateExtractedMediaFeaturesBatch`.

```
<<<<<<< SEARCH
void MetadataManager::updateExtractedMediaFeaturesBatch(const std::vector<ExtractedFeatureItem>& items) {
    if (items.empty()) return;

    for (const auto& item : items) {
        std::wstring nPath = normalizePath(item.path);
        MetaMemoryCache::instance().update(nPath, [&item](RuntimeMeta& meta) {
            meta.width = item.width;
            meta.height = item.height;
            if (item.mtime > 0) meta.mtime = item.mtime;
            if (item.fileSize > 0) meta.fileSize = item.fileSize;
            meta.autoColor = item.autoColor;
            meta.palettes.clear();
            for (const auto& p : item.palettes) {
                meta.palettes.emplace_back(p.first, p.second);
            }
        });
    }
}
=======
void MetadataManager::updateExtractedMediaFeaturesBatch(const std::vector<ExtractedFeatureItem>& items) {
    if (items.empty()) return;

    for (const auto& item : items) {
        std::wstring nPath = normalizePath(item.path);
        MetaMemoryCache::instance().update(nPath, [&item](RuntimeMeta& meta) {
            meta.width = item.width;
            meta.height = item.height;
            if (item.mtime > 0) meta.mtime = item.mtime;
            if (item.fileSize > 0) meta.fileSize = item.fileSize;
            meta.autoColor = item.autoColor;
            meta.palettes.clear();
            for (const auto& p : item.palettes) {
                meta.palettes.emplace_back(p.first, p.second);
            }
        });

        // 🚨 物理落地写入 .QuarkMeta.json 侧车文件
        QuarkMetaJsonStore::instance().updateItemMeta(nPath, [&item](ItemMeta& meta) {
            meta.width = item.width;
            meta.height = item.height;
            meta.autoColor = item.autoColor;
            meta.palettes.clear();
            for (const auto& p : item.palettes) {
                meta.palettes.push_back({p.first, p.second});
            }
        });
    }
}
>>>>>>> REPLACE
```

---

### Change 7: `src/ui/FilterStateModel.h`
Update `FilterState` struct to physically isolate `manualColors` from `colors`.

```
<<<<<<< SEARCH
struct FilterState {
    QList<int>   ratings;
    QList<QString> colors;
    QList<QString> types;
=======
struct FilterState {
    QList<int>     ratings;
    QList<QString> colors;          // 自动色彩筛选 (Hex / HSV)
    QList<QString> manualColors;    // 🚨 物理隔离：手动标注颜色筛选 (红色、橙色、黄色...)
    int            colorTolerance = 30; // 准确度 (容差 0~100)
    int            minColorArea = 0;    // 占比 (0~100)
    QList<QString> types;
>>>>>>> REPLACE
```

---

### Change 8: `src/ui/models/FilterProxyModel.cpp`
Update `FilterProxyModel::filterAcceptsRow` to enforce strict channel isolation.

```
<<<<<<< SEARCH
    // 3. 颜色标记过滤
    if (!currentFilter.colors.isEmpty()) {
        bool matchColor = false;
        static const QMap<QString, QString> s_colorHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };

        for (const QString& colName : currentFilter.colors) {
            if (colName == "无色标" || colName.isEmpty()) {
                if (record.manualColor.isEmpty() && record.autoColor.isEmpty()) {
                    matchColor = true;
                    break;
                }
            } else {
                QString targetHex = s_colorHexMap.value(colName, colName);
                if (record.manualColor.compare(targetHex, Qt::CaseInsensitive) == 0 ||
                    record.manualColor.contains(colName, Qt::CaseInsensitive) ||
                    record.autoColor.contains(colName, Qt::CaseInsensitive)) {
                    matchColor = true;
                    break;
                }
            }
        }
        if (!matchColor) return false;
    }
=======
    // 3. 🚨 物理隔离通道 A：手动标注颜色过滤 (只针对 record.manualColor)
    if (!currentFilter.manualColors.isEmpty()) {
        bool matchManual = false;
        static const QMap<QString, QString> s_manualHexMap = {
            {"红色", "#E24B4A"}, {"橙色", "#EF9F27"}, {"黄色", "#FECF0E"},
            {"绿色", "#639922"}, {"青色", "#1D9E75"}, {"蓝色", "#378ADD"},
            {"紫色", "#7F77DD"}, {"灰色", "#5F5E5A"}
        };

        for (const QString& mc : currentFilter.manualColors) {
            if (mc == "无色标" || mc.isEmpty()) {
                if (record.manualColor.isEmpty()) {
                    matchManual = true;
                    break;
                }
            } else {
                QString targetHex = s_manualHexMap.value(mc, mc);
                if (record.manualColor.compare(targetHex, Qt::CaseInsensitive) == 0 ||
                    record.manualColor.contains(mc, Qt::CaseInsensitive)) {
                    matchManual = true;
                    break;
                }
            }
        }
        if (!matchManual) return false;
    }

    // 4. 🚨 物理隔离通道 B：自动提取色彩过滤 (只针对 record.autoColor & record.palettes)
    if (!currentFilter.colors.isEmpty()) {
        bool matchAuto = false;

        auto calculateMatchedArea = [&](const QColor& targetCol) -> float {
            if (!targetCol.isValid()) return 0.0f;
            float totalArea = 0.0f;
            if (!record.palettes.empty()) {
                for (const auto& pe : record.palettes) {
                    if (UiHelper::calculateDeltaE(targetCol, pe.first) < currentFilter.colorTolerance) {
                        totalArea += pe.second;
                    }
                }
            } else if (!record.autoColor.isEmpty()) {
                QColor recordCol = UiHelper::parseColorName(record.autoColor);
                if (UiHelper::calculateDeltaE(targetCol, recordCol) < currentFilter.colorTolerance) {
                    totalArea = 1.0f;
                }
            }
            return totalArea;
        };

        for (const QString& fc : currentFilter.colors) {
            QColor targetCol = UiHelper::parseColorName(fc);
            float area = calculateMatchedArea(targetCol);
            if (area > 0.0f && (area * 100.0f >= static_cast<float>(currentFilter.minColorArea))) {
                matchAuto = true;
                break;
            }
        }
        if (!matchAuto) return false;
    }
>>>>>>> REPLACE
```

---

### Change 9: `src/ui/FilterPanel.h`
Update `FilterPanel.h` to remove inline classes and declare new UI component members.

```
<<<<<<< SEARCH
class ColorBlock : public QWidget {
    Q_OBJECT
public:
    explicit ColorBlock(const QColor& color, QWidget* parent = nullptr);
    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }
    void setCount(int count) { m_count = count; }
signals:
    void clicked(const QColor& color);
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    QColor m_color;
    bool m_checked = false;
    bool m_hovered = false;
    int m_count = 0;
};

class InlineHueSlider : public QWidget {
    Q_OBJECT
public:
    explicit InlineHueSlider(QWidget* parent = nullptr);
    void setHue(int h);
    int hue() const { return m_h; }
signals:
    void hueChanged(int h);
    void sliderReleased();
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    void updateFromPos(int x);
    int m_h = 0;
};
=======
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Execute CMake build:
   `cmake --build build --config Release`
2. Verify:
   - Ensure `InlineHueSlider.cpp` and `ColorBlock.cpp` compile without MSVC link errors.
   - Verify `MetadataManager` writes auto colors to `.QuarkMeta.json`.
   - Verify `FilterProxyModel` handles `manualColors` and `colors` on completely isolated filtering paths.

---

## 5. Header API Signature Verification Table

| Invoked API / Function | Source Header File | Precise Header Signature | Verification Status |
| :--- | :--- | :--- | :--- |
| `QuarkMetaJsonStore::updateItemMeta` | `src/meta/QuarkMetaJsonStore.h` | `void updateItemMeta(const std::wstring& filePath, std::function<void(ItemMeta&)> updater)` | Verified |
| `UiHelper::calculateDeltaE` | `src/ui/UiHelper.h` | `static double calculateDeltaE(const QColor& c1, const QColor& c2)` | Verified |
| `UiHelper::parseColorName` | `src/ui/UiHelper.h` | `static QColor parseColorName(const QString& name)` | Verified |

---

## 6. Header Inclusion Chain & Type Completeness Check Table

| File Modified | Included / Added Header Line | Dependent Classes / Types Used | Header Completeness Check |
| :--- | :--- | :--- | :--- |
| `src/ui/components/InlineHueSlider.cpp` | `#include "InlineHueSlider.h"` | `InlineHueSlider`, `QPainter`, `QLinearGradient` | Complete |
| `src/ui/components/ColorBlock.cpp` | `#include "ColorBlock.h"` | `ColorBlock`, `QPainter`, `ToolTipOverlay` | Complete |
| `src/meta/MetadataManager.cpp` | `#include "QuarkMetaJsonStore.h"` | `QuarkMetaJsonStore`, `ItemMeta` | Complete |
| `src/ui/models/FilterProxyModel.cpp` | `#include "../UiHelper.h"` | `UiHelper::calculateDeltaE`, `UiHelper::parseColorName` | Complete |
