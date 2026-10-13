# Implementation Plan - Filename Recommended Tags SVG Icons & Distinct Vertical Section Blocks (TagSelectorOverlay)

## 1. Overview
This updated implementation plan addresses user feedback regarding icon rendering and layout structure:
1. **Strict 100% SVG Icon Usage (`UiHelper::getIcon`)**: No text Unicode symbols (e.g., `★`) are used. Recommended tag buttons render vector SVG icons (`UiHelper::getIcon("star_filled", ...)` or `UiHelper::getIcon("sparkles", ...)`).
2. **Distinct Vertical Section Blocks (推荐与历史/全部 独立分行分块)**: "推荐" (Recommended) and "全部/历史" (All/Lexicon) tags are rendered in separate vertical layout blocks (distinct section headers with dedicated sub-FlowLayouts/containers) rather than mixed on the same horizontal row.
3. **Database Lexicon Persistence & Double Binding**: Toggling any recommended tag calls `TagLexiconService::instance().addTag(cleanTag)` to record it in the SQLite database lexicon and applies/removes the tag for the currently selected file(s).

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update Header Signature in `TagSelectorOverlay.h`

```
<<<<<<< SEARCH
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;
=======
public:
    TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths = {}, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QStringList m_selectedTags;
    QStringList m_displayedTags;
=======
    QStringList m_selectedTags;
    QStringList m_displayedTags;
    QStringList m_recommendedTags;

    void extractRecommendedTags(const QStringList& paths);
>>>>>>> REPLACE
```

---

### 3.2 Update Layout & Grid Implementation in `TagSelectorOverlay.cpp`

#### 3.2.1 Filename Token Extraction Logic

```
<<<<<<< SEARCH
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), 
      m_selectedTags(initialSelected) 
{
    setObjectName("TagSelectorOverlay");
=======
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), 
      m_selectedTags(initialSelected) 
{
    setObjectName("TagSelectorOverlay");
    extractRecommendedTags(candidatePaths);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void TagSelectorOverlay::loadTagsAndGroups() {
=======
void TagSelectorOverlay::extractRecommendedTags(const QStringList& paths) {
    m_recommendedTags.clear();
    QSet<QString> seen;
    
    static const QRegularExpression splitRegex("[_\\-\\s\\+\\.\\(\\)\\[\\]【】]+");

    for (const QString& path : paths) {
        if (path.isEmpty() || path.startsWith("trash://")) continue;
        QFileInfo info(path);
        QString baseName = info.completeBaseName();
        if (baseName.isEmpty()) continue;

        QStringList tokens = baseName.split(splitRegex, Qt::SkipEmptyParts);
        for (QString token : tokens) {
            token = token.trimmed();
            if (token.isEmpty()) continue;
            
            bool isPureNum = false;
            token.toInt(&isPureNum);
            if (isPureNum) continue;

            if (!seen.contains(token)) {
                seen.insert(token);
                m_recommendedTags.append(token);
            }
        }
    }
}

void TagSelectorOverlay::loadTagsAndGroups() {
>>>>>>> REPLACE
```

#### 3.2.2 Vertical Section Block Architecture in `populateGrid()`

