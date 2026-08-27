# Implementation Plan - SearchController Refactoring (search.md)

## 1. Overview
This implementation plan covers **Task 2: SearchController Refactoring** from MainWindow.
The goal is to extract `m_searchContainer`, `m_searchEdit`, `m_searchHistoryPanel`, `m_searchTimer`, the 300ms debounce timer, `doSearch` logic, event filtering, and search history panel interactions from `MainWindow` into a dedicated controller class `QuarkMeta::SearchController`.

Key features and strict requirements:
1. **Two-stage construction**:
   - **Stage 1 (`SearchController(QWidget* parent)`)**: Constructs UI components (`m_searchContainer`, `m_searchEdit`, `m_searchTimer`, `m_searchHistoryPanel`). Called during `initToolbar()`.
   - **Stage 2 (`bindContentPanel(ContentPanel* contentPanel)`)**: Binds `doSearch`, connects signals, and installs event filters after `ContentPanel` is instantiated. Called during `initUi()` after `setupSplitters()`.
2. Public accessor `searchEdit()` provided for `GlobalShortcutController`, `PanelMediator`, `unifiedNavigateTo()`, and `changeEvent()` to access `QLineEdit*` without breaking existing behavior.
3. Signal `searchExecuted()` emitted to trigger `MainWindow::updateStatusBar()`, preserving double-status-update behavior exactly as is.
4. `SearchController` registered in `CMakeLists.txt`.

---

## 2. Modified Files List
1. `CMakeLists.txt` (Add `src/ui/SearchController.h` and `src/ui/SearchController.cpp`)
2. `src/ui/SearchController.h` (New File)
3. `src/ui/SearchController.cpp` (New File)
4. `src/ui/MainWindow.h` (Remove extracted 4 search members, add `SearchController* m_searchController`)
5. `src/ui/MainWindow.cpp` (Delegate search toolbar creation and binding to `SearchController`, update references to `m_searchEdit` to `m_searchController->searchEdit()`, remove `watched == m_searchEdit` from `eventFilter()`)
6. `src/ui/GlobalShortcutController.cpp` (Update `m_mainWindow->m_searchEdit` reference to `m_mainWindow->m_searchController->searchEdit()`)
7. `src/ui/PanelMediator.cpp` (Update `m_mainWindow->m_searchEdit` reference to `m_mainWindow->m_searchController->searchEdit()`)

---

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`

```diff
<<<<<<< SEARCH
    src/ui/TaskProgressController.h
    src/ui/TaskProgressController.cpp
=======
    src/ui/TaskProgressController.h
    src/ui/TaskProgressController.cpp
    src/ui/SearchController.h
    src/ui/SearchController.cpp
>>>>>>> REPLACE
```

---

### 3.2 `src/ui/SearchController.h` (New File)

```cpp
#pragma once

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

} // namespace QuarkMeta
```

---

### 3.3 `src/ui/SearchController.cpp` (New File)

```cpp
#include "SearchController.h"
#include "SearchHistoryPanel.h"
#include "ContentPanel.h"
#include "SearchHistoryService.h"
#include "UiHelper.h"
#include <QHBoxLayout>

namespace QuarkMeta {

SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setStyleSheet("background: transparent;");
    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 32);
    m_searchEdit->addAction(UiHelper::getIcon("search", TextMuted), QLineEdit::LeadingPosition);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(QString(
        "QLineEdit { background-color: %1; border: 1px solid %2; border-radius: 4px; padding-left: 8px; color: %3; font-size: 12px; }"
        "QLineEdit:focus { border-color: %4; }"
    ).arg(qssColor(BgLayer3), qssColor(BorderColor), qssColor(TextMain), qssColor(PrimaryBlue)));

    searchLayout->addWidget(m_searchEdit);

    m_searchHistoryPanel = new SearchHistoryPanel(parent);
    m_searchHistoryPanel->setCategory("global");
    m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);
}

