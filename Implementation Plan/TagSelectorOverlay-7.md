# Implementation Plan - Filename Recommended Tags Integration (TagSelectorOverlay)

## 1. Overview
This implementation plan strictly removes all unrequested label additions (such as "全部" headers) and focuses purely on adding filename recommended candidate tags to `TagSelectorOverlay`:
1. **Filename Token Extraction**: Extract candidate tags from the selected file name(s) (e.g. `SCP 奢侈品_06581.eps` $\to$ `SCP`, `奢侈品`).
2. **Blue "推荐" Label & Candidate Tags**: Render a blue "推荐" label followed by the candidate tag buttons, each featuring a vector SVG star icon (`UiHelper::getIcon("star_filled", QColor("#378ADD"), 12)`).
3. **Clean Flow Layout**: Regular lexicon tags follow naturally in the flow layout without inserting any artificial extra headers or labels.
4. **Database Lexicon Sync & Double Binding**: Toggling any recommended tag calls `TagLexiconService::instance().addTag(cleanTag)` to ensure it is registered in the SQLite database lexicon while applying/removing the tag on the selected file(s).

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

#### 3.2.2 Grid Flow Layout in `populateGrid()`

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

    // Render "推荐" label + candidate tags at top
    if (showRec) {
        QLabel* lblRec = new QLabel("推荐", m_tagGridWidget);
        lblRec->setObjectName("TagOverlayBlueLabel");
        lblRec->setStyleSheet("color: #378ADD; font-weight: bold; font-size: 13px; margin-right: 4px;");
        m_gridFlowLayout->addWidget(lblRec);

        for (const QString& recTag : m_recommendedTags) {
            QPushButton* btn = new QPushButton(m_tagGridWidget);
            btn->setObjectName("TagOverlayRecBtn");
            btn->setCheckable(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedHeight(22);

            bool isSelected = m_selectedTags.contains(recTag);
            btn->setText(recTag);
            btn->setIcon(UiHelper::getIcon(isSelected ? "check" : "star_filled", isSelected ? QColor("#FFFFFF") : QColor("#378ADD"), 12));
            btn->setIconSize(QSize(12, 12));

            m_tagButtons.append(btn);
            m_gridFlowLayout->addWidget(btn);

            connect(btn, &QPushButton::clicked, this, [this, recTag]() {
                toggleTagSelection(recTag);
            });
        }
    }

    // Render standard tags naturally
    for (int i = 0; i < m_displayedTags.size(); ++i) {
        QString tag = m_displayedTags[i];
        if (showRec && m_recommendedTags.contains(tag)) {
            continue;
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
   - Verify blue "推荐" label + candidate pills render with vector SVG star icons.
   - Verify no extra "全部" labels or unrequested headers are inserted anywhere.
