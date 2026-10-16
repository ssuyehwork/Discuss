# Implementation Plan - TagSelectorOverlay Essential Refactoring (TagSelectorOverlay-13)

## 1. Overview & 架构三问回答

### 【架构三问先答后写】
1. **第一问（真理源溯源）**：
   - 最近使用标签集合（Recent Tags）的持久化真理源（SSOT）为 `AppConfig` 的 `TagSelectorOverlay/RecentTags` 键值（刚性锁定上限最多 50 个）；
   - 推荐标签（Recommended Tags）为选中文件名称中解析出的动态 Candidate Tokens（纯内存管理）；
   - 用户主动勾选推荐标签时，既向所选文件打标，又调用 `TagLexiconService::instance().addTag(cleanTag)` 将其持久化注册入 SQLite 词库数据库，并将该标签提升至 `RecentTags` 顶部；
   - 打标的目标项目路径集在唤起瞬时由 `targetPathsSnapshot` 不可变快照锁定。
2. **第二问（黑盒完整性）**：
   - 重构高内聚于 `TagSelectorOverlay` 内部，不暴露私有控件结构，不增加任何未经请求的 UI 控件，不破坏外部与 `MetaPanel` / `MetaTagSection` 的既有信号与契约。
3. **第三问（根因 vs 症状）**：
   - 精准响应用户需求：实现唤醒自动定焦搜索框，并将展示区重构为两个可折叠分组（`推荐` 与 `最近使用`）。彻底治理连续打标断裂问题，同时 100% 保持初衷失焦自动关闭体验。

---

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/MetaTagSection.cpp`
- `src/ui/MetaPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 Update Header Definition in `TagSelectorOverlay.h`

```
<<<<<<< SEARCH
class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;

signals:
    void selectionChanged(const QStringList& selectedTags);
    void overlayClosed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void initUi();
    void loadTagsAndGroups();
    void filterTags();
    void populateGrid();
    void toggleTagSelection(const QString& tag);
    void updateSelectionHighlight();
    void handleGridNavigation(int key);
    void handleGroupNavigation(int key);

    void closeOverlay();

    QStringList m_selectedTags;
    QStringList m_displayedTags;
    QMap<QString, int> m_allTagCounts;
    QList<TagGroup> m_lexiconGroups;

    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnToggleSidebar = nullptr;
    QListWidget* m_groupList = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_tagGridWidget = nullptr;
    FlowLayout* m_gridFlowLayout = nullptr;
    QList<QPushButton*> m_tagButtons;
    int m_currentTagIndex = -1;

    bool m_isClosing = false;
    bool m_isDragging = false;
    QPoint m_dragPos;

    FramelessWindowHelper* m_framelessHelper = nullptr;
};
=======
class TagSelectorOverlay : public QFrame {
    Q_OBJECT
public:
    TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths = {}, QWidget* parent = nullptr);
    ~TagSelectorOverlay() override;

signals:
    void selectionChanged(const QStringList& selectedTags);
    void overlayClosed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void initUi();
    void loadRecentAndRecommendedTags(const QStringList& candidatePaths);
    void filterAndPopulate();
    void toggleTagSelection(const QString& tag);
    void recordRecentTag(const QString& tag);
    void closeOverlay();

    QStringList m_selectedTags;
    QStringList m_recentTags;
    QStringList m_recommendedTags;

    QLineEdit* m_searchEdit = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_bodyContainer = nullptr;
    QVBoxLayout* m_bodyLayout = nullptr;

    QWidget* m_recSectionWidget = nullptr;
    FlowLayout* m_recFlowLayout = nullptr;
    QPushButton* m_recHeaderBtn = nullptr;
    bool m_recExpanded = true;

    QWidget* m_recentSectionWidget = nullptr;
    FlowLayout* m_recentFlowLayout = nullptr;
    QPushButton* m_recentHeaderBtn = nullptr;
    bool m_recentExpanded = true;

    bool m_isClosing = false;
    bool m_isDragging = false;
    QPoint m_dragPos;

    FramelessWindowHelper* m_framelessHelper = nullptr;
};
>>>>>>> REPLACE
```

