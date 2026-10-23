# FilterPanel Auto-Color Storage & Multi-Dimension Hue Slider Implementation Plan

> **Note**: As per AGENTS.md Protocol 3.1 (Immutable Plan Files) & SYSTEM_PROMPT.md Five Hard Locks, this plan details the physics-isolated architecture for:
> 1. Storing automatically extracted colors (`autoColor` & `palettes`) into `.QuarkMeta.json` instead of SQLite DB.
> 2. Integrating the 6 filter UI components (`InlineHueSlider`, Accuracy Slider, Coverage Ratio Slider, 12-Standard-Color Matrix, Recent Colors LRU Grid).
> 3. Enforcing **strict physical isolation** between Manual Color (`manualColor`) and Auto Color (`autoColor` / `palettes`).

---

## 1. Overview & Architecture Design

### 1.1 Auto-Color Storage Target (.QuarkMeta.json)
Currently, `QuarkMetaJson.cpp` already reads and writes `auto_color` and `palettes` to `.QuarkMeta.json`.
In `MediaExtractorPipeline.cpp`, after background extraction completes, `MetadataManager::updateExtractedMediaFeaturesBatch` is called.
We need to update `MetadataManager::updateExtractedMediaFeaturesBatch` so that it calls `QuarkMetaJsonStore::instance().updateItemMeta(...)` to write `autoColor` and `palettes` into `.QuarkMeta.json` sidecar files in addition to updating `MetaMemoryCache`.

### 1.2 Strict Physical Isolation between Manual Color & Auto Color
- **Manual Color (`manualColor`)**: Set manually by users in `MetaPanel` (8 preset colors: 红、橙、黄、绿、青、蓝、紫、灰). Filtered strictly in `FilterProxyModel` via `record.manualColor` exact / name match.
- **Auto Color (`autoColor` & `palettes`)**: Extracted automatically from image pixels during thumbnail generation. Filtered strictly via CIELAB Delta E color tolerance (`colorTolerance`) and minimum area ratio (`minColorArea`) against `record.autoColor` and `record.palettes`.
- **Filtering Logic**: In `FilterProxyModel::filterAcceptsRow`, manual color filter criteria (`manualExactColors` / `manualColor`) and auto color filter criteria (`autoColors`, `colorTolerance`, `minColorArea`) operate on separate fields and evaluation paths without interfering with each other.

---

## 2. Modified Files List
1. `src/meta/MetadataManager.cpp`
2. `src/ui/models/FilterProxyModel.h`
3. `src/ui/models/FilterProxyModel.cpp`
4. `src/ui/FilterPanel.h`
5. `src/ui/FilterPanel.cpp`
6. `src/ui/ColorLabelGroup.h`
7. `src/ui/ColorLabelGroup.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/meta/MetadataManager.cpp` - Persist `autoColor` and `palettes` to `.QuarkMeta.json`
In `MetadataManager::updateExtractedMediaFeaturesBatch`, add sidecar persistence via `QuarkMetaJsonStore`.

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

        // 🚨 落地写入 .QuarkMeta.json
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

### Change 2: `src/ui/FilterPanel.h` - Define `InlineHueSlider`, `ColorBlock`, and Auto-Color Filter State
Add `InlineHueSlider` widget class, `ColorBlock` widget class, and update `FilterState` structure in `FilterPanel.h`.

```
<<<<<<< SEARCH
struct FilterState {
    QList<int>   ratings;
    QList<QString> colors;
    QList<QString> types;
=======
struct FilterState {
    QList<int>     ratings;
    QList<QString> colors;          // 自动提取色彩过滤色 (Hex 字符串，如 #E24B4A)
    QList<QString> manualColors;    // 🚨 物理隔离：手动标注颜色过滤 (红、橙、黄、绿、青、蓝、紫、灰、无色标)
    int            colorTolerance = 30; // 准确度 (容差 0~100)
    int            minColorArea = 0;    // 占比 (0~100)
    QList<QString> types;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
namespace QuarkMeta {

class FilterStateModel;
=======
namespace QuarkMeta {

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

class FilterStateModel;
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/models/FilterProxyModel.cpp` - Strict Isolated Filtering Logic
In `FilterProxyModel::filterAcceptsRow`, enforce strict separation between `record.manualColor` and `record.autoColor`.

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
    // 3. 🚨 物理隔离：手动标注颜色过滤 (只针对 record.manualColor)
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

    // 4. 🚨 物理隔离：自动提取色彩过滤 (只针对 record.autoColor & record.palettes)
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

## 4. Build & Verification Steps
1. Clean and build the target:
   `cmake --build build --config Release`
2. Verify:
   - Run thumbnail extraction on images. Inspect `.QuarkMeta.json` in the image directory; confirm `auto_color` and `palettes` are persisted in JSON format.
   - Open `FilterPanel`, verify `InlineHueSlider`, Accuracy slider, Area Ratio slider, Standard 12-Color matrix, and Recent Colors LRU grid rendered in sequence.
   - Test filtering by manual color (e.g. check "红色"). Verify only items with manual color tag match.
   - Test filtering by auto color (slide `InlineHueSlider` / click standard color block). Verify matching operates strictly on image palettes and `autoColor` with Delta E tolerance and ratio thresholds, completely isolated from manual color tags.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Sidecar SSOT**: Reuses `QuarkMetaJsonStore::instance().updateItemMeta` to write `autoColor` and `palettes` into `.QuarkMeta.json`.
- **Delta E SSOT**: Reuses `UiHelper::calculateDeltaE` for CIELAB Delta E color difference calculations.

---

## 6. Header API Signature Verification
- `QuarkMetaJsonStore::updateItemMeta(const std::wstring&, std::function<void(ItemMeta&)>)`: Checked in `QuarkMetaJsonStore.h`.
- `UiHelper::calculateDeltaE(const QColor&, const QColor&)`: Checked in `UiHelper.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/meta/MetadataManager.cpp` includes `QuarkMetaJsonStore.h`.
- `src/ui/models/FilterProxyModel.cpp` includes `UiHelper.h`.
