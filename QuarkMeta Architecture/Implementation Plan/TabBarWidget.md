# Multi-Tab System Architecture Implementation Plan (Option A)

## 1. Overview
This implementation plan specifies the complete architecture and code modifications required to introduce a True Multi-Tab System (Option A) in QuarkMeta.
Each tab is represented by an independent `ContentPanel` instance, managed via a `TabBarWidget` and a central `QStackedWidget`.
`PanelMediator` is enhanced with a `bindActiveContentPanel` method to dynamically unbind the previous `ContentPanel` and rebind the newly activated `ContentPanel` to external panels (`NavPanel`, `AddressBar`, `MetaPanel`, Status Bar, SearchController, and TitleBar).
Shortcuts `Ctrl+T` (New Tab) and `Ctrl+W` (Close Tab) are wired in `AppShortcutController`.

## 2. Modified & Created Files List
- **Created Files**:
  - `src/ui/TabBarWidget.h`
  - `src/ui/TabBarWidget.cpp`
- **Modified Files**:
  - `CMakeLists.txt`
  - `src/ui/AppShortcutController.h`
  - `src/ui/AppShortcutController.cpp`
  - `src/ui/PanelMediator.h`
  - `src/ui/PanelMediator.cpp`
  - `src/ui/MainWindow.h`
  - `src/ui/MainWindow.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 CMakeLists.txt Registration
Register `src/ui/TabBarWidget.h` and `src/ui/TabBarWidget.cpp` into `CMakeLists.txt`:

```
<<<<<<< SEARCH
    src/ui/TitleBarWidget.h
    src/ui/TitleBarWidget.cpp
=======
    src/ui/TitleBarWidget.h
    src/ui/TitleBarWidget.cpp
    src/ui/TabBarWidget.h
    src/ui/TabBarWidget.cpp
>>>>>>> REPLACE
```

### 3.2 Create `src/ui/TabBarWidget.h`
```cpp
#pragma once

#include <QWidget>
#include <QTabBar>
#include <QPushButton>
#include <QHBoxLayout>

namespace QuarkMeta {

class TabBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabBarWidget(QWidget* parent = nullptr);
    ~TabBarWidget() override = default;

    int addTab(const QString& title, const QString& path);
    void setTabTitle(int index, const QString& title);
    void setTabPath(int index, const QString& path);
    QString tabPath(int index) const;

    int currentIndex() const;
    void setCurrentIndex(int index);
    int count() const;
    void removeTab(int index);

signals:
    void currentChanged(int index);
    void tabCloseRequested(int index);
    void newTabRequested();
    void tabMoved(int from, int to);

private:
    QHBoxLayout* m_layout = nullptr;
    QTabBar* m_tabBar = nullptr;
    QPushButton* m_btnNewTab = nullptr;
};

} // namespace QuarkMeta
```

### 3.3 Create `src/ui/TabBarWidget.cpp`
```cpp
#include "TabBarWidget.h"
#include "UiHelper.h"
#include <QVariant>

namespace QuarkMeta {

TabBarWidget::TabBarWidget(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(32);
    setObjectName("TabBarWidget");

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->setSpacing(4);

    m_tabBar = new QTabBar(this);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDrawBase(false);
    m_tabBar->setObjectName("MainTabBar");

    m_btnNewTab = new QPushButton(this);
    m_btnNewTab->setFocusPolicy(Qt::NoFocus);
    m_btnNewTab->setFixedSize(24, 24);
    m_btnNewTab->setIcon(UiHelper::getIcon("add", QColor("#EEEEEE")));
    m_btnNewTab->setIconSize(QSize(16, 16));
    m_btnNewTab->setObjectName("NewTabBtn");

    m_layout->addWidget(m_tabBar);
    m_layout->addWidget(m_btnNewTab);
    m_layout->addStretch();

    connect(m_tabBar, &QTabBar::currentChanged, this, &TabBarWidget::currentChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &TabBarWidget::tabCloseRequested);
    connect(m_tabBar, &QTabBar::tabMoved, this, &TabBarWidget::tabMoved);
    connect(m_btnNewTab, &QPushButton::clicked, this, &TabBarWidget::newTabRequested);
}

int TabBarWidget::addTab(const QString& title, const QString& path) {
    int idx = m_tabBar->addTab(title);
    m_tabBar->setTabData(idx, path);
    return idx;
}

void TabBarWidget::setTabTitle(int index, const QString& title) {
    if (index >= 0 && index < m_tabBar->count()) {
        m_tabBar->setTabText(index, title);
    }
}

void TabBarWidget::setTabPath(int index, const QString& path) {
    if (index >= 0 && index < m_tabBar->count()) {
        m_tabBar->setTabData(index, path);
    }
}

QString TabBarWidget::tabPath(int index) const {
    if (index >= 0 && index < m_tabBar->count()) {
        return m_tabBar->tabData(index).toString();
    }
    return QString();
}

int TabBarWidget::currentIndex() const {
    return m_tabBar->currentIndex();
}

void TabBarWidget::setCurrentIndex(int index) {
    m_tabBar->setCurrentIndex(index);
}

int TabBarWidget::count() const {
    return m_tabBar->count();
}

void TabBarWidget::removeTab(int index) {
    if (index >= 0 && index < m_tabBar->count()) {
        m_tabBar->removeTab(index);
    }
}

} // namespace QuarkMeta
```

### 3.4 `AppShortcutController.h` Signal Enhancements
Add `newTabRequested` and `closeTabRequested` signals:

```
<<<<<<< SEARCH
signals:
    /**
     * @brief Alt+Q 局内快捷键触发置顶状态翻转
     */
    void togglePinRequested();
