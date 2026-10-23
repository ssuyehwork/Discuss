# SearchController 内置搜索图标与结构恢复实施方案 (SearchController-3.md)

## 1. Overview（概述与解决的问题）
为恢复与 `Version-Old-1 ~ 10` 历史经典版本的 100% 相同结构，解决搜索图标在输入框外独立放置（`m_btnSearch` 独立按钮）导致的视觉不符问题。本方案将放大镜搜索图标嵌入回 `QLineEdit#SearchEdit` 内部左侧 (`QLineEdit::LeadingPosition`)，移除独立外置的 `QPushButton#BtnSearchAddress` 控件，恢复极简一框式搜索栏视觉表现。

## 2. Modified Files List（影响文件清单）
1. `src/ui/SearchController.h`
2. `src/ui/SearchController.cpp`
3. `resources/style.qss`

## 3. Detailed Line-by-Line Changes（精准替换块）

### `src/ui/SearchController.h`
```
<<<<<<< SEARCH
    QLineEdit* searchEdit() const { return m_searchEdit; }
    QPushButton* searchButton() const { return m_btnSearch; }
    SearchHistoryPanel* historyPanel() const { return m_searchHistoryPanel; }
=======
    QLineEdit* searchEdit() const { return m_searchEdit; }
    SearchHistoryPanel* historyPanel() const { return m_searchHistoryPanel; }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnSearch = nullptr;
    QTimer* m_searchTimer = nullptr;
=======
    QLineEdit* m_searchEdit = nullptr;
    QTimer* m_searchTimer = nullptr;
>>>>>>> REPLACE
```

### `src/ui/SearchController.cpp`
```
<<<<<<< SEARCH
SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");
    m_searchContainer->setFixedSize(230, 30);

    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setObjectName("SearchEdit");
    m_searchEdit->setFixedHeight(30);

    QAction* clearAction = m_searchEdit->addAction(UiHelper::getIcon("close", TextMuted), QLineEdit::TrailingPosition);
    clearAction->setVisible(false);
    connect(clearAction, &QAction::triggered, m_searchEdit, &QLineEdit::clear);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [clearAction](const QString& text) {
        clearAction->setVisible(!text.isEmpty());
    });

    UiHelper::setupLineEditContextMenu(m_searchEdit);

    m_btnSearch = new QPushButton(m_searchContainer);
    m_btnSearch->setObjectName("BtnSearchAddress");
    m_btnSearch->setFixedSize(28, 30);
    m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#CCCCCC"), 16));
    m_btnSearch->setIconSize(QSize(16, 16));
    m_btnSearch->setCursor(Qt::PointingHandCursor);
    m_btnSearch->setProperty("tooltipText", "搜索");
    m_btnSearch->setAttribute(Qt::WA_Hover);
    m_btnSearch->installEventFilter(this);

    // TODO: 预留搜索按钮扩展功能（例如高级搜索菜单或触发搜索）
    connect(m_btnSearch, &QPushButton::clicked, this, [this]() {
        // TODO: Extended search functionality
        doSearch(m_searchEdit->text().trimmed());
    });

    searchLayout->addWidget(m_btnSearch, 0);
    searchLayout->addWidget(m_searchEdit, 1);
=======
SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");

    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setObjectName("SearchEdit");
    m_searchEdit->setFixedSize(230, 30);
    m_searchEdit->addAction(UiHelper::getIcon("seach-3", TextMuted), QLineEdit::LeadingPosition);

    QAction* clearAction = m_searchEdit->addAction(UiHelper::getIcon("close", TextMuted), QLineEdit::TrailingPosition);
    clearAction->setVisible(false);
    connect(clearAction, &QAction::triggered, m_searchEdit, &QLineEdit::clear);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [clearAction](const QString& text) {
        clearAction->setVisible(!text.isEmpty());
    });

    UiHelper::setupLineEditContextMenu(m_searchEdit);

    searchLayout->addWidget(m_searchEdit);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
bool SearchController::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonDblClick && watched == m_searchEdit) {
        auto history = SearchHistoryService::instance().getHistory("global");
        if (!history.isEmpty()) {
            m_searchHistoryPanel->setHistory(history);
            m_searchHistoryPanel->showBelow(m_searchEdit);
        }
        return true;
    }

    if (watched == m_btnSearch) {
        if (event->type() == QEvent::Enter) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-3", Qt::white, 16));
        } else if (event->type() == QEvent::Leave) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#CCCCCC"), 16));
        }
    }

    return QObject::eventFilter(watched, event);
}
=======
bool SearchController::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonDblClick && watched == m_searchEdit) {
        auto history = SearchHistoryService::instance().getHistory("global");
        if (!history.isEmpty()) {
            m_searchHistoryPanel->setHistory(history);
            m_searchHistoryPanel->showBelow(m_searchEdit);
        }
        return true;
    }

    return QObject::eventFilter(watched, event);
}
>>>>>>> REPLACE
```

### `resources/style.qss`
```
<<<<<<< SEARCH
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
    height: 30px;
}
=======
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps（编译命令与验证方法）
1. 在 build 目录运行构建命令：
   ```bash
   cmake --build build --config Release
   ```
2. 启动应用，检查搜索输入框：
   - 放大镜图标已嵌入回 `QLineEdit#SearchEdit` 的左侧内部 (`LeadingPosition`)；
   - 外部不再有独立的 `QPushButton#BtnSearchAddress` 按钮框；
   - 整个搜索框物理固定尺寸为 `(230, 30)`，与地址栏精确保持 30px 高度对齐。

## 5. SSOT API Reuse & Anti-Redundancy Self-Check（既有 SSOT 通道复用与防另起炉灶自查）
- 完美还原 `Version-Old-1 ~ 10` 历史版本的 `m_searchEdit->addAction(..., QLineEdit::LeadingPosition)` 内置图标机制。

## 6. Header API Signature Verification（头文件 API 物理签名核查表）
- `QLineEdit::addAction(const QIcon&, QLineEdit::ActionPosition)`
- `QLineEdit::setFixedSize(int w, int h)`

## 7. Header Inclusion Chain & Type Completeness Check（头文件包含链与类型完整性检查表）
- 头文件中移除不必要的 `QPushButton` 声明，保持接口整洁。
