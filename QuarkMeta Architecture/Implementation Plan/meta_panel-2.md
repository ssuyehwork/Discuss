# Implementation Plan - MetaPanel Star Rating & Tag Button Border Refactoring

## 1. Overview
This implementation plan addresses two UI refactoring requirements in `MetaPanel.cpp`:
1. **Star Rating Visual Balance**: The star rating section appeared visually smaller than the 8-color mark row beneath it due to hollow star icons (16x16) and 4px spacing. This is fixed by increasing star icon size to `18x18`, button fixed size to `22x22`, and layout spacing to `6px`.
2. **Dashed Border Compliance**: In accordance with the UI standard in `Memories.md` (which forbids dashed borders on all UI controls except empty folder views), the "Add Tag" buttons (`m_btnAddTagBig` and `m_btnAddTagSmall`) are updated to use solid borders (`1px solid`) instead of dashed borders (`1px dashed`).

## 2. Modified Files List
- `src/ui/MetaPanel.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/MetaPanel.cpp`

```diff
<<<<<<< SEARCH
    // 评级行（无评级 SVG + 5 星 SVG）
    QWidget* ratingRow = new QWidget(m_ratingColorBox);
    ratingRow->setStyleSheet("QWidget { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; }");
    QHBoxLayout* starLayout = new QHBoxLayout(ratingRow);
    starLayout->setContentsMargins(8, 4, 8, 4);
    starLayout->setSpacing(4);

    QPushButton* btnClearStar = new QPushButton(ratingRow);
    btnClearStar->setFixedSize(20, 20);
    btnClearStar->setCursor(Qt::PointingHandCursor);
    btnClearStar->setIcon(UiHelper::getIcon("prohibit", QColor("#888888"), 14));
    btnClearStar->setIconSize(QSize(14, 14));
    btnClearStar->setProperty("tooltipText", "清除评级");
    btnClearStar->installEventFilter(this);
    btnClearStar->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333; border-radius: 3px; }");
    connect(btnClearStar, &QPushButton::clicked, this, [this]() {
        setRating(0);
        emit metadataChanged(m_currentRating, m_currentColor);
    });
    starLayout->addWidget(btnClearStar);

    for (int i = 1; i <= 5; ++i) {
        QPushButton* btnStar = new QPushButton(ratingRow);
        btnStar->setFixedSize(20, 20);
        btnStar->setCursor(Qt::PointingHandCursor);
        btnStar->setIcon(UiHelper::getIcon("star", QColor("#555555"), 16));
        btnStar->setIconSize(QSize(16, 16));
        btnStar->setStyleSheet("QPushButton { border: none; background: transparent; }");

        connect(btnStar, &QPushButton::clicked, this, [this, i]() {
            int newRating = (m_currentRating == i) ? 0 : i;
            setRating(newRating);
            emit metadataChanged(m_currentRating, m_currentColor);
        });
        m_starBtns.append(btnStar);
        starLayout->addWidget(btnStar);
    }
=======
    // 评级行（无评级 SVG + 5 星 SVG）
    QWidget* ratingRow = new QWidget(m_ratingColorBox);
    ratingRow->setStyleSheet("QWidget { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; }");
    QHBoxLayout* starLayout = new QHBoxLayout(ratingRow);
    starLayout->setContentsMargins(8, 4, 8, 4);
    starLayout->setSpacing(6);

    QPushButton* btnClearStar = new QPushButton(ratingRow);
    btnClearStar->setFixedSize(22, 22);
    btnClearStar->setCursor(Qt::PointingHandCursor);
    btnClearStar->setIcon(UiHelper::getIcon("prohibit", QColor("#888888"), 14));
    btnClearStar->setIconSize(QSize(14, 14));
    btnClearStar->setProperty("tooltipText", "清除评级");
    btnClearStar->installEventFilter(this);
    btnClearStar->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333; border-radius: 3px; }");
    connect(btnClearStar, &QPushButton::clicked, this, [this]() {
        setRating(0);
        emit metadataChanged(m_currentRating, m_currentColor);
    });
    starLayout->addWidget(btnClearStar);

    for (int i = 1; i <= 5; ++i) {
        QPushButton* btnStar = new QPushButton(ratingRow);
        btnStar->setFixedSize(22, 22);
        btnStar->setCursor(Qt::PointingHandCursor);
        btnStar->setIcon(UiHelper::getIcon("star", QColor("#555555"), 18));
        btnStar->setIconSize(QSize(18, 18));
        btnStar->setStyleSheet("QPushButton { border: none; background: transparent; }");

        connect(btnStar, &QPushButton::clicked, this, [this, i]() {
            int newRating = (m_currentRating == i) ? 0 : i;
            setRating(newRating);
            emit metadataChanged(m_currentRating, m_currentColor);
        });
        m_starBtns.append(btnStar);
        starLayout->addWidget(btnStar);
    }
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // 颜色标记行（无色标 SVG + 8 基础色）
    QWidget* colorRow = new QWidget(m_ratingColorBox);
    colorRow->setStyleSheet("QWidget { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; }");
    QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(8, 4, 8, 4);
    colorLayout->setSpacing(4);

    QPushButton* btnNoColor = new QPushButton(colorRow);
    btnNoColor->setFixedSize(18, 18);
    btnNoColor->setCursor(Qt::PointingHandCursor);
    btnNoColor->setIcon(UiHelper::getIcon("no_color", QColor("#888888"), 14));
    btnNoColor->setIconSize(QSize(14, 14));
    btnNoColor->setProperty("tooltipText", "无色标");
    btnNoColor->installEventFilter(this);
    btnNoColor->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333; border-radius: 9px; }");
=======
    // 颜色标记行（无色标 SVG + 8 基础色）
    QWidget* colorRow = new QWidget(m_ratingColorBox);
    colorRow->setStyleSheet("QWidget { background: #252526; border: 1px solid #3c3c3c; border-radius: 4px; }");
    QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(8, 4, 8, 4);
    colorLayout->setSpacing(4);

    QPushButton* btnNoColor = new QPushButton(colorRow);
    btnNoColor->setFixedSize(20, 20);
    btnNoColor->setCursor(Qt::PointingHandCursor);
    btnNoColor->setIcon(UiHelper::getIcon("no_color", QColor("#888888"), 14));
    btnNoColor->setIconSize(QSize(14, 14));
    btnNoColor->setProperty("tooltipText", "无色标");
    btnNoColor->installEventFilter(this);
    btnNoColor->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #333; border-radius: 10px; }");
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    m_btnAddTagBig = new QPushButton(UiHelper::getIcon("add", QColor("#AAAAAA"), 14), "添加标签", m_tagBox);
    m_btnAddTagBig->setFixedHeight(28);
    m_btnAddTagBig->setCursor(Qt::PointingHandCursor);
    m_btnAddTagBig->setStyleSheet(
        "QPushButton { background-color: #252526; border: 1px dashed #3c3c3c; border-radius: 4px; padding: 0 10px; color: #AAAAAA; font-size: 12px; text-align: center; }"
        "QPushButton:hover { background-color: #2a2d2e; border-color: #378ADD; color: #FFFFFF; }"
        "QPushButton:pressed { background-color: #333333; }"
    );
=======
    m_btnAddTagBig = new QPushButton(UiHelper::getIcon("add", QColor("#AAAAAA"), 14), "添加标签", m_tagBox);
    m_btnAddTagBig->setFixedHeight(28);
    m_btnAddTagBig->setCursor(Qt::PointingHandCursor);
    m_btnAddTagBig->setStyleSheet(
        "QPushButton { background-color: #252526; border: 1px solid #3c3c3c; border-radius: 4px; padding: 0 10px; color: #AAAAAA; font-size: 12px; text-align: center; }"
        "QPushButton:hover { background-color: #2a2d2e; border-color: #378ADD; color: #FFFFFF; }"
        "QPushButton:pressed { background-color: #333333; }"
    );
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    m_btnAddTagSmall = new QPushButton(m_tagContainer);
    m_btnAddTagSmall->setFixedSize(22, 22);
    m_btnAddTagSmall->setCursor(Qt::PointingHandCursor);
    m_btnAddTagSmall->setIcon(UiHelper::getIcon("add", QColor("#CCCCCC"), 12));
    m_btnAddTagSmall->setIconSize(QSize(12, 12));
    m_btnAddTagSmall->setProperty("tooltipText", "添加标签");
    m_btnAddTagSmall->installEventFilter(this);
    m_btnAddTagSmall->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px dashed #555555; border-radius: 4px; padding: 0; }"
        "QPushButton:hover { background-color: #378ADD; border-color: #378ADD; }"
    );
=======
    m_btnAddTagSmall = new QPushButton(m_tagContainer);
    m_btnAddTagSmall->setFixedSize(22, 22);
    m_btnAddTagSmall->setCursor(Qt::PointingHandCursor);
    m_btnAddTagSmall->setIcon(UiHelper::getIcon("add", QColor("#CCCCCC"), 12));
    m_btnAddTagSmall->setIconSize(QSize(12, 12));
    m_btnAddTagSmall->setProperty("tooltipText", "添加标签");
    m_btnAddTagSmall->installEventFilter(this);
    m_btnAddTagSmall->setStyleSheet(
        "QPushButton { background-color: #2D2D30; border: 1px solid #555555; border-radius: 4px; padding: 0; }"
        "QPushButton:hover { background-color: #378ADD; border-color: #378ADD; }"
    );
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void MetaPanel::setRating(int rating) {
    m_currentRating = rating;
    for (int i = 0; i < m_starBtns.size(); ++i) {
        bool active = (i < rating);
        m_starBtns[i]->setIcon(UiHelper::getIcon(
            active ? "star_filled" : "star",
            active ? QColor("#FF551C") : QColor("#555555"),
            16
        ));
    }
}
=======
void MetaPanel::setRating(int rating) {
    m_currentRating = rating;
    for (int i = 0; i < m_starBtns.size(); ++i) {
        bool active = (i < rating);
        m_starBtns[i]->setIcon(UiHelper::getIcon(
            active ? "star_filled" : "star",
            active ? QColor("#FF551C") : QColor("#555555"),
            18
        ));
        m_starBtns[i]->setIconSize(QSize(18, 18));
    }
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Configure and build the CMake target:
   ```bash
   cmake -B build
   cmake --build build --config Release
   ```
2. Launch the application and select any item to populate `MetaPanel`.
3. Verify visually that:
   - Star icons are balanced in proportion relative to the color dot row.
   - The "Add Tag" buttons (`m_btnAddTagBig` and `m_btnAddTagSmall`) use clean solid borders (`1px solid`) without any dashed lines, strictly adhering to `Memories.md`.