```
<<<<<<< SEARCH
void TagSelectorOverlay::populateGrid() {
    QLayoutItem* item;
    while ((item = m_gridFlowLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    m_tagButtons.clear();
    for (int i = 0; i < m_displayedTags.size(); ++i) {
        QString tag = m_displayedTags[i];

        QPushButton* btn = new QPushButton(m_tagGridWidget);
        btn->setObjectName("TagOverlayBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(22);

        m_tagButtons.append(btn);
        m_gridFlowLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, tag]() {
            toggleTagSelection(tag);
        });
    }

    m_currentTagIndex = -1;
    updateSelectionHighlight();
}
=======
void TagSelectorOverlay::populateGrid() {
    QLayoutItem* item;
    while ((item = m_gridFlowLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    m_tagButtons.clear();

    QString kw = m_searchEdit ? m_searchEdit->text().trimmed() : "";
    bool showRec = (kw.isEmpty() && !m_recommendedTags.isEmpty());

    // BLOCK 1: Dedicated Vertical Block for "推荐"
    if (showRec) {
        QWidget* recSection = new QWidget(m_tagGridWidget);
        QVBoxLayout* recSectionLayout = new QVBoxLayout(recSection);
        recSectionLayout->setContentsMargins(0, 0, 0, 8);
        recSectionLayout->setSpacing(4);

        // Header Label with SVG Icon
        QHBoxLayout* recHeaderLayout = new QHBoxLayout();
        recHeaderLayout->setContentsMargins(0, 0, 0, 0);
        recHeaderLayout->setSpacing(4);

        QLabel* recIconLabel = new QLabel(recSection);
        recIconLabel->setPixmap(UiHelper::getIcon("star_filled", QColor("#378ADD"), 14).pixmap(14, 14));
        recHeaderLayout->addWidget(recIconLabel);

        QLabel* recTitleLabel = new QLabel(QString("推荐 (%1)").arg(m_recommendedTags.size()), recSection);
        recTitleLabel->setStyleSheet("color: #378ADD; font-weight: bold; font-size: 13px;");
        recHeaderLayout->addWidget(recTitleLabel);
        recHeaderLayout->addStretch();
        recSectionLayout->addLayout(recHeaderLayout);

        // Sub-FlowLayout for Recommended Tag Pills
        QWidget* recContainer = new QWidget(recSection);
        FlowLayout* recFlow = new FlowLayout(recContainer, 0, 6, 6);

        for (const QString& recTag : m_recommendedTags) {
            QPushButton* btn = new QPushButton(recContainer);
            btn->setObjectName("TagOverlayRecBtn");
            btn->setCheckable(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedHeight(22);

            bool isSelected = m_selectedTags.contains(recTag);
            btn->setText(recTag);
            btn->setIcon(UiHelper::getIcon(isSelected ? "check" : "star_filled", isSelected ? QColor("#FFFFFF") : QColor("#378ADD"), 12));
            btn->setIconSize(QSize(12, 12));

            m_tagButtons.append(btn);
            recFlow->addWidget(btn);

            connect(btn, &QPushButton::clicked, this, [this, recTag]() {
                toggleTagSelection(recTag);
            });
        }

        recSectionLayout->addWidget(recContainer);
        m_gridFlowLayout->addWidget(recSection);
    }

    // BLOCK 2: Dedicated Vertical Block for "全部" / "历史"
    QWidget* allSection = new QWidget(m_tagGridWidget);
    QVBoxLayout* allSectionLayout = new QVBoxLayout(allSection);
    allSectionLayout->setContentsMargins(0, 0, 0, 0);
    allSectionLayout->setSpacing(4);

    if (showRec) {
        QHBoxLayout* allHeaderLayout = new QHBoxLayout();
        allHeaderLayout->setContentsMargins(0, 0, 0, 0);
        allHeaderLayout->setSpacing(4);

        QLabel* allIconLabel = new QLabel(allSection);
        allIconLabel->setPixmap(UiHelper::getIcon("all_data", QColor("#888888"), 14).pixmap(14, 14));
        allHeaderLayout->addWidget(allIconLabel);

        QLabel* allTitleLabel = new QLabel("全部标签", allSection);
        allTitleLabel->setStyleSheet("color: #AAAAAA; font-weight: bold; font-size: 13px;");
        allHeaderLayout->addWidget(allTitleLabel);
        allHeaderLayout->addStretch();
        allSectionLayout->addLayout(allHeaderLayout);
    }

    QWidget* allContainer = new QWidget(allSection);
    FlowLayout* allFlow = new FlowLayout(allContainer, 0, 6, 6);

    for (int i = 0; i < m_displayedTags.size(); ++i) {
        QString tag = m_displayedTags[i];
        if (showRec && m_recommendedTags.contains(tag)) {
            continue; // Deduplicate from all block if already present in recommended block
        }

        QPushButton* btn = new QPushButton(allContainer);
        btn->setObjectName("TagOverlayBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(22);

        m_tagButtons.append(btn);
        allFlow->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, tag]() {
            toggleTagSelection(tag);
        });
    }

    allSectionLayout->addWidget(allContainer);
    m_gridFlowLayout->addWidget(allSection);

    m_currentTagIndex = -1;
    updateSelectionHighlight();
}
>>>>>>> REPLACE
```

---

### 3.3 Pass Candidate Paths in `MetaPanel.cpp`

```
<<<<<<< SEARCH
void MetaPanel::openTagSelectorOverlay(QWidget* targetAnchor) {
    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
        return;
    }

    QWidget* topWidget = this->topLevelWidget();
    m_tagSelectorOverlay = new TagSelectorOverlay(m_currentTagsSet.values(), topWidget);
=======
void MetaPanel::openTagSelectorOverlay(QWidget* targetAnchor) {
    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
        return;
    }

    QWidget* topWidget = this->topLevelWidget();
    m_tagSelectorOverlay = new TagSelectorOverlay(m_currentTagsSet.values(), m_selectedPaths, topWidget);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional Verification**:
   - Verify 100% SVG icons rendered using `UiHelper::getIcon` with vector resolution.
   - Verify "推荐" section appears in a distinct vertical section block above "全部标签".
