# Chrome-Style TitleBar Multi-Tab System Implementation Plan (Option A)

## 1. Overview
This implementation plan specifies the corrected architecture to integrate the Multi-Tab System (`TabBarWidget`) directly into the `TitleBarWidget` (title bar), exactly like modern web browsers (Chrome, Edge) and macOS Finder.

The layout inside `TitleBarWidget` is organized as follows:
`[Logo]` -> `[TabBarWidget (Tabs + '+' Button)]` -> `[Stretch]` -> `[Size Slider]` -> `[View Menu / Controls]` -> `[Window Controls (Min/Max/Close)]`

Central workspace holds `QStackedWidget`, where each page is an independent `ContentPanel` managed by `TabBarWidget`.

## 2. Modified & Created Files List
- **Created Files**:
  - `src/ui/TabBarWidget.h`
  - `src/ui/TabBarWidget.cpp`
- **Modified Files**:
  - `CMakeLists.txt`
  - `src/ui/TitleBarWidget.h`
  - `src/ui/TitleBarWidget.cpp`
  - `src/ui/AppShortcutController.h`
  - `src/ui/AppShortcutController.cpp`
  - `src/ui/PanelMediator.h`
  - `src/ui/PanelMediator.cpp`
  - `src/ui/MainWindow.h`
  - `src/ui/MainWindow.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 Embed `TabBarWidget` in `TitleBarWidget.h`
Add `TabBarWidget* tabBarWidget() const;` to `TitleBarWidget.h`:

```
<<<<<<< SEARCH
    Q_INVOKABLE void setWindowMaximized(bool maximized);
=======
    TabBarWidget* tabBarWidget() const { return m_tabBarWidget; }
    Q_INVOKABLE void setWindowMaximized(bool maximized);
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    QHBoxLayout* m_layout = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_appNameLabel = nullptr;
=======
    QHBoxLayout* m_layout = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_appNameLabel = nullptr;
    TabBarWidget* m_tabBarWidget = nullptr;
>>>>>>> REPLACE
```

### 3.2 Embed `TabBarWidget` in `TitleBarWidget.cpp` Layout
In `TitleBarWidget.cpp`, replace `m_appNameLabel` with `m_tabBarWidget` embedded directly in the title bar:

```
<<<<<<< SEARCH
    m_appNameLabel = new QLabel("QuarkMeta", this);
    m_appNameLabel->setObjectName("AppNameLabel");
    m_layout->addWidget(m_appNameLabel);
    m_layout->addStretch();
=======
    m_tabBarWidget = new TabBarWidget(this);
    m_layout->addWidget(m_tabBarWidget, 1);
    m_layout->addStretch();
>>>>>>> REPLACE
```

### 3.3 Create `src/ui/TabBarWidget.h`
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

### 3.4 Create `src/ui/TabBarWidget.cpp`
```cpp
#include "TabBarWidget.h"
#include "UiHelper.h"
#include <QVariant>

namespace QuarkMeta {

TabBarWidget::TabBarWidget(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(30);
    setObjectName("TitleTabBarWidget");

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);

    m_tabBar = new QTabBar(this);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDrawBase(false);
    m_tabBar->setObjectName("MainTabBar");

    m_btnNewTab = new QPushButton(this);
    m_btnNewTab->setFocusPolicy(Qt::NoFocus);
    m_btnNewTab->setFixedSize(22, 22);
    m_btnNewTab->setIcon(UiHelper::getIcon("add", QColor("#EEEEEE")));
    m_btnNewTab->setIconSize(QSize(14, 14));
    m_btnNewTab->setObjectName("NewTabBtn");

    m_layout->addWidget(m_tabBar);
    m_layout->addWidget(m_btnNewTab);

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

### 3.5 `MainWindow.cpp` Wire TitleBar TabBar to `QStackedWidget`
In `MainWindow.cpp`, connect `TitleBarWidget`'s embedded `TabBarWidget` to `m_contentStack`:

```
<<<<<<< SEARCH
    m_contentPanel = new ContentPanel(this);
    m_mainSplitter->addWidget(m_contentPanel);
=======
    m_contentStack = new QStackedWidget(this);
    m_mainSplitter->addWidget(m_contentStack);

    TabBarWidget* tabBar = m_titleBarWidget ? m_titleBarWidget->tabBarWidget() : nullptr;
    if (tabBar) {
        connect(tabBar, &TabBarWidget::newTabRequested, this, [this]() {
            createNewTab("");
        });
        connect(tabBar, &TabBarWidget::tabCloseRequested, this, &MainWindow::closeTab);
        connect(tabBar, &TabBarWidget::currentChanged, this, &MainWindow::onTabChanged);
        connect(tabBar, &TabBarWidget::tabMoved, this, &MainWindow::onTabMoved);
    }

    // 创建初始首个标签页
    createNewTab("");
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. **Compilation**:
   ```bash
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Debug
   ```
2. **Visual & Functional Verification**:
   - Run QuarkMeta: inspect the top title bar (`TitleBarWidget`).
   - Confirm that the tabs and "+" button are embedded directly inside the title bar (Chrome/Edge style), right after the FERREX logo.
   - Click "+" or press `Ctrl+T`: confirm a new tab is created directly in the title bar.
   - Drag tabs: confirm tab reordering stays synced with `QStackedWidget`.
   - Switch active tab: confirm `PanelMediator` re-anchors to active `ContentPanel`.

## 5. Header API Signature Verification
- `TitleBarWidget.h`: Added `tabBarWidget() const`.
- `TabBarWidget.h`: Added `TabBarWidget` class definition.
- `MainWindow.h`: Added `createNewTab()`, `closeTab()`, `onTabChanged()`, and `onTabMoved()`.
