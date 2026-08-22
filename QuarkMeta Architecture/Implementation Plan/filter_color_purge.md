# Implementation Plan - Filter Panel Color Selector Purge & UI Simplification

## Overview
This implementation plan purges all over-engineered, complex composite color filtering controls from the `FilterPanel` (5th column on the right), restoring a clean, compact, and consistent checkbox list layout across all filter groups.

The components purged include:
- `InlineHueSlider` widget and hue gradient bar.
- `ColorBlock` widget and recent colors grid (`m_recentColors`).
- Color accuracy/tolerance slider (`m_accuracySlider`).
- Color area ratio slider (`m_areaSlider`).
- Fast color search text box (`m_editColor`).
- Standard 12-color grid matrix (`staticGrid`).
- Redundant fields in `FilterState` (`colorTolerance`, `minColorArea`, `colorFilterText`, `manualExactColors`).

---

## Modified Files List
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`
- `src/ui/ContentPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/FilterPanel.h`

```
<<<<<<< SEARCH
// ─── 物理色块控件 (ColorBlock) ─────────────────────────────────────
class ColorBlock : public QWidget {
    Q_OBJECT
public:
    explicit ColorBlock(const QColor& color, QWidget* parent = nullptr);
    void setCount(int count);
    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }
    QColor color() const { return m_color; }

signals:
    void clicked(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent*) override;

private:
    QColor m_color;
    int    m_count = 0;
    bool   m_checked = false;
    bool   m_hovered = false;
};

// ─── 色相滑块 (内嵌版) ─────────────────────────────────────────────
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

struct FilterState {
    QList<int>   ratings;
    QStringList  colors;
    QStringList  manualExactColors; // 🚨 标准色系：手动色标 1:1 精准过滤字段
    QString      keyword; // 2026-07-xx 按照 Plan-92：合并搜索关键词入 FilterState
    QStringList  types;
    QStringList  createDates;   // "YYYY-MM-DD"
    QStringList  modifyDates;
    int          colorTolerance = 30; // 2026-05-17 按照用户要求：自定义颜色相近色容差（0~100），由 ColorPicker 准确度滑条驱动
    int          minColorArea = 0;   // 2026-06-23 按照用户要求：颜色面积最小占比阈值 (0-100)

    // 2026-07-xx 按照 Plan-30：链接、备注及大小筛选
    enum Presence { All, Yes, No };
    Presence linkPresence = All;
    Presence notePresence = All;

    enum AspectRatio { AspectAny, Horizontal, Vertical, Square, Ratio169 };
    AspectRatio ratio = AspectAny;

    long long minSize = -1; // 字节单位，-1 表示不限制
    long long maxSize = -1;

    // 2026-xx-xx 按照用户要求：新增 5 个主选项的快速文本过滤字段
    QString colorFilterText;
    QString typeFilterText;
    QString createDateFilterText;
    QString modifyDateFilterText;

    bool showFolders = true; // 2026-07-xx 按照 Plan-73：显示/隐藏文件夹
    bool showFiles = true;   // 2026-07-xx 按照 Plan-73：显示/隐藏文件

    bool noThumbnailOnly = false; // 仅无缩略图（提取失败/跳过）

    enum DuplicatePresence { DupAll, DuplicateOnly, UniqueOnly };
    DuplicatePresence duplicatePresence = DupAll;

    bool isEmpty() const {
        return ratings.isEmpty() && colors.isEmpty() && manualExactColors.isEmpty() && keyword.isEmpty() && types.isEmpty() &&
               createDates.isEmpty() && modifyDates.isEmpty() &&
               linkPresence == All && notePresence == All && ratio == AspectAny &&
               minSize == -1 && maxSize == -1 && minColorArea == 0 &&
               colorFilterText.trimmed().isEmpty() &&
               typeFilterText.trimmed().isEmpty() && createDateFilterText.trimmed().isEmpty() &&
               modifyDateFilterText.trimmed().isEmpty() && duplicatePresence == DupAll &&
               !noThumbnailOnly;
    }
};
=======
struct FilterState {
    QList<int>   ratings;
    QStringList  colors;
    QString      keyword;
    QStringList  types;
    QStringList  createDates;   // "YYYY-MM-DD"
    QStringList  modifyDates;