void SearchController::bindContentPanel(ContentPanel* contentPanel) {
    m_contentPanel = contentPanel;
    if (!m_contentPanel) return;

    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() {
        doSearch(m_searchEdit->text().trimmed());
    });

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            m_searchTimer->stop();
            doSearch("");
        } else {
            m_searchTimer->start();
        }
    });

    connect(m_searchTimer, &QTimer::timeout, this, [this]() {
        doSearch(m_searchEdit->text().trimmed());
    });

    m_searchEdit->installEventFilter(this);

    connect(m_searchHistoryPanel, &SearchHistoryPanel::historyItemClicked, this, [this](const QString& keyword) {
        m_searchEdit->setText(keyword);
        doSearch(keyword);
    });
}

void SearchController::doSearch(const QString& keyword) {
    if (!m_contentPanel) return;
    m_contentPanel->search(keyword);
    if (!keyword.isEmpty()) {
        SearchHistoryService::instance().addHistory("global", keyword);
        m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));
    }
    m_searchHistoryPanel->hide();
    emit searchExecuted();
}

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

} // namespace QuarkMeta
```

---

### 3.4 `src/ui/MainWindow.h`

```diff
<<<<<<< SEARCH
class TaskProgressToolBar;
class TaskProgressController;
=======
class TaskProgressToolBar;
class TaskProgressController;
class SearchController;
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    // 工具栏组件
    QToolBar* m_toolbar    = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnBack    = nullptr;
    QPushButton* m_btnForward = nullptr;
    QPushButton* m_btnUp      = nullptr;

    // 2026-04-12 按照用户要求：搜索历史悬浮面板及历史记录
    QWidget* m_searchContainer = nullptr; // 搜索框容器
    SearchHistoryPanel* m_searchHistoryPanel = nullptr;
=======
    // 工具栏组件
    QToolBar* m_toolbar    = nullptr;
    QPushButton* m_btnBack    = nullptr;
    QPushButton* m_btnForward = nullptr;
    QPushButton* m_btnUp      = nullptr;

    SearchController* m_searchController = nullptr;
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    QTimer* m_searchTimer = nullptr; // 2026-xx-xx 按照 Plan-106：搜索防抖计时器
=======
>>>>>>> REPLACE
```

---

### 3.5 `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
#include "TaskProgressController.h"
=======
#include "TaskProgressController.h"
#include "SearchController.h"
#include "SearchHistoryPanel.h"
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    m_searchHistoryPanel = new SearchHistoryPanel(this);
    m_searchHistoryPanel->setCategory("global");
    m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);

    auto doSearch = [this](const QString& keyword) {
        if (!m_contentPanel) return;
        m_contentPanel->search(keyword);
        if (!keyword.isEmpty()) {
            SearchHistoryService::instance().addHistory("global", keyword);
            m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));
        }
        m_searchHistoryPanel->hide();
        updateStatusBar();
    };

    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this, doSearch]() {
        doSearch(m_searchEdit->text().trimmed());
    });

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this, doSearch](const QString& text) {
        if (text.isEmpty()) {
            m_searchTimer->stop();
            doSearch("");
        } else {
            m_searchTimer->start();
        }
    });

    connect(m_searchTimer, &QTimer::timeout, this, [this, doSearch]() {
        doSearch(m_searchEdit->text().trimmed());
    });

    m_searchEdit->installEventFilter(this);

    connect(m_searchHistoryPanel, &SearchHistoryPanel::historyItemClicked, this, [this, doSearch](const QString& keyword) {
        m_searchEdit->setText(keyword);
        doSearch(keyword);
    });
=======
    m_searchController->bindContentPanel(m_contentPanel);
    connect(m_searchController, &SearchController::searchExecuted, this, &MainWindow::updateStatusBar);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    if (event->type() == QEvent::MouseButtonDblClick && watched == m_searchEdit) {
        auto history = SearchHistoryService::instance().getHistory("global");
        if (!history.isEmpty()) {
            m_searchHistoryPanel->setHistory(history);
            m_searchHistoryPanel->showBelow(m_searchEdit);
        }
        return true;
    }
