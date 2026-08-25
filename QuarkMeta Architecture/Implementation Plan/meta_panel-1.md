# Implementation Plan - MetaPanel Star Rating UI Visual Balance Refactoring

## 1. Overview
The star rating section in `MetaPanel.cpp` appeared visually smaller and imbalanced compared to the 8-color mark row directly beneath it. This was caused by two issues:
1. Star icons were 16x16 with 20x20 button fixed sizes and 4px spacing, whereas star vector graphics have hollow spaces and lower pixel fill density than solid 16x16 color dots.
2. The rating row contained 6 elements with shorter total width (~140px) compared to 9 elements (~178px) in the color row, leaving noticeable blank space.

This plan refactors `MetaPanel.cpp` by:
- Increasing star icon size from `16x16` to `18x18` and button fixed size from `20x20` to `22x22`.
- Updating `setRating()` to use `18x18` icon size for both active and inactive states.
- Increasing star layout spacing from `4px` to `6px` for a balanced layout.
- Standardizing the clear rating (`btnClearStar`) and no color (`btnNoColor`) buttons to matching proportions.

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
3. Verify visually that the star icons are balanced in proportion relative to the color dot row and alignment is harmonious.