=======
signals:
    /**
     * @brief Ctrl+T 新建标签页
     */
    void newTabRequested();

    /**
     * @brief Ctrl+W 关闭当前标签页
     */
    void closeTabRequested();

    /**
     * @brief Alt+Q 局内快捷键触发置顶状态翻转
     */
    void togglePinRequested();
>>>>>>> REPLACE
```

### 3.5 `AppShortcutController.cpp` Shortcut Wiring
Register `Ctrl+T` and `Ctrl+W` in `initShortcuts()`:

```
<<<<<<< SEARCH
    // 4. Alt+Q: 局内切换窗口置顶
=======
    // 3.5. Ctrl+T: 局内新建标签页
    QShortcut* scNewTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), m_window);
    scNewTab->setContext(Qt::WindowShortcut);
    connect(scNewTab, &QShortcut::activated, this, &AppShortcutController::newTabRequested);

    // 3.6. Ctrl+W: 局内关闭当前标签页
    QShortcut* scCloseTab = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), m_window);
    scCloseTab->setContext(Qt::WindowShortcut);
    connect(scCloseTab, &QShortcut::activated, this, &AppShortcutController::closeTabRequested);

    // 4. Alt+Q: 局内切换窗口置顶
>>>>>>> REPLACE
```

### 3.6 `PanelMediator.h` Dynamic Active Tab Re-anchoring API
Add `bindActiveContentPanel` and connection management declarations to `PanelMediator.h`:

```
<<<<<<< SEARCH
    /**
     * @brief 建立各面板间的信号槽连接
     */
    void setupConnections();
=======
    /**
     * @brief 建立各面板间的信号槽连接
     */
    void setupConnections();

    /**
     * @brief 动态重新绑定当前激活的 ContentPanel 到中介者及外部面板
     */
    void bindActiveContentPanel(ContentPanel* newPanel);

private:
    void disconnectActiveContentPanel();
    void connectActiveContentPanel();