=======
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    m_searchContainer = new QWidget(this);
    m_searchContainer->setStyleSheet("background: transparent;");
    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setFixedSize(230, 32);
    m_searchEdit->addAction(UiHelper::getIcon("search", TextMuted), QLineEdit::LeadingPosition);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(QString(
        "QLineEdit { background-color: %1; border: 1px solid %2; border-radius: 4px; padding-left: 8px; color: %3; font-size: 12px; }"
        "QLineEdit:focus { border-color: %4; }"
    ).arg(qssColor(BgLayer3), qssColor(BorderColor), qssColor(TextMain), qssColor(PrimaryBlue)));

    searchLayout->addWidget(m_searchEdit);
=======
    m_searchController = new SearchController(this);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    m_navBarLayout->addWidget(m_searchContainer);
=======
    m_navBarLayout->addWidget(m_searchController->toolbarWidget());
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    if (m_searchEdit) {
        m_searchEdit->blockSignals(true);
        m_searchEdit->clear();
        m_searchEdit->blockSignals(false);
    }
=======
    if (m_searchController && m_searchController->searchEdit()) {
        m_searchController->searchEdit()->blockSignals(true);
        m_searchController->searchEdit()->clear();
        m_searchController->searchEdit()->blockSignals(false);
    }
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
        if (isMinimized() && m_searchHistoryPanel) {
            m_searchHistoryPanel->hide();
        }
=======
        if (isMinimized() && m_searchController && m_searchController->historyPanel()) {
            m_searchController->historyPanel()->hide();
        }
>>>>>>> REPLACE
```

---

### 3.6 `src/ui/GlobalShortcutController.cpp`

```diff
<<<<<<< SEARCH
    // 4. Ctrl+F: 聚焦搜索过滤框
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        if (m_mainWindow->m_searchEdit) {
            m_mainWindow->m_searchEdit->setFocus(Qt::ShortcutFocusReason);
            m_mainWindow->m_searchEdit->selectAll();
        }
        event->accept();
        return true;
    }
=======
    // 4. Ctrl+F: 聚焦搜索过滤框
    if (event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier)) {
        if (m_mainWindow->m_searchController && m_mainWindow->m_searchController->searchEdit()) {
            m_mainWindow->m_searchController->searchEdit()->setFocus(Qt::ShortcutFocusReason);
            m_mainWindow->m_searchController->searchEdit()->selectAll();
        }
        event->accept();
        return true;
    }
>>>>>>> REPLACE
```

---

### 3.7 `src/ui/PanelMediator.cpp`

```diff
<<<<<<< SEARCH
    // 5b. FilterPanel 状态变化 -> 内容面板过滤
    if (filterPanel && contentPanel) {
        connect(filterPanel, &FilterPanel::filterChanged, m_mainWindow, [this, contentPanel](const FilterState& state) {
            FilterState mergedState = state;
            if (m_mainWindow->m_searchEdit) {
                mergedState.keyword = m_mainWindow->m_searchEdit->text().trimmed();
            }
            contentPanel->applyFilters(mergedState);
            m_mainWindow->updateStatusBar();
        });
    }