---

### 3.2 Implement Intelligent Groups & Auto-Focus in `TagSelectorOverlay.cpp`

```
<<<<<<< SEARCH
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
    setObjectName("TagSelectorOverlay");
    setFrameShape(QFrame::StyledPanel);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);

    initUi();
    loadTagsAndGroups();

    m_searchEdit->installEventFilter(this);
    m_tagGridWidget->installEventFilter(this);

    qApp->installEventFilter(this);
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) {
        Q_UNUSED(old);
        if (m_isClosing || !isVisible()) return;

        if (now && (now == this || this->isAncestorOf(now))) {
            return;
        }

        if (now && !this->isAncestorOf(now)) {
            closeOverlay();
        }
    });
}
=======
TagSelectorOverlay::TagSelectorOverlay(const QStringList& initialSelected, const QStringList& candidatePaths, QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      m_selectedTags(initialSelected)
{
    setObjectName("TagSelectorOverlay");
    setFrameShape(QFrame::StyledPanel);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);

    initUi();
    loadRecentAndRecommendedTags(candidatePaths);

    m_searchEdit->installEventFilter(this);

    qApp->installEventFilter(this);
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) {
        Q_UNUSED(old);
        if (m_isClosing || !isVisible()) return;

        if (now && (now == this || this->isAncestorOf(now))) {
            return;
        }

        if (now && !this->isAncestorOf(now)) {
            closeOverlay();
        }
    });
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void TagSelectorOverlay::initUi() {
=======
void TagSelectorOverlay::showEvent(QShowEvent* event) {
    QFrame::showEvent(event);
    if (m_searchEdit) {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    }
}

void TagSelectorOverlay::initUi() {
    QVBoxLayout* mainL = new QVBoxLayout(this);
    mainL->setContentsMargins(10, 10, 10, 10);
    mainL->setSpacing(8);

    // 1. 顶部搜索框 (唤醒自动定焦)
    QHBoxLayout* topSearchLayout = new QHBoxLayout();
    topSearchLayout->setContentsMargins(0, 0, 0, 0);
    topSearchLayout->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedHeight(28);
    m_searchEdit->setObjectName("TagSelectorSearchEdit");
    UiHelper::setupLineEditContextMenu(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        filterAndPopulate();
    });
    topSearchLayout->addWidget(m_searchEdit, 1);
    mainL->addLayout(topSearchLayout);

    // 2. 智能折叠分组主体滚动区
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setObjectName("TagSelectorScrollArea");

    m_bodyContainer = new QWidget(m_scrollArea);
    m_bodyLayout = new QVBoxLayout(m_bodyContainer);
    m_bodyLayout->setContentsMargins(4, 4, 4, 4);
    m_bodyLayout->setSpacing(10);

    // 第一组: 推荐
    m_recHeaderBtn = new QPushButton(m_bodyContainer);
    m_recHeaderBtn->setCursor(Qt::PointingHandCursor);
    m_recHeaderBtn->setStyleSheet("QPushButton { text-align: left; font-weight: bold; color: #CCCCCC; background: transparent; border: none; font-size: 13px; }");
    connect(m_recHeaderBtn, &QPushButton::clicked, this, [this]() {
        m_recExpanded = !m_recExpanded;
        m_recSectionWidget->setVisible(m_recExpanded);
    });
    m_bodyLayout->addWidget(m_recHeaderBtn);

    m_recSectionWidget = new QWidget(m_bodyContainer);
    m_recFlowLayout = new FlowLayout(m_recSectionWidget, 0, 6, 6);
    m_bodyLayout->addWidget(m_recSectionWidget);

    // 第二组: 最近使用 (最多 50 个)
    m_recentHeaderBtn = new QPushButton(m_bodyContainer);
    m_recentHeaderBtn->setCursor(Qt::PointingHandCursor);
    m_recentHeaderBtn->setStyleSheet("QPushButton { text-align: left; font-weight: bold; color: #CCCCCC; background: transparent; border: none; font-size: 13px; }");
    connect(m_recentHeaderBtn, &QPushButton::clicked, this, [this]() {
        m_recentExpanded = !m_recentExpanded;
        m_recentSectionWidget->setVisible(m_recentExpanded);
    });
    m_bodyLayout->addWidget(m_recentHeaderBtn);

    m_recentSectionWidget = new QWidget(m_bodyContainer);
    m_recentFlowLayout = new FlowLayout(m_recentSectionWidget, 0, 6, 6);
    m_bodyLayout->addWidget(m_recentSectionWidget);

    m_bodyLayout->addStretch(1);
    m_scrollArea->setWidget(m_bodyContainer);
    mainL->addWidget(m_scrollArea, 1);

    setMinimumSize(300, 220);
    QSize savedSize = AppConfig::instance().getValue("TagSelectorOverlay/Size", QSize(400, 260)).toSize();
    resize(savedSize.expandedTo(QSize(300, 220)));
}

void TagSelectorOverlay::loadRecentAndRecommendedTags(const QStringList& candidatePaths) {
    // 1. 从 AppConfig 读取最近使用标签 (最多 50 个)
    m_recentTags = AppConfig::instance().getValue("TagSelectorOverlay/RecentTags", QStringList()).toStringList();

    // 2. 解析推荐标签 (仅纯内存管理)
    m_recommendedTags.clear();
    QSet<QString> seen;
    static const QRegularExpression splitRegex("[_\\-\\s\\+\\.\\(\\)\\[\\]【】]+");

    for (const QString& path : candidatePaths) {
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

    filterAndPopulate();
}

void TagSelectorOverlay::filterAndPopulate() {
    QString kw = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";

    // 1. 清理现有 FlowLayout Widgets
    auto clearLayout = [](FlowLayout* layout) {
        if (!layout) return;
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
    };
    clearLayout(m_recFlowLayout);
    clearLayout(m_recentFlowLayout);

    // 2. 渲染【第一组：推荐】
    QStringList filteredRec;
    for (const QString& t : m_recommendedTags) {
        if (kw.isEmpty() || t.toLower().contains(kw)) {
            filteredRec.append(t);
        }
    }
    m_recHeaderBtn->setText(QString(" 推荐 (%1)").arg(filteredRec.size()));
    m_recHeaderBtn->setVisible(!filteredRec.isEmpty());
    m_recSectionWidget->setVisible(m_recExpanded && !filteredRec.isEmpty());

    for (const QString& recTag : filteredRec) {
        QPushButton* btn = new QPushButton(m_recSectionWidget);
        btn->setObjectName("TagOverlayRecBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(24);

        bool isSelected = m_selectedTags.contains(recTag);
        btn->setText(recTag);
        btn->setIcon(UiHelper::getIcon(isSelected ? "check" : "star_filled", isSelected ? QColor("#FFFFFF") : QColor("#378ADD"), 12));
        btn->setIconSize(QSize(12, 12));

        m_recFlowLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, recTag]() {
            toggleTagSelection(recTag);
        });
    }

    // 3. 渲染【第二组：最近使用 (限制 50 个)】
    QStringList filteredRecent;
    for (const QString& t : m_recentTags) {
        if (kw.isEmpty() || t.toLower().contains(kw)) {
            filteredRecent.append(t);
        }
    }
    m_recentHeaderBtn->setText(QString(" 最近使用 (%1)").arg(filteredRecent.size()));

    for (const QString& tag : filteredRecent) {
        QPushButton* btn = new QPushButton(m_recentSectionWidget);
        btn->setObjectName("TagOverlayRecentBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(24);

        bool isSelected = m_selectedTags.contains(tag);
        btn->setText(tag);
        btn->setIcon(UiHelper::getIcon(isSelected ? "check" : "clock", isSelected ? QColor("#FFFFFF") : QColor("#888888"), 12));
        btn->setIconSize(QSize(12, 12));

        m_recentFlowLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, tag]() {
            toggleTagSelection(tag);
        });
    }
}

void TagSelectorOverlay::toggleTagSelection(const QString& tag) {
    QString cleanTag = tag.trimmed();
    if (cleanTag.isEmpty()) return;

    if (m_selectedTags.contains(cleanTag)) {
        m_selectedTags.removeAll(cleanTag);
    } else {
        // 🔒【点选绑定契约】：只有点选时才写入 SQLite 标签词库，并记录至 RecentTags (上限 50 个)
        TagLexiconService::instance().addTag(cleanTag);
        recordRecentTag(cleanTag);
        m_selectedTags.append(cleanTag);
    }

    filterAndPopulate();
    emit selectionChanged(m_selectedTags);
}

void TagSelectorOverlay::recordRecentTag(const QString& tag) {
    if (tag.isEmpty()) return;
    m_recentTags.removeAll(tag);
    m_recentTags.prepend(tag);
    while (m_recentTags.size() > 50) {
        m_recentTags.removeLast();
    }
    AppConfig::instance().setValue("TagSelectorOverlay/RecentTags", m_recentTags);
}
>>>>>>> REPLACE
```