    enum Presence { All, Yes, No };
    Presence linkPresence = All;
    Presence notePresence = All;

    enum AspectRatio { AspectAny, Horizontal, Vertical, Square, Ratio169 };
    AspectRatio ratio = AspectAny;

    long long minSize = -1; // 字节单位，-1 表示不限制
    long long maxSize = -1;

    QString typeFilterText;
    QString createDateFilterText;
    QString modifyDateFilterText;

    bool showFolders = true;
    bool showFiles = true;

    bool noThumbnailOnly = false; // 仅无缩略图（提取失败/跳过）

    enum DuplicatePresence { DupAll, DuplicateOnly, UniqueOnly };
    DuplicatePresence duplicatePresence = DupAll;

    bool isEmpty() const {
        return ratings.isEmpty() && colors.isEmpty() && keyword.isEmpty() && types.isEmpty() &&
               createDates.isEmpty() && modifyDates.isEmpty() &&
               linkPresence == All && notePresence == All && ratio == AspectAny &&
               minSize == -1 && maxSize == -1 &&
               typeFilterText.trimmed().isEmpty() && createDateFilterText.trimmed().isEmpty() &&
               modifyDateFilterText.trimmed().isEmpty() && duplicatePresence == DupAll &&
               !noThumbnailOnly;
    }
};
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 2026-xx-xx 新增快速输入框成员
    QLineEdit*    m_editColor       = nullptr;
    QLineEdit*    m_editType        = nullptr;
    QLineEdit*    m_editCreateDate  = nullptr;
    QLineEdit*    m_editModifyDate  = nullptr;
    QVBoxLayout*  m_createDateLayout = nullptr; // 2026-07-xx Plan-92: 日期布局指针
    QVBoxLayout*  m_modifyDateLayout = nullptr;
    QSlider*      m_accuracySlider  = nullptr; // 2026-07-xx 按照用户要求：还原颜色准确度控制条
    QSlider*      m_areaSlider      = nullptr; // 2026-06-23 按照用户要求：新增颜色面积占比滑条
=======
    // 基础输入框与布局指针
    QLineEdit*    m_editType        = nullptr;
    QLineEdit*    m_editCreateDate  = nullptr;
    QLineEdit*    m_editModifyDate  = nullptr;
    QVBoxLayout*  m_createDateLayout = nullptr;
    QVBoxLayout*  m_modifyDateLayout = nullptr;
