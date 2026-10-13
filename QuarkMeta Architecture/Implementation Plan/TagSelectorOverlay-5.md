# Implementation Plan - Filename Recommended Tags Exact UI Layout (TagSelectorOverlay)

## 1. Overview
This implementation plan refined from `TagSelectorOverlay-4.md` precisely matches the user's provided design screenshot for `TagSelectorOverlay`:
1. **Blue "推荐" Label (推荐)**: Rendered in distinct accent blue text (`#4a90e2` / `#378ADD`) preceding the candidate tags extracted from the selected item's filename.
2. **Star Icon for Recommended Tags (`★ tag`)**: Recommended tags feature a filled star icon (`UiHelper::getIcon("star_filled", QColor("#4a90e2"), 12)`) to distinguish them as intelligent recommendations.
3. **"全部" Separator Label & Flow Layout**: Following the recommended tag pills, a text label `全部` (or separator) introduces the full lexicon tags below/beside.
4. **Database Lexicon Persistence & Double Binding**: Clicking any recommended tag pill calls `TagLexiconService::instance().addTag(cleanTag)` to register it in the SQLite database lexicon and toggles its tag state on the selected file(s).

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

### 3.2 Update Implementation in `TagSelectorOverlay.cpp`

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

#### 3.2.2 Populate Grid matching Screenshot UI Layout

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

    // 1. Blue "推荐" label
    if (showRec) {
        QLabel* lblRec = new QLabel("推荐", m_tagGridWidget);
        lblRec->setObjectName("TagOverlayBlueLabel");
        lblRec->setStyleSheet("color: #378ADD; font-weight: bold; font-size: 13px; margin-right: 4px;");
        m_gridFlowLayout->addWidget(lblRec);

        // Render Recommended Tag Pills with Star Icon
        for (const QString& recTag : m_recommendedTags) {
            QPushButton* btn = new QPushButton(m_tagGridWidget);
            btn->setObjectName("TagOverlayRecBtn");
            btn->setCheckable(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedHeight(22);

            bool isSelected = m_selectedTags.contains(recTag);
            btn->setText(" " + recTag);
            btn->setIcon(UiHelper::getIcon(isSelected ? "check" : "star_filled", isSelected ? QColor("#FFFFFF") : QColor("#378ADD"), 12));
            btn->setIconSize(QSize(12, 12));

            m_tagButtons.append(btn);
            m_gridFlowLayout->addWidget(btn);

            connect(btn, &QPushButton::clicked, this, [this, recTag]() {
                toggleTagSelection(recTag);
            });
        }

        // "全部" Section Label after Recommended pills
        QLabel* lblAll = new QLabel("全部", m_tagGridWidget);
        lblAll->setObjectName("TagOverlayAllLabel");
        lblAll->setStyleSheet("color: #CCCCCC; font-weight: bold; font-size: 13px; margin-left: 8px; margin-right: 4px;");
        m_gridFlowLayout->addWidget(lblAll);
    }

    // 2. Render Full Lexicon Tags
    for (int i = 0; i < m_displayedTags.size(); ++i) {
        QString tag = m_displayedTags[i];
        if (showRec && m_recommendedTags.contains(tag)) {
            continue; // Avoid duplication if rendered in 推荐
        }

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
   - Compare UI directly with user image.
   - Confirm blue **推荐** text label appears first, followed by `[★ main]` pills with blue star icons.
   - Confirm **全部** label follows, then regular tags with clock/check icons.
