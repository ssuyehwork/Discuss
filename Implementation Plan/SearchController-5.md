# Implementation Plan - SearchController-5.md

## 1. Overview
This implementation plan adds a search scope dropdown menu to the search button (`m_btnSearch`) in `SearchController`.

As requested by the user:
- Clicking the search icon button (`m_btnSearch`) pops up a menu with two checkable options:
  1. `“搜索：当前文件夹”` (Search: Current Folder) - Default checked option.
  2. `“搜索：库”` (Search: Library) - When checked, performs a search across all associated paths in all Library categories.
- The search scope state is maintained in-memory (`SearchScope::CurrentFolder` vs `SearchScope::Library`) and does not persist across application restarts.

In accordance with AGENTS.md rules:
- Source files are NOT modified directly in this plan stage.
- This plan is strictly isolated in `QuarkMeta Architecture/Implementation Plan/SearchController-5.md`.

## 2. Modified Files List
- `src/ui/SearchController.h`
- `src/ui/SearchController.cpp`

## 3. Detailed Line-by-Line Changes

### 1. `src/ui/SearchController.h`

```diff
<<<<<<< SEARCH
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

} // namespace QuarkMeta
=======
namespace QuarkMeta {

class SearchHistoryPanel;
class ContentPanel;

enum class SearchScope {
    CurrentFolder,
    Library
};

class SearchController : public QObject {
    Q_OBJECT
public:
    explicit SearchController(QWidget* parent = nullptr);
    ~SearchController() override = default;

    QWidget* toolbarWidget() const { return m_searchContainer; }
    QLineEdit* searchEdit() const { return m_searchEdit; }
    QPushButton* searchButton() const { return m_btnSearch; }
    SearchHistoryPanel* historyPanel() const { return m_searchHistoryPanel; }
    SearchScope searchScope() const { return m_searchScope; }

    void bindContentPanel(ContentPanel* contentPanel);
    // 仅切换当前搜索目标窗格，不重复接线 UI 信号；供多窗格激活切换时调用
    void setActiveContentPanel(ContentPanel* panel) { m_contentPanel = panel; }

signals:
    void searchExecuted();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void doSearch(const QString& keyword);
    void showSearchMenu();
    void performLibrarySearch();

    QWidget* m_searchContainer = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_btnSearch = nullptr;
    QTimer* m_searchTimer = nullptr;
    SearchHistoryPanel* m_searchHistoryPanel = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    SearchScope m_searchScope = SearchScope::CurrentFolder;
};

} // namespace QuarkMeta
>>>>>>> REPLACE
```

### 2. `src/ui/SearchController.cpp`