>>>>>>> REPLACE
```

---

### 2. `src/ui/FilterPanel.cpp`

```
<<<<<<< SEARCH
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("颜色标记", gl, &hdrLayout);
        m_groupColor = g;

        // 带有左右 5px 缩进外壳的快速输入框
        QWidget* wColor = new QWidget(g);
        QHBoxLayout* lColor = new QHBoxLayout(wColor);
        lColor->setContentsMargins(5, 6, 5, 4);
        lColor->setSpacing(0);

        m_editColor = new QLineEdit(wColor);
        m_editColor->setClearButtonEnabled(true);
        m_editColor->setPlaceholderText("例： 红 / #E24B4A / 无色标");
        m_editColor->setText(m_filter.colorFilterText);
        m_editColor->setObjectName("FilterSearchEdit");
        m_editColor->setFixedHeight(22);
        m_editColor->setStyleSheet(
            "QLineEdit#FilterSearchEdit {"
            "  background: #2D2D2D;"
            "  color: #CCCCCC;"
            "  border: 1px solid #444444;"
            "  border-radius: 4px;"
            "  padding: 0px 6px;"
            "  font-size: 11px;"
            "}"
            "QLineEdit#FilterSearchEdit:focus { border-color: #378ADD; color: #FFFFFF; }"
        );
        m_editColor->installEventFilter(this);
        connect(m_editColor, &QLineEdit::returnPressed, this, [this]() {
            m_filter.colorFilterText = m_editColor->text();
            saveFilterHistory("Color", m_filter.colorFilterText);
            emit filterChanged(m_filter);
        });
        connect(m_editColor, &QLineEdit::textChanged, this, [this](const QString& text) {
            if (text.isEmpty() && !m_filter.colorFilterText.isEmpty()) {
                m_filter.colorFilterText = "";
                emit filterChanged(m_filter);
            }
        });
        lColor->addWidget(m_editColor);
        gl->addWidget(wColor);

        // 2.1 顶部色相滑块
        // 2026-06-xx 物理对齐：滑块及其容器增加 4px 左右边距（相对于 gl 的 0 边距），实现视觉平衡
        QWidget* hueContainer = new QWidget(g);
        QHBoxLayout* hueLayout = new QHBoxLayout(hueContainer);
        hueLayout->setContentsMargins(5, 0, 5, 0);
        hueLayout->setSpacing(0);
        
        InlineHueSlider* hueSlider = new InlineHueSlider(hueContainer);
        hueLayout->addWidget(hueSlider);
        connect(hueSlider, &InlineHueSlider::sliderReleased, this, [this, hueSlider]() {
            int h = hueSlider->hue();
            QColor c;
            if (h == 1000) c = Qt::black;
            else if (h == 1001) c = QColor("#808080");
            else if (h == 1002) c = Qt::white;
            else c = QColor::fromHsv(h, 220, 220);

            QString hex = c.name().toUpper();
            m_filter.colors.clear();
            m_filter.colors.append(hex);
            
            // LRU 更新 (2026-06-xx: 容量扩展至 50 个，且由左上向右下按时间排布)
            m_recentColors.removeAll(hex);
            m_recentColors.prepend(hex);
            if (m_recentColors.size() > 50) m_recentColors.removeLast();
            AppConfig::instance().setValue("Filter/RecentColors", m_recentColors);

            emit filterChanged(m_filter);
            rebuildGroups();
        });
        gl->addWidget(hueContainer);

        // 2.1.5 颜色准确度 (容差) 滑块 ─────────────────────────
        // 2026-07-xx 按照用户要求：还原此前被误删的准确度控制条
        QWidget* accContainer = new QWidget(g);
        QHBoxLayout* accLayout = new QHBoxLayout(accContainer);
        accLayout->setContentsMargins(10, 4, 10, 4);
        accLayout->setSpacing(8);

        QLabel* lblAcc = new QLabel("准确度:", accContainer);
        lblAcc->setStyleSheet("color: #AAAAAA; font-size: 11px;");
        accLayout->addWidget(lblAcc);

        m_accuracySlider = new QSlider(Qt::Horizontal, accContainer);
        m_accuracySlider->setRange(0, 100);
        m_accuracySlider->setValue(m_filter.colorTolerance);
        m_accuracySlider->setCursor(Qt::PointingHandCursor);
        m_accuracySlider->setStyleSheet(
            "QSlider::groove:horizontal { height: 2px; background: #444; border-radius: 1px; }"
            "QSlider::handle:horizontal { background: #EEE; border: 1px solid #777; width: 10px; height: 10px; margin: -4px 0; border-radius: 5px; }"
            "QSlider::handle:horizontal:hover { background: #FFF; border-color: #378ADD; }"
        );
        accLayout->addWidget(m_accuracySlider, 1);

        connect(m_accuracySlider, &QSlider::valueChanged, this, [this](int val) {
            m_filter.colorTolerance = val;
            emit filterChanged(m_filter);
        });

        gl->addWidget(accContainer);

        // 2.1.6 颜色占比滑块 ─────────────────────────────────
        // 2026-06-23 按照用户要求：新增颜色面积占比过滤逻辑
        QWidget* areaContainer = new QWidget(g);
        QHBoxLayout* areaLayout = new QHBoxLayout(areaContainer);
        areaLayout->setContentsMargins(10, 4, 10, 4);
        areaLayout->setSpacing(8);

        QLabel* lblArea = new QLabel("占比:", areaContainer);
        lblArea->setStyleSheet("color: #AAAAAA; font-size: 11px;");
        areaLayout->addWidget(lblArea);

        m_areaSlider = new QSlider(Qt::Horizontal, areaContainer);
        m_areaSlider->setRange(0, 100);
        m_areaSlider->setValue(m_filter.minColorArea);
        m_areaSlider->setCursor(Qt::PointingHandCursor);
        m_areaSlider->setMouseTracking(true); // 2026-06-23 按照用户要求：支持悬停/滑动实时回显百分比
        m_areaSlider->installEventFilter(this);
        m_areaSlider->setStyleSheet(
            "QSlider::groove:horizontal { height: 2px; background: #444; border-radius: 1px; }"
            "QSlider::handle:horizontal { background: #EEE; border: 1px solid #777; width: 10px; height: 10px; margin: -4px 0; border-radius: 5px; }"
            "QSlider::handle:horizontal:hover { background: #FFF; border-color: #378ADD; }"
        );
        areaLayout->addWidget(m_areaSlider, 1);

        connect(m_areaSlider, &QSlider::valueChanged, this, [this](int val) {
            m_filter.minColorArea = val;
            emit filterChanged(m_filter);
        });

        gl->addWidget(areaContainer);

        // 2.2 标准色矩阵 (12色)
        // 2026-06-xx 物理对齐：设置左边距 8px 以对齐下方的复选框视觉线
        QLabel* lblStatic = new QLabel("标准色系", g);
        lblStatic->setStyleSheet("color: #666; font-size: 10px; margin-top: 4px; margin-left: 5px;");
        gl->addWidget(lblStatic);

        QWidget* staticGrid = new QWidget(g);
        staticGrid->setContentsMargins(5, 0, 5, 0); 
        // 2026-06-xx 物理微调：间距从 4px 缩减至 2px
        FlowLayout* staticFlow = new FlowLayout(staticGrid, 0, 2, 2);
        staticGrid->setLayout(staticFlow);
        
        QStringList standardHex = {
            "#E24B4A", "#EF9F27", "#FECF0E", "#639922", 
            "#1D9E75", "#378ADD", "#7F77DD", "#E91E63",
            "#000000", "#808080", "#FFFFFF", "#795548"
        };

        for (const QString& hex : standardHex) {
            ColorBlock* cb = new ColorBlock(QColor(hex), staticGrid);
            cb->setChecked(m_filter.manualExactColors.contains(hex));
            connect(cb, &ColorBlock::clicked, this, [this, hex](const QColor&) {
                if (m_filter.manualExactColors.contains(hex)) {
                    m_filter.manualExactColors.removeAll(hex);
                } else {
                    m_filter.manualExactColors.append(hex);
                }
                emit filterChanged(m_filter);
                rebuildGroups();
            });
            staticFlow->addWidget(cb);
        }
        gl->addWidget(staticGrid);

        // 2.3 最近筛选网格 (LRU)
        if (!m_recentColors.isEmpty()) {
            QLabel* lblRecent = new QLabel("最近筛选", g);
            lblRecent->setStyleSheet("color: #666; font-size: 10px; margin-top: 6px; margin-left: 5px;");
            gl->addWidget(lblRecent);

            QWidget* recentGrid = new QWidget(g);
            recentGrid->setContentsMargins(5, 0, 5, 0); 
            // 2026-06-xx 物理微调：间距缩减至 2px，保持布局对齐
            FlowLayout* recentFlow = new FlowLayout(recentGrid, 0, 2, 2);
            recentGrid->setLayout(recentFlow);

            for (const QString& hex : m_recentColors) {
                ColorBlock* cb = new ColorBlock(QColor(hex), recentGrid);
                cb->setChecked(m_filter.colors.contains(hex));
                connect(cb, &ColorBlock::clicked, this, [this, hex](const QColor&) {
                    if (m_filter.colors.contains(hex)) {
                        m_filter.colors.removeAll(hex);
                    } else {
                        m_filter.colors.append(hex);
                    }
                    emit filterChanged(m_filter);
                    rebuildGroups();
                });
                recentFlow->addWidget(cb);
            }
            gl->addWidget(recentGrid);
        }

        // 2.4 标准颜色标记列表
        static const struct { QString name; QColor color; } colorsList[] = {
            {"无色标", QColor("#808080")},
            {"红色",   QColor("#E24B4A")},
            {"橙色",   QColor("#EF9F27")},
            {"黄色",   QColor("#FECF0E")},
            {"绿色",   QColor("#639922")},
            {"蓝色",   QColor("#378ADD")},
            {"紫色",   QColor("#7F77DD")}
        };

        for (const auto& item : colorsList) {
            int cnt = m_colorCounts.value(item.name, 0);
            QCheckBox* cb = addFilterRow(gl, item.name, cnt, item.color);
            cb->setChecked(m_filter.colors.contains(item.name));
            connect(cb, &QCheckBox::checkStateChanged, this, [this, name = item.name](Qt::CheckState state) {
                if (state == Qt::Checked) {
                    if (!m_filter.colors.contains(name)) m_filter.colors.append(name);
                } else {
                    m_filter.colors.removeAll(name);
                }
                emit filterChanged(m_filter);
            });
        }
