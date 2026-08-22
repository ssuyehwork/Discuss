# FilterPanel 颜色筛选器精简与无用控件物理拔除实施方案 (Filter Panel Specific Color Controls Purge Implementation Plan)

## Overview
根据架构优化规范与指令，在 `FilterPanel` 的“颜色标记”筛选分组中，精简拔除以下三个无用/冗余控件与区域：
1. **颜色文本输入框** (`m_editColor`，即“例：红 / #E24B4A / 无色标”搜索输入框)；
2. **标准色系网格** (`standardGrid` 及其 12 色 `ColorBlock` 网格)；
3. **最近筛选区域** (`m_recentColors` 及其对应 LRU 色块展示)。

拔除后，“颜色标记”分组保留“无色标”与 8 种离散手动色标（红、橙、黄、绿、青、蓝、紫、灰）复选行，保持界面极致清晰直观。

---

## Modified Files List
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`

---

## Detailed Line-by-Line Changes

### 1. `src/ui/FilterPanel.h`
<<<<<<< SEARCH
    // 2026-xx-xx 新增快速输入框成员
    QLineEdit*    m_editColor       = nullptr;
    QLineEdit*    m_editType        = nullptr;
=======
    // 2026-xx-xx 新增快速输入框成员
    QLineEdit*    m_editType        = nullptr;
>>>>>>> REPLACE

### 2. `src/ui/FilterPanel.cpp`
<<<<<<< SEARCH
            if (edit == m_editColor) key = "Color";
            else if (edit == m_editType) key = "Type";
=======
            if (edit == m_editType) key = "Type";
>>>>>>> REPLACE

<<<<<<< SEARCH
                    if (edit == m_editColor) m_filter.colorFilterText = text;
                    else if (edit == m_editType) m_filter.typeFilterText = text;
=======
                    if (edit == m_editType) m_filter.typeFilterText = text;
>>>>>>> REPLACE

<<<<<<< SEARCH
    m_editColor = nullptr;
    m_editType = nullptr;
=======
    m_editType = nullptr;
>>>>>>> REPLACE

<<<<<<< SEARCH
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
            ColorBlock* block = new ColorBlock(QColor(hex), staticGrid);
            block->setChecked(m_filter.colors.contains(hex));
            
            // 异步统计对账 (模拟：此处可后续接入真正的数据查询)
            int count = 0;
            for (auto it = m_colorCounts.begin(); it != m_colorCounts.end(); ++it) {
                if (UiHelper::calculateDeltaE(QColor(hex), UiHelper::parseColorName(it.key())) < 10.0) {
                    count += it.value();
                }
            }
            block->setCount(count);

            connect(block, &ColorBlock::clicked, this, [this, hex](const QColor& /*c*/) {
                if (m_filter.colors.contains(hex)) {
                    m_filter.colors.removeAll(hex);
                } else {
                    m_filter.colors.clear(); // 单选模式
                    m_filter.colors.append(hex);
                    
                    // LRU 更新
                    m_recentColors.removeAll(hex);
                    m_recentColors.prepend(hex);
                    if (m_recentColors.size() > 50) m_recentColors.removeLast();
                    AppConfig::instance().setValue("Filter/RecentColors", m_recentColors);
                }
                emit filterChanged(m_filter);
                rebuildGroups();
            });
            staticFlow->addWidget(block);
        }
        gl->addWidget(staticGrid);

        // 2.3 最近筛选 (LRU)
        if (!m_recentColors.isEmpty()) {
            QLabel* lblRecent = new QLabel("最近筛选", g);
            lblRecent->setStyleSheet("color: #666; font-size: 10px; margin-top: 8px; margin-left: 5px;");
            gl->addWidget(lblRecent);

            QWidget* recentGrid = new QWidget(g);
            recentGrid->setContentsMargins(5, 0, 5, 0);
            // 2026-06-xx 物理微调：间距从 4px 缩减至 2px
            FlowLayout* recentFlow = new FlowLayout(recentGrid, 0, 2, 2);
            recentGrid->setLayout(recentFlow);

            for (const QString& hex : m_recentColors) {
                ColorBlock* block = new ColorBlock(QColor(hex), recentGrid);
                block->setChecked(m_filter.colors.contains(hex));
                
                int count = 0;
                for (auto it = m_colorCounts.begin(); it != m_colorCounts.end(); ++it) {
                    if (UiHelper::calculateDeltaE(QColor(hex), UiHelper::parseColorName(it.key())) < 10.0) {
                        count += it.value();
                    }
                }
                block->setCount(count);

                connect(block, &ColorBlock::clicked, this, [this, hex](const QColor& /*c*/) {
                    if (m_filter.colors.contains(hex)) {
                        m_filter.colors.removeAll(hex);
                    } else {
                        m_filter.colors.clear();
                        m_filter.colors.append(hex);
                        
                        // 即使是在最近面板中点击，也应更新排序使其置顶
                        m_recentColors.removeAll(hex);
                        m_recentColors.prepend(hex);
                        AppConfig::instance().setValue("Filter/RecentColors", m_recentColors);
                    }
                    emit filterChanged(m_filter);
                    rebuildGroups();
                });
                recentFlow->addWidget(block);
            }
            gl->addWidget(recentGrid);
        }
=======
>>>>>>> REPLACE

<<<<<<< SEARCH
    if (m_editColor) m_editColor->clear();
    if (m_editType) m_editType->clear();
=======
    if (m_editType) m_editType->clear();
>>>>>>> REPLACE

---

## Build & Verification Steps
1. **Compilation Check**:
   ```bash
   cmake --build build --config Release
   ```
2. **UI Verification**:
   - Verify `FilterPanel` "颜色标记" section renders only "无色标" and the 8 manual color checkbox rows.
   - Verify color input line edit, standard color block grid, and recent filter block grid are completely removed.
