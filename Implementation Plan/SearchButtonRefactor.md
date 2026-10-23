# Implementation Plan - Search Button Refactor

## Overview
This plan refactors the search widget in `SearchController` to replace the embedded leading SVG action with a dedicated, interactive search button `QPushButton#BtnSearchAddress` using the `seach-3` SVG icon.
The container `SearchContainer` and button `BtnSearchAddress` follow the exact QSS styling, border-left divider, and hover parameters as the address bar refresh button `BtnRefreshAddress`. The button click signal is connected to a TODO slot for future extended search / menu functionality.

---

## Modified Files List
- `src/ui/SearchController.h`
- `src/ui/SearchController.cpp`
- `resources/style.qss`

---

## Detailed Line-by-Line Changes

### 1. Update `src/ui/SearchController.h`

```diff
<<<<<<< SEARCH
#include <QObject>
#include <QLineEdit>
#include <QTimer>
#include <QWidget>
#include <QEvent>

namespace QuarkMeta {

class SearchHistoryPanel;
class ContentPanel;

class SearchController : public QObject {
    Q_OBJECT
public:
    explicit SearchController(QWidget* parent = nullptr);
    ~SearchController() override = default;

    QWidget* toolbarWidget() const { return m_searchContainer; }
    QLineEdit* searchEdit() const { return m_searchEdit; }
    SearchHistoryPanel* historyPanel() const { return m_searchHistoryPanel; }

    void bindContentPanel(ContentPanel* contentPanel);
    // 仅切换当前搜索目标窗格，不重复接线 UI 信号；供多窗格激活切换时调用
    void setActiveContentPanel(ContentPanel* panel) { m_contentPanel = panel; }

signals:
    void searchExecuted();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void doSearch(const QString& keyword);

    QWidget* m_searchContainer = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QTimer* m_searchTimer = nullptr;
    SearchHistoryPanel* m_searchHistoryPanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
};
=======
#include <QObject>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <QEvent>

namespace QuarkMeta {

class SearchHistoryPanel;
class ContentPanel;

class SearchController : public QObject {
    Q_OBJECT
public:
    explicit SearchController(QWidget* parent = nullptr);
    ~SearchController() override = default;

    QWidget* toolbarWidget() const { return m_searchContainer; }
    QLineEdit* searchEdit() const { return m_searchEdit; }
    QPushButton* searchButton() const { return m_btnSearch; }
    SearchHistoryPanel* historyPanel() const { return m_searchHistoryPanel; }

    void bindContentPanel(ContentPanel* contentPanel);
    // 仅切换当前搜索目标窗格，不重复接线 UI 信号；供多窗格激活切换时调用
    void setActiveContentPanel(ContentPanel* panel) { m_contentPanel = panel; }

signals:
    void searchExecuted();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void doSearch(const QString& keyword);

    QWidget* m_searchContainer = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnSearch = nullptr;
    QTimer* m_searchTimer = nullptr;
    SearchHistoryPanel* m_searchHistoryPanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
};
>>>>>>> REPLACE
```

### 2. Update `src/ui/SearchController.cpp`

```diff
<<<<<<< SEARCH
#include "SearchController.h"
#include "SearchHistoryPanel.h"
#include "ContentPanel.h"
#include "../core/SearchHistoryService.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include <QHBoxLayout>

using namespace QuarkMeta::Style;

namespace QuarkMeta {

SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");
    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 32);
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
#include "SearchController.h"
#include "SearchHistoryPanel.h"
#include "ContentPanel.h"
#include "../core/SearchHistoryService.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include <QHBoxLayout>
#include <QPushButton>

using namespace QuarkMeta::Style;

namespace QuarkMeta {

SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");
    m_searchContainer->setFixedSize(230, 32);

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
    m_btnSearch->setIcon(UiHelper::getIcon("seach-3", TextMuted, 16));
    m_btnSearch->setIconSize(QSize(16, 16));
    m_btnSearch->setCursor(Qt::PointingHandCursor);
    m_btnSearch->setProperty("tooltipText", "搜索");

    // TODO: 预留搜索按钮扩展功能（例如高级搜索菜单或触发搜索）
    connect(m_btnSearch, &QPushButton::clicked, this, [this]() {
        // TODO: Extended search functionality
        doSearch(m_searchEdit->text().trimmed());
    });

    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(m_btnSearch, 0);
>>>>>>> REPLACE
```

### 3. Update `resources/style.qss`

```diff
<<<<<<< SEARCH
QWidget#SearchContainer {
    background: transparent;
}
QLineEdit#SearchEdit {
    background-color: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #EEEEEE;
    padding-left: 4px;
    padding-right: 24px;
    font-size: 12px;
}
QLineEdit#SearchEdit:focus {
    border-color: #378ADD;
}
=======
QWidget#SearchContainer {
    background: #1E1E1E;
    border: 1px solid #333333;
    border-radius: 6px;
}
QWidget#SearchContainer[focused='true'] {
    border: 1px solid #3498db;
}
QLineEdit#SearchEdit {
    background: transparent;
    border: none;
    color: #EEEEEE;
    padding-left: 8px;
    font-size: 12px;
}
QPushButton#BtnSearchAddress {
    background: transparent;
    border: none;
    border-left: 1px solid #333333;
    border-top-right-radius: 6px;
    border-bottom-right-radius: 6px;
}
QPushButton#BtnSearchAddress:hover {
    background-color: #3E3E42;
}
QPushButton#BtnSearchAddress:pressed {
    background-color: #4E4E52;
}
>>>>>>> REPLACE
```

---

## Build & Verification Steps

### Build Command
```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

### Verification Steps
1. Launch QuarkMeta.
2. Observe the search box in the top right header:
   - Contains a clean `SearchContainer` with `QLineEdit#SearchEdit` and `QPushButton#BtnSearchAddress` on the right edge.
   - The button uses the `seach-3` icon.
   - Has a `1px #333333` left border separator and 6px top/bottom right border radius matching `BtnRefreshAddress`.
   - Hover and press feedback matches `BtnRefreshAddress`.

---

## SSOT API Reuse & Anti-Redundancy Self-Check
- [x] Reuses `UiHelper::getIcon("seach-3", TextMuted, 16)` from `SvgIcons.h`.
- [x] Mirrors QSS parameters identical to `AddressContainer` / `BtnRefreshAddress`.

---

## Header API Signature Verification
- `SearchController::searchButton() const -> QPushButton*` added to `src/ui/SearchController.h`.

---

## Header Inclusion Chain & Type Completeness Check
- `#include <QPushButton>` added to `src/ui/SearchController.h` and `src/ui/SearchController.cpp`.