=======
    // 5b. FilterPanel 状态变化 -> 内容面板过滤
    if (filterPanel && contentPanel) {
        connect(filterPanel, &FilterPanel::filterChanged, m_mainWindow, [this, contentPanel](const FilterState& state) {
            FilterState mergedState = state;
            if (m_mainWindow->m_searchController && m_mainWindow->m_searchController->searchEdit()) {
                mergedState.keyword = m_mainWindow->m_searchController->searchEdit()->text().trimmed();
            }
            contentPanel->applyFilters(mergedState);
            m_mainWindow->updateStatusBar();
        });
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps

### 4.1 Build Verification
Execute standard CMake build:
```bash
cmake --build build --config Debug
```
Confirm zero compilation errors and zero MOC undefined symbol link errors for `SearchController`.

### 4.2 Behavior Verification
1. Typing in search bar triggers auto-search after 300ms debounce.
2. Pressing Enter triggers immediate search without waiting for debounce.
3. Clearing text in search bar immediately triggers an empty search (`""`).
4. Double-clicking search bar pops up global search history panel.
5. Clicking a history item populates search bar, triggers immediate search, and hides history panel.
6. Pressing Ctrl+F focuses search bar and selects all text.
7. Status bar updates twice after search as originally designed (`searchExecuted` signal).


// =================== 2 ===================


这份 `SearchController` 重构方案主要为了解决以下 **3 个核心问题**，但在落地细节上**存在 1 处潜在编译陷阱和 1 处职责泄漏**：

---

### 一、 该方案主要解决的 3 大问题

#### 1. 根治 `MainWindow` 的职责过载（SRP 单一职责）
* **现状问题**：`MainWindow` 是顶层主窗口，却充斥着大量搜索细节代码（300ms 防抖计时器、搜索框创建与样式、历史记录面板实例化、双击事件拦截等）。
* **方案解法**：将搜索组件与交互完整抽离为 `SearchController`，`MainWindow` 只需一行代码将其控件加入工具栏，不再关心搜索如何防抖和记录历史。

#### 2. 搜索交互状态的内聚与自闭环（Cohesion）
* **现状问题**：搜索框的文本变化、回车触发、防抖计时、双击弹出历史面板、点击历史项回填搜索，这些逻辑散落在 `MainWindow.cpp` 的 `initToolbar`、`initUi`、`eventFilter` 各处。
* **方案解法**：将防抖逻辑、事件拦截（`eventFilter`）、历史面板弹出与隐藏全部收拢在 `SearchController` 内部，外部无需感知。

#### 3. 跨模块按需交互标准化
* **现状问题**：快捷键（Ctrl+F）、面板联动（FilterPanel 关键字合并）、导航跳转（清空搜索框）直接对 `MainWindow::m_searchEdit` 进行杂乱的直接读写。
* **方案解法**：通过 `SearchController` 提供统一接口，规范搜索事件的收发链路。

---

### 二、 必须指出的 2 处潜在缺陷与风险（关键审查）

#### 🚨 1. 致命编译陷阱：方法名不匹配（`addHistory` vs `appendSearch`）
* 文档第 3.3 节代码：
  ```cpp
  SearchHistoryService::instance().addHistory("global", keyword);
  ```
* **实际代码库事实**：在之前审查的 `MainWindow.cpp` 源码中，该单例的方法名是 **`appendSearch`**（而非 `addHistory`）。如果按文档直接写 `addHistory`，编译会立即报 `C2039: 不是 SearchHistoryService 的成员`。

#### ⚠️ 2. 职责封装不彻底（依然泄露底层 `QLineEdit*`）
* 文档中为了兼容，提供了 `searchEdit()` 裸指针让外部去调用 `setFocus()`、`clear()`、`text()`。
* **更优做法**：`SearchController` 应直接暴露语义化方法：
  * `focusAndSelectAll()`（供快捷键 Ctrl+F 调用）；
  * `clear()`（供目录导航重置搜索词调用）；
  * `keyword()`（供 FilterPanel 提取当前关键词调用）。
  这样外部模块完全不需要触碰 `QLineEdit`，彻底解耦。

---

### 总结结论
该方案**架构方向完全正确**（成功将搜索从 `MainWindow` 剥离），但在实施时**必须修正 `SearchHistoryService` 的方法名调用**，并建议**将裸指针操作封装为语义化方法**以彻底达到 SRP 规范。