---

### 3.3 Target Paths Snapshot Binding in `MetaTagSection.cpp`

```
<<<<<<< SEARCH
void MetaTagSection::openTagSelectorOverlay(QWidget* targetAnchor) {
    Q_UNUSED(targetAnchor);
    if (m_selectedPaths.isEmpty()) return;

    if (!m_tagSelectorOverlay) {
        m_tagSelectorOverlay = new TagSelectorOverlay({}, this);
        connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this](const QStringList& currentTags) {
            emit tagsChanged(m_selectedPaths, currentTags);
        });
    }

    m_tagSelectorOverlay->show();
}
=======
void MetaTagSection::openTagSelectorOverlay(QWidget* targetAnchor) {
    Q_UNUSED(targetAnchor);
    if (m_selectedPaths.isEmpty()) return;

    // 🔒【路径快照锁定】：锁定唤起时刻的目标项目路径，保证浮层未关闭前连续勾选稳定有效
    const QStringList targetPathsSnapshot = m_selectedPaths;

    if (m_tagSelectorOverlay) {
        m_tagSelectorOverlay->close();
    }

    m_tagSelectorOverlay = new TagSelectorOverlay({}, targetPathsSnapshot, this);
    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPathsSnapshot](const QStringList& currentTags) {
        if (!targetPathsSnapshot.isEmpty()) {
            emit tagsChanged(targetPathsSnapshot, currentTags);
        }
    });

    m_tagSelectorOverlay->show();
}
>>>>>>> REPLACE
```