>>>>>>> REPLACE
```

### 3.7 `PanelMediator.cpp` Dynamic Active Tab Re-anchoring Implementation
Implement `bindActiveContentPanel` with context-safe connection cleanup in `PanelMediator.cpp`:

```
<<<<<<< SEARCH
void PanelMediator::setupConnections() {
=======
void PanelMediator::disconnectActiveContentPanel() {
    if (!m_contentPanel) return;

    m_contentPanel->disconnect();
    if (m_titleBar) {
        m_titleBar->disconnect(m_contentPanel);
    }
    if (m_metaPanel) {
        m_metaPanel->disconnect(m_contentPanel);
    }
    if (m_filterPanel) {
        m_filterPanel->disconnect(m_contentPanel);
    }
}

void PanelMediator::bindActiveContentPanel(ContentPanel* newPanel) {
    if (m_contentPanel == newPanel) return;

    disconnectActiveContentPanel();
    m_contentPanel = newPanel;
    if (!m_contentPanel) return;

    if (m_searchController) {
        m_searchController->bindContentPanel(m_contentPanel);
    }
    if (m_titleBar) {
        m_titleBar->setZoomLevel(m_contentPanel->zoomLevel());
    }

    connectActiveContentPanel();

    if (!m_contentPanel->currentPath().isEmpty()) {
        NavigationService::instance().navigateTo(m_contentPanel->currentPath());
    }
}

void PanelMediator::connectActiveContentPanel() {
    ContentPanel* contentPanel = m_contentPanel;
    MetaPanel* metaPanel = m_metaPanel;
    FilterPanel* filterPanel = m_filterPanel;
    TitleBarWidget* titleBar = m_titleBar;
    FavoritePanel* favoritePanel = m_favoritePanel;

    if (!contentPanel) return;

    if (titleBar) {
        connect(titleBar, &TitleBarWidget::viewModeRequested, contentPanel, [contentPanel](TitleBarWidget::ViewModeOption option) {
            ContentPanel::ViewMode targetMode = ContentPanel::GridView;
            if (option == TitleBarWidget::JustifiedViewMode) targetMode = ContentPanel::JustifiedViewMode;
            else if (option == TitleBarWidget::GridViewMode) targetMode = ContentPanel::GridView;
            else if (option == TitleBarWidget::ListViewMode) targetMode = ContentPanel::ListView;
            else if (option == TitleBarWidget::ColumnViewMode) targetMode = ContentPanel::ColumnView;
            contentPanel->setViewMode(targetMode);
        });

        connect(titleBar, &TitleBarWidget::createItemRequested, contentPanel, [contentPanel](const QString& type) {
            contentPanel->createNewItem(type);
        });

        connect(titleBar, &TitleBarWidget::zoomLevelChanged, contentPanel, [contentPanel](int value) {
            contentPanel->setZoomLevel(value);
            AppConfig::instance().setValue("UI/GridZoomLevel", value);
        });

        connect(contentPanel, &ContentPanel::zoomLevelChanged, titleBar, [titleBar](int level) {
            titleBar->setZoomLevel(level);
            AppConfig::instance().setValue("UI/GridZoomLevel", level);
        });
    }

    if (contentPanel && metaPanel) {
        connect(contentPanel, &ContentPanel::selectionChanged, metaPanel, [contentPanel, metaPanel](const QStringList& paths) {
            metaPanel->setSelectedPaths(paths);
        });
    }

    if (contentPanel && filterPanel) {
        connect(contentPanel, &ContentPanel::directoryStatsReady, filterPanel, [filterPanel](const ScanStats& stats) {
            filterPanel->populateStats(stats);
        });
        connect(filterPanel, &FilterPanel::filterChanged, contentPanel, [contentPanel](const FilterState& state) {
            contentPanel->applyFilters(state);
        });
    }

    if (contentPanel && favoritePanel) {
        connect(contentPanel, &ContentPanel::requestAddFavorite, favoritePanel, [favoritePanel](const QStringList& paths) {
            for (const QString& p : paths) favoritePanel->addFavoriteItem(p);
            favoritePanel->saveFavorites();
        });
    }
}

void PanelMediator::setupConnections() {
>>>>>>> REPLACE
```

### 3.8 `MainWindow.h` Tab Components & Slots Declaration
Declare `TabBarWidget`, `QStackedWidget`, and tab slots in `MainWindow.h`:

```
<<<<<<< SEARCH
    ContentPanel* m_contentPanel = nullptr;
=======
    TabBarWidget* m_tabBarWidget = nullptr;
    QStackedWidget* m_contentStack = nullptr;
    ContentPanel* m_contentPanel = nullptr;

    void createNewTab(const QString& initialPath = "");
    void closeTab(int index);
    void onTabChanged(int index);
    void onTabMoved(int from, int to);
>>>>>>> REPLACE
```

### 3.9 `MainWindow.cpp` Multi-Tab Stacked Layout & Tab Lifecycle Wiring
Update `MainWindow.cpp` to initialize `TabBarWidget` and `QStackedWidget`, and wire tab creation/destruction:

```
<<<<<<< SEARCH
    m_contentPanel = new ContentPanel(this);
    m_mainSplitter->addWidget(m_contentPanel);
=======
    QWidget* centerContainer = new QWidget(this);
    QVBoxLayout* centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    m_tabBarWidget = new TabBarWidget(centerContainer);
    m_contentStack = new QStackedWidget(centerContainer);

    centerLayout->addWidget(m_tabBarWidget);
    centerLayout->addWidget(m_contentStack);

    m_mainSplitter->addWidget(centerContainer);

    // 建立标签栏事件响应
    connect(m_tabBarWidget, &TabBarWidget::newTabRequested, this, [this]() {
        createNewTab("");
    });
    connect(m_tabBarWidget, &TabBarWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabBarWidget, &TabBarWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabBarWidget, &TabBarWidget::tabMoved, this, &MainWindow::onTabMoved);

    // 创建初始首个标签页
    createNewTab("");
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void MainWindow::setupControllersAndMediators() {
=======
void MainWindow::createNewTab(const QString& initialPath) {
    ContentPanel* panel = new ContentPanel(m_contentStack);
    int stackIdx = m_contentStack->addWidget(panel);

    QString path = initialPath.isEmpty() ? "computer://" : initialPath;
    QFileInfo fi(path);
    QString title = (path == "computer://") ? "此电脑" : (fi.fileName().isEmpty() ? path : fi.fileName());

    int tabIdx = m_tabBarWidget->addTab(title, path);
    m_tabBarWidget->setCurrentIndex(tabIdx);
    m_contentStack->setCurrentIndex(stackIdx);

    if (m_panelMediator) {
        m_panelMediator->bindActiveContentPanel(panel);
    }
}

void MainWindow::closeTab(int index) {
    if (m_tabBarWidget->count() <= 1) return; // 保护首个标签不被关光

    if (index < 0) index = m_tabBarWidget->currentIndex();

    QWidget* widget = m_contentStack->widget(index);
    if (widget) {
        m_contentStack->removeWidget(widget);
        widget->deleteLater();
    }
    m_tabBarWidget->removeTab(index);
}

void MainWindow::onTabChanged(int index) {
    if (index < 0 || index >= m_contentStack->count()) return;
    m_contentStack->setCurrentIndex(index);

    ContentPanel* activePanel = qobject_cast<ContentPanel*>(m_contentStack->widget(index));
    if (activePanel && m_panelMediator) {
        m_panelMediator->bindActiveContentPanel(activePanel);
    }
}

void MainWindow::onTabMoved(int from, int to) {
    if (from < 0 || to < 0 || from >= m_contentStack->count() || to >= m_contentStack->count()) return;

    QWidget* widget = m_contentStack->widget(from);
    m_contentStack->removeWidget(widget);
    m_contentStack->insertWidget(to, widget);
}

void MainWindow::setupControllersAndMediators() {
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 快捷键沉浸模式切换下沉
    if (shortcutController && layoutManager) {
        connect(shortcutController, &AppShortcutController::toggleImmersiveRequested, layoutManager, [layoutManager]() {
            layoutManager->toggleImmersiveMode();
        });
    }
=======
    // 快捷键沉浸模式切换下沉
    if (shortcutController && layoutManager) {
        connect(shortcutController, &AppShortcutController::toggleImmersiveRequested, layoutManager, [layoutManager]() {
            layoutManager->toggleImmersiveMode();
        });
    }

    // 快捷键新建/关闭标签页信号链接
    if (shortcutController) {
        connect(shortcutController, &AppShortcutController::newTabRequested, this, [this]() {
            createNewTab("");
        });
        connect(shortcutController, &AppShortcutController::closeTabRequested, this, [this]() {
            closeTab(-1);
        });
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**:
   ```bash
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Debug
   ```
2. **Tab Functional Verification**:
   - Launch application: verify top tab bar renders with initial tab.
   - Press `Ctrl+T` or click "+": confirm a new independent `ContentPanel` tab opens in `QStackedWidget`.
   - Navigate to different folders in each tab: verify path, selection, view mode, and history in Tab A do not affect Tab B.
   - Switch active tab: confirm `AddressBar`, `NavPanel` highlight, `MetaPanel` and status bar update to reflect active tab via `PanelMediator::bindActiveContentPanel`.
   - Reorder tabs via mouse drag: confirm `m_contentStack` synchronization via `onTabMoved`.
   - Click tab "x" or press `Ctrl+W`: confirm active tab closes gracefully and memory is freed via `deleteLater()`.

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reused `NavigationService::instance()`, `CentralEventHub`, and standard `QStackedWidget` / `QTabBar` APIs.
- Preserved single SSOT state ownership per `ContentPanel` instance.

## 6. Header API Signature Verification
- `AppShortcutController.h`: Added `newTabRequested()` & `closeTabRequested()` signals.
- `PanelMediator.h`: Added `bindActiveContentPanel(ContentPanel*)`.
- `MainWindow.h`: Added `createNewTab()`, `closeTab()`, `onTabChanged()`, and `onTabMoved()` private slots/methods.