```diff
<<<<<<< SEARCH
#include "SearchController.h"
#include "SearchHistoryPanel.h"
#include "ContentPanel.h"
#include "../core/SearchHistoryService.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include <QHBoxLayout>
#include <QStyle>

using namespace QuarkMeta::Style;

namespace QuarkMeta {

SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");
    m_searchContainer->setFixedHeight(32);

    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setObjectName("SearchEdit");
    m_searchEdit->setFixedHeight(30);
    m_searchEdit->installEventFilter(this);

    QAction* clearAction = m_searchEdit->addAction(UiHelper::getIcon("close", TextMuted), QLineEdit::TrailingPosition);
    clearAction->setVisible(false);
    connect(clearAction, &QAction::triggered, m_searchEdit, &QLineEdit::clear);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [clearAction](const QString& text) {
        clearAction->setVisible(!text.isEmpty());
    });

    UiHelper::setupLineEditContextMenu(m_searchEdit);

    m_btnSearch = new QPushButton(m_searchContainer);
    m_btnSearch->setObjectName("BtnSearchAddress");
    m_btnSearch->setFixedSize(30, 30);
    m_btnSearch->setIcon(UiHelper::getIcon("seach-7", QColor("#CCCCCC"), 16));
    m_btnSearch->setIconSize(QSize(16, 16));
    m_btnSearch->setCursor(Qt::ArrowCursor);
    m_btnSearch->setProperty("tooltipText", "搜索");
    m_btnSearch->setAttribute(Qt::WA_Hover);
    m_btnSearch->installEventFilter(this);

    // TODO: 预留搜索按钮扩展功能（例如高级搜索菜单或触发搜索）
    connect(m_btnSearch, &QPushButton::clicked, this, [this]() {
        // TODO: Extended search functionality
        doSearch(m_searchEdit->text().trimmed());
    });
=======
#include "SearchController.h"
#include "SearchHistoryPanel.h"
#include "ContentPanel.h"
#include "../core/SearchHistoryService.h"
#include "../meta/LibraryDao.h"
#include "UiHelper.h"
#include "StyleLibrary.h"
#include <QHBoxLayout>
#include <QStyle>
#include <QMenu>
#include <QActionGroup>

using namespace QuarkMeta::Style;

namespace QuarkMeta {

SearchController::SearchController(QWidget* parent)
    : QObject(parent) {
    m_searchContainer = new QWidget(parent);
    m_searchContainer->setObjectName("SearchContainer");
    m_searchContainer->setFixedHeight(32);

    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    m_searchEdit = new QLineEdit(m_searchContainer);
    m_searchEdit->setPlaceholderText("搜索...");
    m_searchEdit->setObjectName("SearchEdit");
    m_searchEdit->setFixedHeight(30);
    m_searchEdit->installEventFilter(this);

    QAction* clearAction = m_searchEdit->addAction(UiHelper::getIcon("close", TextMuted), QLineEdit::TrailingPosition);
    clearAction->setVisible(false);
    connect(clearAction, &QAction::triggered, m_searchEdit, &QLineEdit::clear);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [clearAction](const QString& text) {
        clearAction->setVisible(!text.isEmpty());
    });

    UiHelper::setupLineEditContextMenu(m_searchEdit);

    m_btnSearch = new QPushButton(m_searchContainer);
    m_btnSearch->setObjectName("BtnSearchAddress");
    m_btnSearch->setFixedSize(30, 30);
    m_btnSearch->setIcon(UiHelper::getIcon("seach-7", QColor("#CCCCCC"), 16));
    m_btnSearch->setIconSize(QSize(16, 16));
    m_btnSearch->setCursor(Qt::ArrowCursor);
    m_btnSearch->setProperty("tooltipText", "搜索范围与选项");
    m_btnSearch->setAttribute(Qt::WA_Hover);
    m_btnSearch->installEventFilter(this);

    connect(m_btnSearch, &QPushButton::clicked, this, &SearchController::showSearchMenu);
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void SearchController::doSearch(const QString& keyword) {
    if (!m_contentPanel) return;
    m_contentPanel->search(keyword);
    if (!keyword.isEmpty()) {
        SearchHistoryService::instance().appendSearch("global", keyword);
        m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));
    }
    m_searchHistoryPanel->hide();
    emit searchExecuted();
}
=======
void SearchController::showSearchMenu() {
    if (!m_btnSearch) return;

    QMenu menu(m_btnSearch);
    UiHelper::applyMenuStyle(&menu);

    QActionGroup* group = new QActionGroup(&menu);
    group->setExclusive(true);

    QAction* actFolder = menu.addAction("搜索：当前文件夹");
    actFolder->setCheckable(true);
    actFolder->setChecked(m_searchScope == SearchScope::CurrentFolder);
    group->addAction(actFolder);

    QAction* actLibrary = menu.addAction("搜索：库");
    actLibrary->setCheckable(true);
    actLibrary->setChecked(m_searchScope == SearchScope::Library);
    group->addAction(actLibrary);

    connect(actFolder, &QAction::triggered, this, [this]() {
        if (m_searchScope != SearchScope::CurrentFolder) {
            m_searchScope = SearchScope::CurrentFolder;
            if (m_contentPanel) {
                m_contentPanel->refreshAll();
            }
            doSearch(m_searchEdit ? m_searchEdit->text().trimmed() : QString());
        }
    });

    connect(actLibrary, &QAction::triggered, this, [this]() {
        if (m_searchScope != SearchScope::Library) {
            m_searchScope = SearchScope::Library;
            performLibrarySearch();
        }
    });

    QPoint globalPos = m_btnSearch->mapToGlobal(QPoint(0, m_btnSearch->height()));
    menu.exec(globalPos);
}

void SearchController::performLibrarySearch() {
    if (!m_contentPanel) return;

    LibraryDao::initTable();
    auto categories = LibraryDao::getAllCategories();
    QStringList allLibraryPaths;
    for (const auto& cat : categories) {
        allLibraryPaths.append(cat.associatedPaths);
    }
    allLibraryPaths.removeDuplicates();

    m_contentPanel->loadPaths(allLibraryPaths);
    QString keyword = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    m_contentPanel->search(keyword);

    if (!keyword.isEmpty()) {
        SearchHistoryService::instance().appendSearch("global", keyword);
        if (m_searchHistoryPanel) {
            m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));
        }
    }
    if (m_searchHistoryPanel) {
        m_searchHistoryPanel->hide();
    }
    emit searchExecuted();
}

void SearchController::doSearch(const QString& keyword) {
    if (!m_contentPanel) return;

    if (m_searchScope == SearchScope::Library) {
        performLibrarySearch();
        return;
    }

    m_contentPanel->search(keyword);
    if (!keyword.isEmpty()) {
        SearchHistoryService::instance().appendSearch("global", keyword);
        if (m_searchHistoryPanel) {
            m_searchHistoryPanel->setHistory(SearchHistoryService::instance().getHistory("global"));
        }
    }
    if (m_searchHistoryPanel) {
        m_searchHistoryPanel->hide();
    }
    emit searchExecuted();
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Build `QuarkMeta` target:
   ```bash
   cmake --build build --config Release
   ```
2. Run `QuarkMeta`:
   - Click the search icon button (`m_btnSearch`).
   - Verify a popup menu appears with two checkable options:
     - `“搜索：当前文件夹”` (Checked by default)
     - `“搜索：库”`
   - Select `“搜索：库”`:
     - Verify that search switches to querying all paths associated with Library categories.
   - Switch back to `“搜索：当前文件夹”`:
     - Verify that the view restores current folder contents and applies filtering.
   - Restart the application:
     - Verify that the scope defaults back to `“搜索：当前文件夹”` (non-persistent).

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **API Reuse**: `UiHelper::applyMenuStyle`, `LibraryDao::getAllCategories()`, `ContentPanel::loadPaths`, and `ContentPanel::search`.
- **Zero Redundancy**: Integrates scope selection directly into existing `SearchController` with zero duplicated widgets or extra persistence files.

## 6. Header API Signature Verification
- `SearchController::searchScope()` -> `SearchScope searchScope() const`
- `ContentPanel::loadPaths(const QStringList& paths, int reqId)`
- `ContentPanel::refreshAll()`

## 7. Header Inclusion Chain & Type Completeness Check
- Added `#include "../meta/LibraryDao.h"`, `#include <QMenu>`, and `#include <QActionGroup>` in `SearchController.cpp`.
- Type completeness checked for `QMenu`, `QActionGroup`, and `LibraryDao`.