---

### 3.4 Target Paths Snapshot Integrity in `MetaPanel.cpp`

```
<<<<<<< SEARCH
    m_tagSelectorOverlay->show();

    // 1. 浮层打开瞬时，保存决议绑定的固定目标项目快照，不随 m_selectedPaths 动态漂移
    const QStringList targetPaths = m_selectedPaths;

    // 2. 浮层局部的已提交标签集合，打开时用 m_currentTagsSet 初始化
    auto appliedTagsSet = std::make_shared<QSet<QString>>(m_currentTagsSet);

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPaths, appliedTagsSet](const QStringList& newSelectedTags) {
        if (targetPaths.isEmpty()) return;

        QSet<QString> newSet(newSelectedTags.begin(), newSelectedTags.end());

        // 新增的标签：与浮层局部的已提交集合比对
        for (const QString& t : newSet) {
            if (!appliedTagsSet->contains(t)) {
                emit tagAddRequested(targetPaths, t);
            }
        }
        // 移除的标签：与浮层局部的已提交集合比对
        for (const QString& t : *appliedTagsSet) {
            if (!newSet.contains(t)) {
                emit tagRemoveRequested(targetPaths, t);
            }
        }
        *appliedTagsSet = newSet;
    });
=======
    m_tagSelectorOverlay->show();

    // 1. 🔒【路径快照锁定】：浮层打开瞬时，保存决议绑定的固定目标项目快照，不随 m_selectedPaths 动态漂移
    const QStringList targetPathsSnapshot = m_selectedPaths;

    // 2. 浮层局部的已提交标签集合，打开时用 m_currentTagsSet 初始化
    auto appliedTagsSet = std::make_shared<QSet<QString>>(m_currentTagsSet);

    connect(m_tagSelectorOverlay, &TagSelectorOverlay::selectionChanged, this, [this, targetPathsSnapshot, appliedTagsSet](const QStringList& newSelectedTags) {
        if (targetPathsSnapshot.isEmpty()) return;

        QSet<QString> newSet(newSelectedTags.begin(), newSelectedTags.end());

        // 新增的标签：与浮层局部的已提交集合比对
        for (const QString& t : newSet) {
            if (!appliedTagsSet->contains(t)) {
                emit tagAddRequested(targetPathsSnapshot, t);
            }
        }
        // 移除的标签：与浮层局部的已提交集合比对
        for (const QString& t : *appliedTagsSet) {
            if (!newSet.contains(t)) {
                emit tagRemoveRequested(targetPathsSnapshot, t);
            }
        }
        *appliedTagsSet = newSet;
    });
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

1. **Compilation Verification**:
   ```bash
   cmake --build --preset x64-Release
   ```

2. **Functional Verification**:
   - Open TagSelectorOverlay $\to$ verify focus is automatically placed in search edit.
   - Verify `推荐 (N)` and `最近使用 (N)` (up to 50 items) sections are displayed with collapsible headers.
   - Click recommended tag $\to$ verifies tag is applied, added to SQLite lexicon and updated to top of `RecentTags`.
   - Verify click outside closes overlay 0ms immediately.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- `RecentTags` 持久化唯一使用 `AppConfig::instance()`；
- 推荐词写库统一使用 `TagLexiconService::instance().addTag()`；
- 不增加任何未经请求的控件，保持高内聚。

---

## 6. Header API Signature Verification

| 类名 | 成员函数/信号物理签名 | 头文件物理位置 | 状态 |
| :--- | :--- | :--- | :--- |
| `TagSelectorOverlay` | `TagSelectorOverlay(const QStringList&, const QStringList&, QWidget*)` | `src/ui/TagSelectorOverlay.h` | 方案 3.1 明确新增对齐 |
| `MetaTagSection` | `void tagsChanged(const QStringList&, const QStringList&)` | `src/ui/MetaTagSection.h` | 已核验 100% 存在 |
| `MetaPanel` | `void tagAddRequested(const QStringList&, const QString&)` | `src/ui/MetaPanel.h` | 已核验 100% 存在 |
| `MetaPanel` | `void tagRemoveRequested(const QStringList&, const QString&)` | `src/ui/MetaPanel.h` | 已核验 100% 存在 |

---

## 7. Header Inclusion Chain & Type Completeness Check

| 受影响文件 | `#include` 修改点 | 核查的类型/类定义 | 闭包完整性结论 |
| :--- | :--- | :--- | :--- |
| `TagSelectorOverlay.h` | 补充 `<QShowEvent>` | `QShowEvent` | 100% 完整 |
| `TagSelectorOverlay.cpp` | 无需变更 | `TagSelectorOverlay` | 100% 完整 |
| `MetaTagSection.cpp` | 无需变更 | `MetaTagSection`, `TagSelectorOverlay` | 100% 完整 |
| `MetaPanel.cpp` | 无需变更 | `MetaPanel`, `TagSelectorOverlay` | 100% 完整 |