=======
        QHBoxLayout* hdrLayout = nullptr;
        QWidget* g = buildGroup("颜色标记", gl, &hdrLayout);
        m_groupColor = g;

        // 标准颜色标记列表（保持一致的复选框列表形式）
        static const struct { QString name; QColor color; } colorsList[] = {
            {"无色标", QColor("#808080")},
            {"红色",   QColor("#E24B4A")},
            {"橙色",   QColor("#EF9F27")},
            {"黄色",   QColor("#FECF0E")},
            {"绿色",   QColor("#639922")},
            {"蓝色",   QColor("#378ADD")},
            {"紫色",   QColor("#7F77DD")}
        };

        for (const auto& item : colorsList) {
            int cnt = m_colorCounts.value(item.name, 0);
            QCheckBox* cb = addFilterRow(gl, item.name, cnt, item.color);
            cb->setChecked(m_filter.colors.contains(item.name));
            connect(cb, &QCheckBox::checkStateChanged, this, [this, name = item.name](Qt::CheckState state) {
                if (state == Qt::Checked) {
                    if (!m_filter.colors.contains(name)) m_filter.colors.append(name);
                } else {
                    m_filter.colors.removeAll(name);
                }
                emit filterChanged(m_filter);
            });
        }
>>>>>>> REPLACE
```

---

### 3. `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
    // 2. 颜色过滤 (Plan-18: 基于 CIELAB Delta E 的感知筛选逻辑)
    if (!currentFilter.colors.isEmpty() || !currentFilter.colorFilterText.isEmpty()) { 
        bool matchColor = false;

        // 计算自动提取色的匹配面积占比
        auto calculateAutoColorMatchedArea = [&](const QColor& targetCol) -> float {
            if (!targetCol.isValid()) return 0.0f;
            float totalMatchedArea = 0.0f;

            // Case A: 有调色盘数据，累加所有符合色差要求的色块占比
            if (!record.palettes.empty()) {
                for (const auto& pe : record.palettes) {
                    if (UiHelper::calculateDeltaE(targetCol, pe.first) < currentFilter.colorTolerance) {
                        totalMatchedArea += pe.second;
                    }
                }
            } else if (!record.autoColor.isEmpty()) {
                // Case B: 仅有自动主色调数据，若自动主色匹配则占比视为 100%
                QColor recordCol = UiHelper::parseColorName(record.autoColor);
                if (UiHelper::calculateDeltaE(targetCol, recordCol) < currentFilter.colorTolerance) {
                    totalMatchedArea = 1.0f;
                }
            }
            return totalMatchedArea;
        };

        // 判断特定的 targetCol 是否与当前记录匹配（结合手动色与自动色）
        auto isColorMatched = [&](const QColor& targetCol) -> bool {
            if (!targetCol.isValid()) return false;

            // 1. 检查手动色：单一颜色值匹配，不受最小面积占比限制
            if (!record.manualColor.isEmpty()) {
                QColor recordCol = UiHelper::parseColorName(record.manualColor);
                if (UiHelper::calculateDeltaE(targetCol, recordCol) < currentFilter.colorTolerance) {
                    return true;
                }
            }

            // 2. 检查自动色：利用 palettes 占比及 minColorArea 限制
            float area = calculateAutoColorMatchedArea(targetCol);
            if (area > 0.0f && area * 100.0f >= (float)currentFilter.minColorArea) {
                return true;
            }

            return false;
        };

        // 2.0 文本过滤逻辑 (如果存在文本)
        if (!currentFilter.colorFilterText.isEmpty()) {
            QColor searchCol = UiHelper::parseColorName(currentFilter.colorFilterText);
            if (searchCol.isValid()) {
                if (isColorMatched(searchCol)) matchColor = true;
            } else {
                // 模糊名称或未标记匹配
                if (record.manualColor.contains(currentFilter.colorFilterText, Qt::CaseInsensitive) ||
                    record.autoColor.contains(currentFilter.colorFilterText, Qt::CaseInsensitive)) {
                    matchColor = true;
                }
            }
        }

        // 2.1 颜色筛选列表匹配
        if (!matchColor && !currentFilter.colors.isEmpty()) {
            for (const QString& colName : currentFilter.colors) {
                QColor filterCol = UiHelper::parseColorName(colName);
                if (filterCol.isValid()) {
                    if (isColorMatched(filterCol)) {
                        matchColor = true;
                        break;
                    }
                } else {
                    // 名称精准匹配（如 "红色" 或 "无色标"）
                    if (record.colorLabel.contains(colName, Qt::CaseInsensitive) ||
                        record.manualColor.contains(colName, Qt::CaseInsensitive) ||
                        (colName == "无色标" && record.colorLabel.isEmpty() && record.manualColor.isEmpty())) {
                        matchColor = true;
                        break;
                    }
                }
            }
        }

        if (!matchColor) return false;
    }
=======
    // 2. 颜色标记过滤（基础名称/标签精准匹配）
    if (!currentFilter.colors.isEmpty()) {
        bool matchColor = false;
        for (const QString& colName : currentFilter.colors) {
            if (colName == "无色标") {
                if (record.colorLabel.isEmpty() && record.manualColor.isEmpty()) {
                    matchColor = true;
                    break;
                }
            } else {
                if (record.colorLabel.contains(colName, Qt::CaseInsensitive) ||
                    record.manualColor.contains(colName, Qt::CaseInsensitive)) {
                    matchColor = true;
                    break;
                }
            }
        }
        if (!matchColor) return false;
    }
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### Step 1: Build Verification
```bash
mkdir -p build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug --target QuarkMeta
```

### Step 2: Visual & Functional Verification
1. Launch `QuarkMeta.exe`.
2. Observe the 5th column (`FilterPanel`) on the right.
3. Verify that the "颜色标记" section contains only clean, standard checkbox rows ("无色标", "红色", "橙色", "黄色", "绿色", "蓝色", "紫色").
4. Verify that sliders (`m_accuracySlider`, `m_areaSlider`, `hueSlider`), search edits (`m_editColor`), and color grids are completely removed and non-existent in the layout.
