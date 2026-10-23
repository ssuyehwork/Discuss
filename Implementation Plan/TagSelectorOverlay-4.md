# Implementation Plan - Filename Recommended Tags Integration & DB Lexicon Sync (TagSelectorOverlay)

## 1. Overview
When a user clicks "Add Tag" (`m_btnAddTagBig` / `m_btnAddTagSmall`) in `MetaPanel`, `TagSelectorOverlay` opens.
This implementation plan enhances `TagSelectorOverlay` to automatically extract tokens from selected item filename(s), display a "Recommended Tags" (推荐) section at the top of the tag overlay grid, and allow 1-click toggling of these recommended tags. Toggling a tag simultaneously applies it to the selected file(s) and registers/persists it in the global database lexicon (`TagLexiconService`).

### Key Enhancements:
1. **Filename Tokenization**: Parse selected item filenames using splitters (`_`, `-`, ` `, `+`, `.`, etc.), filter out digits/extensions/empty strings, and extract candidate tags (e.g. `SCP 奢侈品_06581.eps` $\to$ `SCP`, `奢侈品`).
2. **"Recommended" (推荐) UI Section**: Render a dedicated "推荐" row at the top of the `TagSelectorOverlay` tag list, decorated with spark/star icons (`UiHelper::getIcon("star", ...)` or `UiHelper::getIcon("all_data", ...)`).
3. **Database Lexicon Persistence & Double Binding**: Clicking a recommended tag calls `TagLexiconService::instance().addTag(...)` to ensure DB lexicon insertion, while toggling its active status for the currently selected item(s).

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update Header Signature in `TagSelectorOverlay.h`

Add constructor overload / parameter for selected file paths or filename candidate tokens to `TagSelectorOverlay`:

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

Add private helper and token storage:

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

Add `extractRecommendedTags`:

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
            
            // Skip pure numbers or single characters if desired
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

#### 3.2.2 Populate Grid with Recommended Group

Update `filterTags` & `populateGrid` to include `m_recommendedTags` at top when viewing "全部" or when candidate tags exist:

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

    // 1. Render "推荐" section if recommended tags are available and search filter is empty
    QString kw = m_searchEdit ? m_searchEdit->text().trimmed() : "";
    if (kw.isEmpty() && !m_recommendedTags.isEmpty()) {
        QLabel* lblRecHeader = new QLabel(QString("推荐 (%1)").arg(m_recommendedTags.size()), m_tagGridWidget);
        lblRecHeader->setObjectName("TagOverlayRecHeader");
        lblRecHeader->setStyleSheet("color: #4a90e2; font-weight: bold; margin-top: 4px; margin-bottom: 2px;");
        m_gridFlowLayout->addWidget(lblRecHeader);

        for (const QString& recTag : m_recommendedTags) {
            QPushButton* btn = new QPushButton(m_tagGridWidget);
            btn->setObjectName("TagOverlayRecBtn");
            btn->setCheckable(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedHeight(22);
            
            m_tagButtons.append(btn);
            m_gridFlowLayout->addWidget(btn);

            connect(btn, &QPushButton::clicked, this, [this, recTag]() {
                toggleTagSelection(recTag);
            });
        }
    }

    // 2. Render all or filtered tags
    for (int i = 0; i < m_displayedTags.size(); ++i) {
        QString tag = m_displayedTags[i];
        if (m_recommendedTags.contains(tag) && kw.isEmpty()) {
            // Avoid duplicate rendering in main list if already rendered in 推荐
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

Pass `m_selectedPaths` into `TagSelectorOverlay` when opening:

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
   - Select a file such as `SCP 奢侈品_06581.eps`.
   - Click "添加标签" (`+ 添加标签`).
   - Observe the `TagSelectorOverlay` header showing `推荐 (2): [SCP] [奢侈品]`.
   - Click `[SCP]`. Verify `SCP` is checked, applied to the selected item in `MetaPanel`, and added to the SQLite database lexicon (`TagLexiconService`).

---

## 5. SSOT API Reuse & Header Signatures Verification
- **`TagLexiconService::instance().addTag(cleanTag)`**: Explicitly called inside `TagSelectorOverlay::toggleTagSelection` to ensure double binding (file tag + DB lexicon entry).
- **`Header API Verification`**:
  - `TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths = {}, QWidget* parent = nullptr);`
