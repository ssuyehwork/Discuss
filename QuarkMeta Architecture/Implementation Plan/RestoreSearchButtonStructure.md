# Restore Search Button Structure Implementation Plan

## Overview
Restore the `seach-3` search button widget (`QPushButton#BtnSearchAddress`) inside `SearchController` with 30px height alignment, dynamic hover icon color toggling (from `#CCCCCC` to white on hover), and direct trigger handling for searches while maintaining 30px height alignment in `NavBarWidget`.

## Modified Files List
- `src/ui/SearchController.cpp`
- `src/ui/SearchController.h`

## Detailed Line-by-Line Changes

### 1. `src/ui/SearchController.h`
```
<<<<<<< SEARCH
    QLineEdit* m_searchEdit = nullptr;
=======
    QPushButton* m_btnSearch = nullptr;
    QLineEdit* m_searchEdit = nullptr;
>>>>>>> REPLACE
```

### 2. `src/ui/SearchController.cpp`
```
<<<<<<< SEARCH
SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");
    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 30);
    m_searchEdit->addAction(UiHelper::getIcon("search", TextMuted), QLineEdit::LeadingPosition);

    QAction* clearAction = m_searchEdit->addAction(UiHelper::getIcon("close", TextMuted), QLineEdit::TrailingPosition);
    clearAction->setVisible(false);
    connect(clearAction, &QAction::triggered, m_searchEdit, &QLineEdit::clear);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [clearAction](const QString& text) {
        clearAction->setVisible(!text.isEmpty());
    });

    m_searchEdit->setObjectName("SearchEdit");
    UiHelper::setupLineEditContextMenu(m_searchEdit);

    searchLayout->addWidget(m_searchEdit);
=======
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

    connect(m_btnSearch, &QPushButton::clicked, this, [this]() {
        doSearch(m_searchEdit->text().trimmed());
    });

    searchLayout->addWidget(m_btnSearch, 0);
    searchLayout->addWidget(m_searchEdit, 1);
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

    if (watched == m_btnSearch) {
        if (event->type() == QEvent::Enter) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-3", Qt::white, 16));
        } else if (event->type() == QEvent::Leave) {
            m_btnSearch->setIcon(UiHelper::getIcon("seach-3", QColor("#CCCCCC"), 16));
        }
    }

    return QObject::eventFilter(watched, event);
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Rebuild application.
2. Verify search button on top right uses `seach-3` icon.
3. Hover mouse over search button to verify icon turns white.
4. Click search button or press Enter in edit box to verify search triggers properly.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Re-uses `UiHelper::getIcon("seach-3", ...)` for SVG icon rendering.
- Re-uses `doSearch` internal search execution entry point.

## Header API Signature Verification
- `QPushButton::setFixedSize(int, int)`
- `QPushButton::setIcon(const QIcon&)`
- `QWidget::installEventFilter(QObject*)`

## Header Inclusion Chain & Type Completeness Check
- `QPushButton` forward declaration or inclusion via `#include <QPushButton>` in `SearchController.cpp`.
